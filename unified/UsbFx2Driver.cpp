// UsbFx2Driver.cpp: CUsbFx2Driver �N���X�̃C���v�������e�[�V����
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

	// ��U�N���[�Y����
	CloseDriver();

	// �t�@�[���E�F�A���R�s�[����(cusb2���ŏ����������邽��)
	BYTE *abyFirmBuffer = new BYTE[FIRMWARE_SIZE] ;
	memcpy(abyFirmBuffer, pFirmWare, FIRMWARE_SIZE);

	// �t�@�[���E�F�A�̃_�E�����[�h���s��
	bool result = fwload(byDeviceID, abyFirmBuffer, (BYTE *)lpszFirmName);

	delete [] abyFirmBuffer ;
	return result ;
}

void CUsbFx2Driver::CloseDriver( void )
{
  // �S�ẴX���b�h���I��������
  for ( DWORD dwIndex = 0UL ; dwIndex < m_ThreadArray.size() ; dwIndex++ ) {

    try {

      // �X���b�h�I���V�O�i���Z�b�g
	  HANDLE th = m_ThreadArray[dwIndex].pThreadTcb->th;
	  m_ThreadArray[ dwIndex ].pThreadTcb->abort() ;

      // �X���b�h�I����҂�
	  DWORD time_limit = m_ThreadArray[ dwIndex ].pThreadTcb->wait * 2 + 5000 ;
      if ( ::WaitForSingleObject( th, time_limit ) != WAIT_OBJECT_0 ) {
        // �X���b�h�����I��
        if ( !m_ThreadArray[ dwIndex ].pThreadTcb->fDone ) {
          ::TerminateThread( th, 0UL );
        }
      }

      ::CloseHandle( m_ThreadArray[ dwIndex ].pThreadTcb->th );
      delete m_ThreadArray[ dwIndex ].pThreadTcb;

    }catch ( ... ) { /* No action */ }

  }

  // ������Ԃ��N���A����
  m_EndPointArray.clear();
  m_ThreadArray.clear();
  m_PidIndicesMap.clear();
}

const bool CUsbFx2Driver::AddEndPoint(const BYTE byAddress, DWORD *pdwEpIndex)
{
	// �����̃A�h���X����������
	for(DWORD dwIndex = 0UL ; dwIndex < m_EndPointArray.size() ; dwIndex++){
		if(m_EndPointArray[dwIndex]->Address == byAddress)return false;
	}

	// �G���h�|�C���g�ǉ�
	CCyUSBEndPoint *pEndPoint = get_endpoint(byAddress);
	if(!pEndPoint)return false;

	// ���X�g�ɓo�^
	if(pdwEpIndex)*pdwEpIndex = (DWORD) m_EndPointArray.size();
	m_EndPointArray.push_back(pEndPoint);

	return true;
}

const bool CUsbFx2Driver::TransmitData(const DWORD dwEpIndex, BYTE *pData, DWORD &dwLen)
{
	if(!pData || (dwEpIndex >= m_EndPointArray.size()))return false;

	// �f�[�^�]��
	LONG lLen = dwLen;

	if(!xfer(m_EndPointArray[dwEpIndex], pData, lLen))return false;

	dwLen = (DWORD)lLen;

	return true;
}

const bool CUsbFx2Driver::TransmitFormatedData(const DWORD dwEpIndex, const DWORD dwDataNum, ...)
{
	if(!dwDataNum || (dwDataNum > 256))return false;

	// �ϒ������Ɏw�肵���R�}���h����o�b�t�@�ɓW�J����(�ő�256�o�C�g)
	va_list Args;
	va_start(Args, dwDataNum);

	BYTE abySendBuffer[256];

	for(DWORD dwPos = 0UL ; dwPos < dwDataNum ; dwPos++){
		abySendBuffer[dwPos] = va_arg(Args, BYTE);
	}

	// �R�}���h�𑗐M����
	DWORD dwLen = dwDataNum;
	return TransmitData(dwEpIndex, abySendBuffer, dwLen);
}

const bool CUsbFx2Driver::CreateFifoThread(
  const BYTE byAddress, DWORD *pdwThreadIndex, const DWORD dwBufLen,
  const DWORD dwQueNum, const DWORD dwWait, const int iPrior,
  const BOOL bWriteBackCache )
{
	// �X���b�h�N��
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


	// ���X�g�ɓo�^
    DWORD dwThreadIndex = (DWORD) m_ThreadArray.size() ;
	if(pdwThreadIndex)*pdwThreadIndex = dwThreadIndex ;
    m_PidIndicesMap[ThreadInfo.uiThreadID] = dwThreadIndex ;
	m_ThreadArray.push_back(ThreadInfo);

	// ���W���[��
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
    // �Ăяo��������肷�邽�߂ɃX���b�hID���擾
	for(DWORD dwIndex = 0UL ; dwIndex < m_pThis->m_ThreadArray.size() ; dwIndex++){
		if(m_pThis->m_ThreadArray[dwIndex].dwThreadID == dwThreadID){
			// �C���^�t�F�[�X���Ăяo��
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

