// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#ifndef _WIN32_WINNT		// Windows XP 以降のバージョンに固有の機能の使用を許可します。
#define _WIN32_WINNT 0x0501	// これを Windows の他のバージョン向けに適切な値に変更してください。
#endif

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

#include "dbtrace.h"
#include "pryutil.h"
