// ResolveDlg.h : header file
//

#if !defined(AFX_RESOLVEDLG_H__077CFA6D_0F74_11D7_9558_204C4F4F5020__INCLUDED_)
#define AFX_RESOLVEDLG_H__077CFA6D_0F74_11D7_9558_204C4F4F5020__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if 0
// VC 6.0
#define DEFAULT_PATH "C:\\MSVS\\VC98\\Bin;C:\\MSVS\\Common\\MSDev98\\Bin"
#define DEFAULT_INCPATH "C:\\MSVS\\MsSDK\\include;C:\\MSVS\\VC98\\atl\\include;C:\\MSVS\\VC98\\mfc\\include;C:\\MSVS\\VC98\\include"
#define DEFAULT_LIBPATH "C:\\MSVS\\MsSDK\\Lib;C:\\MSVS\\VC98\\mfc\\lib;C:\\MSVS\\VC98\\lib"
#endif

#if 0
// VC 9.0
#define DEFAULT_PATH "%ProgramFiles%\\Microsoft SDKs\\Windows\\v7.0\\Bin;%ProgramFiles%\\Microsoft Visual Studio 9.0\\VC\\bin;%ProgramFiles%\\Microsoft Visual Studio 9.0\\Common7\\IDE"
#define DEFAULT_INCPATH "%ProgramFiles%\\Microsoft SDKs\\Windows\\v7.0\\Include;%ProgramFiles%\\Microsoft Visual Studio 9.0\\VC\\include;%ProgramFiles%\\Microsoft Visual Studio 9.0\\VC\\atlmfc\\include"
#define DEFAULT_LIBPATH "%ProgramFiles%\\Microsoft SDKs\\Windows\\v7.0\\Lib;%ProgramFiles%\\Microsoft Visual Studio 9.0\\VC\\lib;%ProgramFiles%\\Microsoft Visual Studio 9.0\\VC\\atlmfc\\lib"

#define DEFAULT_PATH_WOW "%ProgramW6432%\\Microsoft SDKs\\Windows\\v7.0\\Bin;%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\\VC\\bin;%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\\Common7\\IDE"
#define DEFAULT_INCPATH_WOW "%ProgramW6432%\\Microsoft SDKs\\Windows\\v7.0\\Include;%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\\VC\\include;%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\\VC\\atlmfc\\include"
#define DEFAULT_LIBPATH_WOW "%ProgramW6432%\\Microsoft SDKs\\Windows\\v7.0\\Lib;%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\\VC\\lib;%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\\VC\\atlmfc\\lib"
#endif

#define DEFAULT_PATH "%ProgramFiles%\\Microsoft SDKs\\Windows\\v7.1\\Bin;%ProgramFiles%\\Microsoft Visual Studio 10.0\\VC\\bin;%ProgramFiles%\\Microsoft Visual Studio 10.0\\Common7\\IDE"
#define DEFAULT_INCPATH "%ProgramFiles%\\Microsoft SDKs\\Windows\\v7.1\\Include;%ProgramFiles%\\Microsoft Visual Studio 10.0\\VC\\include;%ProgramFiles%\\Microsoft Visual Studio 10.0\\VC\\atlmfc\\include"
#define DEFAULT_LIBPATH "%ProgramFiles%\\Microsoft SDKs\\Windows\\v7.1\\Lib;%ProgramFiles%\\Microsoft Visual Studio 10.0\\VC\\lib;%ProgramFiles%\\Microsoft Visual Studio 10.0\\VC\\atlmfc\\lib"

#define DEFAULT_PATH_WOW "%ProgramW6432%\\Microsoft SDKs\\Windows\\v7.1\\Bin;%ProgramFiles(x86)%\\Microsoft Visual Studio 10.0\\VC\\bin;%ProgramFiles(x86)%\\Microsoft Visual Studio 10.0\\Common7\\IDE"
#define DEFAULT_INCPATH_WOW "%ProgramW6432%\\Microsoft SDKs\\Windows\\v7.1\\Include;%ProgramFiles(x86)%\\Microsoft Visual Studio 10.0\\VC\\include;%ProgramFiles(x86)%\\Microsoft Visual Studio 10.0\\VC\\atlmfc\\include"
#define DEFAULT_LIBPATH_WOW "%ProgramW6432%\\Microsoft SDKs\\Windows\\v7.1\\Lib;%ProgramFiles(x86)%\\Microsoft Visual Studio 10.0\\VC\\lib;%ProgramFiles(x86)%\\Microsoft Visual Studio 10.0\\VC\\atlmfc\\lib"

#define DEFAULT_INCS \
"//#pragma warning(disable: 4005) //Uncomment to disable macro redefinition warning\r\n" \
"#include <windows.h>\r\n" \
"//#include <ntstatus.h>\r\n" \
"#include <stdio.h>\r\n" \
"#include <shlobj.h>\r\n" \
"#include <COMMCTRL.H>\r\n\r\n" \
"// Required for detemining\r\n" \
"// constant type name in runtime\r\n" \
"#include <typeinfo.h>\r\n"
#define DEFAULT_LIBS "comctl32.lib\r\nkernel32.lib\r\nuser32.lib\r\ngdi32.lib\r\nwinspool.lib\r\ncomdlg32.lib\r\nadvapi32.lib\r\nshell32.lib\r\nole32.lib\r\noleaut32.lib\r\nuuid.lib\r\nodbc32.lib\r\nodbccp32.lib\r\nkernel32.lib\r\nuser32.lib\r\ngdi32.lib\r\nwinspool.lib\r\ncomdlg32.lib\r\nadvapi32.lib\r\nshell32.lib\r\nole32.lib\r\noleaut32.lib\r\nuuid.lib\r\nodbc32.lib\r\nodbccp32.lib"

#define CPP_CODE \
"int main(int argc, char* argv[]) \r\n" \
"{ \r\n" \
"	char	tp[500]; \r\n" \
"	char	val[500]; \r\n" \
"	char	all[1000]; \r\n" \
"	const	type_info &tpi = typeid(dfMYCONST); \r\n" \
" \r\n" \
"	strcpy ( tp, tpi.name() ); \r\n" \
" \r\n" \
"	if (strcmp(tp,\"int\")==0 || \r\n" \
"		strcmp(tp,\"long\")==0 ) \r\n" \
"	sprintf ( val, \"%i\", dfMYCONST ); else \r\n" \
"	if (strstr(tp,\"int\")!=NULL || \r\n" \
"		strcmp(tp,\"unsigned long\")==0 ) \r\n" \
"	sprintf ( val, \"%u\", dfMYCONST ); else \r\n" \
"	if (strcmp(tp,\"struct HWND__ *\")==0 || \r\n" \
"		strcmp(tp,\"void *\")==0 || \r\n" \
"		strcmp(tp,\"const void *\")==0) \r\n" \
"	sprintf ( val, \"%i\", (long) dfMYCONST ); else \r\n" \
"	if (strcmp(tp,\"struct HKEY__ *\")==0) \r\n" \
"	sprintf ( val, \"%i\", (long) dfMYCONST ); else \r\n" \
"	if (strstr(tp,\"char \") && (strchr(tp,'*')==NULL) && strchr(tp,'[') && strchr(tp,']')) \r\n" \
"	sprintf ( val, \"%s\", dfMYCONST ); else \r\n" \
"	if (strcmp(tp,\"char\")==0) sprintf ( val, \"%c\", dfMYCONST ); else \r\n" \
"	if (strcmp(tp,\"char *\")==0 || \r\n" \
"		strcmp(tp,\"const char *\")==0 || \r\n" \
"		strcmp(tp,\"char const *\")==0) \r\n" \
"   { \r\n" \
"		if ( dfMYCONST == NULL ) \r\n" \
"			sprintf ( val, \"<NULL>\" );  \r\n" \
"		else \r\n" \
"			sprintf ( val, \"%i\", (DWORD) dfMYCONST );  \r\n" \
"	} else \r\n" \
" \r\n" \
"	sprintf ( val, \"Unknown type\" ); \r\n" \
" \r\n" \
"	sprintf ( all, \"Type:  %s\\nValue: %s\", tp, val ); \r\n" \
"	FILE *fl = fopen(\"DefResolve.dat\", \"w+b\"); \r\n" \
"	fwrite(all, strlen(all), 1, fl); \r\n" \
"	fclose(fl); \r\n" \
" \r\n" \
"	return 0; \r\n" \
"} \r\n"


extern char* gsPath;		//("MSVCPATH"));
extern char* gsIncPath;	//("IncPath"));
extern char* gsLibPath;	//("LibPath"));
extern char* gsIncs;		//("Includes"));
extern char* gsLibs;		//("Libraries"));


/////////////////////////////////////////////////////////////////////////////
// CResolveDlg dialog

class CResolveDlg
{
// Construction
public:
	CResolveDlg();	// standard constructor
	~CResolveDlg();	// standard destructor
	BOOL OnResolve(LPCTSTR asTempDir, LPCSTR asConst, LPTSTR* rsError, LPCTSTR* rsType, LPCTSTR* rsValue);

// Dialog Data
	//CButton	m_DefinitionCopy;
	//CButton	m_DefaultsButton;
	//CStatic	m_PathStatic;
	//CStatic	m_LibPathStatic;
	//CStatic	m_LibStatic;
	//CStatic	m_IncPathStatic;
	//CStatic	m_IncludeStatic;
	//CEdit	m_PathEdit;
	//CEdit	m_LibPathEdit;
	//CEdit	m_LibEdit;
	//CEdit	m_IncPathEdit;
	//CEdit	m_IncludeEdit;
	//CButton	m_Resolve;
	//CButton	m_ValueCopy;
	//CStatic	m_TypeStatic;
	//CEdit	m_Type;
	//CEdit	m_Value;
	//CStatic	m_ValueStatic;
	//CComboBox	m_Const;
	//CTabCtrl	m_Tab;

// Implementation
protected:
	BOOL Execute ( LPSTR a_szCmdLine, LPVOID a_lpEnvironment, LPTSTR *r_szReturn, BOOL abAllowOutput );
	//void GetData();
	//void ShowPage ( int nPage, BOOL bShow );
	//void Resize();
	//HICON m_hIcon;
	//BOOL  m_bNoClose;

	//LPSTR m_szPath, m_szIncPath, m_szLibPath;
	//LPSTR m_szIncs, m_szLibs;
	LPCSTR m_szConstant;
	LPTSTR ms_TempDir;
	//LPTSTR m_szConstant, m_szConstList;

	LPTSTR ms_CurDir;

	// Generated message map functions
	//virtual BOOL OnInitDialog();
	//afx_msg void OnPaint();
	//afx_msg HCURSOR OnQueryDragIcon();
	//virtual void OnOK();
	//virtual void OnCancel();
	//afx_msg void OnSize(UINT nType, int cx, int cy);
	//afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//afx_msg void OnCopyValue();
	//afx_msg void OnClose();
	//afx_msg void OnSelchangingTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	//void OnButtonDefaults();
	//afx_msg void OnDblclkConst();
	//afx_msg void OnCopyDefinition();
};


#endif // !defined(AFX_RESOLVEDLG_H__077CFA6D_0F74_11D7_9558_204C4F4F5020__INCLUDED_)