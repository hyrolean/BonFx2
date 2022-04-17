// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// TODO: reference additional headers your program requires here

#include <Tchar.h>
#include <Windows.h>
#include <Crtdbg.h>

// TODO: プログラムに必要な追加ヘッダーをここで参照してください。

// メモリリーク検出
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifndef __BORLANDC__
#ifdef _MSC_VER
#if _MSC_VER <= 1200 // VC++ 6.0 以下
//色々と問題が多いコンパイラなので、場合分け用のフラグを設定しておく
#define __MSVC_1200__
//VC++ for構文のインライン変数除去マクロ
#define for if(0) ; else for
#endif
// C4786 警告を除去
#pragma warning (disable : 4786)
#endif
#endif

#include "..\dbtrace.h"
#include "..\HRTimer.h"
