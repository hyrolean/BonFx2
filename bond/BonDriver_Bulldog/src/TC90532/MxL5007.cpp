/*
 MxL5007 Source Code : V4.1.3

 Copyright, Maxlinear, Inc.
 All Rights Reserved

 File Name:      MxL5007.c

 Description: The source code is for MxL5007 user to quickly integrate MxL5007 into their software.
	There are two functions the user can call to generate a array of I2C command that's require to
	program the MxL5007 tuner. The user should pass an array pointer and an integer pointer in to the
	function. The funciton will fill up the array with format like follow:

		addr1
		data1
		addr2
		data2
		...

	The user can then pass this array to their I2C function to perform progromming the tuner.
*/
//#include "StdAfx.h"
//#include "MxL5007_Common.h"

//#include <stdio.h>
//#include <conio.h>
//#include <dos.h>
#include <windows.h>
//#include "Common.h"
#include "MxL5007.h"


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//																		   //
//					I2C Functions (implement by customer)				   //
//																		   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

unsigned char WriteReg(unsigned char wDevice, unsigned char wReg, unsigned char wVal)
{
	unsigned char	data[2];

	data[0]=wReg;
	data[1]=wVal;

	return I2CWrite(wDevice,2,data)?0:1 ;

	return(0);
}

/**************************************************\
\**************************************************/
unsigned char ReadReg(unsigned char wDevice, unsigned char wReg, unsigned char *rVal)
{
	return I2CRead(wDevice,wReg,1,rVal)?0:1 ;
/*
	I2CWrite(wDevice,1,&wReg);
	I2CRead(wDevice,1,rVal);
*/
	return(0);
}
/**************************************************\
\**************************************************/
unsigned char WriteTuner(unsigned char Device,unsigned char addr, unsigned char reg, unsigned char val)
{
	unsigned char	data[4];

	data[0]=0xFE;
	data[1]=addr;
	data[2]=reg;
	data[3]=val;

	return I2CWrite(Device,4,data)?0:1 ;

	return(0);
}
/**************************************************\
\**************************************************/
unsigned char ReadTuner(unsigned char Device,unsigned char addr, unsigned char reg, unsigned char *val)
{
	return I2CRead(Device,addr,reg,1,val)?0:1 ;
/*
	unsigned char	data[4];

	data[0]=0xFE;
	data[1]=addr;
	data[2]=reg;

	I2CWrite(Device,3,data);

	data[0]=0xFE;
	data[1]=addr | 0x01;

	I2CWrite(Device,2,data);

	I2CWrite(Device,1,val);
*/
	return(0);
}

/******************************************************************************
**
**  Name: MxL_I2C_Write
**
**  Description:    I2C write operations
**
**  Parameters:
**					DeviceAddr	- MxL5007 Device address
**					pArray		- Write data array pointer
**					count		- total number of array
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
UINT32 MxL_I2C_Write(UINT8 DemodAddr,UINT8 DeviceAddr, UINT8* pArray, UINT8 count)
{
	UINT8 counter;
	UINT8 ret = 0;
/*
	for(counter=0; counter<count; counter+=2)
	{

	if(WriteReg(DeviceAddr, pArray[counter], pArray[counter+1]))
            return 1;
    }
*/
    for(counter=0; counter<count; counter+=2)
    {
		WriteTuner(DemodAddr,DeviceAddr,pArray[counter],pArray[counter+1]);
/*
        ret |= StartI2C();
	ret |= Write2Wire(0x20);
	ret |= Write2Wire(0xFE);
	ret |= Write2Wire(DeviceAddr);
	ret |= Write2Wire(pArray[counter]);
	ret |= Write2Wire(pArray[counter+1]);
	ret |= StopI2C();
*/
    }

    return (UINT32)ret;
}

/******************************************************************************
**
**  Name: MxL_I2C_Read
**
**  Description:    I2C read operations
**
**  Parameters:
**					DeviceAddr	- MxL5007 Device address
**					Addr		- register address for read
**					*Data		- data return
**
**  Returns:        0 if success
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
UINT32 MxL_I2C_Read(UINT8 DemodAddr,UINT8 DeviceAddr, UINT8 Addr, UINT8* mData)
{
    UINT8 ret = 0;

	ReadTuner(DemodAddr,DeviceAddr,Addr,mData);

/*
    if(ReadReg(DeviceAddr, Addr, mData))
	return 1;
*/
/*
    ret |= StartI2C();
    ret |= Write2Wire(0x20);
    ret |= Write2Wire(0xFE);
    ret |= Write2Wire(DeviceAddr);
    ret |= Write2Wire(Addr);
    ret |= StopI2C();

    ret |= StartI2C();
    ret |= Write2Wire(0x20);
    ret |= Write2Wire(0xFE);
    ret |= Write2Wire(DeviceAddr|0x01);
    ret |= StartI2C();
    ret |= Write2Wire(0x21);
    ret |= Read2Wire(mData);
    ret |= StopI2C();
*/
    return (UINT32)ret;
}

/******************************************************************************
**
**  Name: MxL_Delay
**
**  Description:    Delay function in milli-second
**
**  Parameters:
**					mSec		- milli-second to delay
**
**  Returns:        0
**
**  Revision History:
**
**   SCR      Date      Author  Description
**  -------------------------------------------------------------------------
**   N/A   12-16-2007   khuang initial release.
**
******************************************************************************/
void MxL_Delay(UINT32 mSec)
{
	Sleep(mSec);
//    delay(mSec);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//																		   //
//					MxL5007 Functions (implement by customer)			   //
//																		   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//


UINT32 MxL5007_Init(UINT8* pArray,				// a array pointer that store the addr and data pairs for I2C write
					UINT32* Array_Size,			// a integer pointer that store the number of element in above array
					UINT8 Mode,
					SINT32 IF_Diff_Out_Level,
					UINT8 Xtal_Freq_Hz,
					UINT8 IF_Freq_Hz,
					UINT8 Invert_IF,
					UINT8 Clk_Out_Enable,
					UINT8 Clk_Out_Amp
					)
{

	UINT32 Reg_Index=0;
	UINT32 Array_Index=0;

	IRVType IRV_Init[]=
	{
		//{ Addr, Data}
		{ 0x02, 0x06},
		{ 0x03, 0x48},
		{ 0x05, 0x04},
		{ 0x06, 0x10},
		{ 0x2E, 0x15},  //Override
		{ 0x30, 0x10},  //Override
		{ 0x45, 0x58},  //Override
		{ 0x48, 0x19},  //Override
		{ 0x52, 0x03},  //Override
		{ 0x53, 0x44},  //Override
		{ 0x6A, 0x4B},  //Override
		{ 0x76, 0x00},  //Override
		{ 0x78, 0x18},  //Override
		{ 0x7A, 0x17},  //Override
		{ 0x85, 0x06},  //Override
		{ 0x01, 0x01}, //TOP_MASTER_ENABLE=1
		{ 0, 0}
	};


	IRVType IRV_Init_Cable[]=
	{
		//{ Addr, Data}
		{ 0x02, 0x06},
		{ 0x03, 0x48},
		{ 0x05, 0x04},
		{ 0x06, 0x10},
		{ 0x09, 0x3F},
		{ 0x0A, 0x3F},
		{ 0x0B, 0x3F},
		{ 0x2E, 0x15},  //Override
		{ 0x30, 0x10},  //Override
		{ 0x45, 0x58},  //Override
		{ 0x48, 0x19},  //Override
		{ 0x52, 0x03},  //Override
		{ 0x53, 0x44},  //Override
		{ 0x6A, 0x4B},  //Override
		{ 0x76, 0x00},  //Override
		{ 0x78, 0x18},  //Override
		{ 0x7A, 0x17},  //Override
		{ 0x85, 0x06},  //Override
		{ 0x01, 0x01}, //TOP_MASTER_ENABLE=1
		{ 0, 0}
	};
	//edit Init setting here

	PIRVType myIRV;

	switch(Mode)
	{
	case MxL_MODE_ISDBT: //ISDB-T Mode
		myIRV = IRV_Init;
		SetIRVBit(myIRV, 0x06, 0x1F, 0x10);
		break;
	case MxL_MODE_DVBT: //DVBT Mode
		myIRV = IRV_Init;
		SetIRVBit(myIRV, 0x06, 0x1F, 0x11);
		break;
	case MxL_MODE_ATSC: //ATSC Mode
		myIRV = IRV_Init;
		SetIRVBit(myIRV, 0x06, 0x1F, 0x12);
		break;
	case MxL_MODE_CABLE:
		myIRV = IRV_Init_Cable;
		SetIRVBit(myIRV, 0x09, 0xFF, 0xC1);
		SetIRVBit(myIRV, 0x0A, 0xFF, UINT8(8-IF_Diff_Out_Level));
		SetIRVBit(myIRV, 0x0B, 0xFF, 0x17);
		break;
	}

	switch(IF_Freq_Hz)
	{
	case MxL_IF_4_MHZ:
		SetIRVBit(myIRV, 0x02, 0x0F, 0x00);
		break;
	case MxL_IF_4_5_MHZ: //4.5MHz
		SetIRVBit(myIRV, 0x02, 0x0F, 0x02);
		break;
	case MxL_IF_4_57_MHZ: //4.57MHz
		SetIRVBit(myIRV, 0x02, 0x0F, 0x03);
		break;
	case MxL_IF_5_MHZ:
		SetIRVBit(myIRV, 0x02, 0x0F, 0x04);
		break;
	case MxL_IF_5_38_MHZ: //5.38MHz
		SetIRVBit(myIRV, 0x02, 0x0F, 0x05);
		break;
	case MxL_IF_6_MHZ:
		SetIRVBit(myIRV, 0x02, 0x0F, 0x06);
		break;
	case MxL_IF_6_28_MHZ: //6.28MHz
		SetIRVBit(myIRV, 0x02, 0x0F, 0x07);
		break;
	case MxL_IF_9_1915_MHZ: //9.1915MHz
		SetIRVBit(myIRV, 0x02, 0x0F, 0x08);
		break;
	case MxL_IF_35_25_MHZ:
		SetIRVBit(myIRV, 0x02, 0x0F, 0x09);
		break;
	case MxL_IF_36_15_MHZ:
		SetIRVBit(myIRV, 0x02, 0x0F, 0x0a);
		break;
	case MxL_IF_44_MHZ:
		SetIRVBit(myIRV, 0x02, 0x0F, 0x0B);
		break;
	}

	if(Invert_IF)
		SetIRVBit(myIRV, 0x02, 0x10, 0x10);   //Invert IF
	else
		SetIRVBit(myIRV, 0x02, 0x10, 0x00);   //Normal IF


	switch(Xtal_Freq_Hz)
	{
	case MxL_XTAL_16_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x00);  //select xtal freq & Ref Freq
		SetIRVBit(myIRV, 0x05, 0x0F, 0x00);
		break;
	case MxL_XTAL_20_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x10);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x01);
		break;
	case MxL_XTAL_20_25_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x20);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x02);
		break;
	case MxL_XTAL_20_48_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x30);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x03);
		break;
	case MxL_XTAL_24_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x40);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x04);
		break;
	case MxL_XTAL_25_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x50);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x05);
		break;
	case MxL_XTAL_25_14_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x60);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x06);
		break;
	case MxL_XTAL_27_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x70);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x07);
		break;
	case MxL_XTAL_28_8_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x80);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x08);
		break;
	case MxL_XTAL_32_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0x90);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x09);
		break;
	case MxL_XTAL_40_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0xA0);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x0A);
		break;
	case MxL_XTAL_44_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0xB0);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x0B);
		break;
	case MxL_XTAL_48_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0xC0);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x0C);
		break;
	case MxL_XTAL_49_3811_MHZ:
		SetIRVBit(myIRV, 0x03, 0xF0, 0xD0);
		SetIRVBit(myIRV, 0x05, 0x0F, 0x0D);
		break;
	}

	if(!Clk_Out_Enable) //default is enable
		SetIRVBit(myIRV, 0x03, 0x08, 0x00);

	//Clk_Out_Amp
	SetIRVBit(myIRV, 0x03, 0x07, Clk_Out_Amp);

	//Generate one Array that Contain Data, Address
	while (myIRV[Reg_Index].Num || myIRV[Reg_Index].Val)
	{
		pArray[Array_Index++] = myIRV[Reg_Index].Num;
		pArray[Array_Index++] = myIRV[Reg_Index].Val;
		Reg_Index++;
	}

	*Array_Size=Array_Index;
	return 0;
}


UINT32 MxL5007_RFTune(UINT8* pArray, UINT32* Array_Size, UINT32 RF_Freq, UINT8 BWMHz)
{
	IRVType IRV_RFTune[]=
	{
	//{ Addr, Data}
		{ 0x0F, 0x00},  //abort tune
		{ 0x0C, 0x15},
		{ 0x0D, 0x40},
		{ 0x0E, 0x0E},
		{ 0x1F, 0x87},  //Override
		{ 0x20, 0x1F},  //Override
		{ 0x21, 0x87},  //Override
		{ 0x22, 0x1F},  //Override
		{ 0x80, 0x01},  //Freq Dependent Setting
		{ 0x0F, 0x01},	//start tune
		{ 0, 0}
	};

	UINT32 dig_rf_freq=0;
	UINT32 temp;
	UINT32 Reg_Index=0;
	UINT32 Array_Index=0;
	UINT32 i;
	UINT32 frac_divider = 1000000;

	switch(BWMHz)
	{
	case MxL_BW_6MHz: //6MHz
			SetIRVBit(IRV_RFTune, 0x0C, 0x3F, 0x15);  //set DIG_MODEINDEX, DIG_MODEINDEX_A, and DIG_MODEINDEX_CSF
		break;
	case MxL_BW_7MHz: //7MHz
			SetIRVBit(IRV_RFTune, 0x0C, 0x3F, 0x2A);
		break;
	case MxL_BW_8MHz: //8MHz
			SetIRVBit(IRV_RFTune, 0x0C, 0x3F, 0x3F);
		break;
	}


	//Convert RF frequency into 16 bits => 10 bit integer (MHz) + 6 bit fraction
	dig_rf_freq = RF_Freq / MHz;
	temp = RF_Freq % MHz;
	for(i=0; i<6; i++)
	{
		dig_rf_freq <<= 1;
		frac_divider /=2;
		if(temp > frac_divider)
		{
			temp -= frac_divider;
			dig_rf_freq++;
		}
	}

	//add to have shift center point by 7.8124 kHz
	if(temp > 7812)
		dig_rf_freq ++;

	SetIRVBit(IRV_RFTune, 0x0D, 0xFF, (UINT8)dig_rf_freq);
	SetIRVBit(IRV_RFTune, 0x0E, 0xFF, (UINT8)(dig_rf_freq>>8));

	if (RF_Freq >=333*MHz)
		SetIRVBit(IRV_RFTune, 0x80, 0x40, 0x40);

	//Generate one Array that Contain Data, Address
	while (IRV_RFTune[Reg_Index].Num || IRV_RFTune[Reg_Index].Val)
	{
		pArray[Array_Index++] = IRV_RFTune[Reg_Index].Num;
		pArray[Array_Index++] = IRV_RFTune[Reg_Index].Val;
		Reg_Index++;
	}

	*Array_Size=Array_Index;

	return 0;
}

//local functions called by Init and RFTune
UINT32 SetIRVBit(PIRVType pIRV, UINT8 Num, UINT8 Mask, UINT8 Val)
{
	while (pIRV->Num || pIRV->Val)
	{
		if (pIRV->Num==Num)
		{
			pIRV->Val&=~Mask;
			pIRV->Val|=Val;
		}
		pIRV++;

	}
	return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//																		   //
//							MxL Tuner Functions							   //
//																		   //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

MxL_ERR_MSG MxL_Set_Register(MxL5007_TunerConfigS* myTuner, UINT8 RegAddr, UINT8 RegData)
{
	UINT32 Status=0;
	UINT8 pArray[2];
	pArray[0] = RegAddr;
	pArray[1] = RegData;
	Status = MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, pArray, 2);
	if(Status) return MxL_ERR_SET_REG;

	return MxL_OK;

}

MxL_ERR_MSG MxL_Get_Register(MxL5007_TunerConfigS* myTuner, UINT8 RegAddr, UINT8 *RegData)
{
	if(MxL_I2C_Read(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, RegAddr, RegData))
		return MxL_ERR_GET_REG;

	return MxL_OK;
}

MxL_ERR_MSG MxL_Soft_Reset(MxL5007_TunerConfigS* myTuner)
{
//	UINT32 Status=0;
	UINT8 reg_reset[2];
	reg_reset[0] = 0xFF;
	reg_reset[1] = 0x00;

	if(MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, reg_reset, 1))
		return MxL_ERR_OTHERS;

	return MxL_OK;
}

MxL_ERR_MSG MxL_Loop_Through_On(MxL5007_TunerConfigS* myTuner, MxL5007_LoopThru isOn)
{
	UINT8 pArray[2];	// a array pointer that store the addr and data pairs for I2C write

	pArray[0]=0x04;
	if(isOn)
	 pArray[1]= 0x01;
	else
	 pArray[1]= 0x0;

	if(MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, pArray, 2))
		return MxL_ERR_OTHERS;

	return MxL_OK;
}

MxL_ERR_MSG MxL_Stand_By(MxL5007_TunerConfigS* myTuner)
{
	UINT8 pArray[4];	// a array pointer that store the addr and data pairs for I2C write

	pArray[0] = 0x01;
	pArray[1] = 0x0;
	pArray[2] = 0x0F;
	pArray[3] = 0x0;

	if(MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, pArray, 4))
		return MxL_ERR_OTHERS;

	return MxL_OK;
}

MxL_ERR_MSG MxL_Wake_Up(MxL5007_TunerConfigS* myTuner)
{
	UINT8 pArray[2];	// a array pointer that store the addr and data pairs for I2C write

	pArray[0] = 0x01;
	pArray[1] = 0x01;

	if(MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, pArray, 2))
		return MxL_ERR_OTHERS;

	if(MxL_Tuner_RFTune(myTuner, myTuner->RF_Freq_Hz, myTuner->BW_MHz))
		return MxL_ERR_RFTUNE;

	return MxL_OK;
}

MxL_ERR_MSG MxL_Tuner_Init(MxL5007_TunerConfigS* myTuner)
{
	UINT8 pArray[MAX_ARRAY_SIZE];	// a array pointer that store the addr and data pairs for I2C write
	UINT32 Array_Size;							// a integer pointer that store the number of element in above array

	//Soft reset tuner
	if(MxL_Soft_Reset(myTuner))
		return MxL_ERR_INIT;

	//perform initialization calculation
	MxL5007_Init(pArray, &Array_Size, (UINT8)myTuner->Mode, myTuner->IF_Diff_Out_Level, (UINT32)myTuner->Xtal_Freq,
				(UINT32)myTuner->IF_Freq, (UINT8)myTuner->IF_Spectrum, (UINT8)myTuner->ClkOut_Setting, (UINT8)myTuner->ClkOut_Amp);

	//perform I2C write here
	if(MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, pArray, Array_Size))
		return MxL_ERR_INIT;

	return MxL_OK;
}


MxL_ERR_MSG MxL_Tuner_RFTune(MxL5007_TunerConfigS* myTuner, UINT32 RF_Freq_Hz, MxL5007_BW_MHz BWMHz)
{
//	UINT32 Status=0;
	UINT8 pArray[MAX_ARRAY_SIZE];	// a array pointer that store the addr and data pairs for I2C write
	UINT32 Array_Size;							// a integer pointer that store the number of element in above array

	//Store information into struc
	myTuner->RF_Freq_Hz = RF_Freq_Hz;
	myTuner->BW_MHz = BWMHz;

	//perform Channel Change calculation
	MxL5007_RFTune(pArray,&Array_Size,RF_Freq_Hz,BWMHz);

	//perform I2C write here
	if(MxL_I2C_Write(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, pArray, Array_Size))
		return MxL_ERR_RFTUNE;

	//wait for 3ms
	MxL_Delay(3);

	return MxL_OK;
}

MxL5007_ChipVersion MxL_Check_ChipVersion(MxL5007_TunerConfigS* myTuner)
{
	UINT8 Data;
	if(MxL_I2C_Read(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, 0xD9, &Data))
		return MxL_GET_ID_FAIL;

	switch(Data)
	{
	case 0x14: return MxL_5007T_V4;
	default: return MxL_UNKNOWN_ID;
	}
}

MxL_ERR_MSG MxL_RFSynth_Lock_Status(MxL5007_TunerConfigS* myTuner, BOOL* isLock)
{
	UINT8 Data;
	*isLock = FALSE;
	if(MxL_I2C_Read(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, 0xD8, &Data))
		return MxL_ERR_OTHERS;
	Data &= 0x0C;
	if (Data == 0x0C)
		*isLock = TRUE;  //RF Synthesizer is Lock
	return MxL_OK;
}

MxL_ERR_MSG MxL_REFSynth_Lock_Status(MxL5007_TunerConfigS* myTuner, BOOL* isLock)
{
	UINT8 Data;
	*isLock = FALSE;
	if(MxL_I2C_Read(myTuner->DemodAddr,(UINT8)myTuner->I2C_Addr, 0xD8, &Data))
		return MxL_ERR_OTHERS;
	Data &= 0x03;
	if (Data == 0x03)
		*isLock = TRUE;   //REF Synthesizer is Lock
	return MxL_OK;
}
