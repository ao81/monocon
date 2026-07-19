# Monocon Tools

高校生ものづくりコンテスト用の補助機能です。課題フォルダの一括生成と、シリアルモニターを安全に停止してから行う Arduino 書き込みを提供します。

## Arduinoへの書き込み

`F2` または `Ctrl+Space` を押すと、次の処理を自動で行います。

1. 編集中のファイルを保存する。
2. 使用中のシリアルモニターを停止する。
3. `Arduino: Upload` タスクを実行する。
4. 書き込み前に開いていたシリアルモニターを再開する。

各処理にはタイムアウトがあるため、拡張機能やタスクが応答しない場合でも「アップロードは既に実行中です」の状態が残り続けません。詳しい動作記録は出力パネルの `Monocon` で確認できます。

シリアルモニターはDTR/RTSを無効にして開くため、監視開始時にArduinoが自動リセットされません。

## 生成される構造

例: ベース名 `kadai`、個数 5、ヘッダ `mono_con.h` を指定した場合

```
親ディレクトリ/
├─kadai1/
│   ├─kadai1.ino   (空ファイル)
│   └─mono_con.h   (元ファイルのコピー)
├─kadai2/
│   ├─kadai2.ino
│   └─mono_con.h
├─kadai3/
│   ├─kadai3.ino
│   └─mono_con.h
├─kadai4/
│   ├─kadai4.ino
│   └─mono_con.h
└─kadai5/
    ├─kadai5.ino
    └─mono_con.h
```

## 使い方

1. VS Codeのエクスプローラで、コピー元のヘッダファイル(例: `mono_con.h`)と生成先の親ディレクトリを Ctrl/Cmd+クリックで両方選択する。
2. 右クリックメニューから「Template Generator: フォルダ構造を生成」を選ぶ。
3. 入力欄に個数を入れて Enter。
4. 親ディレクトリ直下に `kadai1 〜 kadaiN` が生成される。

選択を片方しかしていない場合や、コマンドパレットから呼んだ場合は、不足分をダイアログで選び直せます。

## 設定

- `template.baseName` (デフォルト: `kadai`) … サブフォルダ名と .ino ファイル名のベース部分。
- `monoconUpload.baudRate` (デフォルト: `9600`) … 書き込み後に再開するシリアルモニターの通信速度。
- `monoconUpload.reopenMonitor` (デフォルト: `true`) … 書き込み後にシリアルモニターを再開するかどうか。
- `monoconUpload.reopenDelayMs` (デフォルト: `0`) … 書き込み完了後、再開まで待つ時間。
- `monoconUpload.portReleaseDelayMs` (デフォルト: `0`) … 停止後、ポートが解放されるまで待つ時間。通常はネイティブ書き込み側が必要な時だけ再試行します。
- `monoconUpload.taskTimeoutMs` (デフォルト: `180000`) … 書き込みタスクの待機上限。
- `monoconUpload.serialOperationTimeoutMs` (デフォルト: `5000`) … シリアルモニター操作の待機上限。

## 既存フォルダがある場合

`kadai1` などが既に存在する場合は警告ダイアログが出ます。「続行」を選ぶとヘッダのコピーと空 .ino の書き出しを行います(既存の .ino は空ファイルで上書きされる点に注意)。
