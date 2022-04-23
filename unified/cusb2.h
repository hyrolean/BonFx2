//
//Chameleon USB FX2 API Ver1.00 By OPTIMIZE
//

//#include <afxwin.h>         // MFC core and standard components
#include "dbt.h"
#include "cyapi.h"

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned int u32;
typedef signed int s32;

#define CUSB_DEBUG   0

typedef bool (*cb_func_t)(u8 *data, u32 ln, u32 idTh) ;
typedef void (*xfer_lock_func_t)(bool locking, u32 idTh) ;
typedef void *(*write_back_begin_func_t)(u32 max_size, u32 idTh) ;
typedef void (*write_back_finish_func_t)(void *,u32 wrote_size, u32 idTh) ;

struct _cusb2_tcb
{
	HANDLE th;
    DWORD idTh;
	CCyUSBEndPoint *ep;
	u8	epaddr;
	LONG xfer;
	u32 ques;
	cb_func_t cb_func ;
    xfer_lock_func_t xfer_lock_func ;
    write_back_begin_func_t wb_begin_func ;
    write_back_finish_func_t wb_finish_func ;
	u32 nsuccess;
	u32 nfailure;
    bool fResetOrdered;
    HANDLE evAbortOrdered ;
    HANDLE evResetCompleted ;
    bool fPause;
    bool fTerminated;
    bool fDone;
    //---- wait & thread priority -----
    u32             wait  ;
    int             prior ;
    //----
    _cusb2_tcb() {
      th = 0 ;
      idTh = 0 ;
      ep = 0 ;
      epaddr = 0 ;
      xfer = 0 ;
      cb_func = 0 ;
      xfer_lock_func = 0 ;
      wb_begin_func = 0 ;
      wb_finish_func = 0 ;
      nsuccess = 0 ;
      nfailure = 0 ;
      fResetOrdered = 0 ;
      evAbortOrdered = 0 ;
      evResetCompleted = 0 ;
      fPause = 0 ;
      fTerminated = 0 ;
      fDone = 0 ;
      wait = 500 ;
      prior = 0 ;
    }
    void abort() {
      fTerminated = true ;
      if(evAbortOrdered) SetEvent(evAbortOrdered) ;
    }
    void reset(bool pause=false) {
      fPause=pause;
      if(pause) return;
      if(evResetCompleted) ResetEvent(evResetCompleted) ;
      fResetOrdered = true ;
      if(evAbortOrdered) SetEvent(evAbortOrdered) ;
      if(evResetCompleted) WaitForSingleObject(evResetCompleted,wait*3) ;
    }
    void xfer_lock(bool locking) {
      if(xfer_lock_func)  xfer_lock_func(locking, idTh) ;
    }
};

typedef struct _cusb2_tcb cusb2_tcb;

class cusb2{
	CCyUSBDevice *_USBDevice;
	bool bDevNodeChange;
	bool bArrived;
	bool loading;
	bool cons_mode;
	u8 _id;

public:
	CCyUSBDevice *USBDevice;
	cusb2(HANDLE h);
	virtual ~cusb2();

	bool fwload(u8 id, u8 *fw, u8 *string1);
	CCyUSBEndPoint* get_endpoint(u8 addr);
	bool xfer(CCyUSBEndPoint *ep, PUCHAR buf, LONG &len);
	cusb2_tcb *start_thread(u8 epaddr, u32 xfer, s32 ques, u32 wait, int prior, cb_func_t cb_func );
	void delete_thread(cusb2_tcb *tcb);
	bool PnpEvent(WPARAM wParam, LPARAM lParam);

// Modified by ägí£ÉcÅ[ÉãíÜÇÃêl ( Fixed by ÅüPRY8EAlByw )
protected:
	static unsigned int __stdcall thread_proc(LPVOID pv);

private:
	s32 check_fx2(u8 id);
};


