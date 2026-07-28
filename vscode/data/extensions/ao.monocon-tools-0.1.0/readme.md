# Monocon Tools

高校生ものづくりコンテスト向けのVS Code拡張機能です。ポータブル版・通常版のどちらでも利用できます（Windows x64、Arduino Mega 2560専用）。

## 機能

### Arduinoへ書き込み

コマンド「Monocon Tools: Arduinoへ書き込み」で、次の処理を自動実行します。

1. 編集中のファイルを保存
2. 使用中のシリアルモニターを停止
3. ネイティブワーカーでコンパイル・書き込み・読み戻し検証
4. 書き込み前に開いていたシリアルモニターを再開

通常は拡張機能内のワーカースレッドからC++ネイティブエンジンを直接実行します。独立した常駐デーモンやCLIプロセスは使用しません。AVR GCC、Arduino Mega 2560コア、事前生成済み`core.a`を同梱し、Visual C++ Runtimeもネイティブエンジンへ静的リンクしているため、Windows x64版VS Codeへこの拡張機能をインストールするだけでオフライン動作します。ネイティブエンジンを初期化できない場合に限り、互換用の`Arduino: Upload`タスクへ切り替えます。

シリアルモニターはDTR/RTSを無効にして開くため、監視開始時のArduino自動リセットを防ぎます。各処理にはタイムアウトがあり、失敗後に「アップロードは既に実行中です」の状態が残りません。動作記録は出力パネルの`Monocon Tools`で確認できます。

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
- `native/win32-x64/monocon_native.node`: Visual C++ Runtimeを内蔵したビルド・書き込み・検証エンジン
- `resources/arduino/`: 同梱AVRツールチェーンとMega 2560コア
- `out/task-folders.js`: 課題フォルダーの一括作成
- `test/arduino-upload.test.js`: コマンド連携と書き込み処理の回帰テスト
- `test/task-folders.test.js`: 課題フォルダー生成の回帰テスト
