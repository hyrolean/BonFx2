//
//Chameleon USB FX2 API Ver1.00 By OPTIMIZE
//
#include "stdafx.h"
#include <stdlib.h>
#include <process.h>
#include "cusb2.h"


// Modified by 拡張ツール中の人
#pragma comment(lib, "CyAPI.lib")

#define USBEP_FINISHXFER_AFTER_ABORTED 1

cusb2::cusb2(HANDLE h)  //コンソールモード時はhはNULLを指定
{
    loading = false;
    _id = 0;
    USBDevice = new CCyUSBDevice(h);
    bDevNodeChange = false;
    bArrived = false;
    if(h == NULL)
    {
        cons_mode = true;
        _USBDevice= NULL;
    }
    else
    {
        cons_mode = false;
        _USBDevice= new CCyUSBDevice();
    }
}

cusb2::~cusb2()
{
    if(USBDevice) {
      if(USBDevice->IsOpen()) USBDevice->Close();
      delete USBDevice;
    }
    if(_USBDevice) delete _USBDevice;
}

	static bool check_manufacturer(CCyUSBDevice *USBDevice, u8 *string1) {
        wchar_t wcs[64];
        size_t n;
        mbstowcs_s(&n, wcs, 64, (char *)string1, 64);
        TRACE(L"fwload: Manufacturer=\"%s\", Compare=\"%s\"\r\n"
          ,USBDevice->Manufacturer,wcs) ;
        if(wcscmp(USBDevice->Manufacturer, wcs) == 0)
            return true;
		return false ;
	}

bool cusb2::fwload(u8 id, u8 *fw, u8 *string1)
{
    u8 d;
    bool find = false ;
    _id = id;
    if(USBDevice->IsOpen()) USBDevice->Close();

    if(cons_mode || (loading == false)){
        for(d=0; d < USBDevice->DeviceCount(); d++)
        {
            USBDevice->Manufacturer[0]=0;   //前回openしたデバイスの情報が残るので消しておく
            USBDevice->Open(d);
            if(((id == 0)&&((USBDevice->BcdDevice & 0x0f00) == 0x0000)) //FX2LP:0xA0nn FX2:0x00nn
                || (USBDevice->BcdDevice == 0xff00+id))
            {
                if((USBDevice->VendorID == 0x04B4) && (USBDevice->ProductID == 0x8613)) find = true;
                if((USBDevice->VendorID == 0x04B4) && (USBDevice->ProductID == 0x1004)) find = true;
            }
            if(find) break;
            USBDevice->Close();
        }
        if(!find) return false;

        //必要なファームがロードされているか文字列で判断
        if(string1!=NULL)
        {
            if(check_manufacturer(USBDevice,string1))
			  return true ;
        }

        //IICイメージのロード＆8051リセット
        u16 len, ofs;
        long llen;

        //8051停止
        USBDevice->ControlEndPt->ReqCode = 0xA0;
        USBDevice->ControlEndPt->Value = 0xE600;
        USBDevice->ControlEndPt->Index = 0;
        llen = 1;
        u8 tmp = 1;
        USBDevice->ControlEndPt->Write(&tmp, llen);

        fw += 8;
        u8 *fw_patch_ptr = NULL ;
        for(;;)
        {
            len = fw[0]*0x100 + fw[1];
            ofs = fw[2]*0x100 + fw[3];

            //FWのデバイスデスクリプタ VID=04B4 PID=1004 Bcd=0000を見つけ出し、
            //idに適合するBcd(FFxx)に書き換える
            const u8 des[]={0xb4, 0x04, 0x04, 0x10, 0x00, 0x00};
            u32 i,j;
            for(i = 0; i < (len & (unsigned)0x7fff); i++)
            {
                for(j = 0; j < 6; j++) if(fw[4+i+j] != des[j]) break;
                if(j >= 6)
                {
                    fw[4+i+4] = id;
                    fw[4+i+5] = 0xff;
                    fw_patch_ptr = fw+4+i+4;
                    break;
                }
            }
            //ベンダリクエスト'A0'を使用して8051に書き込む
            USBDevice->ControlEndPt->Value = ofs;
            llen = len & 0x7fff;
            USBDevice->ControlEndPt->Write(fw+4, llen);

            if(len & 0x8000)
                break;  //最終（リセット）
            fw += len+4;
        }
        if(fw_patch_ptr!=NULL)
        {
            fw_patch_ptr[0]=fw_patch_ptr[1]=0;
        }
        USBDevice->Close();
        if(!cons_mode) return false;
        loading = true;
        Sleep(100);
    }

    //再起動後のファームに接続
    find = false;
    u32 i;
    for(i = 0; i < 40; i++)
    {
        for(d=0; d < USBDevice->DeviceCount(); d++)
        {
            USBDevice->Manufacturer[0]=0;   //前回openしたデバイスの情報が残るので消しておく
            USBDevice->Open(d);
            if(USBDevice->BcdDevice == 0xff00+id) {
              if((USBDevice->VendorID == 0x04B4) && (USBDevice->ProductID == 0x8613)) find = true ;
              if((USBDevice->VendorID == 0x04B4) && (USBDevice->ProductID == 0x1004)) find = true ;
			}
			if(find) {
              if(string1!=NULL) {
                find = check_manufacturer(USBDevice,string1) ;
              }
			  break;
			}
            USBDevice->Close();
        }
        if(find || !cons_mode) break;
        Sleep(100);
    }
    loading = false;
    if(!find) return false;
    return true;
}

CCyUSBEndPoint* cusb2::get_endpoint(u8 addr)
{
    u8 ep;
    for(ep = 0; ep < USBDevice->EndPointCount(); ep++)
    {
        if(USBDevice->EndPoints[ep]->Address == addr)
        {
            USBDevice->EndPoints[ep]->Reset();
            return USBDevice->EndPoints[ep];
        }
    }
    return NULL;
}

bool cusb2::xfer(CCyUSBEndPoint *ep, PUCHAR buf, LONG &len)
{
    if(ep->GetXferSize() < (unsigned)len)
        ep->SetXferSize(len);
    return(ep->XferData(buf, len));
}


// Modified by 拡張ツール中の人 ( Fixed by ◆PRY8EAlByw )
unsigned int __stdcall cusb2::thread_proc(LPVOID pv)
{
  register cusb2_tcb *tcb = static_cast<cusb2_tcb*>(pv) ;
  u32 i;
  bool success = false;
  LONG len;
  CCyUSBEndPoint *ep;


  const SIZE_T szIniHeap =
    sizeof(HANDLE) * tcb->ques * 2    // event
    + sizeof(OVERLAPPED) * tcb->ques  // ovlp
    + sizeof(PUCHAR) * tcb->ques      // data
    + sizeof(PUCHAR) * tcb->ques ;    // context

  HANDLE heap = HeapCreate( 0 , szIniHeap , szIniHeap + tcb->xfer * tcb->ques ) ;
  if (!heap)
    return 1 ;


  tcb->idTh = ::GetCurrentThreadId() ;

  ep = tcb->ep;
  tcb->nsuccess = 0;
  tcb->nfailure = 0;

  tcb->fResetOrdered = false ;
  tcb->evAbortOrdered = CreateEvent(NULL, false, false, NULL) ;
  tcb->evResetCompleted = CreateEvent(NULL, false, false, NULL) ;
  tcb->fTerminated = false ;
  tcb->fDone = false ;

  HANDLE *event = (HANDLE*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(HANDLE) * tcb->ques * 2); //new HANDLE[tcb->ques * 2];
  OVERLAPPED *ovlp = (OVERLAPPED*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(OVERLAPPED) * tcb->ques); //new OVERLAPPED[tcb->ques];
  PUCHAR *data = (PUCHAR*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(PUCHAR) * tcb->ques); //new PUCHAR[tcb->ques];
  PUCHAR *context = (PUCHAR*) HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(PUCHAR) * tcb->ques); //new PUCHAR[tcb->ques];
  ep->SetXferSize(tcb->xfer);
  BOOL ret = SetThreadPriority(tcb->th, tcb->prior);

  const DWORD WAIT_MSEC_LIMIT = tcb->wait ;

  const bool incoming = (tcb->epaddr & 0x80) ? true : false ;

  if (incoming) {   //IN(FX2->PC)転送
    for (i = 0; i < tcb->ques; i++) {
      if (tcb->wb_begin_func)
        data[i] = (PUCHAR) tcb->wb_begin_func(tcb->xfer, tcb->idTh) ;
      else
        data[i] = (PUCHAR) HeapAlloc(heap, HEAP_ZERO_MEMORY, tcb->xfer) ; //new UCHAR[tcb->xfer];
      event[i] = CreateEvent(NULL, false, false, NULL) ;
      ovlp[i].hEvent = event[i] ;
      if(data[i]) {
	    tcb->xfer_lock(true) ;
	    context[i] = ep->BeginDataXfer(data[i], tcb->xfer, &(ovlp[i]));
	    tcb->xfer_lock(false) ;
      }else
	    context[i] = NULL ;
      if(!context[i]) {
        if (tcb->wb_finish_func&&data[i]) {
            tcb->wb_finish_func(data[i], 0, tcb->idTh);
            data[i]=NULL ;
        }
        ResetEvent(event[i]) ;
      }
	}
    HeapCompact(heap, 0) ;
    event[tcb->ques] = tcb->evAbortOrdered ;
	for(i=0;i<tcb->ques-1;i++) {
      event[tcb->ques+1+i]=event[i] ;
	}

    u32 next_wait_index = 0 ;
    bool retried=false ;
    for (i = 0;!tcb->fTerminated;) {
      bool aborted = tcb->fResetOrdered ;
      bool timeover = false ;
      if (!aborted) {
        #if 0
        // 単にOVERLAPPEDのhEventを待機しているだけだと思われる…
        aborted = !ep->WaitForXfer(&(ovlp[i]), WAIT_MSEC_LIMIT) ;
        #else
        if (i == next_wait_index ) { // 待機完了しているものはスキップする
          // ※ WaitForMultipleObjects で待てるハンドル数は最大64個まで
          if(context[i] && data[i]) {
            DWORD waitRes = WaitForMultipleObjects(tcb->ques + 1, &event[i], FALSE, WAIT_MSEC_LIMIT) ;
            bool inner = (waitRes >= WAIT_OBJECT_0 && waitRes < WAIT_OBJECT_0 + tcb->ques + 1) ;
            aborted = inner && (waitRes - WAIT_OBJECT_0) + i == tcb->ques;
            if (!aborted) {
              // 次期待機インデックス更新
              if(inner) {
                next_wait_index = (waitRes-WAIT_OBJECT_0)+i ;
                if(next_wait_index>=tcb->ques) next_wait_index-- ; // dec abort ev
                next_wait_index++ ;
                if (next_wait_index >= tcb->ques)
                  next_wait_index -= tcb->ques ;
              }else {
                timeover = true ;
              }
            }
          }else {
            next_wait_index = i+1 ;
            if (next_wait_index >= tcb->ques)
              next_wait_index -= tcb->ques ;
          }
        }
        #endif
      }
      if (tcb->fTerminated)
        break ;

      if (aborted) {
        ::ResetEvent(tcb->evAbortOrdered) ;
        tcb->fResetOrdered = false ;
        //WaitForSingleObject(&(ovlp[i]),WAIT_MSEC_LIMIT);
        len = 0;
        success = false;
      }else if(timeover) {
        if(retried) {
          len = 0 ;
          success = false ;
          if(data[i]&&context[i]) {
            len=tcb->xfer;
            DBGOUT("cusb2: Force DataXfering...\r\n");
        	tcb->xfer_lock(true) ;
            success = ep->FinishDataXfer(data[i], len, &(ovlp[i]), context[i]);
        	tcb->xfer_lock(false) ;
            DBGOUT("cusb2: Force DataXfering %s.\r\n",success?"completed":"failed");
            if(!success) aborted=true ;
            ResetEvent(event[i]) ;
          }
          if(success) next_wait_index = i+1 ;
          if (next_wait_index >= tcb->ques)
            next_wait_index -= tcb->ques ;
        }else {
          if(data[i]&&context[i]) {
            retried = true ;
            continue ;
          }
          len = 0;
          success = false ;
          next_wait_index = i+1 ;
          if (next_wait_index >= tcb->ques)
            next_wait_index -= tcb->ques ;
        }
      }else {
        len = tcb->xfer;
        if(data[i]&&context[i]) {
          tcb->xfer_lock(true) ;
          success = ep->FinishDataXfer(data[i], len, &(ovlp[i]), context[i]);
          tcb->xfer_lock(false) ;
          if(!success) aborted=true ;
        }
        else
          success = false ;
        ResetEvent(event[i]) ;
      }
      if(retried)
        retried = false ;

      if (success) {
        if (tcb->cb_func) {
          if (!tcb->cb_func(data[i], (u32)len, tcb->idTh)) {
            tcb->fTerminated = true;
          }
        }
        tcb->nsuccess++;
      }else {
        tcb->nfailure++;
      }

      if (tcb->wb_finish_func&&data[i]) {
        tcb->wb_finish_func(data[i], success ? len : 0, tcb->idTh);
        data[i]=NULL ;
      }
      if(!aborted) {
        if (tcb->wb_begin_func && tcb->wb_finish_func)
          data[i] = (PUCHAR)tcb->wb_begin_func(tcb->xfer, tcb->idTh);
        if(data[i]) {
          tcb->xfer_lock(true) ;
          context[i] = ep->BeginDataXfer(data[i], tcb->xfer, &(ovlp[i]));
          tcb->xfer_lock(false) ;
        }else
          context[i] = NULL ;
        if(!context[i]) {
          if (tcb->wb_finish_func&&data[i]) {
            tcb->wb_finish_func(data[i], 0, tcb->idTh);
            data[i]=NULL ;
          }
          ResetEvent(event[i]) ;
        }
      }

      if (aborted) {
        ep->Abort();
        // Restart queueing all.
        for (u32 j = 0;j < tcb->ques ;j++) {
          u32 t = (i + j) % tcb->ques ;
          if(data[t]&&context[t]) {
            tcb->xfer_lock(true) ;
            LONG n=tcb->xfer;
            ep->FinishDataXfer(data[t], n, &(ovlp[t]), context[t]);
            tcb->xfer_lock(false) ;
          }
          if (tcb->wb_finish_func&&data[t])
            tcb->wb_finish_func(data[t], 0, tcb->idTh);
          ResetEvent(ovlp[t].hEvent) ;
          if (tcb->wb_begin_func && tcb->wb_finish_func)
            data[t] = (PUCHAR)tcb->wb_begin_func(tcb->xfer, tcb->idTh);
          if(data[t]) {
            tcb->xfer_lock(true) ;
            context[t] = ep->BeginDataXfer(data[t], tcb->xfer, &(ovlp[t]));
            tcb->xfer_lock(false) ;
          }else
            context[t] = NULL ;
          if(!context[t]) {
            if (tcb->wb_finish_func&&data[t])
              tcb->wb_finish_func(data[t], 0, tcb->idTh);
            data[t]=NULL ;
            ResetEvent(event[t]) ;
          }
        }
        SetEvent(tcb->evResetCompleted) ;
        next_wait_index = i ;
      }else {
        if (++i >= tcb->ques)
          i = 0;
      }

    }
  }else {   //OUT(PC->FX2)転送
    for (i = 0; i < tcb->ques; i++) {
      data[i] = (PUCHAR) HeapAlloc(heap, HEAP_ZERO_MEMORY, tcb->xfer) ; //new UCHAR[tcb->xfer];
      event[i] = CreateEvent(NULL, false, false, NULL) ;
      ovlp[i].hEvent = event[i] ;
      if (!tcb->cb_func(data[i], (u32)tcb->xfer, tcb->idTh)) {
        tcb->fTerminated = true;
      }
      tcb->xfer_lock(true) ;
      context[i] = ep->BeginDataXfer(data[i], tcb->xfer, &(ovlp[i]));
      tcb->xfer_lock(false) ;
    }
    HeapCompact(heap, 0) ;
    for (i = 0;!tcb->fTerminated;) {
      if (!ep->WaitForXfer(&(ovlp[i]), WAIT_MSEC_LIMIT)) {
        ep->Abort();
        //WaitForSingleObject(&(ovlp[i]),WAIT_MSEC_LIMIT);
      }


      len = tcb->xfer;
      tcb->xfer_lock(true) ;
      success = ep->FinishDataXfer(data[i], len, &(ovlp[i]), context[i]);
      tcb->xfer_lock(false) ;

      if (success) {
        tcb->nsuccess++;
      }else {
        tcb->nfailure++;
      }

      if (!tcb->cb_func(data[i], (u32)tcb->xfer, tcb->idTh)) {
        tcb->fTerminated = true;
      }
      tcb->xfer_lock(true) ;
      context[i] = ep->BeginDataXfer(data[i], tcb->xfer, &(ovlp[i]));
      tcb->xfer_lock(false) ;

      if (++i == tcb->ques)
        i = 0;
    }
  }

  tcb->fTerminated = true ;

  ep->Abort() ;
  for (u32 j = 0; j < tcb->ques; j++) {
    u32 t = (i + j) % tcb->ques ;
    if(data[t]&&context[t]) {
      tcb->xfer_lock(true) ;
      LONG n=tcb->xfer;
      ep->FinishDataXfer(data[t], n, &(ovlp[t]), context[t]);
      tcb->xfer_lock(false) ;
    }
    if (tcb->wb_begin_func && tcb->wb_finish_func)
      tcb->wb_finish_func(data[t], 0, tcb->idTh) ;
    CloseHandle(event[t]);
    //HeapFree(heap,0,data[i]); //delete [] data[i];
  }

  CloseHandle(tcb->evResetCompleted) ;
  tcb->evResetCompleted = NULL ;
  CloseHandle(tcb->evAbortOrdered) ;
  tcb->evAbortOrdered = NULL ;

  //delete [] context;
  //delete [] data;
  //delete [] ovlp;
  //delete [] event;

  HeapDestroy(heap) ;

  tcb->fDone = true ;
  _endthreadex(0) ;
  return 0;
}

cusb2_tcb* cusb2::start_thread(u8 epaddr, u32 xfer, s32 ques, u32 wait, int prior, cb_func_t cb_func)
{
    cusb2_tcb *tcb = new cusb2_tcb;
    tcb->epaddr = epaddr;
    tcb->ep = get_endpoint(epaddr);
    tcb->epaddr = epaddr;
    tcb->xfer = xfer;
    tcb->ques = ques;
    tcb->wait = wait;
    tcb->prior = prior;
    tcb->cb_func = cb_func;                                 // Modified by 拡張ツール中の人
    tcb->th = (HANDLE)_beginthreadex(NULL, 0, cusb2::thread_proc, tcb, 0, NULL);
    Sleep(100);
    return tcb;
}

void cusb2::delete_thread(cusb2_tcb *tcb)
{
    tcb->fTerminated = true ;
    WaitForSingleObject(tcb->th, INFINITE);
    CloseHandle(tcb->th) ;
    delete tcb;
}

s32 cusb2::check_fx2(u8 id)
{
    u8 d;
    s32 ret = 0;
    for(d=0; d < _USBDevice->DeviceCount(); d++)
    {
        _USBDevice->Open(d);
        if(((id == 0)&&((_USBDevice->BcdDevice & 0x0f00) == 0x0000))    //FX2LP:0xA0nn FX2:0x00nn
            || (_USBDevice->BcdDevice == 0xff00+id))
        {
            if((_USBDevice->VendorID == 0x04B4) && (_USBDevice->ProductID == 0x8613)) ret = 1;
            if((_USBDevice->VendorID == 0x04B4) && (_USBDevice->ProductID == 0x1004)) ret = 2;
        }
        _USBDevice->Close();
        if(ret) break;
    }
    return ret;
}

bool cusb2::PnpEvent(WPARAM wParam, LPARAM lParam)
{
    s32 st;
    if (wParam == DBT_DEVICEARRIVAL)
        if (bDevNodeChange)
            bArrived = true;

    if (wParam == DBT_DEVICEREMOVECOMPLETE)
    {
        if (bDevNodeChange) {
            bDevNodeChange = false;
            if(check_fx2(_id) == 0)
                return true;        //対象FX2がリムーブ
        }
    }

    if (wParam == DBT_DEVNODES_CHANGED)
    {
        bDevNodeChange = true;

        if (bArrived) {
            bArrived = false;
            bDevNodeChange = false;
            st = check_fx2(_id);
            if(st == 1) return true;                //FWロード要
            if((st == 2) && loading) return true;   //RENUM
        }
    }
    return false;
}
