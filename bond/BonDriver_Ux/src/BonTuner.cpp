// BonTuner.cpp: CBonTuner �N���X�̃C���v�������e�[�V����
//   (DT300�p)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <SetupApi.h>
#include <MMSystem.h>
#include <Malloc.h>
#include <InitGuid.h>
#include <Math.h>
#include <cstdio>
#include <io.h>
#include "Resource.h"
#include "BonTuner.h"


#pragma comment(lib, "SetupApi.lib")
#pragma comment(lib, "WinMM.lib")


using namespace std ;

//////////////////////////////////////////////////////////////////////
// �萔��`
//////////////////////////////////////////////////////////////////////

// ��M�T�C�Y�iFX���̐ݒ�j
DWORD TSDATASIZE          =   48128UL  ;       // TS�f�[�^�̃T�C�Y
DWORD TSQUEUENUM          =   36UL     ;       // TS�f�[�^�̊�X�g�b�N��(�ő吔63�܂�)
DWORD TSTHREADWAIT        =   1000UL   ;       // TS�X���b�h�L���[���ɑ҂ő厞��
int   TSTHREADPRIORITY    =   THREAD_PRIORITY_HIGHEST ; // TS�X���b�h�̗D��x
BOOL  TSWRITEBACK         =   TRUE     ;       // TS�f�[�^�̏����߂��ɂ��o�b�t�@����������
BOOL  TSWRITEBACKDUMMY    =   FALSE    ;       // TS�f�[�^�̏����߂��c�ʂ��Ȃ��ꍇ�Ƀ_�~�[�̈��L���ɂ��邩�ǂ���
BOOL  TSALLOCWAITING      =   FALSE    ;       // TS�f�[�^�̃A���P�[�V�����̊�����҂��ǂ���
int   TSALLOCPRIORITY     =   THREAD_PRIORITY_HIGHEST ; // TS�A���P�[�V�����X���b�h�̗D��x

// FIFO�o�b�t�@�ݒ�
DWORD ASYNCTSQUEUENUM    =   66UL   ;        // �񓯊�TS�f�[�^�̊�X�g�b�N��(�����l)
DWORD ASYNCTSQUEUEMAX    =   660UL  ;        // �񓯊�TS�f�[�^�̊�X�g�b�N�ő吔
DWORD ASYNCTSEMPTYBORDER =   22UL   ;        // �񓯊�TS�f�[�^�̋󂫃X�g�b�N����l臒l(�A���P�[�V�����J�n臒l)
DWORD ASYNCTSEMPTYLIMIT  =   11UL   ;        // �񓯊�TS�f�[�^�̍Œ���m�ۂ���󂫃X�g�b�N��(�I�[�o�[���b�v����̕ۏ�)

// �d���ݒ�
BOOL AUTOPOWERON   = TRUE ;
BOOL AUTOPOWEROFF  = TRUE ;
DWORD U3BSFIXTUNE  = 0 ;
DWORD U3BSFIXWAIT  = 15500 ;
DWORD U3BSFIXCH    = 0 ;
BOOL AUTOTUNEBACKTONHK = FALSE ;

// �G���h�|�C���g�C���f�b�N�X
#define EPINDEX_IN          0UL
#define EPINDEX_OUT         1UL

// �R�}���h
#define CMD_EP6IN_START     0x50U   //
#define CMD_EP6IN_STOP      0x51U   //
#define CMD_EP2OUT_START    0x52U   //
#define CMD_EP2OUT_STOP     0x53U   //
#define CMD_PORT_CFG        0x54U   //addr_mask, out_pins
#define CMD_REG_READ        0x55U   //addr  (return 1byte)
#define CMD_REG_WRITE       0x56U   //addr, value
#define CMD_PORT_READ       0x57U   //(return 1byte)
#define CMD_PORT_WRITE      0x58U   //value
#define CMD_IFCONFIG        0x59U   //value
#define CMD_MODE_IDLE       0x5AU
#define CMD_EP4IN_START     0x5BU   // B-CAS
#define CMD_EP4IN_STOP      0x5CU   // B-CAS
#define CMD_IR_CODE         0x5DU   //val_l, val_h (0x0000:RBUF capture 0xffff:BWUF output)
#define CMD_IR_WBUF         0x5EU   //ofs(0 or 64 or 128 or 192), len(max 64), data, ....
#define CMD_IR_RBUF         0x5FU   //ofs(0 or 64 or 128 or 192) (return 64byte)

#define PIO_START           0x20
#define PIO_IR_OUT          0x10
#define PIO_IR_IN           0x08
#define PIO_TS_BACK         0x04

// �`���[�i�[�̃����R���ԍ�
DWORD IRNUMBER = 1UL ; // 1, 2 or 3

// �h���C�o�g�p���Ƀ����R�������b�N���Ďg�p�ł��Ȃ����邩�ǂ���
// (���b�N���Ă��p�l���͎g�p�\)
BOOL IRLOCK = FALSE ;

// �]����r���ōs�����ǂ���
BOOL EXCLXFER = TRUE ;

// �E�F�C�g
  // �ǉ�: ButtonPressTimes,ButtonReleaseTimes,ButtonInterimWait @ 2013/06/03
  //       (�ŐV�t�@�[���ł���������BS�`�����l���X�L���j���O�o�O�΍��p)
DWORD COMMANDSENDTIMES   = 1 ; //2 ;
DWORD COMMANDSENDINTERVAL= 3000;
DWORD COMMANDSENDWAIT    = 100 ;
DWORD CHANNELCHANGEWAIT  = 250 ;
DWORD BUTTONTXWAIT       = 10 ;
DWORD BUTTONPRESSTIMES   = 2 ;
DWORD BUTTONPRESSWAIT    = 200 ;
DWORD BUTTONINTERIMWAIT  = 200 ;
DWORD BUTTONRELEASETIMES = 2  ;
DWORD BUTTONRELEASEWAIT  = 400 ;
DWORD BUTTONSPACEWAIT    = 400 ;
DWORD BUTTONPOWERWAIT    = 4000 ;
DWORD BUTTONPOWEROFFDELAY= 0 ;
BOOL REDUCESPACECHANGE   = TRUE ;

//�����R���̃R�}���h���񌅁D0x04NN
#define REMOCON_POWERON     0x8BU
#define REMOCON_POWEROFF    0x8CU

#define REMOCON_CHIDEJI     0xD6U
#define REMOCON_BS          0xE2U
#define REMOCON_CS          0xE3U

#define REMOCON_3DIGITS     0xD8U

#define REMOCON_NUMBER_0    0x80U   //10�ƈꏏ
#define REMOCON_NUMBER_1    0x81U
#define REMOCON_NUMBER_2    0x82U
#define REMOCON_NUMBER_3    0x83U
#define REMOCON_NUMBER_4    0x84U
#define REMOCON_NUMBER_5    0x85U
#define REMOCON_NUMBER_6    0x86U
#define REMOCON_NUMBER_7    0x87U
#define REMOCON_NUMBER_8    0x88U
#define REMOCON_NUMBER_9    0x89U
#define REMOCON_NUMBER_10   0x80U
#define REMOCON_NUMBER_11   0xD1U
#define REMOCON_NUMBER_12   0xD2U

#define REMOCON_MENU        0x8FU
#define REMOCON_UP          0x91U
#define REMOCON_DOWN        0x92U
#define REMOCON_LEFT        0x93U
#define REMOCON_RIGHT       0x94U
#define REMOCON_ENTER       0x95U
#define REMOCON_BACK        0x96U


// FX2�t�@�[���E�F�A
static const BYTE abyFirmWare[] =
#include "Fw.inc"

//////////////////////////////////////////////////////////////////////
// �`�����l����`�e�[�u��
//////////////////////////////////////////////////////////////////////

const struct {
    LPCTSTR pszSpace ;
    LPCTSTR pszChName ;
    const char* pszRCode ;
} asChTbl[] = { // �����Â��i�v�F�`�����l���t�@�C���ҏW
    { TEXT("�n�f�W"), TEXT("�����R�� 1"),              "X1" },
    { TEXT("�n�f�W"), TEXT("�����R�� 2"),              "X2" },
    { TEXT("�n�f�W"), TEXT("�����R�� 3"),              "X3" },
    { TEXT("�n�f�W"), TEXT("�����R�� 4"),              "X4" },
    { TEXT("�n�f�W"), TEXT("�����R�� 5"),              "X5" },
    { TEXT("�n�f�W"), TEXT("�����R�� 6"),              "X6" },
    { TEXT("�n�f�W"), TEXT("�����R�� 7"),              "X7" },
    { TEXT("�n�f�W"), TEXT("�����R�� 8"),              "X8" },
    { TEXT("�n�f�W"), TEXT("�����R�� 9"),              "X9" },
    { TEXT("�n�f�W"), TEXT("�����R�� 10"),             "XA" },
    { TEXT("�n�f�W"), TEXT("�����R�� 11"),             "XB" },
    { TEXT("�n�f�W"), TEXT("�����R�� 12"),             "XC" },
    { TEXT("BS"),     TEXT("BS1/TS0 BS����"),          "YD151" },
    { TEXT("BS"),     TEXT("BS1/TS1 BS-i"),            "YD161" },
    { TEXT("BS"),     TEXT("BS3/TS0 WOWOW"),           "YD191" },
    { TEXT("BS"),     TEXT("BS3/TS1 BS�W���p��"),      "YD171" },
    { TEXT("BS"),     TEXT("BS9/TS0 BS11"),            "YD211" },
    { TEXT("BS"),     TEXT("BS9/TS1 Star Channel HV"), "YD200" },
    { TEXT("BS"),     TEXT("BS9/TS2 TwellV"),          "YD222" },
    { TEXT("BS"),     TEXT("BS13/TS0 BS���e��"),       "YD141" },
    { TEXT("BS"),     TEXT("BS13/TS1 BS�t�W"),         "YD181" },
    { TEXT("BS"),     TEXT("BS15/TS1 NHK BS1/2"),      "YD101" },
    { TEXT("BS"),     TEXT("BS15/TS2 NHK BS-hi"),      "YD103" },
    { TEXT("110CS"),  TEXT("ND2 110CS #1"),            "ZD239" },
    { TEXT("110CS"),  TEXT("ND4 110CS #2"),            "ZD194" },
    { TEXT("110CS"),  TEXT("ND6 110CS #3"),            "ZD310" },
    { TEXT("110CS"),  TEXT("ND8 110CS #4"),            "ZD055" },
    { TEXT("110CS"),  TEXT("ND10 110CS #5"),           "ZD228" },
    { TEXT("110CS"),  TEXT("ND12 110CS #6"),           "ZD323" },
    { TEXT("110CS"),  TEXT("ND14 110CS #7"),           "ZD251" },
    { TEXT("110CS"),  TEXT("ND16 110CS #8"),           "ZD342" },
    { TEXT("110CS"),  TEXT("ND18 110CS #9"),           "ZD314" },
    { TEXT("110CS"),  TEXT("ND20 110CS #10"),          "ZD340" },
    { TEXT("110CS"),  TEXT("ND22 110CS #11"),          "ZD330" },
    { TEXT("110CS"),  TEXT("ND24 110CS #12"),          "ZD321" },
} ;

//////////////////////////////////////////////////////////////////////
// �C���X�^���X�������\�b�h
//////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4273 )

extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{
    // �X�^���X����(�����̏ꍇ�̓C���X�^���X�̃|�C���^��Ԃ�)
    return (CBonTuner::m_pThis)? CBonTuner::m_pThis : ((IBonDriver *) new CBonTuner());
}

#pragma warning( default : 4273 )

    class mm_interval_lock {
        DWORD period_;
    public:
        mm_interval_lock(DWORD period) : period_(period) {
            timeBeginPeriod(period_);
        }
        ~mm_interval_lock() {
            timeEndPeriod(period_);
        }
    };
    #define MMINTERVAL_PERIOD 10

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

// �ÓI�����o������
CBonTuner * CBonTuner::m_pThis = NULL;
HINSTANCE CBonTuner::m_hModule = NULL;

CBonTuner::CBonTuner()
    : m_pUsbFx2Driver(NULL)
    , m_hOnStreamEvent(NULL)
    , m_AsyncTSFifo(NULL)
    , m_dwCurSpace(0)
    , m_cLastSpace(0xFF)
    , m_dwCurChannel(50)
    , m_bU3BSFixDone(FALSE)
{
    m_pThis = this;
    m_bytesProceeded = 0 ;
    m_tickProceeding = GetTickCount() ;
    InitTunerProperty() ;
}

CBonTuner::~CBonTuner()
{
    // �J����Ă�ꍇ�͕���
    CloseTuner();

    m_pThis = NULL;
}

const BOOL CBonTuner::OpenTuner()
{
    // ��U�N���[�Y
    CloseTuner();


    is_channel_valid = FALSE;

    m_cLastSpace = 0xFF ;

    m_bU3BSFixDone = FALSE ;

    // �J�����I��USB FX2�̃h���C�o�C���X�^���X����
    m_pUsbFx2Driver = new CUsbFx2Driver(this);
    if(!m_pUsbFx2Driver)return false;


    // FX2�̏������V�[�P���X
    try{

        mm_interval_lock interlock(MMINTERVAL_PERIOD);

		// �񓯊�FIFO�o�b�t�@�I�u�W�F�N�g�쐬
        m_AsyncTSFifo = new CAsyncFifo(
          ASYNCTSQUEUENUM,ASYNCTSQUEUEMAX,ASYNCTSEMPTYBORDER,
          TSDATASIZE,TSTHREADWAIT,TSALLOCPRIORITY ) ;
        m_AsyncTSFifo->SetEmptyLimit(ASYNCTSEMPTYLIMIT) ;
        m_TSWriteBackDummyCache.resize(TSDATASIZE);

        // �C�x���g�쐬
        if(!(m_hOnStreamEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL)))throw 0UL;

        // �h���C�o�I�[�v��
        if(!m_pUsbFx2Driver->OpenDriver( m_yFx2Id, abyFirmWare, sizeof(abyFirmWare),"FX2_FIFO"))throw 0UL;

        // �G���h�|�C���g�ǉ�
        if(!m_pUsbFx2Driver->AddEndPoint(0x81U))throw 1UL;  // EPINDEX_IN
        if(!m_pUsbFx2Driver->AddEndPoint(0x01U))throw 2UL;  // EPINDEX_OUT


        // FX2 I/O�|�[�g�ݒ�@���@MAX2�����p�����[�^�ݒ� �� MAX2���Z�b�g
        exclusive_lock elock(&m_coXfer);
        if(!m_pUsbFx2Driver->TransmitFormatedData(EPINDEX_OUT, 6UL, CMD_PORT_CFG, 0x00U, PIO_START | PIO_IR_OUT | PIO_TS_BACK, CMD_MODE_IDLE, CMD_IFCONFIG, 0xE3U))throw 3UL;
        elock.unlock();

        // �X���b�h�N��
        if(!m_pUsbFx2Driver->CreateFifoThread(0x86U, &m_dwFifoThreadIndex, TSDATASIZE, TSQUEUENUM, TSTHREADWAIT, TSTHREADPRIORITY, TSWRITEBACK))throw 4UL;
        // �J�n�R�}���h���M
        elock.lock();
        if(!m_pUsbFx2Driver->TransmitFormatedData(EPINDEX_OUT, 3UL, CMD_EP6IN_START, CMD_PORT_WRITE, PIO_START | PIO_IR_OUT | PIO_TS_BACK))throw 3UL;
        elock.unlock();


        // �����R���x�[�X�R�[�h����
        m_wIRBase = WORD(max(min(IRNUMBER,3UL),1UL)-1UL) * 0x80U ;

        // �����R���R�}���h�o�b�t�@�m��
        m_IRCmdBuffer.resize(max(BUTTONPRESSTIMES,BUTTONRELEASETIMES)*3) ;

        m_tkLastCommandSend = PastSleep() - COMMANDSENDINTERVAL ;

        // ����
        if(AUTOPOWERON) {
          IRCodeTX(REMOCON_POWERON);    //�d��������
          Sleep(BUTTONPOWERWAIT) ;
        }

        // DT-300 �Â��t�@�[���E�F�A�� BS �`���[�j���O�o�O�΍� Method #1
        if(U3BSFIXTUNE) {
          if(U3BSFIXTUNE==1)
            m_bU3BSFixDone = U3BSFixTune() ;
          IRCodeTX(REMOCON_CHIDEJI) ;
        }


        return TRUE;
        }
    catch(DWORD dwErrorStep){
        // �G���[����
        dwErrorStep += 0 ; // to avoid warnning4101
        CloseTuner();

        return FALSE;
        }

}

void CBonTuner::CloseTuner()
{
    BOOL power=FALSE ;
    DWORD power_start = 0 ;

    is_channel_valid = FALSE;

    // �h���C�o�����
    if(m_pUsbFx2Driver){

        mm_interval_lock interlock(MMINTERVAL_PERIOD);

        // ����N����BS���������p��BS�̃`�����l����߂��Ă����ݒ�
        // (�N�������BS�`�����l����F�����Ȃ��̂ł��̃^�C�~���O�Őݒ肵�Ă���)
        if(U3BSFIXTUNE) {
          if(m_bU3BSFixDone&&U3BSFIXCH) {
            U3BSFixResetChannel() ;
          }
        }

        // �I������NHK�Ƀ`�����l����߂����ǂ���
        if(AUTOTUNEBACKTONHK) {
          Sleep(COMMANDSENDWAIT) ;
          IRCodeTX(REMOCON_CHIDEJI) ;
          IRCodeTX(REMOCON_NUMBER_1) ;
        }

        // �d����؂�
        if(AUTOPOWEROFF) {
          Sleep(COMMANDSENDWAIT) ;
          IRCodeTX(REMOCON_POWEROFF);
          power = TRUE ;
          power_start = PastSleep() ;
        }

        // �����R�����b�N�̊J��
        if(IRLOCK) {
          Sleep(COMMANDSENDWAIT) ;
          IRCodeTX(0) ;
        }

        m_pUsbFx2Driver->CloseDriver();
        delete m_pUsbFx2Driver;
        m_pUsbFx2Driver = NULL;
    }

    // �n���h�������
    if(m_hOnStreamEvent){
        ::CloseHandle(m_hOnStreamEvent);
        m_hOnStreamEvent = NULL;
    }

    // �񓯊�FIFO�o�b�t�@�I�u�W�F�N�g�j��
    if(m_AsyncTSFifo) {
      delete m_AsyncTSFifo ;
      m_AsyncTSFifo=NULL ;
    }

    // ���̑�������
    m_dwCurSpace = 0 ;
    m_dwCurChannel = 50 ;

    // �d��OFF��̃X���[�v����
    if(power) PastSleep(BUTTONPOWEROFFDELAY,power_start);
}

BOOL CBonTuner::U3BSFixTune()
{
  Sleep(COMMANDSENDWAIT) ;
  if(
    !IRCodeTX(REMOCON_BS)    ||
    !IRCodeTX(REMOCON_MENU)  ||
    !IRCodeTX(REMOCON_DOWN)  ||
    !IRCodeTX(REMOCON_ENTER) ||
    !IRCodeTX(REMOCON_ENTER)    )  return FALSE ;
  Sleep(U3BSFIXWAIT) ;  // BS-Digital ���o�܂Ŗ�14-5�b���炢������
  if(!IRCodeTX(REMOCON_MENU)) return FALSE ;
  Sleep(BUTTONSPACEWAIT) ;
  return TRUE ;
}

BOOL CBonTuner::U3BSFixResetChannel()
{
  Sleep(COMMANDSENDWAIT) ;
  if(
    !IRCodeTX(REMOCON_BS) ||
    !IRCodeTX(REMOCON_3DIGITS) ||
    !IRCodeTX(REMOCON_NUMBER_0+(U3BSFIXCH/100U)%10U) ||
    !IRCodeTX(REMOCON_NUMBER_0+(U3BSFIXCH/10U)%10U) ||
    !IRCodeTX(REMOCON_NUMBER_0+U3BSFIXCH%10U) ) return FALSE ;
  return TRUE ;
}

void CBonTuner::ResetFxFifo()
{
    if(m_pUsbFx2Driver)
      m_pUsbFx2Driver->ResetFifoThread(m_dwFifoThreadIndex) ;
}

const BOOL CBonTuner::SetChannel(const BYTE bCh)
{
    //�Ƃ肠�����X�g���[�����Ƃ߂�
    is_channel_valid = FALSE;

    if(bCh < 0 || bCh >= m_Channels.size()){
        return FALSE;
    }

    // TS�f�[�^�p�[�W
    PurgeTsStream();

    //DT300�ւ̃����R���R�}���h���M�̃^�C�~���O�͂��Ȃ�V�r�A�ȈׁA
    //�`�����l���ؑւ̑O�ɃV�X�e���̊����݌������Ɍ��܂ŏグ�Ă���
    mm_interval_lock interlock(MMINTERVAL_PERIOD);

    PastSleep(COMMANDSENDINTERVAL,m_tkLastCommandSend);
    for(size_t j = 0; j < COMMANDSENDTIMES; j++){
        Sleep(COMMANDSENDWAIT) ;
        for(size_t i = 0; i < min<size_t>(m_Channels[bCh].rcode.size(),REMOCON_CMD_LEN); i++){
            BYTE code = m_Channels[bCh].rcode[i] ;
            if(!IRButtonTX(code)){
                 DBGOUT("Failed to send button. [code=%d]",code) ;
                 return FALSE ;
            }
        }
    }
    m_tkLastCommandSend=PastSleep() ;

    // �`�����l�������X�V
    /*
    m_dwCurSpace=0;
    m_dwCurChannel=0;
    wstring space;
    for(int i=0;i<bCh;i++) {
      if(!i) space = m_Channels[bCh].space ;
      else if (m_Channels[bCh].space!=space) {
        space = m_Channels[bCh].space ;
        m_dwCurSpace++,m_dwCurChannel=0 ;
        continue ;
      }
      m_dwCurChannel++;
    }
    */

	//Fx��FIFO�o�b�t�@������
	ResetFxFifo();

	Sleep(CHANNELCHANGEWAIT);

    //�X�g���[������
    is_channel_valid = TRUE;
    return TRUE;
}


const float CBonTuner::GetSignalLevel(void)
{
    #if 0
    // FIFO�o�b�t�@��Fullness��Ԃ�
    return ((float)m_FifoBuffer.size() / (float)ASYNCTSQUEUENUM * 100.0f);
    #else
    // ���ۂɏ��������X�g���[���̓]���� Mbps ��Ԃ�
    DWORD tick = GetTickCount() ;
    float duration ;
    duration = (float) Elapsed(m_tickProceeding,tick) ;
	float result = 0.f;
	if (duration>1.f) {
		duration /= 1000.f;
		result = (float)m_bytesProceeded*8.f / duration / 1024.f / 1024.f;
		m_tickProceeding = tick;
		m_bytesProceeded = 0;
		if (result > 100.f) result = 0.f;
	}
    return result ;
    #endif
}

const DWORD CBonTuner::WaitTsStream(const DWORD dwTimeOut)
{
    // �I���`�F�b�N
    if(!m_pUsbFx2Driver)return WAIT_ABANDONED;

	// �o�b�t�@�ς�
	if(!m_AsyncTSFifo->Empty()) return WAIT_OBJECT_0;

    // �C�x���g���V�O�i����ԂɂȂ�̂�҂�
    const DWORD dwRet = ::WaitForSingleObject(m_hOnStreamEvent, dwTimeOut);

    switch(dwRet){
        case WAIT_ABANDONED :
            // �`���[�i������ꂽ
            return WAIT_ABANDONED;

        case WAIT_OBJECT_0 :
        case WAIT_TIMEOUT :
            // �X�g���[���擾�\ or �`���[�i������ꂽ
            return (m_pUsbFx2Driver)? dwRet : WAIT_ABANDONED;

        case WAIT_FAILED :
        default:
            // ��O
            return WAIT_FAILED;
        }
}

const DWORD CBonTuner::GetReadyCount()
{
    // ���o���\TS�f�[�^�����擾����
    return (DWORD)m_AsyncTSFifo->Size();
}

const BOOL CBonTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
    // TS�f�[�^���o�b�t�@������o��
    BYTE *pSrc = NULL;

    if(GetTsStream(&pSrc, pdwSize, pdwRemain)){
        if(*pdwSize){
            CopyMemory(pDst, pSrc, *pdwSize);
            }

        return TRUE;
        }

    return FALSE;
}

const BOOL CBonTuner::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	if(!m_pUsbFx2Driver)
		return FALSE;

    m_AsyncTSFifo->Pop(ppDst,pdwSize,pdwRemain) ;
    return TRUE ;
}

void CBonTuner::PurgeTsStream()
{
	m_AsyncTSFifo->Purge() ;
}

void CBonTuner::Release()
{
    // �C���X�^���X�J��
    delete this;
}

const bool CBonTuner::OnRecvFifoData(const DWORD dwThreadIndex, const BYTE *pData, const DWORD dwLen, CUsbFx2Driver *pDriver)
{
    if (dwThreadIndex == m_dwFifoThreadIndex) {
      if(!TSWRITEBACK) {
        //�L���ȃ`�����l�����I������Ă��鎞�Ɍ�����
        if(is_channel_valid){
          // �񓯊�FIFO�o�b�t�@�Ƀv�b�V��
          if(m_AsyncTSFifo->Push(pData,dwLen)>0) {
            // �C�x���g�Z�b�g
            ::SetEvent(m_hOnStreamEvent);
          }
          m_bytesProceeded+=dwLen ;
        }
      }
    }
    return true;
}

void CBonTuner::OnXferLock(const DWORD dwThreadIndex, bool locking, CUsbFx2Driver *pDriver)
{
  if(EXCLXFER) {
    if ( dwThreadIndex == m_dwFifoThreadIndex ) {
      if(locking)     m_coXfer.lock() ;
      else            m_coXfer.unlock() ;
    }
  }
}

BYTE *CBonTuner::OnBeginWriteBack(const DWORD dwThreadIndex, DWORD dwMaxSize, CUsbFx2Driver *pDriver)
{
  if (dwThreadIndex == m_dwFifoThreadIndex) {
    if(TSWRITEBACK) {
      CAsyncFifo::CACHE *cache = m_AsyncTSFifo->BeginWriteBack(TSALLOCWAITING?true:false) ;
      if(!cache)
        return !TSWRITEBACKDUMMY ? NULL : m_TSWriteBackDummyCache.data() ; // �e�ʃI�[�o�[(�_�~�[����)
      cache->resize(dwMaxSize) ;
      m_TSWriteBackMap[cache->data()] = cache ;
      return cache->data() ;
    }
  }
  return NULL ;
}


void CBonTuner::OnFinishWriteBack(const DWORD dwThreadIndex, BYTE *pData, DWORD dwWroteSize, CUsbFx2Driver *pDriver)
{
  if (dwThreadIndex == m_dwFifoThreadIndex) {
    if(TSWRITEBACK) {
      WRITEBACKMAP::iterator pos = m_TSWriteBackMap.find(pData) ;
      DWORD nWrote = is_channel_valid?dwWroteSize:0 ;
      if(pData==m_TSWriteBackDummyCache.data()) {
        // �_�~�[�Ȃ̂Ń_�u�������Ȃǂɂ����Ă���\����������...
        if(is_channel_valid&&nWrote>0) {
          if(m_AsyncTSFifo->Push(pData,nWrote,true,TSALLOCWAITING?true:false)>0) {
            ::SetEvent(m_hOnStreamEvent);
          }
        }
      }
      else if(pos!=m_TSWriteBackMap.end()) {
        CAsyncFifo::CACHE *cache = pos->second ;
        cache->resize(nWrote) ;
        if(m_AsyncTSFifo->FinishWriteBack(cache))
          ::SetEvent(m_hOnStreamEvent);
        m_bytesProceeded += nWrote ;
        m_TSWriteBackMap.erase(pos) ;
      }
    }
  }
}


// 0x04NN�𑗐M
BOOL CBonTuner::IRCodeTX(BYTE code)
{
    BYTE *cmd = static_cast<BYTE*>(m_IRCmdBuffer.data()) ;
    DWORD len = 0 ;

    switch(code) {
      //�`���[�i�[�X�y�[�X�؂�ւ����ȗ��\�ȏꍇ�͉������Ȃ�
      case REMOCON_CHIDEJI:
      case REMOCON_BS:
      case REMOCON_CS:
        if(REDUCESPACECHANGE && code==m_cLastSpace) {
          return TRUE ;
        }
        break ;
    }

    DWORD success = 0 ;
    DWORD s = Elapsed() ;

    //�{�^������
    exclusive_lock elock(&m_coXfer);
    for(DWORD e=0;BUTTONPRESSWAIT>e;e=Elapsed(s)) {
      len = 0;
      for(DWORD i=0;i<BUTTONPRESSTIMES;i++) {
        cmd[len++] = CMD_IR_CODE;
        cmd[len++] = code ;
        cmd[len++] = 0x04U;
        *LPWORD(&cmd[len-2]) += m_wIRBase ;
      }
      if(m_pUsbFx2Driver->TransmitData(EPINDEX_OUT,cmd,len)) {
        ::Sleep(BUTTONTXWAIT) ;
        success++ ;
      }
    }
    if(!success)
      return FALSE ;
    //PastSleep(BUTTONPRESSWAIT,s);

    elock.unlock();
    Sleep(BUTTONINTERIMWAIT) ;

    //�{�^���J��
    success = 0 ;
    s = Elapsed() ;
    elock.lock();
    for(DWORD e=0;BUTTONRELEASEWAIT>e;e=Elapsed(s)) {
      len = 0;
      for(DWORD i=0;i<BUTTONRELEASETIMES;i++) {
        cmd[len++] = CMD_IR_CODE;
        cmd[len++] = IRLOCK ? code : 0 ;
        cmd[len++] = 0;
        *LPWORD(&cmd[len-2]) += m_wIRBase ;
      }
      if(m_pUsbFx2Driver->TransmitData(EPINDEX_OUT,cmd,len)) {
        ::Sleep(BUTTONTXWAIT) ;
        success++ ;
      }
    }
    if(!success)
      return FALSE ;
    //PastSleep(BUTTONRELEASEWAIT,s);

    elock.unlock();
    switch(code) {
      //�`���[�i�[�X�y�[�X�؂�ւ�����͔����������̂ő҂�
      case REMOCON_CHIDEJI:
      case REMOCON_BS:
      case REMOCON_CS:
        if(m_cLastSpace!=code) {
          m_cLastSpace = code ;
          Sleep(BUTTONSPACEWAIT) ;
        }
        break ;
    }

    return TRUE;
}


// �����̖��O����`���[�i�[�^�C�v��ID������
BOOL CBonTuner::InitTunerProperty(void)
{
    //�����̖��O���擾
    char szMyPath[_MAX_PATH] ;
    GetModuleFileNameA( m_hModule, szMyPath, _MAX_PATH ) ;
    char szMyDrive[_MAX_FNAME] ;
    char szMyDir[_MAX_FNAME] ;
    char szMyName[_MAX_FNAME] ;
    _splitpath_s( szMyPath, szMyDrive, _MAX_FNAME,szMyDir, _MAX_FNAME, szMyName, _MAX_FNAME, NULL, 0 ) ;
    _strupr_s( szMyName, sizeof(szMyName) ) ;
    //�o���h����FX2ID���擾
    int nNumBands = 3, nFx2Id = 0 ;
    sscanf_s( szMyName, "BONDRIVER_U%1dID%1d", &nNumBands, &nFx2Id ) ;
    LPCTSTR pszTunerName ;
    m_Channels.clear() ;
    int numChannels = (sizeof(asChTbl)/sizeof(asChTbl[0])) ;
    string strUnitedPrefix ;
    if ( nNumBands == 3 ) {
      pszTunerName = TEXT("���j�f���R�g") ;
      strUnitedPrefix = "BonDriver_U3" ;
    }
    else {
      pszTunerName = TEXT("���j�f���n�f�W") ;
      numChannels = 12 ;
      strUnitedPrefix = "BonDriver_U1" ;
    }
    m_yFx2Id = BYTE(nFx2Id) ;
    //�`���[�i�[��������
    _stprintf_s( m_szTunerName, 100, TEXT("%s(ID=%d)"), pszTunerName, nFx2Id ) ;
    for(int i=0;i<numChannels;i++) {
      m_Channels.push_back(CHANNEL(asChTbl[i].pszSpace,asChTbl[i].pszChName,asChTbl[i].pszRCode)) ;
    }
    //Ini�t�@�C���̃��[�h
    #if 0
    if(!LoadIniFile(string(szMyDrive)+string(szMyDir)+string(szMyName)+".ini"))
      LoadIniFile(string(szMyDrive)+string(szMyDir)+strUnitedPrefix+".ini") ;
    #else
    LoadIniFile(string(szMyDrive)+string(szMyDir)+strUnitedPrefix+".ini") ;
    LoadIniFile(string(szMyDrive)+string(szMyDir)+string(szMyName)+".ini") ;
    #endif
    //�`�����l���t�@�C���̃��[�h
    DBGOUT("Channel File Try Loading...\r\n") ;
    if(!LoadChannelFile(string(szMyDrive)+string(szMyDir)+string(szMyName)+".ch.txt"))
      LoadChannelFile(string(szMyDrive)+string(szMyDir)+strUnitedPrefix+".ch.txt") ;
    return TRUE ;
}

bool CBonTuner::LoadIniFile(string strIniFileName)
{
  if(_access(strIniFileName.c_str(), 0)) return false ;
  const DWORD BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  ZeroMemory(buffer, BUFFER_SIZE);
  const char *Section = "BonTuner";
  DBGOUT("Loading Ini \"%s\"...\n",strIniFileName.c_str()) ;
  #define LOADSTR2(val,key) do { \
	  GetPrivateProfileStringA(Section,key,val.c_str(), \
		buffer,BUFFER_SIZE,strIniFileName.c_str()) ; \
	  val = buffer ; \
	}while(0)
  #define LOADSTR(val) LOADSTR2(val,#val)
  #define LOADINT(key) do { \
	  string temp("") ; \
	  LOADSTR2(temp,#key); \
	  key = acalci(temp.c_str(),key); \
	}while(0)
  #define LOADWSTR(val) do { \
	  string temp = wcs2mbcs(val) ; \
	  LOADSTR2(temp,#val) ; val = mbcs2wcs(temp) ; \
	}while(0)
  LOADINT(TSDATASIZE) ;
  LOADINT(TSQUEUENUM) ;
  LOADINT(TSTHREADWAIT) ;
  LOADINT(TSTHREADPRIORITY) ;
  LOADINT(TSWRITEBACK) ;
  LOADINT(TSWRITEBACKDUMMY) ;
  LOADINT(TSALLOCWAITING) ;
  LOADINT(TSALLOCPRIORITY) ;
  LOADINT(ASYNCTSQUEUENUM) ;
  LOADINT(ASYNCTSQUEUEMAX) ;
  LOADINT(ASYNCTSEMPTYBORDER) ;
  LOADINT(ASYNCTSEMPTYLIMIT) ;
  LOADINT(AUTOPOWERON) ;
  LOADINT(AUTOPOWEROFF) ;
  LOADINT(U3BSFIXTUNE) ;
  LOADINT(U3BSFIXWAIT) ;
  LOADINT(U3BSFIXCH) ;
  LOADINT(AUTOTUNEBACKTONHK) ;
  LOADINT(IRNUMBER) ;
  LOADINT(IRLOCK) ;
  LOADINT(EXCLXFER) ;
  LOADINT(COMMANDSENDINTERVAL) ;
  LOADINT(COMMANDSENDTIMES) ;
  LOADINT(COMMANDSENDWAIT) ;
  LOADINT(CHANNELCHANGEWAIT) ;
  LOADINT(BUTTONTXWAIT) ;
  LOADINT(BUTTONPRESSTIMES) ;
  LOADINT(BUTTONPRESSWAIT) ;
  LOADINT(BUTTONINTERIMWAIT) ;
  LOADINT(BUTTONRELEASETIMES) ;
  LOADINT(BUTTONRELEASEWAIT) ;
  LOADINT(BUTTONSPACEWAIT) ;
  LOADINT(BUTTONPOWERWAIT) ;
  LOADINT(BUTTONPOWEROFFDELAY) ;
  LOADINT(REDUCESPACECHANGE) ;
  #undef LOADINT
  #undef LOADSTR2
  #undef LOADSTR
  #undef LOADWSTR
  return true ;
}

bool CBonTuner::LoadChannelFile(string strChannelFileName)
{
  FILE *st=NULL ;
  fopen_s(&st,strChannelFileName.c_str(),"rt") ;
  if(!st) return false;
  char s[512] ;
  m_Channels.clear() ;
  while(!feof(st)) {
    s[0]='\0' ;
    fgets(s,512,st) ;
    int t=0 ;
    string params[3] ;
    params[0] = params[1] = params[2] = "" ;
    for(int i=0;s[i];i++) {
      if(s[i]==';') break ;
      else if(s[i]==',') {
        t++ ; if(t==3) break ;
      }else params[t] += s[i] ;
    }
    if(t>=2) {
      //DBGOUT("Channel(%d)= %s, %s, %s\r\n",m_Channels.size(),trim(params[0]).c_str(),trim(params[1]).c_str(),trim(params[2]).c_str()) ;
      CHANNEL channel(mbcs2wcs(trim(params[0])),mbcs2wcs(trim(params[1])),trim(params[2])) ;
      m_Channels.push_back(channel) ;
      TRACE(L"Channel(%d): %s, %s, %s\r\n",m_Channels.size()
        ,channel.c_space(),channel.c_name(),mbcs2wcs(channel.rcode).c_str()) ;

    }
  }
  DBGOUT("Channel File Loaded. (Total %d Channels)\r\n",m_Channels.size()) ;
  fclose(st) ;
  return true ;
}


BYTE CBonTuner::ConvButtonToCode(BYTE button)
{
    switch(button){
        case 'X':
            return REMOCON_CHIDEJI;
        case 'Y':
            return REMOCON_BS;
        case 'Z':
            return REMOCON_CS;
        case 'D' :
            return REMOCON_3DIGITS;

        case '0':
            return REMOCON_NUMBER_0;    //A�ƈꏏ�Ȃ킯�ł���
        case '1':
            return REMOCON_NUMBER_1;
        case '2':
            return REMOCON_NUMBER_2;
        case '3':
            return REMOCON_NUMBER_3;
        case '4':
            return REMOCON_NUMBER_4;
        case '5':
            return REMOCON_NUMBER_5;
        case '6':
            return REMOCON_NUMBER_6;
        case '7':
            return REMOCON_NUMBER_7;
        case '8':
            return REMOCON_NUMBER_8;
        case '9':
            return REMOCON_NUMBER_9;
        case 'A':
            return REMOCON_NUMBER_10;
        case 'B':
            return REMOCON_NUMBER_11;
        case 'C':
            return REMOCON_NUMBER_12;

        case 'P':
            return REMOCON_POWERON;
        case 'Q':
            return REMOCON_POWEROFF;

        default:
            return 0;
    }
}

BOOL CBonTuner::IRButtonTX(BYTE button)
{
  BYTE code = ConvButtonToCode(button) ;

  // DT-300 �Â��t�@�[���E�F�A�� BS �`���[�j���O�o�O�΍� Method #2
  if( U3BSFIXTUNE==2 && !m_bU3BSFixDone && code==REMOCON_BS ) {
    return m_bU3BSFixDone = U3BSFixTune() ;
  }

  return IRCodeTX(code) ;
}

LPCTSTR CBonTuner::GetTunerName(void)
{
    // �`���[�i����Ԃ�
    return m_szTunerName ;
}

const BOOL CBonTuner::IsTunerOpening(void)
{
    // �`���[�i�̎g�p���̗L����Ԃ�(�S�v���Z�X��ʂ���)
    if( m_pUsbFx2Driver != NULL ){
        return TRUE;
    }else{
        return FALSE;
    }
}

LPCTSTR CBonTuner::EnumTuningSpace(const DWORD dwSpace)
{
    // 0�͍ŏ��̋�Ԃ�Ԃ�
    if ( dwSpace == 0 ) return m_Channels[0].c_space() ;
    // ���̋�Ԗ���T��
    DWORD dwSpcIdx = 0 ;
    for ( CHANNELS::size_type nIdx0=0, nIdx=0 ; nIdx < m_Channels.size() ; nIdx++ ) {
        if ( m_Channels[nIdx0].space == m_Channels[nIdx].space ) continue ;
        nIdx0 = nIdx ;
        dwSpcIdx++ ;
        if ( dwSpace == dwSpcIdx ) return m_Channels[nIdx].c_space() ;
    }
    return NULL ;
}

LPCTSTR CBonTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
    // ��Ԗ����擾����
    LPCTSTR pszSpace = EnumTuningSpace( dwSpace ) ;
    if ( !pszSpace ) return NULL ;
    wstring space = wstring(pszSpace) ;
    // ��Ԗ��̈�v����`�����l����T��
    DWORD dwChIdx = 0 ;
    for ( CHANNELS::size_type nIdx = 0 ; nIdx < m_Channels.size() ; nIdx++ ) {
        if ( space != m_Channels[nIdx].space ) continue ;
        if ( dwChIdx == dwChannel ) return m_Channels[nIdx].c_name() ;
        dwChIdx++ ;
    }
    return NULL ;
}

const BOOL CBonTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
    // ��Ԗ����擾����
    LPCTSTR pszSpace = EnumTuningSpace( dwSpace ) ;
    if ( !pszSpace ) return FALSE ;
    wstring space = wstring(pszSpace) ;
    // ��Ԗ��̈�v����`�����l����T��
    DWORD dwChIdx = 0 ;
    for ( CHANNELS::size_type nIdx = 0 ; nIdx < m_Channels.size() ; nIdx++ ) {
        if ( space != m_Channels[nIdx].space ) continue ;
        if ( dwChIdx == dwChannel ) {
            // �`�����l����ݒ肷��
            if( SetChannel((BYTE)nIdx) == TRUE ){
                m_dwCurSpace = dwSpace;
                m_dwCurChannel = dwChannel;
                return TRUE;
            }else{
                return FALSE;
            }
        }
        dwChIdx++ ;
    }
    return FALSE ;
}

const DWORD CBonTuner::GetCurSpace(void)
{
    // ���݂̃`���[�j���O��Ԃ�Ԃ�
    return m_dwCurSpace;
}

const DWORD CBonTuner::GetCurChannel(void)
{
    // ���݂̃`�����l����Ԃ�
    return m_dwCurChannel;
}

