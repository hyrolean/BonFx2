// BonTuner.cpp: CBonTuner クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <SetupApi.h>
#include <functional>
#include <iterator>
#include <Malloc.h>
#include <InitGuid.h>
#include <Math.h>
#include <assert.h>
#include "Resource.h"

#include "BonTuner.h"
#include "HRTimer.h"
#include "TC90532\TC90532.h"
#include "TC90532\MxL5007.h"


#pragma comment(lib, "SetupApi.lib")


//////////////////////////////////////////////////////////////////////
// 定数定義
//////////////////////////////////////////////////////////////////////

// 受信サイズ
DWORD TSDATASIZE        =   48128UL  ;       // TSデータのサイズ
DWORD TSQUEUENUM        =   36UL     ;       // TSデータの環状ストック数(最大数63まで)
DWORD TSTHREADWAIT      =   2000UL   ;       // TSスレッドキュー毎に待つ最大時間
int   TSTHREADPRIORITY  =   THREAD_PRIORITY_HIGHEST ; // TSスレッドの優先度
BOOL  TSWRITEBACK       =   TRUE     ;       // TSデータの書き戻しによるバッファ書き高速化
BOOL  TSWRITEBACKDUMMY  =   FALSE    ;       // TSデータの書き戻し残量がない場合にダミー領域を有効にするかどうか
BOOL  TSALLOCWAITING    =   FALSE    ;       // TSデータのアロケーションの完了を待つかどうか
int   TSALLOCPRIORITY   =   THREAD_PRIORITY_HIGHEST ; // TSアロケーションスレッドの優先度
BOOL  TSDROPNULLPACKETS =   TRUE     ;       // TSデータのヌルパケットを除去するかどうか

// FIFOバッファ設定
DWORD ASYNCTSQUEUENUM    =   66UL   ;        // 非同期TSデータの環状ストック数(初期値)
DWORD ASYNCTSQUEUEMAX    =   660UL  ;        // 非同期TSデータの環状ストック最大数
DWORD ASYNCTSEMPTYBORDER =   22UL   ;        // 非同期TSデータの空きストック数底値閾値(アロケーション開始閾値)
DWORD ASYNCTSEMPTYLIMIT  =   11UL   ;        // 非同期TSデータの最低限確保する空きストック数(オーバーラップからの保障)

// 排他ロック
BOOL EXCLXFER            =   TRUE   ;

// ウェイト
DWORD ISDBTCOMMANDSENDTIMES = 2 ;
DWORD ISDBTCOMMANDSENDWAIT  = 100 ;
DWORD ISDBSCOMMANDSENDTIMES = 1 ;
DWORD ISDBSCOMMANDSENDWAIT  = 100 ;
DWORD ISDBSSETTSIDTIMES		= 2 ;
DWORD ISDBSSETTSLOCKWAIT    = 10 ;
DWORD ISDBSSETTSIDWAIT		= 800 ;
DWORD CHANNELWAIT   		= 800 ;

// 高精度割込タイマー
BOOL USEHRTIMER = FALSE ; // ハイレゾリューションタイマー使用有無

// 既定のチャンネル情報
BOOL DEFSPACEVHF               = FALSE ; // VHFを含めるかどうか
BOOL DEFSPACEUHF               = TRUE  ; // UHFを含めるかどうか
BOOL DEFSPACECATV              = FALSE ; // CATVを含めるかどうか
BOOL DEFSPACEBS                = TRUE  ; // BSを含めるかどうか
int  DEFSPACEBSSTREAMS         = 8     ; // BSの各ストリーム数(0-8)
BOOL DEFSPACEBSSTREAMSTRIDE    = FALSE ; // BSをストリーム基準に配置するかどうか
BOOL DEFSPACECS110             = TRUE  ; // CS110を含めるかどうか
BOOL DEFSPACECS110STREAMSTRIDE = FALSE ; // CS110をストリーム基準に配置するかどうか
int  DEFSPACECS110STREAMS      = 8     ; // CS110の各ストリーム数(0-8)

// 高速スキャン対応
BOOL FASTSCAN	= FALSE ;

// エンドポイントインデックス
#define EPINDEX_IN			0UL
#define EPINDEX_OUT			1UL

// コマンド
#define CMD_EP6IN_START		0x50U	//
#define	CMD_EP6IN_STOP		0x51U	//
#define	CMD_EP2OUT_START	0x52U	//
#define	CMD_EP2OUT_STOP		0x53U	//
#define	CMD_PORT_CFG		0x54U	//addr_mask, out_pins
#define	CMD_REG_READ		0x55U	//addr	(return 1byte)
#define	CMD_REG_WRITE		0x56U	//addr, value
#define	CMD_PORT_READ		0x57U	//(return 1byte)
#define	CMD_PORT_WRITE		0x58U	//value
#define	CMD_IFCONFIG		0x59U	//value
#define	CMD_MODE_IDLE		0x5AU
#define CMD_EP4IN_START		0x5BU	//
#define	CMD_EP4IN_STOP		0x5CU	//
#define	CMD_IR_CODE			0x5DU	//val_l, val_h (0x0000:RBUF capture 0xffff:BWUF output)
#define CMD_IR_WBUF			0x5EU	//ofs(0 or 64 or 128 or 192), len(max 64), data, ....
#define	CMD_IR_RBUF			0x5FU	//ofs(0 or 64 or 128 or 192) (return 64byte)
#define	CMD_I2C_READ		0x60	//adrs,len (return length bytes)(max 16bytes)
#define	CMD_I2C_WRITE		0x61	//adrs,len,data... (max 16bytes)

#define	PIO_START			0x20
#define	PIO_IR_OUT			0x10
#define	PIO_IR_IN			0x08
#define PIO_TS_BACK			0x04

#define	DEFFWSIZE			3905

// FX2ファームウェア
static BYTE BulldogFirmWare[8*1024];

static BYTE	DefBulldogFW[]=
#include	"BulldogFW.inc"


using namespace std ;

//////////////////////////////////////////////////////////////////////
// インスタンス生成メソッド
//////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4273 )

extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{
	// スタンス生成(既存の場合はインスタンスのポインタを返す)
	return (CBonTuner::m_pThis)? CBonTuner::m_pThis : ((IBonDriver *) new CBonTuner());
}

#pragma warning( default : 4273 )


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

// 静的メンバ初期化
CBonTuner * CBonTuner::m_pThis = NULL;
HINSTANCE CBonTuner::m_hModule = NULL;

//*****	Constructor	*****

CBonTuner::CBonTuner()
	: m_pUsbFx2Driver(NULL)
	, m_hOnStreamEvent(NULL)
	, m_AsyncTSFifo(NULL)
	, m_hMutex(NULL)
	, is_channel_valid(FALSE)
{
	//char	bf[256];
	//char	key[8];

	m_pThis = this;
	fOpened=false;
    fModeTerra=true ;

	m_yFx2Id = 0 ;
	InitTunerProperty();
}

//*****	Destructor	*****

CBonTuner::~CBonTuner()
{
//	OutputDebugString(_T("~CBonTuner\n"));

	// 開かれてる場合は閉じる
	CloseTuner();

	m_pThis = NULL;

//	OutputDebugString(_T("~CBonTuner-ret\n"));
}

//*****	Open Tuner	*****

const BOOL CBonTuner::OpenTuner()
{

//	OutputDebugString(_T("OpenTuner\n"));

	// 一旦クローズ
	CloseTuner();

	// カメレオンUSB FX2のドライバインスタンス生成
	m_pUsbFx2Driver = new CUsbFx2Driver(this);
	if(!m_pUsbFx2Driver)return false;

	// ストリーム一時停止
	is_channel_valid = FALSE ;

	// FX2の初期化シーケンス
	try{
		// 非同期FIFOバッファオブジェクト作成
		m_AsyncTSFifo = new CAsyncFifo(
		  ASYNCTSQUEUENUM,ASYNCTSQUEUEMAX,ASYNCTSEMPTYBORDER,
		  TSDATASIZE,TSTHREADWAIT,TSALLOCPRIORITY ) ;
		m_AsyncTSFifo->SetEmptyLimit(ASYNCTSEMPTYLIMIT) ;
		m_TSWriteBackDummyCache.resize(TSDATASIZE);

		// イベント作成
		if(!(m_hOnStreamEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL)))throw (const DWORD)__LINE__;

		int sz=sizeof(DefBulldogFW);
		memcpy(BulldogFirmWare,DefBulldogFW,sz);

		#ifdef _DEBUG
		FILE	*fp;
		fopen_s(&fp, "Bulldog.fw","rb");
		if (fp)
		{
			sz=fread(BulldogFirmWare, 1, sizeof(BulldogFirmWare), fp);
			if (sz==0)
			{
				sz=sizeof(DefBulldogFW);
				memcpy(BulldogFirmWare,DefBulldogFW,sz);
				puts("FW Load Error");
			}
			fclose(fp);
		}
		#endif

		// ドライバオープン
		if(!m_pUsbFx2Driver->OpenDriver(m_yFx2Id, BulldogFirmWare, sz,/*"Bulldog_FIFO"*/"FX2_FIFO"))throw (const DWORD)__LINE__;				//xxxxx

		// エンドポイント追加
		if(!m_pUsbFx2Driver->AddEndPoint(0x81U))throw (const DWORD)__LINE__;	// EPINDEX_IN
		if(!m_pUsbFx2Driver->AddEndPoint(0x01U))throw (const DWORD)__LINE__;	// EPINDEX_OUT


		// FX2 I/Oポート設定　＆　MAX2初期パラメータ設定 ＆ MAX2リセット
//		if(!m_pUsbFx2Driver->TransmitFormatedData(EPINDEX_OUT, 6UL, CMD_PORT_CFG, 0x00U, PIO_START | PIO_IR_OUT | PIO_TS_BACK, CMD_MODE_IDLE, CMD_IFCONFIG, 0xE3U))throw (const DWORD)__LINE__;
		if(!m_pUsbFx2Driver->TransmitFormatedData(EPINDEX_OUT, 6UL, CMD_PORT_CFG, 0x00U, PIO_START, CMD_MODE_IDLE, CMD_IFCONFIG, 0xE3U))throw (const DWORD)__LINE__;

		// スレッド起動
		if(!m_pUsbFx2Driver->CreateFifoThread(0x86U, &m_dwFifoThreadIndex, TSDATASIZE, TSQUEUENUM, TSTHREADWAIT, TSTHREADPRIORITY, TSWRITEBACK))throw (const DWORD)__LINE__;

		// 開始コマンド送信
//		if(!m_pUsbFx2Driver->TransmitFormatedData(EPINDEX_OUT, 3UL, CMD_EP6IN_START, CMD_PORT_WRITE, PIO_START | PIO_IR_OUT | PIO_TS_BACK))throw (const DWORD)__LINE__;
		if(!m_pUsbFx2Driver->TransmitFormatedData(EPINDEX_OUT, 3UL, CMD_EP6IN_START, CMD_PORT_WRITE, PIO_START))throw (const DWORD)__LINE__;

		// ミューテックス作成
		if(!(m_hMutex = ::CreateMutex(NULL, TRUE, m_szTunerName)))throw (const DWORD)__LINE__;

		// 成功
	}
	catch(...)
	{
		// エラー発生
		CloseTuner();

		return FALSE;
	}

	fOpened=true;
    SuspendTC90502(fModeTerra?ISDB_T:ISDB_S,FALSE);

	return(TRUE);
}

//*****	Close Tuner	*****

void CBonTuner::CloseTuner()
{
//	OutputDebugString(_T("CloseTuner\n"));

    if(fModeTerra) {
      if(fOpened) {
        SetMxl5007(0); // Terra Sleep
        SuspendTC90502(ISDB_T,TRUE);
      }
    }else if(fOpened) {
      SuspendTC90502(ISDB_S,TRUE);
    }

	fOpened=false;
	is_channel_valid = FALSE ;

	// ドライバを閉じる
	if(m_pUsbFx2Driver)
	{
		m_pUsbFx2Driver->CloseDriver();
		delete m_pUsbFx2Driver;
		m_pUsbFx2Driver = NULL;
	}

	// ハンドルを閉じる
	if(m_hOnStreamEvent)
	{
		::CloseHandle(m_hOnStreamEvent);
		m_hOnStreamEvent = NULL;
	}

	// ミューテックス開放
	if(m_hMutex)
	{
		::ReleaseMutex(m_hMutex);
		::CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}

	// 非同期FIFOバッファオブジェクト破棄
	if(m_AsyncTSFifo) {
	  delete m_AsyncTSFifo ;
	  m_AsyncTSFifo=NULL ;
	}

//	OutputDebugString(_T("CloseTuner-ret\n"));
}

//*****	Set TSID	*****

bool	CBonTuner::SetTSID(int tsid)
{
	if(WriteReg(DEMODSADRS, 0x8f, ((tsid >> 8) & 0xff))) return false ;		// MSB of iits
	if(WriteReg(DEMODSADRS, 0x90, (tsid & 0xff))) return false ;			// LSB of iits
	return true ;
}

//*****	Get TSID	*****

WORD	CBonTuner::GetTSID()
{
    BYTE data[2] ;
    int ln=I2CRead(DEMODSADRS, 0xe6,2,data) ; // tsido
    if(ln!=2) return 0 ;
    return data[0]<<8 | data[1] ; // big endian
}

//*****	Select TSID	*****
/*
	Select the TS stream that matched to the lower 3-bits of tsid0-tsid7 WORDs
	extracted from the register (CEh-DDh) of the PSK decoder.

	stream:	0-7		stream number
    result:			TSID
            0		: not matched
            other	: matched TSID
*/
WORD  CBonTuner::SelectTSID(BYTE stream/*0-7*/)
{
    BYTE data[16] ;
    fill_n(data,16,0xFF) ;
    int ln=I2CRead(DEMODSADRS, 0xCE,16,data) ; // tsid0 - tsid7
    if(!ln) return 0 ;
    for(int i=0;i+1<ln;i+=2) {
      WORD tsid = data[i+0]<<8 | data[i+1] ; // big endian
      if(tsid!=0&&tsid!=0xFFFF&&((tsid&7)==stream||stream==255))
        return tsid ;
    }
    return 0 ;
}

//*****	Enum TSID	*****

DWORD  CBonTuner::EnumTSID(DWORD *lpTSID, DWORD dwNum)
{
    BYTE data[16] ;
    fill_n(data,16,0xFF) ;
    int ln=I2CRead(DEMODSADRS, 0xCE,16,data) ;
    if(!ln) return 0 ;
    fill_n(lpTSID,dwNum,0xFFFFFFFF) ;
    DWORD n=0;
    for(int i=0;n<dwNum&&i+1<ln;i+=2) {
      WORD tsid = data[i+0]<<8 | data[i+1] ; // big endian
      lpTSID[n++] = tsid==0 || tsid==0xFFFF ? 0xFFFFFFFF : DWORD(tsid) ;
    }
    return n ;
}

//*****	Set ISDB-T Channel	*****
/*
	ch:物理チャンネル
	ch:1-12		VHF
	ch:13-62	UHF
	ch:113-163	ケーブルTV
*/
bool	CBonTuner::SetISDBTChannel(int ch,DWORD kHz)
{
	DBGOUT("SetISDBTChannel: ch=%d kHz=%d\n",ch,kHz) ;

	bool tuned = false;
	for(DWORD i=ISDBTCOMMANDSENDTIMES;i;i--) {
	  //if(i) Sleep(100) ;
	  SetMxl5007(ch,kHz);
	  ::HRSleep(30);
	  tuned=false ;
	  if(TC90502_OK==SetTC90502(ISDB_T)) {
		  tuned = true ;
	  }
  	  if(i>1) HRSleep(ISDBTCOMMANDSENDWAIT);
	}
	DBGOUT("SetISDBTChannel: tuning %s.\n",tuned?"succeeded":"failed") ;
	return tuned ;
}

bool	CBonTuner::SetISDBSFreq(int ch, DWORD kHz)
{
  bool ok=true ;
  SetStv6110a(ch,kHz);
  ::HRSleep(30);
  ok = TC90502_OK==SetTC90502(ISDB_S) ;
  if(TC90502_OK==SetTC90502(ISDB_S)) {
    ok = ResetTC90502(ISDB_S) == TC90502_OK ;
    DBGOUT("SetISDBSFreq: reset DEMOD %s.\n",ok?"done":"failed") ;
  }
  return ok;
}

//*****	Set ISDB-S Channel	*****
/*
	ch:トランスポンダ番号
	ch:1-23 (奇数)	BS
	ch:2-24 (偶数)	CS
*/
// MARK : bool CBonTuner::SetISDBSChannel(int ch,WORD stream,WORD tsid,DWORD kHz)
bool	CBonTuner::SetISDBSChannel(int ch,WORD stream,WORD tsid,DWORD kHz)
{
	DBGOUT("SetISDBSChannel: ch=%d stream=%d tsid=%d kHz=%d\n",ch,stream,tsid,kHz) ;

	bool tuned = false;
	for(DWORD i=ISDBSCOMMANDSENDTIMES;i;i--) {
		bool ok = SetISDBSFreq(ch, kHz) ;
        //bool reset=false ;
        if(ok) {
          SetTSID(0);
          bool locked = false ;
          tuned=false ;
          for(DWORD j=ISDBSSETTSIDTIMES;j;j--) {
            for(DWORD e=0,s=Elapsed();ISDBSSETTSIDWAIT>e;e=Elapsed(s)) {
              if(!locked) {
                locked = IsLockISDBS()==stLock ;
                if(locked) {
                  ::HRSleep(ISDBSSETTSLOCKWAIT) ;
                }
              }else {
                if(!tsid||tsid==0xFFFF) {
                  //::HRSleep(40) ;
                  tsid = stream<8 ? SelectTSID((BYTE)stream) : stream ;
                  if(tsid!=0&&tsid!=0xFFFF) {
                    DBGOUT("SetISDBSChannel: TSID=%04x\n",tsid) ;
                  }
                }
                if(tsid!=0&&tsid!=0xFFFF&&SetTSID(tsid)) {
                  tuned = true ;
                  break ;
                }
              }
              ::HRSleep(0,500) ;
            }
            if(j>1&&tuned)
              ::HRSleep(40) ;
          }
        }
        if(i>1) HRSleep(ISDBSCOMMANDSENDWAIT);
	}
/*
	DWORD	tm=GetTickCount()+3000;

	while(GetTickCount()<tm)
	{
		if (IsLockISDBS()==stLock)
			break;
	}
*/
	DBGOUT("SetISDBSChannel: tuning %s.\n",tuned?"succeeded":"failed") ;
	return tuned ;
}

//*****	Check ISDB-T Lock	*****

CLockStatus	CBonTuner::IsLockISDBT(void)
{
	BYTE	st,seq;

	ReadReg(DEMODTADRS,0x80,&st);
	ReadReg(DEMODTADRS,0xB0,&seq);

	if (((st & 0x28)==0) && ((seq & 0x0F)>=8))
		return(stLock);

	if ( (st & 0x80) == 0x80 )									//リトライオーバー
		return(stNoSignal);											//アンロック

	return(stLostLock);												//ロスト(呼び出し元でタイムアウトチェック)
}

//*****	Check ISDB-S Lock	*****

CLockStatus	CBonTuner::IsLockISDBS(void)
{
	BYTE	st;

	ReadReg(DEMODSADRS,0xC3,&st);

	if ((st & 0x10)==0)
		return(stLock);

	return(stLostLock);
}

//*****	Check Lock Status	*****

CLockStatus	CBonTuner::IsLock(void)
{
	DWORD ch = GetCurRealChannel() ;
    CHANNEL &channel =
        m_dwCurChannel & TRANSPONDER_CHMASK ?
            m_Transponders[ch] : m_Channels[ch] ;
	if (channel.isISDBT())
		return(IsLockISDBT());
	else if (channel.isISDBS())
		return(IsLockISDBS());
	return stUnknown ;
}

//*****	Get C/N	*****

double	CBonTuner::GetCN(void)
{
	unsigned char	Data[3];
	int				cndata;
	double			cn;
	double			p,p2,p3,p4;

	if (!fOpened||!is_channel_valid)
		return(0);

	DWORD ch = GetCurRealChannel() ;

    CHANNEL &channel =
        m_dwCurChannel & TRANSPONDER_CHMASK ?
            m_Transponders[ch] : m_Channels[ch] ;

	if (channel.isISDBT())														//ISDB-T
	{
		I2CRead(DEMODTADRS,0x8B,3,Data);
		cndata=(Data[0]<<16) | (Data[1]<<8) | Data[2];
		p=10*log10((double)5505024/cndata);
		p2=p*p;
		p3=p2*p;
		p4=p3*p;
		cn=(double)0.000024*p4-0.0016*p3+0.0398*p2+0.5491*p+3.0965;
	}
	else if(channel.isISDBS()) {																	//BS/CS
		I2CRead(DEMODSADRS,0xBC,2,Data);
		cndata=(Data[0]<<8) | Data[1];
		if (cndata>=3000)
		{
			p=sqrt((double)cndata-3000)/64.0;
			cn=-1.6346*pow(p,5)+14.341*pow(p,4)-50.259*pow(p,3)+88.977*p*p-89.565*p+58.857;
		}
		else {
			cn=0;
		}
	}else {
	  cn = 0 ;
	}

	if ((cn<0) || (cndata==0))
		cn=0;

	return(cn);								//[dB]
}

//*****	Get BER	*****

double	CBonTuner::GetBER(void)
{
	BYTE	darray[3],carray[2];
	int		berdata,packetcycle;
	double	ber;

	DWORD ch = GetCurRealChannel() ;

    CHANNEL &channel =
        m_dwCurChannel & TRANSPONDER_CHMASK ?
            m_Transponders[ch] : m_Channels[ch] ;

	if (channel.isISDBT())														//ISDB-T
	{
		I2CRead(DEMODTADRS,0xA0,3,darray);
		I2CRead(DEMODTADRS,0xA6,2,carray);
	}
	else if(channel.isISDBS()) {																	//BS/CS
		I2CRead(DEMODTADRS+1,0xEB,3,darray);
		I2CRead(DEMODTADRS+1,0xEE,2,carray);
	}else {
		return 0.0 ;
	}

	packetcycle=(carray[0]<<8) | carray[1];
	berdata=(darray[0]<<16) | (darray[1]<<8) | darray[2];

	if (packetcycle)
			ber=(double)berdata/(204*8*packetcycle);
	else	ber=0;

	return(ber);
}

void CBonTuner::ResetFxFifo(bool fPause)
{
	if(m_pUsbFx2Driver)
	  m_pUsbFx2Driver->ResetFifoThread(m_dwFifoThreadIndex,fPause) ;
}

bool CBonTuner::ResetDemod()
{
  if(ResetTC90502(fModeTerra?ISDB_T:ISDB_S)!=TC90502_OK) return false;
  bool locking=false ;
  for(DWORD e=0,s=Elapsed();CHANNELWAIT>e;e=Elapsed(s)) {
    HRSleep(40);
    locking = (fModeTerra?IsLockISDBT():IsLockISDBS())==stLock ;
    if(locking) break ;
  }
  if(!locking) return false ;
  if(fModeTerra) {
    SetNuValTC90502(fModeTerra?ISDB_T:ISDB_S,TSDROPNULLPACKETS) ;
  }else {
    SetNuValTC90502(ISDB_S,TSDROPNULLPACKETS) ;
  }
  HRSleep(40);
  return true ;
}


//*****	SetRealChannel	*****

const BOOL CBonTuner::SetRealChannel(const DWORD dwCh)
{
	//ストリーム一時停止
	is_channel_valid = FALSE;

	if(dwCh >= m_Channels.size()) {
		return FALSE;
	}

	// Fx側バッファ停止
	ResetFxFifo(true) ;
	// 撮り溜めたTSストリームの破棄
	PurgeTsStream();

	//チューニング
	bool tuned=false ;
	if(m_Channels[dwCh].isISDBT()) {  // 地上波
      if(!fModeTerra) {
        SuspendTC90502(ISDB_S,TRUE);
        SuspendTC90502(ISDB_T,FALSE);
        HRSleep(40);
      }
	  tuned = SetISDBTChannel(m_Channels[dwCh].Channel,
	    m_Channels[dwCh].Freq) ;
      fModeTerra=true ;
	}else if(m_Channels[dwCh].isISDBS()) { // BS/CS
      if(fModeTerra) {
        SetMxl5007(0) ; // Terra Suspend
        SuspendTC90502(ISDB_T,TRUE);
        SuspendTC90502(ISDB_S,FALSE);
        HRSleep(40);
      }
      WORD stream = m_Channels[dwCh].Stream ;
	  WORD tsid = m_Channels[dwCh].TSID ;
	  tuned = SetISDBSChannel(m_Channels[dwCh].Channel,
	    stream, tsid,m_Channels[dwCh].Freq) ;
	  fModeTerra=false ;
	}else {
	  return FALSE ;
	}

	if(tuned) {
	  if(!ResetDemod()) tuned = false ;
	}

	// Fx側バッファ初期化
	ResetFxFifo(false) ;

	//ストリーム再開
	is_channel_valid = tuned ? TRUE : FALSE ;

	return FASTSCAN&&!tuned ? FALSE : TRUE ;
}


//*****	Set Channel	*****

const BOOL CBonTuner::SetChannel(const BYTE ch)
{
	if(m_ChannelAnchors.size()<=ch) return FALSE ;

	if(!SetRealChannel(m_ChannelAnchors[ch])) return FALSE ;

	// チャンネル情報を更新
	m_dwCurSpace=space_index_of(ch) ;
	m_dwCurChannel=channel_index_of(ch) ;

	return TRUE ;
}

//*****	Get Signal Level	*****

const float CBonTuner::GetSignalLevel(void)
{
    double	d=GetCN();

	return((float)d);
}

//*****	Wait TS Stream	*****

const DWORD CBonTuner::WaitTsStream(const DWORD dwTimeOut)
{
//	OutputDebugString(_T("WaitTsStream\n"));
//	return 0UL;

	// 終了チェック
	if(!m_pUsbFx2Driver)return WAIT_ABANDONED;

	// バッファ済みの場合は無駄に待機せずに制御を戻す @ 2014/01/26(Sun)
	if(!m_AsyncTSFifo->Empty()) return WAIT_OBJECT_0;

	// イベントがシグナル状態になるのを待つ
	const DWORD dwRet = ::WaitForSingleObject(m_hOnStreamEvent, dwTimeOut);

	switch(dwRet){
		case WAIT_ABANDONED :
			// チューナが閉じられた
			return WAIT_ABANDONED;

		case WAIT_OBJECT_0 :
		case WAIT_TIMEOUT :
			// ストリーム取得可能 or チューナが閉じられた
			return (m_pUsbFx2Driver)? dwRet : WAIT_ABANDONED;

		case WAIT_FAILED :
		default:
			// 例外
			return WAIT_FAILED;
		}
}

//*****	Get Ready Count	*****

const DWORD CBonTuner::GetReadyCount()
{
	// 取り出し可能TSデータ数を取得する
	return (DWORD)m_AsyncTSFifo->Size();
}

//*****	Get TS Stream	*****

const BOOL CBonTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	BYTE *pSrc = NULL;

	if (fOpened==false)
		return(FALSE);

	// TSデータをバッファから取り出す
	if(GetTsStream(&pSrc, pdwSize, pdwRemain))
	{
		if(*pdwSize)
		{
			CopyMemory(pDst, pSrc, *pdwSize);
		}

		return TRUE;
	}

	return FALSE;
}

//*****	Get TS Stream	*****

const BOOL CBonTuner::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	if (fOpened==false)
		return(FALSE);

	if(!m_pUsbFx2Driver)
		return FALSE;

	m_AsyncTSFifo->Pop(ppDst,pdwSize,pdwRemain) ;
	return TRUE ;
}

//*****	Purge TS Stream	*****

void CBonTuner::PurgeTsStream()
{
	m_AsyncTSFifo->Purge() ;
}

//*****	Get Tuner Name	*****

LPCTSTR CBonTuner::GetTunerName(void)
{
	// チューナ名を返す
	return m_szTunerName ;
}

//*****	Is Tuner Opening	*****

const BOOL CBonTuner::IsTunerOpening(void)
{
#if 1
	// チューナの使用中の有無を返す(全プロセスを通して)
	HANDLE hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, m_szTunerName);

	if(hMutex)
	{
		// 既にチューナは開かれている
		::CloseHandle(hMutex);
		return TRUE;
	}
	else{
		// チューナは開かれていない
		return FALSE;
	}
#else
	return fOpened ? TRUE : FALSE ;
#endif
}

//*****	Enum Tuning Space	*****

LPCTSTR CBonTuner::EnumTuningSpace(const DWORD dwSpace)
{
	if(m_SpaceAnchors.size()<=dwSpace)
	  return NULL ;
	return m_Channels[m_SpaceAnchors[dwSpace]].Space.c_str() ;
}

//*****	Enum Channel Name	*****

LPCTSTR CBonTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	if (is_invalid_space(dwSpace))
		return NULL;

	DWORD start = m_SpaceAnchors[dwSpace] ;
	DWORD end = dwSpace + 1 >= m_SpaceAnchors.size() ? (DWORD)m_Channels.size() : m_SpaceAnchors[dwSpace + 1];

	if(dwChannel<end-start)
	  return m_Channels[start+dwChannel].Name.c_str() ;

	return NULL ;
}

//*****	Set Channel	*****

const BOOL CBonTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if (is_invalid_space(dwSpace))
		return NULL;

	DWORD start = m_SpaceAnchors[dwSpace] ;
	DWORD end = dwSpace + 1 >= m_SpaceAnchors.size() ? (DWORD)m_Channels.size() : m_SpaceAnchors[dwSpace + 1];

	if(dwChannel<end-start) {
      if( SetRealChannel(start+dwChannel) ){
		m_dwCurSpace = dwSpace;
		m_dwCurChannel = dwChannel;
		return TRUE ;
	  }
	}
	return FALSE ;
}

//*****	Get Current Space	*****

const DWORD CBonTuner::GetCurSpace(void)
{
	// 現在のチューニング空間を返す
	return m_dwCurSpace;
}

//*****	Get Current Channel	*****

const DWORD CBonTuner::GetCurChannel(void)
{
	// 現在のチャンネルを返す
	return m_dwCurChannel;
}

//*****	Get Cur Real Channel	*****

DWORD CBonTuner::GetCurRealChannel() const
{

  if(m_dwCurSpace>=m_SpaceAnchors.size())
	return 0 ; // error
  DWORD start = m_SpaceAnchors[m_dwCurSpace] ;
  DWORD end = m_dwCurSpace + 1 >= m_SpaceAnchors.size() ? (DWORD)m_Channels.size() : m_SpaceAnchors[m_dwCurSpace + 1];
  DWORD rch = start + (m_dwCurChannel & ~TRANSPONDER_CHMASK) ;
  if(rch>=end)
	return 0 ; // error
  return rch ;
}


//*****	Release	*****

void CBonTuner::Release()
{
	// インスタンス開放
	delete this;
}

//*****	On Recive Fifo Data	*****

const bool CBonTuner::OnRecvFifoData(const DWORD dwThreadIndex, const BYTE *pData, const DWORD dwLen, CUsbFx2Driver *pDriver)
{
  if (dwThreadIndex == m_dwFifoThreadIndex) {
	if(!TSWRITEBACK) {
	  //有効なチャンネルが選択されている時に限って
	  if(is_channel_valid){
		// 非同期FIFOバッファにプッシュ
		if(m_AsyncTSFifo->Push(pData,dwLen)>0) {
		  // イベントセット
		  ::SetEvent(m_hOnStreamEvent);
		}
	  }
	}
  }
  return(true);
}

//***** On Xfer Lock *****

void CBonTuner::OnXferLock(const DWORD dwThreadIndex, bool locking, CUsbFx2Driver *pDriver)
{
  if(EXCLXFER) {
    if (dwThreadIndex == m_dwFifoThreadIndex) {
      if(locking)     m_coXfer.lock() ;
      else            m_coXfer.unlock() ;
    }
  }
}

//***** On Begin Write Back *****

BYTE *CBonTuner::OnBeginWriteBack(const DWORD dwThreadIndex, DWORD dwMaxSize, CUsbFx2Driver *pDriver)
{
  if (dwThreadIndex == m_dwFifoThreadIndex) {
	if(TSWRITEBACK) {
	  CAsyncFifo::CACHE *cache = m_AsyncTSFifo->BeginWriteBack(TSALLOCWAITING?true:false) ;
	  if(!cache)
		return !TSWRITEBACKDUMMY ? NULL : m_TSWriteBackDummyCache.data() ; // 容量オーバー(ダミー書き)
	  cache->resize(dwMaxSize) ;
	  m_TSWriteBackMap[cache->data()] = cache ;
	  return cache->data() ;
	}
  }
  return NULL ;
}

//***** On Finish Write Back *****

void CBonTuner::OnFinishWriteBack(const DWORD dwThreadIndex, BYTE *pData, DWORD dwWroteSize, CUsbFx2Driver *pDriver)
{
  if (dwThreadIndex == m_dwFifoThreadIndex) {
	if(TSWRITEBACK) {
	  WRITEBACKMAP::iterator pos = m_TSWriteBackMap.find(pData) ;
	  DWORD nWrote = is_channel_valid?dwWroteSize:0 ;
	  if(pData==m_TSWriteBackDummyCache.data()) {
		// ダミーなのでダブル書きなどにより壊れている可能性が高いが...
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
		m_TSWriteBackMap.erase(pos) ;
	  }
	}
  }
}

//***** Init Tuner Property *****

void CBonTuner::InitTunerProperty()
{
	//自分の名前を取得
	char szMyPath[_MAX_PATH] ;
	GetModuleFileNameA( m_hModule, szMyPath, _MAX_PATH ) ;
	char szMyDrive[_MAX_FNAME] ;
	char szMyDir[_MAX_FNAME] ;
	char szMyName[_MAX_FNAME] ;
	_splitpath_s( szMyPath, szMyDrive, _MAX_FNAME,szMyDir, _MAX_FNAME, szMyName, _MAX_FNAME, NULL, 0 ) ;
	_strupr_s( szMyName, sizeof(szMyName) ) ;

	// Fx2 の ID を決定
	int nFx2Id=0 ;
	sscanf_s( szMyName, "BONDRIVER_BULLDOG_DEV%d", &nFx2Id ) ;
	m_yFx2Id = BYTE(nFx2Id) ;

	//チューナー名を決定
	_stprintf_s( m_szTunerName, 100, TEXT("%s(ID=%d)"), TEXT("ブルドッグ"), nFx2Id ) ;

	// Ini ファイルをロード
	m_InvisibleSpaces.clear() ;
	m_InvalidSpaces.clear() ;
	m_SpaceArrangement.clear() ;
	LoadIniFile(string(szMyDrive)+string(szMyDir)+"BonDriver_Bulldog.ini") ;
	LoadIniFile(string(szMyDrive)+string(szMyDir)+string(szMyName)+".ini") ;

	// Channel ファイルをロード
	if(!LoadChannelFile(string(szMyDrive)+string(szMyDir)+string(szMyName)+".ch.txt")) {
	   if(!LoadChannelFile(string(szMyDrive)+string(szMyDir)+"BonDriver_Bulldog.ch.txt"))
		 InitChannelToDefault() ;
	}

	// チャンネル情報再構築
	RebuildChannels() ;
}

//*****	Load Ini File *****

bool    CBonTuner::LoadIniFile(std::string strIniFileName)
{
  if(GetFileAttributesA(strIniFileName.c_str())==-1) return false ;
  const DWORD BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  ZeroMemory(buffer, BUFFER_SIZE);
  const char *Section = "BonTuner";
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
  LOADINT(TSALLOCPRIORITY);
  LOADINT(TSDROPNULLPACKETS) ;
  LOADINT(ASYNCTSQUEUENUM) ;
  LOADINT(ASYNCTSQUEUEMAX) ;
  LOADINT(ASYNCTSEMPTYBORDER) ;
  LOADINT(ASYNCTSEMPTYLIMIT) ;
  LOADINT(EXCLXFER) ;
  LOADINT(DEFSPACEVHF);
  LOADINT(DEFSPACEUHF);
  LOADINT(DEFSPACECATV);
  LOADINT(DEFSPACEBS);
  LOADINT(DEFSPACEBSSTREAMS);
  LOADINT(DEFSPACEBSSTREAMSTRIDE);
  LOADINT(DEFSPACECS110);
  LOADINT(DEFSPACECS110STREAMS);
  LOADINT(DEFSPACECS110STREAMSTRIDE);
  LOADINT(FASTSCAN);
  LOADINT(ISDBTCOMMANDSENDTIMES) ;
  LOADINT(ISDBTCOMMANDSENDWAIT) ;
  LOADINT(ISDBSCOMMANDSENDTIMES) ;
  LOADINT(ISDBSCOMMANDSENDWAIT) ;
  LOADINT(ISDBSSETTSIDTIMES) ;
  LOADINT(ISDBSSETTSLOCKWAIT);
  LOADINT(ISDBSSETTSIDWAIT) ;
  LOADINT(CHANNELWAIT) ;
  LOADINT(USEHRTIMER) ;
  wstring InvisibleSpaces ;
  LOADWSTR(InvisibleSpaces) ;
  split(m_InvisibleSpaces,InvisibleSpaces,L',') ;
  vector<wstring> vInvalidSpaces ;
  wstring InvalidSpaces ;
  LOADWSTR(InvalidSpaces) ;
  split(vInvalidSpaces,InvalidSpaces,L',') ;
  if(!vInvalidSpaces.empty()) {
	copy(vInvalidSpaces.begin(),vInvalidSpaces.end(),
	  inserter(m_InvalidSpaces,m_InvalidSpaces.begin())) ;
  }
  wstring SpaceArrangement ;
  LOADWSTR(SpaceArrangement) ;
  split(m_SpaceArrangement,SpaceArrangement,L',') ;
  #undef LOADINT
  #undef LOADSTR2
  #undef LOADSTR
  #undef LOADWSTR
  SetHRTimerMode(USEHRTIMER);
  return true ;
}

//*****	Load Channel File *****

bool	CBonTuner::LoadChannelFile(std::string chFName)
{
  m_Channels.clear() ;
  m_Transponders.clear() ;
  //m_SpaceIndices.clear() ;

  FILE *st=NULL ;
  fopen_s(&st,chFName.c_str(),"rt") ;
  if(!st) return false ;
  char s[512] ;

  std::wstring space_name=L"" ;
  while(!feof(st)) {
	s[0]='\0' ;
	fgets(s,512,st) ;
	string strLine = trim(string(s)) ;
	if(strLine.empty()) continue ;
	wstring wstrLine = mbcs2wcs(strLine) ;
	int t=0 ;
	vector<wstring> params ;
	split(params,wstrLine,L';') ;
	wstrLine = params[0] ; params.clear() ;
	split(params,wstrLine,L',') ;
	if(params.size()>=2&&!params[0].empty()) {
	  BAND band = BAND_na ;
	  int channel = 0 ;
	  DWORD freq = 0 ;
	  int stream = 0 ;
	  int tsid = 0 ;
	  wstring &space = params[0] ;
	  wstring name = params.size()>=3 ? params[2] : wstring(L"") ;
	  wstring subname = params[1] ;
	  vector<wstring> phyChDiv ;
	  split(phyChDiv,params[1],'/') ;
	  for(size_t i=0;i<phyChDiv.size();i++) {
		wstring phyCh = phyChDiv[i] ;
		if( phyCh.length()>3&&
			phyCh.substr(phyCh.length()-3)==L"MHz" ) {
		  float megaHz = 0.f ;
		  if(swscanf_s(phyCh.c_str(),L"%fMHz",&megaHz)==1) {
			freq=DWORD(megaHz*1000.f) ;
			channel = CHANNEL::BandFromFreq(freq)!=BAND_na ? -1 : 0 ;
		  }
		}else {
		  if(swscanf_s(phyCh.c_str(),L"TS%d",&stream)==1)
			;
		  else if(swscanf_s(phyCh.c_str(),L"ID%i",&tsid)==1)
			;
		  else if(band==BAND_na) {
			if(swscanf_s(phyCh.c_str(),L"BS%d",&channel)==1)
			  band = BAND_BS ;
			else if(swscanf_s(phyCh.c_str(),L"ND%d",&channel)==1)
			  band = BAND_ND ;
			else if(swscanf_s(phyCh.c_str(),L"C%d",&channel)==1)
			  band = BAND_VU, subname=L"C"+itows(channel)+L"ch", channel+=100 ;
			else if(swscanf_s(phyCh.c_str(),L"%d",&channel)==1)
			  band = BAND_VU, subname=itows(channel)+L"ch" ;
		  }
		}
	  }
	  if(name==L"")
		name=subname ;
	  if(freq>0&&channel<0)
		m_Channels.push_back(
		  CHANNEL(space,freq,name,stream,tsid)) ;
	  else if(band!=BAND_na&&channel>0)
		m_Channels.push_back(
		  CHANNEL(space,band,channel,name,stream,tsid)) ;
	  else
		continue ;
	  if(space_name!=space) {
		//m_SpaceIndices.push_back(m_Channels.size()-1) ;
		space_name=space ;
	  }
	}
  }

  fclose(st) ;

  return true ;
}

//*****	Init Channel To Default	*****

void    CBonTuner::InitChannelToDefault()
{
    m_Channels.clear() ;
    m_Transponders.clear() ;

	wstring space; BAND band;

	if(DEFSPACEVHF) {
		space = L"VHF" ;
		band = BAND_VU ;
		for(int i=1;	i<=12;	i++)
			m_Channels.push_back(
			  CHANNEL(space,band,i,itows(i)+L"ch")) ;
	}

	if(DEFSPACEUHF) {
		space = L"UHF" ;
		band = BAND_VU ;
		for(int i=13;	i<=62;	i++)
			m_Channels.push_back(
			  CHANNEL(space,band,i,itows(i)+L"ch")) ;
	}

	if(DEFSPACECATV) {
		space = L"CATV" ;
		band = BAND_VU ;
		for(int i=13;	i<=63;	i++)
			m_Channels.push_back(
			  CHANNEL(space,band,i+100,L"C"+itows(i)+L"ch")) ;
	}

	copy(m_Channels.begin(), m_Channels.end(), back_inserter(m_Transponders)) ;

	if(DEFSPACEBS) {
		space = L"BS" ;
		band = BAND_BS ;
		if(DEFSPACEBSSTREAMS>0) {
			if(DEFSPACEBSSTREAMSTRIDE) {
				for(int j=0;	j<DEFSPACEBSSTREAMS;	j++)
				for(int i=1;	i <= 23;	i+=2)
					m_Channels.push_back(
					  CHANNEL(space,band,i,L"BS"+itows(i)+L"/TS"+itows(j),j)) ;
			}else {
				for(int i=1;	i <= 23;	i+=2)
				for(int j=0;	j<DEFSPACEBSSTREAMS;	j++)
					m_Channels.push_back(
					  CHANNEL(space,band,i,L"BS"+itows(i)+L"/TS"+itows(j),j)) ;
			}
		}else {
			for(int i=1;	i <= 23;	i+=2)
				m_Channels.push_back(
				  CHANNEL(space,band,i,L"BS"+itows(i))) ;
		}
		for(int i=1;	i <= 23;	i+=2)
		for(int j=0;	j<max(DEFSPACEBSSTREAMS,1);	j++)
			m_Transponders.push_back(
				CHANNEL(space,band,i,L"BS"+itows(i))) ;
	}

	if(DEFSPACECS110) {
		space = L"CS110" ;
		band = BAND_ND ;
		if(DEFSPACECS110STREAMS>0) {
			if(DEFSPACECS110STREAMSTRIDE) {
				for(int j=0;	j<DEFSPACECS110STREAMS;	j++)
				for(int i=2;	i <= 24;	i+=2)
					m_Channels.push_back(
					  CHANNEL(space,band,i,L"ND"+itows(i)+L"/TS"+itows(j),j)) ;
			}else {
				for(int i=2;	i <= 24;	i+=2)
				for(int j=0;	j<DEFSPACECS110STREAMS;	j++)
					m_Channels.push_back(
					  CHANNEL(space,band,i,L"ND"+itows(i)+L"/TS"+itows(j),j)) ;
			}
		}else {
			for(int i=2;	i <= 24;	i+=2)
				m_Channels.push_back(
				  CHANNEL(space,band,i,L"ND"+itows(i))) ;
		}
		for(int i=2;	i <= 24;	i+=2)
		for(int j=0;	j<max(DEFSPACECS110STREAMS,1);	j++)
			m_Transponders.push_back(
				CHANNEL(space,band,i,L"ND"+itows(i))) ;
	}

}


//*****	Rebuild Channels	*****

  void CBonTuner::ArrangeChannels(CHANNELS &channels) {
    struct space_finder : public std::unary_function<CHANNEL, bool> {
      std::wstring space ;
      space_finder(std::wstring space_) {
        space = space_ ;
      }
      bool operator ()(const CHANNEL &ch) const {
        return space == ch.Space;
      }
    };
    if (!m_InvisibleSpaces.empty() || !m_SpaceArrangement.empty()) {
      CHANNELS newChannels ;
      //CHANNELS oldChannels(channels) ;
      CHANNELS &oldChannels = channels ;
      CHANNELS::iterator beg = oldChannels.begin() ;
      CHANNELS::iterator end = oldChannels.end() ;
      for (CHANNELS::size_type i = 0; i < m_InvisibleSpaces.size(); i++) {
        end = remove_if(beg, end, space_finder(m_InvisibleSpaces[i]));
      }
      for (CHANNELS::size_type i = 0; i < m_SpaceArrangement.size(); i++) {
        space_finder finder(m_SpaceArrangement[i]) ;
        remove_copy_if(beg, end, back_inserter(newChannels), not1(finder)) ;
        end = remove_if(beg, end, finder) ;
      }
      copy(beg, end, back_inserter(newChannels)) ;
      channels.swap(newChannels) ;
    }
  }

void CBonTuner::RebuildChannels()
{
  // チャンネル並べ替え
  ArrangeChannels(m_Channels);
  if(!m_Transponders.empty())
    ArrangeChannels(m_Transponders);
  // チャンネルアンカー構築
  m_SpaceAnchors.clear() ;
  m_ChannelAnchors.clear() ;
  wstring space = L"" ;
  for (CHANNELS::size_type i = 0;i < m_Channels.size();i++) {
	if (m_Channels[i].Space != space) {
	  space = m_Channels[i].Space ;
	  m_SpaceAnchors.push_back(DWORD(i)) ;
	}
	if (m_InvalidSpaces.find(space) == m_InvalidSpaces.end())
	  m_ChannelAnchors.push_back(DWORD(i)) ;
  }
}

// 通しチャンネル番号から空間番号取得
DWORD CBonTuner::space_index_of(DWORD sch) const
{
  for(size_t i=m_SpaceAnchors.size();i>0;i--)
	if(m_SpaceAnchors[i-1]<sch)
	  return DWORD(i-1) ;
  return 0 ;
}

// 通しチャンネル番号から空間チャンネル番号取得
DWORD CBonTuner::channel_index_of(DWORD sch) const
{
  if(sch>=m_ChannelAnchors.size()) return 0 ;
  return m_ChannelAnchors[sch] - m_SpaceAnchors[space_index_of(sch)] ;
}

// 有効でない空間番号かどうか
BOOL CBonTuner::is_invalid_space(DWORD spc) const
{
  if(spc>=m_SpaceAnchors.size()) return TRUE ;
  std::wstring space_name = m_Channels[m_SpaceAnchors[spc]].Space ;
  return m_InvalidSpaces.find(space_name)==m_InvalidSpaces.end() ? FALSE:TRUE ;
}

//*****	I2C Write	*****

int CBonTuner::I2CWrite(unsigned char adrs,int len,unsigned char *data)
{
	BYTE	cmd[64];
	DWORD	cmd_len=0;
	int		i;

    exclusive_lock elock(&m_coXfer) ;

	cmd[cmd_len++]=CMD_I2C_WRITE;
	cmd[cmd_len++]=adrs;
	cmd[cmd_len++]=len;

	for(i=0; i<len; i++)
	{
		cmd[cmd_len++]=data[i];
	}
//	cmd[cmd_len++]=CMD_MODE_IDLE;

	if(!m_pUsbFx2Driver->TransmitData(EPINDEX_OUT, cmd, cmd_len))
      cmd_len=0;
/*
	usb->xfer(outep, cmd, cmd_len);
	i=cmd_len;
	cmd_len=0;
*/
	return(cmd_len);
}

//*****	I2C Read	*****

int CBonTuner::I2CRead(unsigned char adrs,int len,unsigned char *data)
{
	BYTE	cmd[64];
	DWORD	cmd_len=0;
	//int		i;

    exclusive_lock elock(&m_coXfer) ;

	cmd[cmd_len++]=CMD_I2C_READ;
	cmd[cmd_len++]=adrs;
	cmd[cmd_len++]=len;
	if(m_pUsbFx2Driver->TransmitData(EPINDEX_OUT, cmd, cmd_len)) {
      cmd_len=len;
      if(!m_pUsbFx2Driver->TransmitData(EPINDEX_IN, data, cmd_len))
        cmd_len=0 ;
    }else
      cmd_len=0 ;
/*
	usb->xfer(outep, cmd, cmd_len);
	cmd_len=0;
	ret_len=len;
	usb->xfer(inep, data, ret_len);
*/
	return(cmd_len);
}

//*****	I2C Read (Reg)	*****

int CBonTuner::I2CRead(unsigned char adrs,unsigned char reg,int len,unsigned char *data)
{
	BYTE	cmd[64];
	DWORD	cmd_len=0;
	//int		i;

    exclusive_lock elock(&m_coXfer) ;

	cmd[cmd_len++]=CMD_I2C_WRITE;
	cmd[cmd_len++]=adrs;
	cmd[cmd_len++]=1;
	cmd[cmd_len++]=reg;
	cmd[cmd_len++]=CMD_I2C_READ;
	cmd[cmd_len++]=adrs;
	cmd[cmd_len++]=len;
//	cmd[cmd_len++]=CMD_MODE_IDLE;
	if(m_pUsbFx2Driver->TransmitData(EPINDEX_OUT, cmd, cmd_len)) {
	  cmd_len=len;
      if(!m_pUsbFx2Driver->TransmitData(EPINDEX_IN, data, cmd_len)) {
        cmd_len=0 ;
      }
    }else
      cmd_len=0 ;
/*
	usb->xfer(outep, cmd, cmd_len);
	cmd_len=0;
	ret_len=len;
	usb->xfer(inep, data, ret_len);
*/
	return(cmd_len);
}

//*****	I2C Read (Tuner)	*****

int CBonTuner::I2CRead(unsigned char adrs,unsigned tadrs,unsigned char reg,int len,unsigned char *data)
{
	BYTE	cmd[64];
	DWORD	cmd_len=0;
	//int		i;

    exclusive_lock elock(&m_coXfer) ;

	cmd[cmd_len++]=CMD_I2C_WRITE;
	cmd[cmd_len++]=adrs;
	cmd[cmd_len++]=6;
	cmd[cmd_len++]=0xFE;
	cmd[cmd_len++]=tadrs;
	cmd[cmd_len++]=reg;
	cmd[cmd_len++]=adrs<<1;
	cmd[cmd_len++]=0xFE;
	cmd[cmd_len++]=tadrs | 0x01;

	cmd[cmd_len++]=CMD_I2C_READ;
	cmd[cmd_len++]=adrs;
	cmd[cmd_len++]=len;
//	cmd[cmd_len++]=CMD_MODE_IDLE;
	if(m_pUsbFx2Driver->TransmitData(EPINDEX_OUT, cmd, cmd_len)) {
	  cmd_len=len;
	  if(!m_pUsbFx2Driver->TransmitData(EPINDEX_IN, data, cmd_len))
        cmd_len=0 ;
    }else
      cmd_len=0 ;
/*
	usb->xfer(outep, cmd, cmd_len);
	cmd_len=0;
	ret_len=len;
	usb->xfer(inep, data, ret_len);
*/
	return(cmd_len);
}

//-------------------------------------------------------------------------------

//*****	I2C Write	*****

int I2CWrite(unsigned char adrs,int len,unsigned char *data)
{
	return(CBonTuner::m_pThis->I2CWrite(adrs,len,data));
}

//*****	I2C Read	*****

int I2CRead(unsigned char adrs,int len,unsigned char *data)
{
	return(CBonTuner::m_pThis->I2CRead(adrs,len,data));
}

//*****	I2C Read (Reg)	*****

int I2CRead(unsigned char adrs,unsigned char reg,int len,unsigned char *data)
{
	return(CBonTuner::m_pThis->I2CRead(adrs,reg,len,data));
}

//*****	I2C Read (Tuner)	*****

int I2CRead(unsigned char adrs,unsigned tadrs,unsigned char reg,int len,unsigned char *data)
{
	return(CBonTuner::m_pThis->I2CRead(adrs,tadrs,reg,len,data));
}

	//IBonTransponder

int CBonTuner::transponder_index_of(DWORD dwSpace, DWORD dwTransponder) const
{
  if(m_Transponders.empty())
    return -1 ; // transponder is not entried

  if(m_SpaceAnchors.size()<=dwSpace)
    return -1 ; // space is over

  size_t si = m_SpaceAnchors[dwSpace] ;

  if(!m_Transponders[si].isISDBS())
    return -1 ; // transponder is not supported

  size_t tp = 0 , i = si ;
  for(int ch=m_Transponders[si].Channel; tp<dwTransponder&&i<m_Transponders.size(); i++) {
    if(m_Transponders[i].Space != m_Transponders[si].Space) break ;
    if(m_Transponders[i].Channel != ch) {
      ch = m_Transponders[i].Channel ; tp++;
      if(tp==dwTransponder) break;
    }
  }

  if(tp!=dwTransponder)
    return -1 ; // transponder is not found

  return (int) i ;
}

LPCTSTR CBonTuner::TransponderEnumerate(const DWORD dwSpace, const DWORD dwTransponder)
{
  int idx = transponder_index_of(dwSpace, dwTransponder) ;
  if(idx<0) return NULL ;

  return m_Transponders[idx].Name.c_str();
}

const BOOL CBonTuner::TransponderSelect(const DWORD dwSpace, const DWORD dwTransponder)
{
  if(!fOpened) return FALSE;

  int idx = transponder_index_of(dwSpace, dwTransponder) ;
  if(idx<0) return FALSE ;

  if(fModeTerra) {
    SetMxl5007(0) ; // Terra Suspend
    SuspendTC90502(ISDB_T,TRUE);
    SuspendTC90502(ISDB_S,FALSE);
    HRSleep(40);
    fModeTerra = false ;
  }

  BOOL res = SetISDBSFreq(
      m_Transponders[idx].Channel,
      m_Transponders[idx].Freq ) ? TRUE:FALSE ;

  if(res&&!ResetDemod()) res = FALSE;

  if(res) {
    m_dwCurSpace = dwSpace;
    m_dwCurChannel = dwTransponder | TRANSPONDER_CHMASK ;
    is_channel_valid = FALSE; // TransponderSetCurID はまだ行っていないので
  }

  return res ;
}

const BOOL CBonTuner::TransponderGetIDList(LPDWORD lpIDList, LPDWORD lpdwNumID)
{
  if(!fOpened||fModeTerra) return FALSE;

  const DWORD numId = 8 ;

  if(lpdwNumID==NULL) {
    return FALSE ;
  }else if(lpIDList==NULL) {
    *lpdwNumID = numId ;
    return TRUE ;
  }

  *lpdwNumID = EnumTSID(lpIDList, *lpdwNumID) ;

  return TRUE ;
}

const BOOL CBonTuner::TransponderSetCurID(const DWORD dwID)
{
  if(!fOpened||fModeTerra) return FALSE;

  is_channel_valid = FALSE ;

  // Fx側バッファ停止
  ResetFxFifo(true) ;
  // 撮り溜めたTSストリームの破棄
  PurgeTsStream();

  BOOL res = FALSE ;

  bool locked = false, tuned=false;
  SetTSID(0);
  for(DWORD j=ISDBSSETTSIDTIMES;j;j--) {
    for(DWORD e=0,s=Elapsed();ISDBSSETTSIDWAIT>e;e=Elapsed(s)) {
      if(!locked) {
        locked = IsLockISDBS()==stLock ;
        if(locked) {
          ::HRSleep(ISDBSSETTSLOCKWAIT) ;
        }
      }else if(SetTSID(int(dwID&0xFFFF))) {
        tuned = true ;
        break ;
      }
      ::HRSleep(0,500) ;
    }
    if(j>1&&tuned)
      ::HRSleep(40) ;
  }

  if(tuned) res = TRUE ;

  // Fx側バッファ初期化
  ResetFxFifo(false) ;

  if(res) is_channel_valid = TRUE ;

  return res ;
}

const BOOL CBonTuner::TransponderGetCurID(LPDWORD lpdwID)
{
  if(!fOpened||fModeTerra) return FALSE;

  if(!is_channel_valid) {
    *lpdwID=0xFFFFFFFF;
    return TRUE;
  }

  WORD id = GetTSID() ;
  if(id!=0 && id!=0xFFFF)
    *lpdwID = DWORD(id) ;
  else
    *lpdwID = 0xFFFFFFFF ;

  return TRUE;
}

