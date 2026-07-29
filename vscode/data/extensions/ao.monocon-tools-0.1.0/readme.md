# Monocon Tools

高校生ものづくりコンテスト向けのVS Code拡張機能です。ポータブル版・通常版のどちらでも利用できます（Windows x64、Arduino Mega 2560専用）。

## 機能

### Arduinoへ書き込み

コマンド「Monocon Tools: Arduinoへ書き込み」で、次の処理を自動実行します。

1. 編集中のファイルを保存
2. 使用中のシリアルモニターを停止
3. ネイティブワーカーでコンパイル・書き込み・読み戻し検証
4. 書き込み前に開いていたシリアルモニターを再開

通常は拡張機能内のワーカースレッドからC++ネイティブエンジンを直接実行します。独立した常駐デーモンやCLIプロセスは使用しません。AVR GCC、Arduino Mega 2560コア、事前生成済み`core.a`を同梱し、Visual C++ Runtimeもネイティブエンジンへ静的リンクしているため、Windows x64版VS Codeへこの拡張機能をインストールするだけでオフライン動作します。ネイティブエンジンを初期化できない場合に限り、同梱CLIを使う互換タスクへ切り替えます。コンパイルだけのコマンドにも同じ互換経路があります。ポータブル版の互換CLIとデーモンもVisual C++ Runtimeを静的リンクし、同じ同梱リソースを自動検出して、Unicodeパスと設定済みCOMポートをそのまま引き継ぎます。IPC名はエンジン世代ごと、書き込み完了通知はVS Codeの実行ごとに分離されるため、更新前のデーモンや別ウィンドウの通知と混線しません。

日本語、絵文字、空白を含むフォルダー名・ファイル名に対応しています。Arduino IDEと同様に、フォルダー名と同じ`.ino`を先頭タブとして結合し、後方で定義した関数のプロトタイプを自動生成します。条件コンパイル、複数行マクロ、演算子オーバーロード、`extern "C"`、`src/`配下のC/C++/AVRアセンブリにも対応します。COMポートが複数ある場合は書き込み先を明示的に選びます。同じCOMポートへの同時書き込みはVS Codeウィンドウやプロセスをまたいで排他制御します。さらに、フラッシュへ触る前にMCU署名がATmega2560（`1E 98 01`）であることを照合し、別のAVRや異常応答では何も書き込まず停止します。

差分キャッシュは更新日時だけに依存せず、ソース、ヘッダー、オブジェクト、`core.a`、ELF、HEXの内容を検証します。大きい生成物は固定メモリのストリーミングハッシュで検証し、コンパイルやリンクが途中で失敗した場合は旧HEXを成功扱いせず、安全に再ビルドします。破損したキャッシュメタデータや`core.a`は自動修復します。削除・改名済みソースの中間物も回収し、30日以上使われないキャッシュまたは64課題を超えた古いキャッシュを安全なプロセス間ロック下で整理します。

Arduino公式のEEPROM、Ethernet、HID、LiquidCrystal、SD、Servo、SoftwareSerial、SPI、Stepper、TFT、Wireを同梱しています。スケッチから参照されたライブラリとその依存先だけを自動検出・差分コンパイルするため、オフラインのまま利用できます。

シリアルモニターはDTR/RTSを無効にして開くため、監視開始時のArduino自動リセットを防ぎます。各処理にはタイムアウトがあり、範囲外の設定値は安全な既定値へ正規化され、失敗後に「アップロードは既に実行中です」の状態が残りません。動作記録は出力パネルの`Monocon Tools`で確認できます。

コンパイルエラーはターミナル用の色制御文字を除去し、ファイル名、行・列、原因、候補、クリック可能な場所に整理して日本語表示します。同じ内容をVS Codeの「問題」パネルとエディター上の波線にも登録します。

VS Code下部では、書き込み中を黄色、失敗時を赤、完了時を緑の文字色で統一して表示します。CLIの`Total client time`出力直後に検証完了通知を直接受け取り、「Arduinoへの書き込みが完了しました。」という通知を表示します。VS Codeのタスク終了通知を待たないため表示遅延がなく、ステータスバーにも完了状態を残します。

既定のショートカットは`F2`と`Ctrl+Space`です。

### Arduinoをコンパイル

コマンド「Monocon Tools: Arduinoをコンパイル」では書き込みを行わず、同じ高速差分キャッシュを使ってビルドだけを実行します。`.ino`ファイルを開いて実行してください。

### 課題フォルダーを一括作成

コマンド「Monocon Tools: 課題フォルダーを一括作成」で、選択したヘッダーファイルを含む課題用フォルダーをまとめて作成します。

作成数を入力したあと、課題フォルダーと`.ino`ファイルに使用する基本名を指定できます。既定値は`mon`です。

例: 基本名`mon`、作成数3、ヘッダー`mono_con.h`

```text
作成先/
├─ mon1/
│  ├─ mon1.ino
│  └─ mono_con.h
├─ mon2/
│  ├─ mon2.ino
│  └─ mono_con.h
└─ mon3/
   ├─ mon3.ino
   └─ mono_con.h
```

エクスプローラーでヘッダーファイルと作成先フォルダーを選択して右クリックするか、コマンドパレットから実行できます。

## 設定

- `monoconTools.taskFolders.baseName`: 課題フォルダーの基本名
- `monoconTools.upload.baudRate`: 再開するシリアルモニターのボーレート
- `monoconTools.upload.port`: 書き込み先COMポート（空欄は自動検出、複数候補時は選択）
- `monoconTools.upload.reopenMonitor`: 書き込み後にモニターを再開するか
- `monoconTools.upload.reopenDelayMs`: 書き込み後から再開までの待機時間
- `monoconTools.upload.portReleaseDelayMs`: モニター停止後のポート解放待機時間
- `monoconTools.upload.taskTimeoutMs`: 書き込みタスクの待機上限
- `monoconTools.upload.serialOperationTimeoutMs`: モニター操作の待機上限

## 内部構成

- `out/extension.js`: 拡張機能のエントリーポイント
- `out/arduino-upload.js`: Arduino書き込みとシリアルモニター連携
- `out/compiler-report.js`: コンパイル診断の日本語化と読みやすいレポート生成
- `out/upload-status.js`: 書き込み完了通知とステータスバー表示
- `out/native-service.js`: ネイティブワーカーのライフサイクルと要求管理
- `out/native-worker.js`: C++ネイティブアドオンの実行ワーカー
- `native/win32-x64/monocon_native_v170.node`: Visual C++ Runtimeを内蔵したビルド・書き込み・検証エンジン
- `resources/arduino/`: 同梱AVRツールチェーンとMega 2560コア
- `out/task-folders.js`: 課題フォルダーの一括作成
- `test/arduino-upload.test.js`: コマンド連携と書き込み処理の回帰テスト
- `test/native-service.test.js`: タイムアウト、異常終了、ワーカー再起動の故障注入テスト
- `test/native-builder.test.js`: Arduino互換性、Unicode、キャッシュ破損、入力上限の統合テスト
- `test/standalone-daemon.test.js`: ポータブル互換経路とIPCの統合テスト
- `test/task-folders.test.js`: 課題フォルダー生成の回帰テスト
