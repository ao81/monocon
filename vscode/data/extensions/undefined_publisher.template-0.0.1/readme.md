# TemplateGenerator

ヘッダファイルと親ディレクトリを選んで実行すると、指定個数のサブフォルダを一括生成します。

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
2. 右クリックメニューから「Kadai Generator: フォルダ構造を生成」を選ぶ。
3. 入力欄に個数を入れて Enter。
4. 親ディレクトリ直下に `kadai1 〜 kadaiN` が生成される。

選択を片方しかしていない場合や、コマンドパレットから呼んだ場合は、不足分をダイアログで選び直せます。

## 設定

- `kadaiGenerator.baseName` (デフォルト: `kadai`) … サブフォルダ名と .ino ファイル名のベース部分。例えば `task` に変えると `task1.ino`, `task2.ino`, ... が生成されます。

## 既存フォルダがある場合

`kadai1` などが既に存在する場合は警告ダイアログが出ます。「続行」を選ぶとヘッダのコピーと空 .ino の書き出しを行います(既存の .ino は空ファイルで上書きされる点に注意)。
