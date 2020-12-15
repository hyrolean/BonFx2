//===========================================================================
#include "stdafx.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <clocale>

#include "dbtrace.h"
//---------------------------------------------------------------------------
#define DEBUG_TO_X_DRIVE

#ifdef _DEBUG
	void MYTARACEFUNC(LPCTSTR szFormat, ...)
	{
		va_list Args;
		TCHAR szTempStr[1024];

		va_start(Args , szFormat);
		::wvsprintf(szTempStr, szFormat, Args);
		va_end(Args);

        #ifndef DEBUG_TO_X_DRIVE
        ::OutputDebugString(szTempStr) ;
        #else
        FILE *fp = NULL ;
        fopen_s(&fp,"X:\\Debug.txt","a+t") ;
        if(fp) {
          size_t ln = wcslen(szTempStr) ;
          if(ln) {
            char *mbcs = new char[ln*2 + 2] ;
            size_t mbLen = 0 ;
            setlocale(LC_ALL,"japanese");
            wcstombs_s(&mbLen, mbcs, ln*2+1, szTempStr, _TRUNCATE);
            std::fputs(mbcs,fp) ;
            delete [] mbcs;
          }
          std::fclose(fp) ;
        }
        #endif
	}
#endif

#ifdef _DEBUG
    void DBGOUT( const char* pszFormat,... )
    {
        std::va_list marker ;
        char edit_str[1024] ;
        va_start( marker, pszFormat ) ;
        vsprintf_s( edit_str, sizeof(edit_str), pszFormat, marker ) ;
        va_end( marker ) ;
        #ifndef DEBUG_TO_X_DRIVE
        OutputDebugStringA(edit_str) ;
        #else
        FILE *fp = NULL ;
        fopen_s(&fp,"X:\\Debug.txt","a+t") ;
        if(fp) {
          std::fputs(edit_str,fp) ;
          std::fclose(fp) ;
        }
        #endif
    }
#endif

//===========================================================================
