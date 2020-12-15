// BonTuner.h: CBonTuner クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "IBonDriver2.h"
#include "UsbFx2Driver.h"
#include <vector>
#include <queue>
#include <string>
#include <cstdlib>

#define REMOCON_CMD_LEN     8   //各チャンネルのリモコンを押す回数の最大



class CBonTuner : public IBonDriver2, protected IUsbFx2DriverHost
{
private:
    // CHANNEL/CHANNELS
    struct CHANNEL {
        std::wstring space ;
        std::wstring name ;
        std::string rcode ;
        CHANNEL()
         : space(L""),name(L""),rcode("") {}
        CHANNEL(const CHANNEL &src)
         : space(src.space),name(src.name),rcode(src.rcode) {}
        CHANNEL(const std::wstring &_space,const std::wstring &_name,const std::string &_rcode)
         : space(_space),name(_name),rcode(_rcode) {}
        const wchar_t *c_space() const { return space.c_str() ; }
        const wchar_t *c_name() const { return name.c_str() ; }
    };
    typedef std::vector<CHANNEL> CHANNELS ;
public:
    CBonTuner();
    virtual ~CBonTuner();

    const BOOL OpenTuner(void);
    void CloseTuner(void);

    void    ResetFxFifo() ;

    const BOOL SetChannel(const BYTE bCh);
    const float GetSignalLevel(void);

    const DWORD WaitTsStream(const DWORD dwTimeOut = 0);
    const DWORD GetReadyCount(void);

    const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain);
    const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain);

    void PurgeTsStream(void);

// IBonDriver2(暫定)
    LPCTSTR GetTunerName(void);

    const BOOL IsTunerOpening(void);

    LPCTSTR EnumTuningSpace(const DWORD dwSpace);
    LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);

    const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

    const DWORD GetCurSpace(void);
    const DWORD GetCurChannel(void);

    void Release(void);

    static CBonTuner * m_pThis;
    static HINSTANCE m_hModule;

    /*この辺追加*/
    BOOL InitTunerProperty(void);
    BOOL IRCodeTX(BYTE code);
    BOOL IRButtonTX(BYTE button);
    BYTE ConvButtonToCode(BYTE button);

protected:
    virtual const bool OnRecvFifoData(const DWORD dwThreadIndex, const BYTE *pData, const DWORD dwLen, CUsbFx2Driver *pDriver);
    virtual void OnXferLock(const DWORD dwThreadIndex, bool locking, CUsbFx2Driver *pDriver);
    virtual BYTE *OnBeginWriteBack(const DWORD dwThreadIndex, DWORD dwMaxSize, CUsbFx2Driver *pDriver) ;
    virtual void OnFinishWriteBack(const DWORD dwThreadIndex, BYTE *pData, DWORD dwWroteSize, CUsbFx2Driver *pDriver) ;
    typedef std::map<BYTE*,CAsyncFifo::CACHE*> WRITEBACKMAP ;

    CUsbFx2Driver *m_pUsbFx2Driver;
    HANDLE m_hOnStreamEvent;

    CAsyncFifo      *m_AsyncTSFifo ;

    WRITEBACKMAP        m_TSWriteBackMap ;
    CAsyncFifo::CACHE   m_TSWriteBackDummyCache ;

    BOOL is_channel_valid;
    //event_object eoChanneling ;

    DWORD m_dwCurSpace;  BYTE m_cLastSpace ;
    DWORD m_dwCurChannel;

    // 排他ロック用
    exclusive_object m_coXfer ;

    // ユニデンチューナーのプロパティ
    TCHAR m_szTunerName[100] ;
    //int m_nChannels ;
    CHANNELS m_Channels ;

    DWORD m_tkLastCommandSend ;

    BYTE m_yFx2Id ;

    WORD m_wIRBase ;
    BUFFER<BYTE> m_IRCmdBuffer ;

    DWORD m_dwFifoThreadIndex;

    DWORD m_bytesProceeded ;
    DWORD m_tickProceeding ;

    BOOL m_bU3BSFixDone ;
    BOOL U3BSFixTune() ;
    BOOL U3BSFixResetChannel() ;

    bool LoadIniFile(std::string strIniFileName) ;
    bool LoadChannelFile(std::string strChannelFileName) ;
};

