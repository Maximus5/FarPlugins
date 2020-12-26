#include "utils.hpp"
#include "sysutils.hpp"

HINSTANCE g_h_instance = nullptr;

wstring get_system_message(HRESULT hr, DWORD lang_id) {
  wostringstream st;
  wchar_t* sys_msg;
  DWORD len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, lang_id, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
  if (!len && lang_id && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND)
    len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, 0, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
  if (!len) {
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
      HMODULE h_winhttp = GetModuleHandle("winhttp");
      if (h_winhttp) {
        len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, h_winhttp, HRESULT_CODE(hr), lang_id, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
        if (!len && lang_id && GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND)
          len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, h_winhttp, HRESULT_CODE(hr), 0, reinterpret_cast<LPWSTR>(&sys_msg), 0, nullptr);
      }
    }
  }
  if (len) {
    wstring message;
    try {
      message = sys_msg;
    }
    catch (...) {
      LocalFree(static_cast<HLOCAL>(sys_msg));
      throw;
    }
    LocalFree(static_cast<HLOCAL>(sys_msg));
    st << strip(message) << L" (0x" << hex << uppercase << setw(8) << setfill(L'0') << hr << L")";
  }
  else {
    st << L"HRESULT: 0x" << hex << uppercase << setw(8) << setfill(L'0') << hr;
  }
  return st.str();
}

wstring get_console_title() {
  Buffer<wchar_t> buf(10000);
  DWORD size = GetConsoleTitleW(buf.data(), static_cast<DWORD>(buf.size()));
  return wstring(buf.data(), size);
}

bool wait_for_single_object(HANDLE handle, DWORD timeout) {
  DWORD res = WaitForSingleObject(handle, timeout);
  CHECK_SYS(res != WAIT_FAILED);
  if (res == WAIT_OBJECT_0)
    return true;
  else if (res == WAIT_TIMEOUT)
    return false;
  else
    FAIL(E_FAIL);
}

wstring ansi_to_unicode(const string& str, unsigned code_page) {
  unsigned str_size = static_cast<unsigned>(str.size());
  if (str_size == 0)
    return wstring();
  int size = MultiByteToWideChar(code_page, 0, str.data(), str_size, nullptr, 0);
  Buffer<wchar_t> out(size);
  size = MultiByteToWideChar(code_page, 0, str.data(), str_size, out.data(), size);
  CHECK_SYS(size);
  return wstring(out.data(), size);
}

string unicode_to_ansi(const wstring& str, unsigned code_page) {
  unsigned str_size = static_cast<unsigned>(str.size());
  if (str_size == 0)
    return string();
  int size = WideCharToMultiByte(code_page, 0, str.data(), str_size, nullptr, 0, nullptr, nullptr);
  Buffer<char> out(size);
  size = WideCharToMultiByte(code_page, 0, str.data(), str_size, out.data(), size, nullptr, nullptr);
  CHECK_SYS(size);
  return string(out.data(), size);
}

wstring expand_env_vars(const wstring& str) {
  Buffer<wchar_t> buf(MAX_PATH);
  unsigned size = ExpandEnvironmentStringsW(str.c_str(), buf.data(), static_cast<DWORD>(buf.size()));
  if (size > buf.size()) {
    buf.resize(size);
    size = ExpandEnvironmentStringsW(str.c_str(), buf.data(), static_cast<DWORD>(buf.size()));
  }
  CHECK_SYS(size);
  return wstring(buf.data(), size - 1);
}

wstring get_full_path_name(const wstring& path) {
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.size()), buf.data(), nullptr);
  if (size > buf.size()) {
    buf.resize(size);
    size = GetFullPathNameW(path.c_str(), static_cast<DWORD>(buf.size()), buf.data(), nullptr);
  }
  CHECK_SYS(size);
  return wstring(buf.data(), size);
}

wstring get_current_directory() {
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD size = GetCurrentDirectoryW(static_cast<DWORD>(buf.size()), buf.data());
  if (size > buf.size()) {
    buf.resize(size);
    size = GetCurrentDirectoryW(static_cast<DWORD>(buf.size()), buf.data());
  }
  CHECK_SYS(size);
  return wstring(buf.data(), size);
}

File::File(): h_file(INVALID_HANDLE_VALUE) {
}

File::~File() {
  close();
}

File::File(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD dlags_and_attributes): h_file(INVALID_HANDLE_VALUE) {
  open(file_path, desired_access, share_mode, creation_disposition, dlags_and_attributes);
}

void File::open(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD dlags_and_attributes) {
  CHECK_SYS(open_nt(file_path, desired_access, share_mode, creation_disposition, dlags_and_attributes));
}

bool File::open_nt(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD flags_and_attributes) {
  close();
  h_file = CreateFileW(long_path(file_path).c_str(), desired_access, share_mode, nullptr, creation_disposition, flags_and_attributes, nullptr);
  return h_file != INVALID_HANDLE_VALUE;
}

void File::close() {
  if (h_file != INVALID_HANDLE_VALUE) {
    CloseHandle(h_file);
    h_file = INVALID_HANDLE_VALUE;
  }
}

HANDLE File::handle() const {
  return h_file;
}

unsigned __int64 File::size() {
  unsigned __int64 file_size;
  CHECK_SYS(size_nt(file_size));
  return file_size;
}

bool File::size_nt(unsigned __int64& file_size) {
  LARGE_INTEGER fs;
  if (GetFileSizeEx(h_file, &fs)) {
    file_size = fs.QuadPart;
    return true;
  }
  else
    return false;
}

unsigned File::read(void* data, unsigned size) {
  unsigned size_read;
  CHECK_SYS(read_nt(data, size, size_read));
  return size_read;
}

bool File::read_nt(void* data, unsigned size, unsigned& size_read) {
  DWORD sz;
  if (ReadFile(h_file, data, size, &sz, nullptr)) {
    size_read = sz;
    return true;
  }
  else
    return false;
}

unsigned File::write(const void* data, unsigned size) {
  unsigned size_written;
  CHECK_SYS(write_nt(data, size, size_written));
  return size_written;
}

bool File::write_nt(const void* data, unsigned size, unsigned& size_written) {
  DWORD sz;
  if (WriteFile(h_file, data, size, &sz, nullptr)) {
    size_written = sz;
    return true;
  }
  else
    return false;
}

void File::set_time(const FILETIME& ctime, const FILETIME& atime, const FILETIME& mtime) {
  CHECK_SYS(set_time_nt(ctime, atime, mtime));
};

bool File::set_time_nt(const FILETIME& ctime, const FILETIME& atime, const FILETIME& mtime) {
  return SetFileTime(h_file, &ctime, &atime, &mtime) != 0;
};

unsigned __int64 File::set_pos(__int64 offset, DWORD method) {
  unsigned __int64 new_pos;
  CHECK_SYS(set_pos_nt(offset, method, &new_pos));
  return new_pos;
}

bool File::set_pos_nt(__int64 offset, DWORD method, unsigned __int64* new_pos) {
  LARGE_INTEGER distance_to_move, new_file_pointer;
  distance_to_move.QuadPart = offset;
  if (!SetFilePointerEx(h_file, distance_to_move, &new_file_pointer, method))
    return false;
  if (new_pos)
    *new_pos = new_file_pointer.QuadPart;
  return true;
}

void File::set_end() {
  CHECK_SYS(set_end_nt());
}

bool File::set_end_nt() {
  return SetEndOfFile(h_file) != 0;
}

void Key::close() {
  if (h_key) {
    RegCloseKey(h_key);
    h_key = nullptr;
  }
}

Key::Key(): h_key(nullptr) {
}

Key::~Key() {
  close();
}

Key::Key(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create) {
  open(h_parent, sub_key, sam_desired, create);
}

Key& Key::open(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create) {
  close();
  CHECK_SYS(open_nt(h_parent, sub_key, sam_desired, create));
  return *this;
}

bool Key::open_nt(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create) {
  close();
  LONG res;
  if (create)
    res = RegCreateKeyExW(h_parent, sub_key, 0, nullptr, REG_OPTION_NON_VOLATILE, sam_desired, nullptr, &h_key, nullptr);
  else
    res = RegOpenKeyExW(h_parent, sub_key, 0, sam_desired, &h_key);
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  return true;
}

bool Key::query_bool(const wchar_t* name) {
  bool value;
  CHECK_SYS(query_bool_nt(value, name));
  return value;
}

bool Key::query_bool_nt(bool& value, const wchar_t* name) {
  DWORD type = REG_DWORD;
  DWORD data;
  DWORD data_size = sizeof(data);
  LONG res = RegQueryValueExW(h_key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&data), &data_size);
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  value = data != 0;
  return true;
}

unsigned Key::query_int(const wchar_t* name) {
  unsigned value;
  CHECK_SYS(query_int_nt(value, name));
  return value;
}

bool Key::query_int_nt(unsigned& value, const wchar_t* name) {
  DWORD type = REG_DWORD;
  DWORD data;
  DWORD data_size = sizeof(data);
  LONG res = RegQueryValueExW(h_key, name, nullptr, &type, reinterpret_cast<LPBYTE>(&data), &data_size);
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  value = data;
  return true;
}

wstring Key::query_str(const wchar_t* name) {
  wstring value;
  CHECK_SYS(query_str_nt(value, name));
  return value;
}

bool Key::query_str_nt(wstring& value, const wchar_t* name) {
  DWORD type = REG_SZ;
  DWORD data_size;
  LONG res = RegQueryValueExW(h_key, name, nullptr, &type, nullptr, &data_size);
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  Buffer<wchar_t> buf(data_size / sizeof(wchar_t));
  res = RegQueryValueExW(h_key, name, nullptr, &type, reinterpret_cast<LPBYTE>(buf.data()), &data_size);
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  value.assign(buf.data(), buf.size() - 1);
  return true;
}

void Key::set_bool(const wchar_t* name, bool value) {
  CHECK_SYS(set_bool_nt(name, value));
}

bool Key::set_bool_nt(const wchar_t* name, bool value) {
  DWORD data = value ? 1 : 0;
  LONG res = RegSetValueExW(h_key, name, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&data), sizeof(data));
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  return true;
}

void Key::set_int(const wchar_t* name, unsigned value) {
  CHECK_SYS(set_int_nt(name, value));
}

bool Key::set_int_nt(const wchar_t* name, unsigned value) {
  DWORD data = value;
  LONG res = RegSetValueExW(h_key, name, 0, REG_DWORD, reinterpret_cast<LPBYTE>(&data), sizeof(data));
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  return true;
}

void Key::set_str(const wchar_t* name, const wstring& value) {
  CHECK_SYS(set_str_nt(name, value));
}

bool Key::set_str_nt(const wchar_t* name, const wstring& value) {
  LONG res = RegSetValueExW(h_key, name, 0, REG_SZ, reinterpret_cast<LPBYTE>(const_cast<wchar_t*>(value.c_str())), (static_cast<DWORD>(value.size()) + 1) * sizeof(wchar_t));
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  return true;
}

void Key::delete_value(const wchar_t* name) {
  CHECK_SYS(delete_value_nt(name));
}

bool Key::delete_value_nt(const wchar_t* name) {
  LONG res = RegDeleteValueW(h_key, name);
  if (res != ERROR_SUCCESS) {
    SetLastError(res);
    return false;
  }
  return true;
}

FileEnum::FileEnum(const wstring& dir_path): dir_path(dir_path), h_find(INVALID_HANDLE_VALUE) {
}

FileEnum::~FileEnum() {
  if (h_find != INVALID_HANDLE_VALUE)
    FindClose(h_find);
}

bool FileEnum::next() {
  bool more;
  CHECK_SYS(next_nt(more));
  return more;
}

bool FileEnum::next_nt(bool& more) {
  while (true) {
    if (h_find == INVALID_HANDLE_VALUE) {
      h_find = FindFirstFileW(long_path(add_trailing_slash(dir_path) + L'*').c_str(), &find_data);
      if (h_find == INVALID_HANDLE_VALUE)
        return false;
    }
    else {
      if (!FindNextFileW(h_find, &find_data)) {
        if (GetLastError() == ERROR_NO_MORE_FILES) {
          more = false;
          return true;
        }
        return false;
      }
    }
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if ((find_data.cFileName[0] == L'.') && ((find_data.cFileName[1] == 0) || ((find_data.cFileName[1] == L'.') && (find_data.cFileName[2] == 0))))
        continue;
    }
    more = true;
    return true;
  }
}

FindData get_find_data(const wstring& path) {
  FindData find_data;
  HANDLE h_find = FindFirstFileW(long_path(path).c_str(), &find_data);
  CHECK_SYS(h_find != INVALID_HANDLE_VALUE);
  FindClose(h_find);
  return find_data;
}

TempFile::TempFile() {
  Buffer<wchar_t> buf(MAX_PATH);
  DWORD len = GetTempPathW(static_cast<DWORD>(buf.size()), buf.data());
  CHECK(len <= buf.size());
  CHECK_SYS(len);
  wstring temp_path = wstring(buf.data(), len);
  CHECK_SYS(GetTempFileNameW(temp_path.c_str(), L"", 0, buf.data()));
  path.assign(buf.data());
}

TempFile::~TempFile() {
  DeleteFileW(path.c_str());
}

unsigned __stdcall Thread::thread_proc(void* arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);
  try {
    try {
      thread->run();
      return TRUE;
    }  
    catch (const Error& e) {
      thread->error = e;
    }
    catch (const std::exception& e) {
      thread->error = e;
    }
  }
  catch (...) {
    thread->error.code = E_FAIL;
  }
  return FALSE;
}

Thread::Thread(): h_thread(nullptr) {
}

Thread::~Thread() {
  if (h_thread) {
    wait(INFINITE);
    CloseHandle(h_thread);
  }
}

void Thread::start() {
  unsigned th_id;
  h_thread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, thread_proc, this, 0, &th_id));
  CHECK_SYS(h_thread);
}

bool Thread::wait(unsigned wait_time) {
  return wait_for_single_object(h_thread, wait_time);
}

bool Thread::get_result() {
  DWORD exit_code;
  CHECK_SYS(GetExitCodeThread(h_thread, &exit_code));
  return exit_code == TRUE ? true : false;
}

Event::Event(bool manual_reset, bool initial_state) {
  h_event = CreateEvent(nullptr, manual_reset, initial_state, nullptr);
  CHECK_SYS(h_event);
}

Event::~Event() {
  CloseHandle(h_event);
}

void Event::set() {
  CHECK_SYS(SetEvent(h_event));
}

WindowClass::WindowClass(const wstring& name, WindowProc window_proc): name(name) {
  WNDCLASSW wndclass;
  memset(&wndclass, 0, sizeof(wndclass));
  wndclass.lpfnWndProc = window_proc;
  wndclass.cbWndExtra = sizeof(this);
  wndclass.hInstance = g_h_instance;
  wndclass.lpszClassName = name.c_str();
  CHECK_SYS(RegisterClassW(&wndclass));
}

WindowClass::~WindowClass() {
  UnregisterClassW(name.c_str(), nullptr);
}

LRESULT CALLBACK MessageWindow::message_window_proc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  try {
    MessageWindow* message_window = reinterpret_cast<MessageWindow*>(GetWindowLongPtrW(h_wnd, 0));
    if (message_window) return message_window->window_proc(msg, w_param, l_param);
  }
  catch (...) {
  }
  return DefWindowProcW(h_wnd, msg, w_param, l_param);
}

MessageWindow::MessageWindow(const wstring& name): WindowClass(name, message_window_proc) {
  h_wnd = CreateWindowW(name.c_str(), name.c_str(), 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, g_h_instance, this);
  CHECK_SYS(h_wnd);
  SetWindowLongPtrW(h_wnd, 0, reinterpret_cast<LONG_PTR>(this));
}

MessageWindow::~MessageWindow() {
  SetWindowLongPtrW(h_wnd, 0, 0);
  DestroyWindow(h_wnd);
}

unsigned MessageWindow::message_loop(HANDLE h_abort) {
  while (true) {
    DWORD res = MsgWaitForMultipleObjects(1, &h_abort, FALSE, INFINITE, QS_POSTMESSAGE | QS_SENDMESSAGE);
    CHECK_SYS(res != WAIT_FAILED);
    if (res == WAIT_OBJECT_0) {
      FAIL(E_ABORT);
    }
    else if (res == WAIT_OBJECT_0 + 1) {
      MSG msg;
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
          return static_cast<unsigned>(msg.wParam);
      }
    }
    else FAIL(E_FAIL);
  }
}

void MessageWindow::end_message_loop(unsigned result) {
  PostQuitMessage(result);
}

Icon::Icon(HMODULE h_module, WORD icon_id, int width, int height) {
  h_icon = static_cast<HICON>(LoadImage(h_module, MAKEINTRESOURCE(icon_id), IMAGE_ICON, width, height, LR_DEFAULTCOLOR));
  CHECK_SYS(h_icon);
}

Icon::~Icon() {
  DestroyIcon(h_icon);
}

wstring format_file_time(const FILETIME& file_time) {
  FILETIME local_ft;
  CHECK_SYS(FileTimeToLocalFileTime(&file_time, &local_ft));
  SYSTEMTIME st;
  CHECK_SYS(FileTimeToSystemTime(&local_ft, &st));
  Buffer<wchar_t> buf(1024);
  CHECK_SYS(GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, buf.data(), static_cast<int>(buf.size())));
  wstring date_str = buf.data();
  CHECK_SYS(GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, nullptr, buf.data(), static_cast<int>(buf.size())));
  wstring time_str = buf.data();
  return date_str + L' ' + time_str;
}

wstring upcase(const wstring& str) {
  Buffer<wchar_t> up_str(str.size());
  wmemcpy(up_str.data(), str.data(), str.size());
  CharUpperBuffW(up_str.data(), static_cast<DWORD>(up_str.size()));
  return wstring(up_str.data(), up_str.size());
}
