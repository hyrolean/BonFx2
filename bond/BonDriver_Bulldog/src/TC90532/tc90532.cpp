#include	<windows.h>
#include	<winbase.h>
#include <math.h>
/*
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <stdlib.h>
#include <process.h>
#include <Windows.h>
*/
//#include "Common.h"
#include "MxL5007.h"
#include "tc90532.h"


#define	CH_MAX					62//-13			//The Ch maximum number

UINT8 XTAL = 16;
typedef struct{
    UINT8        wTMCCSts;						// Status
    UINT8        wTMCCInfo[10];				// Data(0xB0-0xB9)
}TMCC;

static TMCC TMCCPrm[CH_MAX];

unsigned long MxL5007IFFreq[] =
{
    4000000,
    4500000,
    4570000,
    5000000,
    5380000,
    6000000,
    6280000,
    9191500,
    35250000,
    36150000,
    44000000
};

unsigned long MxL5007XtalFreq[] =
{
    16000000,
    20000000,
    20250000,
    20480000,
    24000000,
    25000000,
    25140000,
    27000000,
    28800000,
    32000000,
    40000000,
    44000000,
    48000000,
    49381100
};

float BS_table[12] =
{
	1049.48f,	//CH 1
	1087.84f,	//CH 3
	1126.20f,	//CH 5
	1164.56f,	//CH 7
	1202.92f,	//CH 9
	1241.28f,	//CH 11
	1279.64f,	//CH 13
	1318.00f,	//CH 15
	1356.36f,	//CH 17
	1394.72f,	//CH 19
	1433.08f,	//CH 21
	1471.44f	//CH 23
};

float CS_table[12] =
{
	1613,		//CH 2
	1653,		//CH 4
	1693,		//CH 6
	1733,		//CH 8
	1773,		//CH 10
	1813,		//CH 12
	1853,		//CH 14
	1893,		//CH 16
	1933,		//CH 18
	1973,		//CH 20
	2013,		//CH 22
	2053		//CH 24
};

UINT8 SetMxl5007(UINT8 CH, DWORD kHz)
{
    MxL_ERR_MSG Status = MxL_OK;
    BOOL RFSynthLock, REFSynthLock;
    //SINT32 RF_Input_Level;
    MxL5007_TunerConfigS myTuner;
    unsigned long rf;

    if(!CH) {
      rf = UINT32(kHz * 1000) ;
    }else {
      if(CH < 4)        rf =  93UL + (CH - 1)   * 6UL ;
      else if(CH < 8)   rf = 173UL + (CH - 4)   * 6UL ;
      else if(CH < 13)  rf = 195UL + (CH - 8)   * 6UL ;
      else if(CH < 63)	rf = 473UL + (CH - 13)  * 6UL ;
      else if(CH < 122)	rf = 111UL + (CH - 113) * 6UL ;
      else if(CH ==122) rf = 167UL ; // C22
      #if 0
      else if(CH ==123) rf = 225UL ;
      else if(CH < 128)	rf = 233UL + (CH - 124) * 6UL ;
      else if(CH < 136)	rf = 255UL + (CH - 128) * 6UL ;
      #else
      else if(CH < 136) rf = 225UL + (CH - 123) * 6UL ;
      #endif
      else				rf = 303UL + (CH - 136) * 6UL ;
      rf *= 1000000UL ;
      rf +=  142857UL ;
    }

	myTuner.DemodAddr = DEMODTADRS;
    //Set Tuner's I2C Address
    myTuner.I2C_Addr = (MxL5007_I2CAddr)(MxL_I2C_ADDR_96 << 1);
//    printf("I2C_Addr = %d\n",myTuner.I2C_Addr);
    //Set Tuner Mode to DVB-T/ATSC mode
    myTuner.Mode = MxL_MODE_ISDBT;
//    printf("Mode = %d\n",myTuner.Mode);
    //Setting for Cable mode only
    myTuner.IF_Diff_Out_Level = -8;
//    printf("IF Out Level = %d\n",myTuner.IF_Diff_Out_Level);
    //Set Tuner's XTAL freq
    myTuner.Xtal_Freq = MxL_XTAL_16_MHZ;
//    printf("Xtal = %d\n",myTuner.Xtal_Freq);
    //Set Tuner's IF Freq
    myTuner.IF_Freq = MxL_IF_4_MHZ;
//    printf("IF = %d\n",myTuner.IF_Freq);
    myTuner.IF_Spectrum = MxL_NORMAL_IF;
    //Set Tuner's Clock out setting
    myTuner.ClkOut_Setting = MxL_CLKOUT_DISABLE;
    myTuner.ClkOut_Amp = MxL_CLKOUT_AMP_0;
//    printf("Clock Out = %d\n",myTuner.ClkOut_Setting);
//    printf("Amp = %d\n",myTuner.ClkOut_Amp);
    //Init Tuner
    if(Status == MxL_Tuner_Init(&myTuner))
    {
	    //Init Tuner fail
    }
    //Tune Tuner

    if(Status == MxL_Tuner_RFTune(&myTuner, rf, MxL_BW_6MHz))
    {
	    //Tune Tuner fail
    }
//    printf("RF = %lu\n",rf);
//    printf("BW = %d\n",bw);
    //Check Lock Status
    MxL_RFSynth_Lock_Status(&myTuner, &RFSynthLock);
//    printf("RF Lock = %d\n",RFSynthLock);
    MxL_REFSynth_Lock_Status(&myTuner, &REFSynthLock);
//    printf("REF Lock = %d\n",REFSynthLock);

    //Enter Stand By Mode
    MxL_Stand_By(&myTuner);

    //Wake Up
    if(rf>0) MxL_Wake_Up(&myTuner);

    return 0;
}

UINT8 SetStv6110a(UINT8 BSCh, DWORD kHz)
{
	UINT8 STB6110A[8] = {0x07,0x13,0xdc,0x85,0x17,0x01,0xe6,0x1e};
	UINT8 K;
	UINT8 Presc, P;
	UINT16 rDiv, r, divider;
	int pVal;
	int pCalcOpt = 1000;
	int rDivOpt = 0;
	int pCalc, i;
	UINT8 CF;
	//UINT8 buffer;
	float RF;
	float symb = 28.86f;

	if(!BSCh) {
	  RF = float(kHz)/1000.f ;
	}else {
	  if (BSCh & 1)
		  RF = BS_table[BSCh/2];
	  else
		  RF = CS_table[BSCh/2-1];
	}

	for(i=0; i<8; i++)
	{
		if(WriteTuner(DemodAddress+1,0xC6,i,STB6110A[i]) != 0)
			return 1;
	}

	K = XTAL - 16;
	STB6110A[0] = (STB6110A[0] & 0x07) | (K << 3);

	if(WriteTuner(DemodAddress+1,0xC6,0x0,STB6110A[0]) != 0)
		return 1;

	if(RF <= 1023.0)
	{
		P = 1;
		Presc = 0;
	}
	else if(RF <= 1300.0)
	{
		P = 1;
		Presc = 1;
	}
	else if(RF <= 2046.0)
	{
		P = 0;
		Presc = 0;
	}
	else
	{
		P = 0;
		Presc = 1;
	}

	pVal = (int)pow(2.0,(double)(P+1)) * 10;

	for(rDiv=0;rDiv<=3;rDiv++)
	{
		pCalc = XTAL * 10 / (int)pow(2.0,(double)(rDiv+1));

		if(abs(pCalc-pVal) < abs(pCalcOpt-pVal))
			rDivOpt = rDiv;

		pCalcOpt = XTAL * 10 / (int)pow(2.0,(double)(rDivOpt+1));
	}

	r = (UINT16)pow(2.0,(double)(rDivOpt+1));

	divider = (UINT16)(((RF * (float)r * pow(2.0,(double)(P+1)) * 10.0 / (float)XTAL) + 5.0) / 10.0) ;
	STB6110A[2] = divider & 0x00ff;                                // Fixed by ◆PRY8EAlByw
	STB6110A[3] = ((rDivOpt & 0x3) << 6) | (Presc << 5) | (P << 4) | ((divider & 0xF00) >> 8);

	if(WriteTuner(DemodAddress+1,0xC6,0x02,STB6110A[2]) != 0)
		return 1;
	if(WriteTuner(DemodAddress+1,0xC6,0x03,STB6110A[3]) != 0)
		return 1;

	if(WriteTuner(DemodAddress+1,0xC6,0x05,STB6110A[5]|0x04) != 0)
		return 1;

	i=0;
	do{
//		if(ReadTuner(DemodAddress+1,0xC6,0x00,&buffer) != 0)
//			return 1;
		i++;
		Sleep(10);
//printf("%2x",buffer);
	}while((i<10)/* && ((buffer & 0x04) != 0)*/);

	if((symb/2.) > 36.0)
		CF = 31;
	else if((symb/2.) < 5.0)
		CF = 0;
	else
		CF = (UINT8)((symb/2.)-5.);
CF = 0x18;
	STB6110A[4] = (STB6110A[4] & 0xE0) | (CF & 0x1F);
	if(WriteTuner(DemodAddress+1,0xC6,0x04,STB6110A[4]) != 0)
		return 1;

	if(WriteTuner(DemodAddress+1,0xC6,0x05,STB6110A[5]|0x02) != 0)
		return 1;

	i=0;
	do{
//		if(ReadTuner(DemodAddress+1,0xC6,5,&buffer) != 0)
//			return 1;
		i++;
		Sleep(10);
	}while((i<10)/* && ((buffer & 0x02) != 0)*/);

	return 0;
}


// Set Demod ISDB-T TC90502
UINT8 SetTC90502(UINT8 TS)
{
	if(TS == ISDB_T)
	{
	    if(WriteReg(DemodAddress,0x01,0x40))    return TC90502_ERR;				//復調リセット
//	    if(WriteReg(DemodAddress,0x0E,0x47))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress,0x0F,0x14))    return TC90502_ERR;				//JSDT,SDT両方に地上D TSを出力
	    if(WriteReg(DemodAddress,0x0E,0x11))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x0F,0x11))    return TC90502_ERR;				//JSDT,SDT両方に地上D TSを出力

//	    if(WriteReg(DemodAddress,0x13,0x2C))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress,0x14,0x0F))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x13,0x33))    return TC90502_ERR;				//XSEL
	    if(WriteReg(DemodAddress,0x14,0x20))    return TC90502_ERR;				//XSEL
	    if(WriteReg(DemodAddress,0x23,0x38))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x25,0x00))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x30,0x20))    return TC90502_ERR;				//XSEL
//	    if(WriteReg(DemodAddress,0x31,0x11))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress,0x32,0x22))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x31,0x0D))    return TC90502_ERR;				//XSEL
	    if(WriteReg(DemodAddress,0x32,0x56))    return TC90502_ERR;				//XSEL
	    if(WriteReg(DemodAddress,0x47,0x10))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x58,0x08))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x7D,0xD2))    return TC90502_ERR;				//?
	    if(WriteReg(DemodAddress,0xF4,0x80))    return TC90502_ERR;				//ISIC限界時に動作継続

		if(WriteReg(DemodAddress,0x73,0x01))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x74,0x02))    return TC90502_ERR;

//	    if(WriteReg(DemodAddress,0x71,0x08))    return TC90502_ERR;				//Don't set TEI
	    if(WriteReg(DemodAddress,0x71,0x20))    return TC90502_ERR;				//パリティ期間クロック停止,データ停止
//	    if(WriteReg(DemodAddress,0x75,0x02))    return TC90502_ERR;				//CLK反転
//	    if(WriteReg(DemodAddress,0x76,0x0A))    return TC90502_ERR;				//NullパケットはValid信号を立てない
	    if(WriteReg(DemodAddress,0x76,0x08))    return TC90502_ERR;				//NullパケットはValid信号を立てない
//	    if(WriteReg(DemodAddress,0x77,0x01))    return TC90502_ERR;				//ビダビ復号後測定モード
	}
	if(TS == ISDB_S)
	{
	    if(WriteReg(DemodAddress+1,0x01,0x80))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress+1,0x03,0x01))    return TC90502_ERR;			//PSK復調リセット
//	    if(WriteReg(DemodAddress+1,0x04,0x02))    return TC90502_ERR;			//BSCS SCLK反転


	    if(WriteReg(DemodAddress+1,0x06,0x40))    return TC90502_ERR;

	    if(WriteReg(DemodAddress,0x0E,0x77))    return TC90502_ERR;
	    if(WriteReg(DemodAddress,0x0F,0x77))    return TC90502_ERR;				//PSK出力
//	    if(WriteReg(DemodAddress+1,0x07,0x41))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress+1,0x08,0x44))    return TC90502_ERR;
	    if(WriteReg(DemodAddress+1,0x07,0x11))    return TC90502_ERR;
	    if(WriteReg(DemodAddress+1,0x08,0x11))    return TC90502_ERR;

	    if(WriteReg(DemodAddress+1,0x0C,0x59))    return TC90502_ERR;			//XSEL
	    if(WriteReg(DemodAddress+1,0x0D,0xF2))    return TC90502_ERR;			//XSEL
	    if(WriteReg(DemodAddress+1,0x0E,0x50))    return TC90502_ERR;			//XSEL

	    if(WriteReg(DemodAddress+1,0x10,0xB1))    return TC90502_ERR;
	    if(WriteReg(DemodAddress+1,0x11,0x40))    return TC90502_ERR;

	    if(WriteReg(DemodAddress+1,0x85,0x7A))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress+1,0x87,0x04))    return TC90502_ERR;
//	    if(WriteReg(DemodAddress+1,0xA3,0x77))    return TC90502_ERR;
	    if(WriteReg(DemodAddress+1,0xA3,0xF7))    return TC90502_ERR;
	    if(WriteReg(DemodAddress+1,0xA5,0x00))    return TC90502_ERR;

//	    if(WriteReg(DemodAddress+1,0x8D,0x20))    return TC90502_ERR;			//Don't set TEI
//	    if(WriteReg(DemodAddress+1,0x8E,0x02))    return TC90502_ERR;			//NullパケットはValid信号を立てない
	    if(WriteReg(DemodAddress+1,0x8E,0x26))    return TC90502_ERR;			//NullパケットはValid信号を立てない
	}
    return TC90502_OK;
}

