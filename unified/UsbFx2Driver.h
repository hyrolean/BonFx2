// UsbFx2Driver.h: CUsbFx2Driver クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include <map>
#include "cusb2.h"


//using std::vector;


// デフォルトのFIFOバッファパラメータ設定
#define DEF_BUFLEN	(1024UL * 64UL)		// cap_stsから
#define DEF_QUENUM	16UL				// cap_stsから
#define DEF_WAIT	500UL
#define DEF_PRIOR	THREAD_PRIORITY_HIGHEST


/////////////////////////////////////////////////////////////////////////////
// FIFOデータ受信インタフェースクラス
/////////////////////////////////////////////////////////////////////////////

class CUsbFx2Driver;

class IUsbFx2DriverHost
{
friend CUsbFx2Driver;

protected:
	virtual const bool OnRecvFifoData(const DWORD dwThreadIndex, const BYTE *pData, const DWORD dwLen, CUsbFx2Driver *pDriver) { return true ; }
    virtual void OnXferLock(const DWORD dwThreadIndex, bool locking, CUsbFx2Driver *pDriver) {}
    virtual BYTE *OnBeginWriteBack(const DWORD dwThreadIndex, DWORD dwMaxSize, CUsbFx2Driver *pDriver) { return NULL ; }
    virtual void OnFinishWriteBack(const DWORD dwThreadIndex, BYTE *pData, DWORD dwWroteSize, CUsbFx2Driver *pDriver) {}
};


/////////////////////////////////////////////////////////////////////////////
// cusb2 ラッパークラス
/////////////////////////////////////////////////////////////////////////////

class CUsbFx2Driver : private cusb2
{
public:
	CUsbFx2Driver(IUsbFx2DriverHost *pHostClass, HWND hHostHwnd = NULL);
	virtual ~CUsbFx2Driver();

	const bool OpenDriver(const BYTE byDeviceID, const BYTE *pFirmWare,
	   const DWORD FIRMWARE_SIZE, const char *lpszFirmName = NULL);
	void CloseDriver(void);

	const bool AddEndPoint(const BYTE byAddress, DWORD *pdwEpIndex = NULL);
	const bool TransmitData(const DWORD dwEpIndex, BYTE *pData, DWORD &dwLen);
	const bool TransmitFormatedData(const DWORD dwEpIndex, const DWORD dwDataNum, ...);
	const bool CreateFifoThread(
      const BYTE byAddress, DWORD *pdwThreadIndex = NULL, const DWORD dwBufLen = DEF_BUFLEN,
      const DWORD dwQueNum = DEF_QUENUM, const DWORD dwWait = DEF_WAIT, const int iPrior = DEF_PRIOR,
      const BOOL bWriteBackCache = FALSE );

    const bool ResetFifoThread(const DWORD dwThreadIndex) ;

protected:
	static bool FifoRecvCallback(u8 *pData, u32 dwLen, u32 dwThreadID);
    static void *FifoWriteBackBeginCallback(u32 dwMaxLen, u32 dwThreadID);
    static void FifoWriteBackFinishCallback(void *pData, u32 dwWroteLen, u32 dwThreadID);
    static void FifoXferLockCallback(bool locking, u32 dwThreadID);

	struct TAG_THREADINFO
	{
		unsigned int uiThreadID;
		cusb2_tcb *pThreadTcb;
	};

	static CUsbFx2Driver *m_pThis;

	IUsbFx2DriverHost *m_pDriverHost;

	std::vector<CCyUSBEndPoint *> m_EndPointArray;
	std::vector<TAG_THREADINFO> m_ThreadArray;
    std::map<unsigned int/*pid*/,int/*index*/> m_PidIndicesMap ;
};
