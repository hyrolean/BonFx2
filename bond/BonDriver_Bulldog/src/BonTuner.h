// BonTuner.h: CBonTuner クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include <queue>
#include <string>
#include <set>
#include "IBonDriver2.h"
#include "UsbFx2Driver.h"

enum	CLockStatus	{stLock, stLostLock, stNoSignal, stUnknown };

class CBonTuner :	public IBonDriver2,
			protected IUsbFx2DriverHost
{
private:
	// BAND
	enum BAND {
	  BAND_na, // [n/a]
	  BAND_VU, // VHS, UHF or CATV
	  BAND_BS, // BS
	  BAND_ND  // CS110
	};
	// CHANNEL/CHANNELS
	struct CHANNEL {
		std::wstring Space ;
		BAND        Band ;
		int			Channel ;
		WORD		Stream ;
        WORD        TSID ;
		DWORD       Freq ;
		std::wstring	Name ;
		CHANNEL(std::wstring space, BAND band, int channel, std::wstring name,unsigned stream=0,unsigned tsid=0) {
			Space = space ;
			Band = band ;
			Name = name ;
			Channel = channel ;
			Freq = FreqFromBandCh(band,channel) ;
			Stream = stream ;
            TSID = tsid ;
		}
		CHANNEL(std::wstring space, DWORD freq,std::wstring name,unsigned stream=0,unsigned tsid=0) {
			Space = space ;
			Band = BandFromFreq(freq) ;
			Name = name ;
			Channel = 0 ;
			Freq = freq ;
			Stream = stream ;
            TSID = tsid ;
		}
		CHANNEL(const CHANNEL &src) {
			Space = src.Space ;
			Band = src.Band ;
			Name = src.Name ;
			Channel = src.Channel ;
			Freq = src.Freq ;
			Stream = src.Stream ;
            TSID = src.TSID;
		}
		static DWORD FreqFromBandCh(BAND band,int ch) {
			DWORD freq =0 ;
			switch(band) {
				case BAND_VU:
					if(ch < 4)          freq =  93UL + (ch - 1)   * 6UL ;
					else if(ch < 8)     freq = 173UL + (ch - 4)   * 6UL ;
					else if(ch < 13)    freq = 195UL + (ch - 8)   * 6UL ;
					else if(ch < 63)    freq = 473UL + (ch - 13)  * 6UL ;
					else if(ch < 122)   freq = 111UL + (ch - 113) * 6UL ;
					else if(ch ==122)   freq = 167UL ; // C22
					else if(ch < 136)   freq = 225UL + (ch - 123) * 6UL ;
					else                freq = 303UL + (ch - 136) * 6UL ;
					freq *= 1000UL ; // kHz
					freq +=  143UL ; // + 1000/7 kHz
					break ;
				case BAND_BS:
					freq = ch * 19180UL + 1030300UL ;
					break ;
				case BAND_ND:
					freq = ch * 20000UL + 1573000UL ;
					break ;
			}
			return freq ;
		}
		bool isISDBT() { return Band==BAND_VU; }
		bool isISDBS() { return Band==BAND_BS||Band==BAND_ND; }
		static BAND BandFromFreq(DWORD freq) {
			if(freq < 60000UL || freq > 2456123UL )
				return BAND_na ;
			if(freq < 900000UL )
				return BAND_VU ;
			if(freq < 1573000UL )
				return BAND_BS ;
			return BAND_ND ;
		}
	} ;
	typedef std::vector<CHANNEL> CHANNELS ;

public:
	CBonTuner();
	virtual ~CBonTuner();

	int I2CWrite(unsigned char adrs,int len,unsigned char *data);
	int I2CRead(unsigned char adrs,int len,unsigned char *data);
	int I2CRead(unsigned char adrs,unsigned char reg,int len,unsigned char *data);
	int I2CRead(unsigned char adrs,unsigned tadrs,unsigned char reg,int len,unsigned char *data);
	bool	SetTSID(int tsid);
	WORD    SelectTSID(BYTE stream);
	bool	SetISDBTChannel(int ch,DWORD kHz=0);
	bool	SetISDBSChannel(int ch,WORD stream,WORD tsid,DWORD kHz=0);
	CLockStatus	IsLockISDBT(void);
	CLockStatus	IsLockISDBS(void);
	CLockStatus	IsLock(void);
	double	GetCN(void);
	double	GetBER(void);

// IBonDriver
	const BOOL OpenTuner(void);
	void CloseTuner(void);

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

protected:
	virtual const bool OnRecvFifoData(const DWORD dwThreadIndex, const BYTE *pData, const DWORD dwLen, CUsbFx2Driver *pDriver);
    virtual void OnXferLock(const DWORD dwThreadIndex, bool locking, CUsbFx2Driver *pDriver) ;
    virtual BYTE *OnBeginWriteBack(const DWORD dwThreadIndex, DWORD dwMaxSize, CUsbFx2Driver *pDriver) ;
    virtual void OnFinishWriteBack(const DWORD dwThreadIndex, BYTE *pData, DWORD dwWroteSize, CUsbFx2Driver *pDriver) ;
    typedef std::map<BYTE*,CAsyncFifo::CACHE*> WRITEBACKMAP ;


    void	InitTunerProperty() ;
	bool	LoadIniFile(std::string strIniFileName);
	bool    LoadChannelFile(std::string strChannelFileName);
	void    InitChannelToDefault() ;
    const BOOL SetRealChannel(const DWORD dwCh) ;
    void    ResetFxFifo() ;

	CUsbFx2Driver	*m_pUsbFx2Driver;
	HANDLE			m_hOnStreamEvent;

    CAsyncFifo          *m_AsyncTSFifo ;
    WRITEBACKMAP        m_TSWriteBackMap ;
    CAsyncFifo::CACHE   m_TSWriteBackDummyCache ;
    DWORD               m_dwFifoThreadIndex;

	bool			fOpened;
    bool            fModeTerra;
	HANDLE			m_hMutex;

    // チューナーのプロパティ
    BYTE m_yFx2Id ;
    TCHAR m_szTunerName[100] ;

    // チャンネル情報
    CHANNELS m_Channels ;
    std::vector<DWORD> m_SpaceAnchors ;
    std::vector<DWORD> m_ChannelAnchors ;
    std::vector<std::wstring> m_InvisibleSpaces ;
    std::set<std::wstring> m_InvalidSpaces ;
    std::vector<std::wstring> m_SpaceArrangement ;
    DWORD space_index_of(DWORD sch) const ;
    DWORD channel_index_of(DWORD sch) const ;
    BOOL is_invalid_space(DWORD spc) const ;
    void RebuildChannels() ;

	DWORD			m_dwCurSpace;
	DWORD			m_dwCurChannel;
	DWORD			GetCurRealChannel() const ;

    // 排他ロック用
    exclusive_object m_coXfer ;

    BOOL is_channel_valid ;
    //event_object eoChanneling ;
};
