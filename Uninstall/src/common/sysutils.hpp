#pragma once

#define nullptr NULL

#ifdef DEBUG
  #define DEBUG_OUTPUT(msg) OutputDebugStringW(((msg) + L"\n").c_str())
#else
  #define DEBUG_OUTPUT(msg)
#endif

extern HINSTANCE g_h_instance;

wstring get_system_message(HRESULT hr, DWORD lang_id = 0);
wstring get_console_title();
bool wait_for_single_object(HANDLE handle, DWORD timeout);
wstring ansi_to_unicode(const string& str, unsigned code_page);
string unicode_to_ansi(const wstring& str, unsigned code_page);
wstring expand_env_vars(const wstring& str);
wstring get_full_path_name(const wstring& path);
wstring get_current_directory();

class CriticalSection: private NonCopyable, private CRITICAL_SECTION {
public:
  CriticalSection() {
    InitializeCriticalSection(this);
  }
  virtual ~CriticalSection() {
    DeleteCriticalSection(this);
  }
  friend class CriticalSectionLock;
};

class CriticalSectionLock: private NonCopyable {
private:
  CriticalSection& cs;
public:
  CriticalSectionLock(CriticalSection& cs): cs(cs) {
    EnterCriticalSection(&cs);
  }
  ~CriticalSectionLock() {
    LeaveCriticalSection(&cs);
  }
};

class File: private NonCopyable {
protected:
  HANDLE h_file;
public:
  File() throw();
  ~File() throw();
  File(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD flags_and_attributes);
  void open(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD flags_and_attributes);
  bool open_nt(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD flags_and_attributes) throw();
  void close() throw();
  HANDLE handle() const throw();
  unsigned __int64 size();
  bool size_nt(unsigned __int64& file_size) throw();
  unsigned read(void* data, unsigned size);
  bool read_nt(void* data, unsigned size, unsigned& size_read) throw();
  unsigned write(const void* data, unsigned size);
  bool write_nt(const void* data, unsigned size, unsigned& size_written) throw();
  void set_time(const FILETIME& ctime, const FILETIME& atime, const FILETIME& mtime);
  bool set_time_nt(const FILETIME& ctime, const FILETIME& atime, const FILETIME& mtime) throw();
  unsigned __int64 set_pos(__int64 offset, DWORD method = FILE_BEGIN);
  bool set_pos_nt(__int64 offset, DWORD method = FILE_BEGIN, unsigned __int64* new_pos = nullptr);
  void set_end();
  bool set_end_nt();
};

class Key: private NonCopyable {
protected:
  HKEY h_key;
public:
  Key() throw();
  ~Key() throw();
  Key(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create);
  Key& open(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create);
  bool open_nt(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create) throw();
  void close() throw();
  HKEY handle() const throw();
  bool query_bool(const wchar_t* name);
  bool query_bool_nt(bool& value, const wchar_t* name) throw();
  unsigned query_int(const wchar_t* name);
  bool query_int_nt(unsigned& value, const wchar_t* name) throw();
  wstring query_str(const wchar_t* name);
  bool query_str_nt(wstring& value, const wchar_t* name) throw();
  void set_bool(const wchar_t* name, bool value);
  bool set_bool_nt(const wchar_t* name, bool value) throw();
  void set_int(const wchar_t* name, unsigned value);
  bool set_int_nt(const wchar_t* name, unsigned value) throw();
  void set_str(const wchar_t* name, const wstring& value);
  bool set_str_nt(const wchar_t* name, const wstring& value) throw();
  void delete_value(const wchar_t* name);
  bool delete_value_nt(const wchar_t* name) throw();
};

struct FindData: public WIN32_FIND_DATAW {
  bool is_dir() const {
    return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  unsigned __int64 size() const {
    return (static_cast<unsigned __int64>(nFileSizeHigh) << 32) | nFileSizeLow;
  }
};

class FileEnum: private NonCopyable {
protected:
  wstring dir_path;
  HANDLE h_find;
  FindData find_data;
public:
  FileEnum(const wstring& dir_path) throw();
  ~FileEnum() throw();
  bool next();
  bool next_nt(bool& more) throw();
  const FindData& data() const {
    return find_data;
  }
};

FindData get_find_data(const wstring& path);

class TempFile: private NonCopyable {
private:
  wstring path;
public:
  TempFile();
  ~TempFile();
  wstring get_path() const {
    return path;
  }
};

class Thread: private NonCopyable {
private:
  Error error;
  static unsigned __stdcall thread_proc(void* arg);
protected:
  HANDLE h_thread;
  virtual void run() = 0;
public:
  Thread();
  virtual ~Thread();
  void start();
  bool wait(unsigned wait_time);
  bool get_result();
  Error get_error() {
    return error;
  }
};

class Event: private NonCopyable {
protected:
  HANDLE h_event;
public:
  Event(bool manual_reset, bool initial_state);
  ~Event();
  void set();
};

typedef LRESULT (CALLBACK *WindowProc)(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);

class WindowClass: private NonCopyable {
protected:
  wstring name;
public:
  WindowClass(const wstring& name, WindowProc window_proc);
  virtual ~WindowClass();
};

class MessageWindow: public WindowClass {
private:
  static LRESULT CALLBACK message_window_proc(HWND h_wnd, UINT msg, WPARAM w_param, LPARAM l_param);
protected:
  HWND h_wnd;
  virtual LRESULT window_proc(UINT msg, WPARAM w_param, LPARAM l_param) = 0;
  void end_message_loop(unsigned result);
public:
  MessageWindow(const wstring& name);
  virtual ~MessageWindow();
  unsigned message_loop(HANDLE h_abort);
};

class Icon: private NonCopyable {
protected:
  HICON h_icon;
public:
  Icon(HMODULE h_module, WORD icon_id, int width, int height);
  ~Icon();
};

wstring format_file_time(const FILETIME& file_time);
wstring upcase(const wstring& str);
