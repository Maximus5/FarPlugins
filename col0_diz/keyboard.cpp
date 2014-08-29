
#include "headers.hpp"
#pragma hdrstop

#include "keyboard.hpp"

bool CheckForEsc(int nMsg /*= 0*/)
{
	bool EC=false;
	INPUT_RECORD rec;
	static HANDLE hConInp=GetStdHandle(STD_INPUT_HANDLE);
	DWORD ReadCount;
	while (1)
	{
		PeekConsoleInput(hConInp,&rec,1,&ReadCount);
		if (ReadCount==0) break;
		ReadConsoleInput(hConInp,&rec,1,&ReadCount);
		if (rec.EventType==KEY_EVENT)
			if (rec.Event.KeyEvent.wVirtualKeyCode==VK_ESCAPE &&
				rec.Event.KeyEvent.bKeyDown) EC=true;
	}

	if (EC && nMsg) {
		// "Windows Search\nStop search?"
		TCHAR *pszMsg = MSG(nMsg);
		int nBtn = psi.Message(&guid_DizC0,&guid_DizC0,FMSG_ALLINONE|FMSG_WARNING|FMSG_MB_YESNO,NULL,
			(const TCHAR* const*)pszMsg, 1, 0);
		if (nBtn!=0)
			EC = false;
	}
	return(EC);
}
