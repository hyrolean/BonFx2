// IBonDriver3.h: IBonDriver3 �N���X�̃C���^�[�t�F�C�X
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include "IBonDriver2.h"


/////////////////////////////////////////////////////////////////////////////
// Bon�h���C�o�C���^�t�F�[�X3
/////////////////////////////////////////////////////////////////////////////

class IBonDriver3 : public IBonDriver2
{
public:
// IBonDriver3
	virtual const DWORD GetTotalDeviceNum(void) = 0;
	virtual const DWORD GetActiveDeviceNum(void) = 0;
	virtual const BOOL SetLnbPower(const BOOL bEnable) = 0;
	
// IBonDriver
	virtual void Release(void) = 0;
};
