
#pragma once

/*

	Отображение прогресса операций

*/

class REProgress : public CScreenRestore
{
protected:
	unsigned __int64 nAllCount;
	unsigned __int64 nCurrent;
	int    nProgressLen;
	int    nCounter;
	TCHAR  sProgress[61], cHollow, cFull, *pszFormat, sFileInfo[61];
	TCHAR* psPercentTitle;
	TCHAR* psLastTitle;
	DWORD  nLastTick, nStepDuration, nStartTick;
	BOOL   bGraphic;
	void   Update(/*LPCTSTR asFileName = NULL*/);
	BOOL   bEscaped;
public:
	REProgress(const TCHAR* aszTitle, BOOL abGraphic = FALSE, const TCHAR* aszFileInfo = NULL);
	REProgress(const TCHAR* aszMsg, const TCHAR* aszTitle);
	virtual ~REProgress();
public:
	BOOL IncAll(size_t anAddAllCount = 1);
	BOOL Step(size_t anProcessed = 1);
	// Absolute size
	BOOL SetAll(unsigned __int64 anAllCount);
	BOOL SetStep(unsigned __int64 anAllStep, BOOL abForce = FALSE, LPCWSTR asFileName = NULL);
	// Длительность
	void ShowDuration();
	// EscCheck
	BOOL CheckForEsc(BOOL abForceRead = FALSE);
};

extern REProgress *gpProgress;
