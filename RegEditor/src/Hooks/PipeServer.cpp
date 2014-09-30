
// Minimum version - Win2000
//#undef WINVER
//#define WINVER 0x0500
//#undef _WIN32_WINNT
//#define _WIN32_WINNT 0x0500


#include <Windows.h>
#include <Tlhelp32.h>
#include <commctrl.h>
#include <tchar.h>
#ifdef _DEBUG
	#include <crtdbg.h>
#else
	#define _ASSERT(x)
	#define _ASSERTE(x)
#endif

#define REGEDIT_INPROCESS
#include "RegEditExe.h"


#define NEED_REGEDIT_STYLE (WS_VISIBLE|WS_THICKFRAME|WS_CAPTION|WS_MAXIMIZEBOX)


#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define INSTANCES 1
#define PIPE_TIMEOUT 5000
#define INBUFSIZE  64*1024 // Размер в WCHAR's
#define OUTBUFSIZE 1024    // Обратно просто код возврата

#include <PshPack4.h>

typedef struct tag_OutData
{
	DWORD cbDataSize; // текущая длина в БАЙТАХ
	int   nResult;    // результат выполнения SelectTreeItem
	DWORD nErrCode;   // GetLastError()
} OutData;

typedef struct 
{ 
	HANDLE hPipeInst; 
	OVERLAPPED oOverlap; 
	WCHAR chRequest[INBUFSIZE]; 
	DWORD cbRead;
	//WCHAR chReply[OUTBUFSIZE];
	DWORD nMaxOutLen;
	OutData Reply;
	DWORD cbToWrite;
	DWORD dwState; 
	BOOL fPendingIO; 
} PIPEINST, *LPPIPEINST; 

#include <PopPack.h>


VOID DisconnectAndReconnect(DWORD); 
BOOL ConnectToNewClient(HANDLE, LPOVERLAPPED); 
VOID GetAnswerToRequest(LPPIPEINST); 
int SelectServerInit();
int SelectServerDone();

HANDLE hQuitEvent;
PIPEINST Pipe[INSTANCES];
HANDLE hEvents[INSTANCES+1];
HANDLE hPipe;
HANDLE hThreadHandle;
DWORD dwThreadID;
wchar_t sPipeName[MAX_PATH];
HWND hParent, hTree, hList;
bool bQuit = false;
wchar_t sLastErrorMsg[512];
WNDPROC lpPrevParentProc;
UINT nSetFocusMsg;



SECURITY_ATTRIBUTES* GetLocalSecurity()
{
	static PSECURITY_DESCRIPTOR pLocalDesc = NULL;
	static SECURITY_ATTRIBUTES  LocalSecurity = {0};
	static PACL pACL = NULL;
	//static PSID pNetworkSid = NULL, pSidWorld = NULL;
	static DWORD LastError = 0;
	DWORD dwSize = 0;


	if (pLocalDesc) {
		_ASSERTE(LocalSecurity.lpSecurityDescriptor==pLocalDesc);
		return (&LocalSecurity);
	}
	pLocalDesc = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH); 

	if (pLocalDesc == NULL) {
		LastError = GetLastError();
		return NULL;
	}

	if (!InitializeSecurityDescriptor(pLocalDesc, SECURITY_DESCRIPTOR_REVISION)) {
		LastError = GetLastError();
		LocalFree(pLocalDesc); pLocalDesc = NULL;
		return NULL;
	}

	// Создать SID для Network & Everyone
	BYTE NetworkSidSrc[] = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x02, 0x00, 0x00, 0x00};
	BYTE WorldSidSrc[]   = {0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
	PSID pNetworkSid = (PSID)NetworkSidSrc;
	PSID pSidWorld = (PSID)WorldSidSrc;
//	dwSize = SECURITY_MAX_SID_SIZE;
//	pNetworkSid = (PSID)LocalAlloc(LMEM_FIXED, dwSize);
//	pSidWorld   = (PSID)LocalAlloc(LMEM_FIXED, dwSize);
//	if (!pNetworkSid || !pSidWorld) {
//		LastError = GetLastError();
//		return NULL;
//	}
//	dwSize = SECURITY_MAX_SID_SIZE;
//	if (!CreateWellKnownSid(WinNetworkSid, NULL, pNetworkSid, &dwSize)) {
//		LastError = GetLastError();
//		return NULL;
//	}
//#ifdef _DEBUG
//	//12: 01 01 00 00 00 00 00 05 02 00 00 00
//	DWORD nNetLen = GetLengthSid(pNetworkSid);
//#endif
//	dwSize = SECURITY_MAX_SID_SIZE;
//	if (!CreateWellKnownSid(WinWorldSid, NULL, pSidWorld, &dwSize)) {
//		LastError = GetLastError();
//		return NULL;
//	}
//#ifdef _DEBUG
//	//12: 01 01 00 00 00 00 00 01 00 00 00 00
//	DWORD nWorldLen = GetLengthSid(pSidWorld);
//#endif

	// Создать DACL
	dwSize = sizeof(ACL) 
		+ sizeof(ACCESS_DENIED_ACE) + GetLengthSid(pNetworkSid)
		+ sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSidWorld);
	pACL = (PACL)LocalAlloc(LPTR, dwSize);
	if (!InitializeAcl(pACL, dwSize, ACL_REVISION)) {
		LastError = GetLastError();
		return NULL;
	}
	// Теперь - собственно права
	if (!AddAccessDeniedAce(pACL, ACL_REVISION, GENERIC_ALL, pNetworkSid)) {
		LastError = GetLastError();
		return NULL;
	}
	if (!AddAccessAllowedAce(pACL, ACL_REVISION, GENERIC_ALL, pSidWorld)) {
		LastError = GetLastError();
		return NULL;
	}


	// Add a null DACL to the security descriptor. 
	if (!SetSecurityDescriptorDacl(pLocalDesc, TRUE, pACL, FALSE)) {
		LastError = GetLastError();
		LocalFree(pLocalDesc); pLocalDesc = NULL;
		return NULL;
	}

	LocalSecurity.nLength = sizeof(LocalSecurity);
	LocalSecurity.lpSecurityDescriptor = pLocalDesc;
	LocalSecurity.bInheritHandle = TRUE; 

	return (&LocalSecurity);
};



int InitializePipes()
{ 
	DWORD i;

	// The initial loop creates several instances of a named pipe 
	// along with an event object for each instance.  An 
	// overlapped ConnectNamedPipe operation is started for 
	// each instance. 

	for (i = 0; i < INSTANCES; i++) 
	{ 

		// Create an event object for this instance. 

		if (hEvents[i] == NULL)
			hEvents[i] = CreateEvent( 
				NULL,    // default security attribute 
				TRUE,    // manual-reset event 
				TRUE,    // initial state = signaled 
				NULL);   // unnamed event object 

		if (hEvents[i] == NULL) 
		{
			//printf("CreateEvent failed with %d.\n", GetLastError()); 
			return -100;
		}

		Pipe[i].oOverlap.hEvent = hEvents[i]; 

		Pipe[i].hPipeInst = CreateNamedPipe( 
			sPipeName,               // pipe name 
			PIPE_ACCESS_DUPLEX |     // read/write access 
			FILE_FLAG_OVERLAPPED,    // overlapped mode 
			PIPE_TYPE_MESSAGE |      // message-type pipe 
			PIPE_READMODE_MESSAGE |  // message-read mode 
			PIPE_WAIT,               // blocking mode 
			INSTANCES,               // number of instances 
			INBUFSIZE*sizeof(WCHAR), // output buffer size 
			OUTBUFSIZE*sizeof(WCHAR),// input buffer size 
			PIPE_TIMEOUT,            // client time-out 
			GetLocalSecurity());     // Access only from local computer

		if (Pipe[i].hPipeInst == INVALID_HANDLE_VALUE) 
		{
			//printf("CreateNamedPipe failed with %d.\n", GetLastError());
			return -101;
		}

		// Call the subroutine to connect to the new client

		Pipe[i].fPendingIO = ConnectToNewClient( 
			Pipe[i].hPipeInst, 
			&Pipe[i].oOverlap); 

		Pipe[i].dwState = Pipe[i].fPendingIO ? 
CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read 
	} 

	return 0;
}

int ProcessPipes()
{
	DWORD i, dwWait, cbRet, dwErr; 
	BOOL fSuccess; 

	while (!bQuit)
	{ 
		// Wait for the event object to be signaled, indicating 
		// completion of an overlapped read, write, or 
		// connect operation. 

		dwWait = WaitForMultipleObjects( 
			INSTANCES+1,  // number of event objects 
			hEvents,      // array of event objects 
			FALSE,        // does not wait for all 
			INFINITE);    // waits indefinitely 

		// dwWait shows which pipe completed the operation. 

		i = dwWait - WAIT_OBJECT_0;  // determines which pipe 
		if (i < 0 || i >= INSTANCES) 
		{
			// Если i == INSTANCES, это значит что затребовано завершение нитей
			return -1;
		}

		// Get the result if the operation was pending. 

		if (Pipe[i].fPendingIO) 
		{ 
			fSuccess = GetOverlappedResult( 
				Pipe[i].hPipeInst, // handle to pipe 
				&Pipe[i].oOverlap, // OVERLAPPED structure 
				&cbRet,            // bytes transferred 
				FALSE);            // do not wait 

			switch (Pipe[i].dwState) 
			{ 
				// Pending connect operation 
			case CONNECTING_STATE: 
				if (! fSuccess) 
				{
					dwErr = GetLastError(); 
					//printf("Error %d.\n", GetLastError()); 
					_ASSERTE(fSuccess);
					return -1;
				}
				Pipe[i].dwState = READING_STATE; 
				break; 

				// Pending read operation 
			case READING_STATE: 
				if (! fSuccess || cbRet == 0) 
				{ 
					DisconnectAndReconnect(i); 
					continue; 
				} 
				Pipe[i].dwState = WRITING_STATE; 
				break; 

				// Pending write operation 
			case WRITING_STATE: 
				if (! fSuccess || cbRet != Pipe[i].cbToWrite) 
				{ 
					DisconnectAndReconnect(i); 
					continue; 
				} 
				Pipe[i].dwState = READING_STATE; 
				break; 

			default: 
				{
					//printf("Invalid pipe state.\n"); 
					_ASSERTE(Pipe[i].dwState<=WRITING_STATE);
					return -1;
				}
			}  
		} 

		// The pipe state determines which operation to do next. 

		switch (Pipe[i].dwState) 
		{ 
			// READING_STATE: 
			// The pipe instance is connected to the client 
			// and is ready to read a request from the client. 

		case READING_STATE: 
			fSuccess = ReadFile( 
				Pipe[i].hPipeInst, 
				Pipe[i].chRequest, 
				INBUFSIZE*sizeof(WCHAR), 
				&Pipe[i].cbRead, 
				&Pipe[i].oOverlap); 

			// The read operation completed successfully. 

			if (fSuccess && Pipe[i].cbRead != 0) 
			{ 
				Pipe[i].fPendingIO = FALSE; 
				Pipe[i].dwState = WRITING_STATE; 
				continue; 
			} 

			// The read operation is still pending. 

			dwErr = GetLastError(); 
			if (! fSuccess && (dwErr == ERROR_IO_PENDING)) 
			{ 
				Pipe[i].fPendingIO = TRUE; 
				continue; 
			} 

			// An error occurred; disconnect from the client. 

			DisconnectAndReconnect(i); 
			break; 

			// WRITING_STATE: 
			// The request was successfully read from the client. 
			// Get the reply data and write it to the client. 

		case WRITING_STATE: 
			GetAnswerToRequest(&Pipe[i]); 

			fSuccess = WriteFile( 
				Pipe[i].hPipeInst, 
				&(Pipe[i].Reply), 
				Pipe[i].cbToWrite, 
				&cbRet, 
				&Pipe[i].oOverlap); 

			// The write operation completed successfully. 

			if (fSuccess && cbRet == Pipe[i].cbToWrite) 
			{ 
				Pipe[i].fPendingIO = FALSE; 
				Pipe[i].dwState = READING_STATE; 
				continue; 
			} 

			// The write operation is still pending. 

			dwErr = GetLastError(); 
			if (! fSuccess && (dwErr == ERROR_IO_PENDING)) 
			{ 
				Pipe[i].fPendingIO = TRUE; 
				continue; 
			} 

			// An error occurred; disconnect from the client. 

			DisconnectAndReconnect(i); 
			break; 

		default: 
			{
				//printf("Invalid pipe state.\n"); 
				_ASSERTE(Pipe[i].dwState<=WRITING_STATE);
				return -1;
			}
		} 
	} 

	// По идее, из цикла мы никогда не выйдем.
	// Если затребовано завершение нити - сразу идет "return -1;"
	return -1;
}


// DisconnectAndReconnect(DWORD) 
// This function is called when an error occurs or when the client 
// closes its handle to the pipe. Disconnect from this client, then 
// call ConnectNamedPipe to wait for another client to connect. 

VOID DisconnectAndReconnect(DWORD i)
{ 
	// Disconnect the pipe instance. 

	if (! DisconnectNamedPipe(Pipe[i].hPipeInst) ) 
	{
		wsprintf(sLastErrorMsg, L"DisconnectNamedPipe failed with %d.\n", GetLastError());
	}

	// Call a subroutine to connect to the new client. 

	Pipe[i].fPendingIO = ConnectToNewClient( 
		Pipe[i].hPipeInst, 
		&Pipe[i].oOverlap); 

	Pipe[i].dwState = Pipe[i].fPendingIO ? 
		CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read 
} 

// ConnectToNewClient(HANDLE, LPOVERLAPPED) 
// This function is called to start an overlapped connect operation. 
// It returns TRUE if an operation is pending or FALSE if the 
// connection has been completed. 

BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo) 
{ 
	BOOL fConnected, fPendingIO = FALSE; 

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, lpo); 

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected) 
	{
		wsprintf(sLastErrorMsg, L"ConnectNamedPipe failed with %d.\n", GetLastError()); 
		return 0;
	}

	switch (GetLastError()) 
	{ 
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING: 
		fPendingIO = TRUE; 
		break; 

		// Client is already connected, so signal an event. 

	case ERROR_PIPE_CONNECTED: 
		if (SetEvent(lpo->hEvent)) 
			break; 

		// If an error occurs during the connect operation... 
	default: 
		{
			wsprintf(sLastErrorMsg, L"ConnectNamedPipe failed with %d.\n", GetLastError());
			return 0;
		}
	} 

	return fPendingIO; 
}


LRESULT CALLBACK ParentWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == nSetFocusMsg)
	{
		SetFocus((HWND)lParam);
		return 0; // в RegEdit не пересылаем - это чисто наше
	}
	else if (uMsg == WM_DESTROY)
	{
		SelectServerDone();
	}
	// Вернуть в RegEdit
	return CallWindowProc(lpPrevParentProc, hwnd, uMsg, wParam, lParam);
}

VOID GetAnswerToRequest(LPPIPEINST pipe)
{
	int iRc = 0;
	
	wchar_t* pszKeyPath = pipe->chRequest;
	int nLen = lstrlenW(pszKeyPath);
	wchar_t* pszValueName = pszKeyPath+nLen+1;
	if (*pszValueName == 1)
		pszValueName++;
	else
		pszValueName = NULL;
	
	// Запускаем выделялку
	iRc = SelectTreeItem(GetCurrentProcess(), hParent, hTree, hList, pszKeyPath, pszValueName);
	
	pipe->Reply.cbDataSize = sizeof(pipe->Reply);
	pipe->Reply.nResult = iRc;
	pipe->Reply.nErrCode = GetLastError();
	pipe->cbToWrite = sizeof(pipe->Reply);
}



DWORD WINAPI ServerThread(LPVOID apParm)
{
	// Создать пайп-instance
	int nInitRc = InitializePipes();
	if (nInitRc != 0) {
		DWORD nErrCode = GetLastError();
		wchar_t szErrInfo[512]; wsprintfW(szErrInfo, L"Can't initialize pipe!\n%s\nErrCode=0x%08X", sPipeName, nErrCode);
		MessageBoxW(NULL, szErrInfo, L"RegEditor (Far plugin)", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_OK);
		return nInitRc;
	}
	hEvents[INSTANCES] = hQuitEvent;

	// Поехали.
	while (!bQuit) {
		ProcessPipes();
	}

	// Выходим
	CloseHandle(hThreadHandle); hThreadHandle = NULL;
	return 0;
}


int SelectServerInit()
{
	hQuitEvent = NULL;
	memset(Pipe, 0, sizeof(Pipe));
	memset(hEvents, 0, sizeof(hEvents));
	hPipe = NULL;
	hThreadHandle = NULL;
	dwThreadID = 0;
	bQuit = false;
	sLastErrorMsg[0] = 0;
	DWORD nRegeditPID = GetCurrentProcessId();

	// hParent = ahParent; hTree = ahTree; hList = ahList;
	// Найти окно RegEdit!
	hParent = hTree = hList = NULL;
	while ((hParent = FindWindowEx(NULL, hParent, _T("RegEdit_RegEdit"), NULL)) != NULL)
	{
		DWORD dwPID = 0;
		if (nRegeditPID && GetWindowThreadProcessId(hParent, &dwPID) && dwPID != nRegeditPID)
			continue;
		
		DWORD dwStyle = (DWORD)GetWindowLongPtr(hParent, GWL_STYLE);
		if ((dwStyle & NEED_REGEDIT_STYLE) == NEED_REGEDIT_STYLE)
		{
			hTree = FindWindowEx(hParent, NULL, _T("SysTreeView32"), NULL);
			hList = FindWindowEx(hParent, NULL, _T("SysListView32"), NULL);
			if (hTree && hList)
				break;
		}
	}
	if (!hParent)
	{
		return -100;
	}
	
	WNDPROC lpProc = (WNDPROC)GetWindowLongPtr(hParent, GWLP_WNDPROC);
	if (lpProc != ParentWindowProc)
	{
		lpPrevParentProc = lpProc;
		nSetFocusMsg = RegisterWindowMessage(L"FarRegEditor.SetFocus");
		SetWindowLongPtr(hParent, GWLP_WNDPROC, (LONG_PTR)ParentWindowProc);
	}
	
	
	wsprintfW(sPipeName, L"\\\\.\\pipe\\FarRegEditor.%u", GetCurrentProcessId());

	hQuitEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	ResetEvent(hQuitEvent); // на всякий

	
	// Теперь - запускаем нить
	hThreadHandle = CreateThread(NULL, 0, ServerThread, NULL, 0, &dwThreadID);
	if (!hThreadHandle) {
		DWORD nErrCode = GetLastError();
		wchar_t szErrInfo[512]; wsprintfW(szErrInfo, L"Can't start monitoring thread! ErrCode=0x%08X", nErrCode);
		MessageBoxW(NULL, szErrInfo, L"RegEditor (Far plugin)", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_OK);
		// Закрыть все что можно
		SelectServerDone();
		return -1;
	}

	return 0;
}

int SelectServerDone()
{
	bQuit = true;
	if (hQuitEvent) SetEvent(hQuitEvent);

	if (hThreadHandle) {
		DWORD nWait = WaitForSingleObject(hThreadHandle, 500);
		if (nWait != WAIT_OBJECT_0 && hThreadHandle) {
			TerminateThread(hThreadHandle, 100);
			if (hThreadHandle) {
				CloseHandle(hThreadHandle); hThreadHandle = NULL;
			}
		}
	}

	if (hQuitEvent) {
		CloseHandle(hQuitEvent); hQuitEvent = NULL;
	}

	for (int i = 0; i < INSTANCES; i++) {
		if (Pipe[i].hPipeInst) {
			CloseHandle(Pipe[i].hPipeInst);
			Pipe[i].hPipeInst = NULL;
		}
	}

	return 0;
}
