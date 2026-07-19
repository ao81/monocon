# Monocon Tools

高校生ものづくりコンテスト向けのポータブルVS Code補助拡張機能です。

## 機能

### Arduinoへ書き込み

コマンド「Monocon Tools: Arduinoへ書き込み」で、次の処理を自動実行します。

1. 編集中のファイルを保存
2. 使用中のシリアルモニターを停止
3. `Arduino: Upload`タスクでコンパイル・書き込み
4. 書き込み前に開いていたシリアルモニターを再開

シリアルモニターはDTR/RTSを無効にして開くため、監視開始時のArduino自動リセットを防ぎます。各処理にはタイムアウトがあり、失敗後に「アップロードは既に実行中です」の状態が残りません。動作記録は出力パネルの`Monocon Tools`で確認できます。

書き込み中はVS Code下部に進行状況を表示します。CLIの`Total client time`出力後、正常終了なら「Arduinoへの書き込みが完了しました。」という通知を表示し、ステータスバーにも完了状態を残します。

既定のショートカットは`F2`と`Ctrl+Space`です。

### 課題フォルダーを一括作成

コマンド「Monocon Tools: 課題フォルダーを一括作成」で、選択したヘッダーファイルを含む課題用フォルダーをまとめて作成します。

例: 基本名`kadai`、作成数3、ヘッダー`mono_con.h`

```text
作成先/
├─ kadai1/
│  ├─ kadai1.ino
│  └─ mono_con.h
├─ kadai2/
│  ├─ kadai2.ino
│  └─ mono_con.h
└─ kadai3/
   ├─ kadai3.ino
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
- `out/upload-status.js`: 書き込み完了通知とステータスバー表示
- `out/task-folders.js`: 課題フォルダーの一括作成
- `test/arduino-upload.test.js`: コマンド連携と書き込み処理の回帰テスト
- `test/task-folders.test.js`: 課題フォルダー生成の回帰テスト
