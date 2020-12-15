Bulldog チューナー用 BonDriver 改変MODソース @ 2020/5/1

  以下の2ファイルを BonDriver_Bulldog.dll と同じ場所において使用する。

  BonDriver_Bulldog.ini     ; 設定ファイル
  BonDriver_Bulldog.ch.txt  ; チャンネル定義ファイル

  オプティマイズで配布しているカメレオンＵＳＢFX2のＩＤ書きツール(FX2WRID)を
  利用すると大量のBulldogチューナーを同時利用することもできる筈だが未検証。

  ＜例＞ ID=0 のチューナー
    BonDriver_Bulldog_dev0.dll     ; BonDriver_Bulldog.dll を複製したもの
    BonDriver_Bulldog_dev0.ini     ; BonDriver_Bulldog.ini を複製したもの
    BonDriver_Bulldog_dev0.ch.txt  ; BonDriver_Bulldog.ch.txt を複製したもの

  ＜例＞ ID=1 のチューナー
    BonDriver_Bulldog_dev1.dll     ; BonDriver_Bulldog.dll を複製したもの
    BonDriver_Bulldog_dev1.ini     ; BonDriver_Bulldog.ini を複製したもの
    BonDriver_Bulldog_dev1.ch.txt  ; BonDriver_Bulldog.ch.txt を複製したもの

  ＜例＞ ID=2 のチューナー
    BonDriver_Bulldog_dev2.dll     ; BonDriver_Bulldog.dll を複製したもの
    BonDriver_Bulldog_dev2.ini     ; BonDriver_Bulldog.ini を複製したもの
    BonDriver_Bulldog_dev2.ch.txt  ; BonDriver_Bulldog.ch.txt を複製したもの

2019/12/21 からの修正内容

    ・CyApi転送命令を排他で扱うかを決定するフラグ ExclXfer を .ini に追加[5/1]
    ・Cypressの署名付最新ドライバ(v1.2.3.10)を利用すると場合によってはフリーズ
      することのある現象を修正
     ( CyApi転送命令全般をスレッドセーフで扱うコードを追加して対応 )

2015/1/21 からの修正内容

    ・チャンネル定義ファイルの記述を簡易化(拡張子.ch.txtの添付ファイル参照)
    ・機器のID指定の方式を変更

	  BonDriver_Bulldog_0.dll → BonDriver_Bulldog_dev0.dll (devが付く)

    ・ビルド環境を VS2015 にアップグレード

2014/12/21 からの修正内容

    ・TSデータの書き戻しによる高速化を行う TSWriteBack フラグを .ini に追加
    ・TSDualThreading フラグを .ini から削除

2014/12/09 からの修正内容

    ・終了時にメモリが破壊されている等のエラーが発生する不具合を修正[rev]
    ・TSDualThreading=1 にすると終了時に酷くもたつくことのある現象を修正
    ・非同期アロケーション処理を非シリアライズ化
    ・共通ソース部分を亀系チューナーと統合
    ・ビルド環境を VS2013 にアップグレード

2014/11/26 からの修正内容

    ・FIFOアロケーション処理を非同期化
    ・リソースファイル(BonDrvier.rc)をVSExpressシリーズに対応

2014/10/24 からの修正内容

    ・ini にスペース並べ替えに関するオプションを追加[rev8]
     ( InvisibleSpaces / InvalidSpaces / SpaceArrangement )
    ・チャンネル切替時のドロップ／スクランブル削減処理を追加[rev7]
     ( CBonTuner::ResetFxFifo / CUsbFx2Driver::ResetFifoThread )
    ・非同期TSデータの環状ストックを自動で追加充填する機能を追加[rev6]

      AsyncTsQueueNum : 非同期TSデータの環状ストック数(初期値)
      AsyncTsQueueMax : 非同期TSデータの環状ストック最大数

       バッファが足りなくなると AsyncTsQueueNum ～ AsyncTsQueueMax の間で
       環状ストックに自動で追加充填される。(AsyncTsQueueMax=0で追加充填無効)

    ・FIFOデータ受信時の無駄なクリティカルロックを排除(PushFifoBuff)[rev5]
    ・終了時に稀に例外が発生して強制終了することのあるバグの修正[rev2-4]
    ・Spinelと組み合わせるとVHFチャンネル(1～9)が選択できないバグを修正[rev]
    ・FX2側のバッファ待機処理にマルチスレッドレベルで最適化できる処理を追加
     ( ドロップが稀に発生する環境で効果を発揮 )

       ini に TSDualThreading=1 と書くことで有効化
       （※非力PCでは 0 かコメントアウト推奨）

    ・FX2側のバッファ待機限界秒数を500msから1000msに初期値を変更
     ( ini に TSThreadWait=2000 などと書くことによって待機時間を別途変更可能 )
    ・FX2側の待機スレッドの優先順位を変えることのできるパラメータを ini に追加

       TSThreadPriority= -15 ～ 15
                              15 : TimeCritical(最高)
                               2 : Highest(高い) ※デフォルト
                               1 : Higher(やや高い)
                               0 : Normal(普通)
                              -1 : Lower(やや低い)
                              -2 : Lowest(低い)
                             -15 : Idle(最低)

2014/1/26 からの修正内容

    ・CATV C22 チャンネル周波数修正

2013/12/28 からの修正内容

    ・チューナーを開いた時に発生するUSB抜き差し音の除去(OpenDriver)
    ・チャンネルファイルを書換えた場合の受信レベル表示修正(GetCN/GetBER)
    ・Spinel用バッファ待機最適化（WaitTsStream)

P.S.

  このソースを利用した、または利用できなかった事に対するいかなる責任について、
  製作者はその責任を負わないものとすることが本ソース使用の前提条件です。
  このソースを利用して起こった不具合は各人の責任であり、またソースから生成
  されたバイナリの使用は、個人の使用の範囲に留めてください。

  ソースの改変、配布は自由ですが、変更点を明記してなるべくフルソースで
  配布してください。

  また、本ソースの製作者は AKIBASTOCK とはまったく無関係の人間です。
  本ソースについて、問い合わせるような行為は控えてください。

  hyrolean-dtv◆PRY8EAlByw 2013-2020

