
#ifndef TC90532_H
#define TC90532_H

#define TC90502_OK              0
#define TC90502_ERR             1

#define DemodAddress            0x10

#define	DEMODTADRS				0x10
#define	DEMODSADRS				0x11

#define	TUNERTADRS				0x60
#define	TINERSADRS				0xC6

#define ISDB_T					1
#define ISDB_S					2


UINT8 SetMxl5007(UINT8 CH,DWORD kHz=0);
UINT8 SetStv6110a(UINT8 BSCh,DWORD kHz=0);
UINT8 SetTC90502(UINT8 TS);
UINT8 ResetTC90502(UINT8 TS);
UINT8 SuspendTC90502(UINT8 TS, BOOL Suspended);
UINT8 SetNuValTC90502(UINT8 TS,BOOL NullDrop);


#endif		//TC90532_H
