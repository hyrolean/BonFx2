/*
 
 Driver APIs for MxL5007 Tuner
 
 Copyright, Maxlinear, Inc.
 All Rights Reserved
 
 File Name:      MxL5007.h

 */

#ifndef __MxL5007_H
#define __MxL5007_H

//#include "MxL5007_Common.h"
//#include "Common.h"

/****************************************************************************
*       Imports and definitions for WIN32                             
****************************************************************************/
//#include <windows.h>
/*
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef char           SINT8;
typedef short          SINT16;
typedef long           SINT32;
typedef float          REAL32;
*/
typedef long           SINT32;

/****************************************************************************\
*      Imports and definitions for non WIN32 platforms                   *
\****************************************************************************/
/*
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef char           SINT8;
typedef short          SINT16;
typedef int            SINT32;
typedef float          REAL32;

// create a boolean 
#ifndef __boolean__
#define __boolean__
typedef enum {FALSE=0,TRUE} BOOL;
#endif //boolean
*/


/****************************************************************************\
*          Definitions for all platforms					                 *
\****************************************************************************/
#ifndef NULL
#define NULL (void*)0
#endif

/******************************/
/*	MxL5007 Err message  	  */
/******************************/

typedef enum{
    MxL_FALSE = 0,
    MxL_TRUE
}MxL_BOOL;

/******************************/
/*	MxL5007 Err message  	  */
/******************************/
typedef enum{
	MxL_OK				=   0,
	MxL_ERR_INIT		=   1,
	MxL_ERR_RFTUNE		=   2,
	MxL_ERR_SET_REG		=   3,
	MxL_ERR_GET_REG		=	4,
	MxL_ERR_OTHERS		=   10
}MxL_ERR_MSG;

/******************************/
/*	MxL5007 Chip verstion     */
/******************************/
typedef enum{
	MxL_UNKNOWN_ID		= 0x00,
	MxL_5007T_V4		= 0x14,
	MxL_GET_ID_FAIL		= 0xFF
}MxL5007_ChipVersion;


/******************************************************************************
    CONSTANTS
******************************************************************************/

#ifndef MHz
	#define MHz 1000000
#endif

#define MAX_ARRAY_SIZE 100


// Enumeration of Mode
// Enumeration of Mode
typedef enum 
{
	MxL_MODE_ISDBT = 0,
	MxL_MODE_DVBT = 1,
	MxL_MODE_ATSC = 2,	
	MxL_MODE_CABLE = 0x10
} MxL5007_Mode ;

typedef enum
{
	MxL_IF_4_MHZ	  = 0,
	MxL_IF_4_5_MHZ	  ,
	MxL_IF_4_57_MHZ	  ,
	MxL_IF_5_MHZ	  ,
	MxL_IF_5_38_MHZ	  ,
	MxL_IF_6_MHZ	  ,
	MxL_IF_6_28_MHZ	  ,
	MxL_IF_9_1915_MHZ ,
	MxL_IF_35_25_MHZ  ,
	MxL_IF_36_15_MHZ  ,
	MxL_IF_44_MHZ	  
} MxL5007_IF_Freq ;

typedef enum
{
	MxL_XTAL_16_MHZ		= 0,
	MxL_XTAL_20_MHZ		,
	MxL_XTAL_20_25_MHZ	,
	MxL_XTAL_20_48_MHZ	,
	MxL_XTAL_24_MHZ		,
	MxL_XTAL_25_MHZ     ,
	MxL_XTAL_25_14_MHZ	,
	MxL_XTAL_27_MHZ		,
	MxL_XTAL_28_8_MHZ	,
	MxL_XTAL_32_MHZ		,
	MxL_XTAL_40_MHZ		,
	MxL_XTAL_44_MHZ		,
	MxL_XTAL_48_MHZ		,
	MxL_XTAL_49_3811_MHZ
} MxL5007_Xtal_Freq ;

typedef enum
{
	MxL_BW_6MHz = 6,
	MxL_BW_7MHz = 7,
	MxL_BW_8MHz = 8
} MxL5007_BW_MHz;

typedef enum
{
	MxL_NORMAL_IF = 0 ,
	MxL_INVERT_IF

} MxL5007_IF_Spectrum ;

typedef enum
{
	MxL_LT_DISABLE = 0 ,
	MxL_LT_ENABLE

} MxL5007_LoopThru ;

typedef enum
{
	MxL_CLKOUT_DISABLE = 0 ,
	MxL_CLKOUT_ENABLE

} MxL5007_ClkOut;

typedef enum
{
	MxL_CLKOUT_AMP_0 = 0 ,
	MxL_CLKOUT_AMP_1,
	MxL_CLKOUT_AMP_2,
	MxL_CLKOUT_AMP_3,
	MxL_CLKOUT_AMP_4,
	MxL_CLKOUT_AMP_5,
	MxL_CLKOUT_AMP_6,
	MxL_CLKOUT_AMP_7
} MxL5007_ClkOut_Amp;

typedef enum
{
	MxL_I2C_ADDR_96 = 96 ,				//0x60
	MxL_I2C_ADDR_97 = 97 ,
	MxL_I2C_ADDR_98 = 98 ,
	MxL_I2C_ADDR_99 = 99 	
} MxL5007_I2CAddr ;

//
// MxL5007 TunerConfig Struct
//
typedef struct _MxL5007_TunerConfigS
{
	UINT8				DemodAddr;
	MxL5007_I2CAddr		I2C_Addr;
	MxL5007_Mode		Mode;
	SINT32				IF_Diff_Out_Level;
	MxL5007_Xtal_Freq	Xtal_Freq;
	MxL5007_IF_Freq	    IF_Freq;
	MxL5007_IF_Spectrum IF_Spectrum;
	MxL5007_ClkOut		ClkOut_Setting;
    MxL5007_ClkOut_Amp	ClkOut_Amp;
	MxL5007_BW_MHz		BW_MHz;
	UINT32				RF_Freq_Hz;
} MxL5007_TunerConfigS;


int I2CWrite(unsigned char adrs,int len,unsigned char *data);
int I2CRead(unsigned char adrs,int len,unsigned char *data);
int I2CRead(unsigned char adrs,unsigned char reg,int len,unsigned char *data);
int I2CRead(unsigned char adrs,unsigned tadrs,unsigned char reg,int len,unsigned char *data);

unsigned char WriteReg(unsigned char wDevice, unsigned char wReg, unsigned char wVal);
unsigned char ReadReg(unsigned char wDevice, unsigned char wReg, unsigned char *rVal);
unsigned char WriteTuner(unsigned char Device,unsigned char addr, unsigned char reg, unsigned char val);
unsigned char ReadTuner(unsigned char Device,unsigned char addr, unsigned char reg, unsigned char *val);


typedef struct
{
	UINT8 Num;	//Register number
	UINT8 Val;	//Register value
} IRVType, *PIRVType;


UINT32 MxL5007_Init(UINT8* pArray,				// a array pointer that store the addr and data pairs for I2C write
					UINT32* Array_Size,			// a integer pointer that store the number of element in above array
					UINT8 Mode,				
					SINT32 IF_Diff_Out_Level,
					UINT8 Xtal_Freq_Hz,
					UINT8 IF_Freq_Hz,
					UINT8 Invert_IF,			
					UINT8 Clk_Out_Enable,    
					UINT8 Clk_Out_Amp		
					);
UINT32 MxL5007_RFTune(UINT8* pArray, UINT32* Array_Size, 
					 UINT32 RF_Freq,			// RF Frequency in Hz
					 UINT8 BWMHz		// Bandwidth in MHz
					 );
UINT32 SetIRVBit(PIRVType pIRV, UINT8 Num, UINT8 Mask, UINT8 Val);

/******************************************************************************
**
**  Name: MxL_Set_Register
**
**  Description:    Write one register to MxL5007
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**					RegAddr				- Register address to be written
**					RegData				- Data to be written
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_SET_REG if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Set_Register(MxL5007_TunerConfigS* myTuner, UINT8 RegAddr, UINT8 RegData);

/******************************************************************************
**
**  Name: MxL_Get_Register
**
**  Description:    Read one register from MxL5007
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**					RegAddr				- Register address to be read
**					RegData				- Pointer to register read
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_GET_REG if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Get_Register(MxL5007_TunerConfigS* myTuner, UINT8 RegAddr, UINT8 *RegData);

/******************************************************************************
**
**  Name: MxL_Tuner_Init
**
**  Description:    MxL5007 Initialization
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_INIT if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Tuner_Init(MxL5007_TunerConfigS* );

/******************************************************************************
**
**  Name: MxL_Tuner_RFTune
**
**  Description:    Frequency tunning for channel
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**					RF_Freq_Hz			- RF Frequency in Hz
**					BWMHz				- Bandwidth 6, 7 or 8 MHz
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_RFTUNE if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Tuner_RFTune(MxL5007_TunerConfigS*, UINT32 RF_Freq_Hz, MxL5007_BW_MHz BWMHz);		

/******************************************************************************
**
**  Name: MxL_Soft_Reset
**
**  Description:    Software Reset the MxL5007 Tuner
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Soft_Reset(MxL5007_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_Loop_Through_On
**
**  Description:    Turn On/Off on-chip Loop-through
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**					isOn				- True to turn On Loop Through
**										- False to turn off Loop Through
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Loop_Through_On(MxL5007_TunerConfigS*, MxL5007_LoopThru);

/******************************************************************************
**
**  Name: MxL_Standby
**
**  Description:    Enter Standby Mode
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Stand_By(MxL5007_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_Wakeup
**
**  Description:    Wakeup from Standby Mode (Note: after wake up, please call RF_Tune again)
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_Wake_Up(MxL5007_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_Check_ChipVersion
**
**  Description:    Return the MxL5007 Chip ID
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**			
**  Returns:        MxL_ChipVersion			
**
******************************************************************************/
MxL5007_ChipVersion MxL_Check_ChipVersion(MxL5007_TunerConfigS*);

/******************************************************************************
**
**  Name: MxL_RFSynth_Lock_Status
**
**  Description:    RF synthesizer lock status of MxL5007
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**					isLock				- Pointer to Lock Status
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail
**
******************************************************************************/
MxL_ERR_MSG MxL_RFSynth_Lock_Status(MxL5007_TunerConfigS *myTuner , BOOL *isLock);

/******************************************************************************
**
**  Name: MxL_REFSynth_Lock_Status
**
**  Description:    REF synthesizer lock status of MxL5007
**
**  Parameters:    	myTuner				- Pointer to MxL5007_TunerConfigS
**					isLock				- Pointer to Lock Status
**
**  Returns:        MxL_ERR_MSG			- MxL_OK if success	
**										- MxL_ERR_OTHERS if fail	
**
******************************************************************************/
MxL_ERR_MSG MxL_REFSynth_Lock_Status(MxL5007_TunerConfigS* , BOOL* isLock);

#endif //__MxL5007_H
