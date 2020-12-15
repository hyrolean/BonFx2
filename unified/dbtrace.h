//===========================================================================
#pragma once
#ifndef _DBTRACE_20141218004913510_H_INCLUDED_
#define _DBTRACE_20141218004913510_H_INCLUDED_
//---------------------------------------------------------------------------

// �g���[�X�o��
#ifdef _DEBUG
	#define TRACE MYTARACEFUNC
	void MYTARACEFUNC(LPCTSTR szFormat, ...);
#else
	#define TRACE __noop
#endif

// DBGOUT
#ifdef _DEBUG
    void DBGOUT( const char* pszFormat,... ) ;
#else
    #define DBGOUT __noop
#endif

#define LINEDEBUG DBGOUT("-*-LINE-*- %s(%d) passed.\r\n",__FILE__,__LINE__)

//===========================================================================
#endif // _DBTRACE_20141218004913510_H_INCLUDED_
