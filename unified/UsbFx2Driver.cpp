// UsbFx2Driver.cpp: CUsbFx2Driver クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <process.h>
#include "UsbFx2Driver.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace std ;

CUsbFx2Driver *CUsbFx2Driver::m_pThis = NULL;

CUsbFx2Driver::CUsbFx2Driver(IUsbFx2DriverHost *pHostClass, HWND hHostHwnd)
	: cusb2(hHostHwnd)
	, m_pDriverHost(pHostClass)
{
	m_pThis = this;
}

CUsbFx2Driver::~CUsbFx2Driver()
{
	CloseDriver();
}

const bool CUsbFx2Driver::OpenDriver(
  const BYTE byDeviceID, const BYTE *pFirmWare,
   const DWORD FIRMWARE_SIZE, const char *lpszFirmName)
{
	if(!pFirmWare)return false;

	// 一旦クローズする
	CloseDriver();

	// ファームウェアをコピーする(cusb2内で書き換えられるため)
	BYTE *abyFirmBuffer = new BYTE[FIRMWARE_SIZE] ;
	memcpy(abyFirmBuffer, pFirmWare, FIRMWARE_SIZE);

	// ファームウェアのダウンロードを行う
	bool result = fwload(byDeviceID, abyFirmBuffer, (BYTE *)lpszFirmName);

	delete [] abyFirmBuffer ;
	return result ;
}

void CUsbFx2Driver::CloseDriver( void )
{
  // 全てのスレッドを終了させる
  for ( DWORD dwIndex = 0UL ; dwIndex < m_ThreadArray.size() ; dwIndex++ ) {

    try {

      // スレッド終了シグナルセット
	  HANDLE th = m_ThreadArray[dwIndex].pThreadTcb->th;
	  m_ThreadArray[ dwIndex ].pThreadTcb->abort() ;

      // スレッド終了を待つ
	  DWORD time_limit = m_ThreadArray[ dwIndex ].pThreadTcb->wait * 2 + 5000 ;
      if ( ::WaitForSingleObject( th, time_limit ) != WAIT_OBJECT_0 ) {
        // スレッド強制終了
        if ( !m_ThreadArray[ dwIndex ].pThreadTcb->fDone ) {
          ::TerminateThread( th, 0UL );
        }
      }

      ::CloseHandle( m_ThreadArray[ dwIndex ].pThreadTcb->th );
      delete m_ThreadArray[ dwIndex ].pThreadTcb;

    }catch ( ... ) { /* No action */ }

  }

  // 内部状態をクリアする
  m_EndPointArray.clear();
  m_ThreadArray.clear();
  m_PidIndicesMap.clear();
}

const bool CUsbFx2Driver::AddEndPoint(const BYTE byAddress, DWORD *pdwEpIndex)
{
	// 既存のアドレスを検索する
	for(DWORD dwIndex = 0UL ; dwIndex < m_EndPointArray.size() ; dwIndex++){
		if(m_EndPointArray[dwIndex]->Address == byAddress)return false;
	}

	// エンドポイント追加
	CCyUSBEndPoint *pEndPoint = get_endpoint(byAddress);
	if(!pEndPoint)return false;

	// リストに登録
	if(pdwEpIndex)*pdwEpIndex = (DWORD) m_EndPointArray.size();
	m_EndPointArray.push_back(pEndPoint);

	return true;
}

const bool CUsbFx2Driver::TransmitData(const DWORD dwEpIndex, BYTE *pData, DWORD &dwLen)
{
	if(!pData || (dwEpIndex >= m_EndPointArray.size()))return false;

	// データ転送
	LONG lLen = dwLen;

	if(!xfer(m_EndPointArray[dwEpIndex], pData, lLen))return false;

	dwLen = (DWORD)lLen;

	return true;
}

const bool CUsbFx2Driver::TransmitFormatedData(const DWORD dwEpIndex, const DWORD dwDataNum, ...)
{
	if(!dwDataNum || (dwDataNum > 256))return false;

	// 可変長引数に指定したコマンド列をバッファに展開する(最大256バイト)
	va_list Args;
	va_start(Args, dwDataNum);

	BYTE abySendBuffer[256];

	for(DWORD dwPos = 0UL ; dwPos < dwDataNum ; dwPos++){
		abySendBuffer[dwPos] = va_arg(Args, BYTE);
	}

	// コマンドを送信する
	DWORD dwLen = dwDataNum;
	return TransmitData(dwEpIndex, abySendBuffer, dwLen);
}

const bool CUsbFx2Driver::CreateFifoThread(
  const BYTE byAddress, DWORD *pdwThreadIndex, const DWORD dwBufLen,
  const DWORD dwQueNum, const DWORD dwWait, const int iPrior,
  const BOOL bWriteBackCache )
{
	// スレッド起動
	TAG_THREADINFO ThreadInfo;
	::ZeroMemory(&ThreadInfo, sizeof(ThreadInfo));

	ThreadInfo.pThreadTcb = new cusb2_tcb;
	if(!ThreadInfo.pThreadTcb)return false;

	ThreadInfo.pThreadTcb->epaddr = byAddress;
	ThreadInfo.pThreadTcb->ep = get_endpoint(byAddress);
	ThreadInfo.pThreadTcb->xfer = dwBufLen;
	ThreadInfo.pThreadTcb->ques = dwQueNum;
    ThreadInfo.pThreadTcb->wait = dwWait ;
    ThreadInfo.pThreadTcb->prior = iPrior ;
    ThreadInfo.pThreadTcb->cb_func = CUsbFx2Driver::FifoRecvCallback;
    ThreadInfo.pThreadTcb->xfer_lock_func = CUsbFx2Driver::FifoXferLockCallback;
	if(bWriteBackCache) {
      ThreadInfo.pThreadTcb->wb_begin_func = CUsbFx2Driver::FifoWriteBackBeginCallback;
      ThreadInfo.pThreadTcb->wb_finish_func = CUsbFx2Driver::FifoWriteBackFinishCallback;
    }

	ThreadInfo.pThreadTcb->th = (HANDLE)_beginthreadex(NULL, 0UL, cusb2::thread_proc, (LPVOID)ThreadInfo.pThreadTcb, CREATE_SUSPENDED, &ThreadInfo.uiThreadID);
	if(!ThreadInfo.pThreadTcb->th)return false;


	// リストに登録
    DWORD dwThreadIndex = (DWORD) m_ThreadArray.size() ;
	if(pdwThreadIndex)*pdwThreadIndex = dwThreadIndex ;
    m_PidIndicesMap[ThreadInfo.uiThreadID] = dwThreadIndex ;
	m_ThreadArray.push_back(ThreadInfo);

	// レジューム
	::ResumeThread(ThreadInfo.pThreadTcb->th);

	return true;
}

const bool CUsbFx2Driver::ResetFifoThread(const DWORD dwThreadIndex)
{
	if(dwThreadIndex>=m_ThreadArray.size()) return false ;
	m_ThreadArray[dwThreadIndex].pThreadTcb->reset() ;
	return true ;
}

bool CUsbFx2Driver::FifoRecvCallback(u8 *pData, u32 dwLen, u32 dwThreadID)
{
    #if 0
    // 呼び出し元を特定するためにスレッドIDを取得
	for(DWORD dwIndex = 0UL ; dwIndex < m_pThis->m_ThreadArray.size() ; dwIndex++){
		if(m_pThis->m_ThreadArray[dwIndex].dwThreadID == dwThreadID){
			// インタフェースを呼び出す
			return m_pThis->m_pDriverHost->OnRecvFifoData(dwIndex, pData, dwLen, m_pThis);
			}
		}
    #else
	map<u32,int>::iterator pos=m_pThis->m_PidIndicesMap.find(dwThreadID) ;
    if(pos!=m_pThis->m_PidIndicesMap.end())
	  return m_pThis->m_pDriverHost->OnRecvFifoData(pos->second, pData, dwLen, m_pThis);
    #endif
	return false;
}

void CUsbFx2Driver::FifoXferLockCallback(bool locking, u32 dwThreadID)
{
	map<u32,int>::iterator pos=m_pThis->m_PidIndicesMap.find(dwThreadID) ;
    if(pos!=m_pThis->m_PidIndicesMap.end())
	  m_pThis->m_pDriverHost->OnXferLock(pos->second, locking, m_pThis);
}

void *CUsbFx2Driver::FifoWriteBackBeginCallback(u32 dwMaxLen, u32 dwThreadID)
{
	map<u32,int>::iterator pos=m_pThis->m_PidIndicesMap.find(dwThreadID) ;
    if(pos!=m_pThis->m_PidIndicesMap.end())
	  return m_pThis->m_pDriverHost->OnBeginWriteBack(pos->second, dwMaxLen, m_pThis);
    return NULL ;
}

void CUsbFx2Driver::FifoWriteBackFinishCallback(void *pData, u32 dwWroteLen, u32 dwThreadID)
{
	map<u32,int>::iterator pos=m_pThis->m_PidIndicesMap.find(dwThreadID) ;
    if(pos!=m_pThis->m_PidIndicesMap.end())
	  m_pThis->m_pDriverHost->OnFinishWriteBack(pos->second, (BYTE*)pData, dwWroteLen, m_pThis);
}

