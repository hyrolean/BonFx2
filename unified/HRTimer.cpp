//===========================================================================
#include "stdafx.h"
#include <algorithm>

#include "HRTimer.h"
//---------------------------------------------------------------------------

//===========================================================================
namespace PRY8EAlByw {
//---------------------------------------------------------------------------

static void sleep_(DWORD msec, DWORD usec)
{
  msec += usec/1000;
  Sleep(msec>0?msec:1);
}

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

static DWORD doTimerSleep(HANDLE hTimer, const LARGE_INTEGER &time, HANDLE hObj=NULL)
{
	if(!SetWaitableTimer(hTimer, &time, 0, NULL, NULL, 0))
		return hObj ? WAIT_FAILED : FALSE ;
	if(hObj!=NULL) {
		HANDLE handles[2] ;
		handles[0] = hObj ;
		handles[1] = hTimer ;
		DWORD r = WaitForMultipleObjects(2,handles,FALSE,INFINITE);
		return (r==WAIT_OBJECT_0+1) ? WAIT_TIMEOUT : r ;
	}
	return WaitForSingleObject(hTimer,INFINITE)==WAIT_OBJECT_0 ? TRUE : FALSE ;
}

//---------------------------------------------------------------------------

static BOOL s_bHighResolutionTimerMode = FALSE;
void SetHRTimerMode(BOOL useHighResolution)
{ s_bHighResolutionTimerMode = useHighResolution ; }

//---------------------------------------------------------------------------

void HRSleep(DWORD msec, DWORD usec)
{
	if(!s_bHighResolutionTimerMode&&!usec)
	{ sleep_(msec,usec); return; }

	HANDLE hTimer =
#if _WIN_VER >= 0x0600
		s_bHighResolutionSleepMode ?
			CreateWaitableTimerEx(NULL, NULL,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS):
			CreateWaitableTimer(NULL, FALSE, NULL);
#else
		CreateWaitableTimer(NULL, FALSE, NULL);
#endif

	if(hTimer == NULL)
	{ sleep_(msec,usec); return; }

	LARGE_INTEGER time;
	time.QuadPart = - (msec*1000LL + usec) * 10LL ;

	if(!doTimerSleep(hTimer, time))
		sleep_(msec,usec);

	CloseHandle(hTimer);
}

//---------------------------------------------------------------------------

DWORD HRWaitForSingleObject(HANDLE hObj, DWORD msec, DWORD usec)
{
	if(hObj == NULL) {
		HRSleep(msec,usec);
		return WAIT_TIMEOUT;
	}

	if(msec==INFINITE||(!msec&&!usec)||(!s_bHighResolutionTimerMode&&!usec))
		return WaitForSingleObject(hObj,msec);

	HANDLE hTimer =
#if _WIN_VER >= 0x0600
		s_bHighResolutionTimerMode ?
			CreateWaitableTimerEx(NULL, NULL,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS):
			CreateWaitableTimer(NULL, FALSE, NULL);
#else
		CreateWaitableTimer(NULL, FALSE, NULL);
#endif

	if(hTimer == NULL) {
		return WaitForSingleObject(hObj, msec + usec/1000);
	}

	LARGE_INTEGER time;
	time.QuadPart = - (msec*1000LL + usec) * 10LL ;

	DWORD r = doTimerSleep(hTimer, time, hObj) ;

	CloseHandle(hTimer);

	return r ;
}

//---------------------------------------------------------------------------

DWORD HRWaitForMultipleObjects(DWORD numObjs, const HANDLE *hObjs, BOOL waitAll,
  DWORD msec, DWORD usec)
{
	if(waitAll||msec==INFINITE||(!msec&&!usec)||numObjs>=MAXIMUM_WAIT_OBJECTS) {
		if(msec==INFINITE) usec=0;
		return WaitForMultipleObjects(numObjs,hObjs,TRUE,msec+usec/1000);
	}

	HANDLE hTimer =
#if _WIN_VER >= 0x0600
		s_bHighResolutionTimerMode ?
			CreateWaitableTimerEx(NULL, NULL,
				CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS):
			CreateWaitableTimer(NULL, FALSE, NULL);
#else
		CreateWaitableTimer(NULL, FALSE, NULL);
#endif

	if(hTimer == NULL)
		return WaitForMultipleObjects(numObjs,hObjs,FALSE,msec+usec/1000);

	LARGE_INTEGER time;
	time.QuadPart = - (msec*1000LL + usec) * 10LL ;

	HANDLE *hHRObjs = new HANDLE[numObjs+1] ;
	std::copy(&hObjs[0],&hObjs[numObjs],&hHRObjs[0]);
	hHRObjs[numObjs]=hTimer;

	DWORD r = WAIT_FAILED;

	do {

		if(!SetWaitableTimer(hTimer, &time, 0, NULL, NULL, 0))
			break ;

		r = WaitForMultipleObjects(numObjs+1,hHRObjs,FALSE,INFINITE);
		if(r==WAIT_OBJECT_0+numObjs) r = WAIT_TIMEOUT;

	}while(0);

	CloseHandle(hTimer);
	delete [] hHRObjs;

	return r ;
}

//---------------------------------------------------------------------------
} // End of namespace PRY8EAlByw
//===========================================================================
