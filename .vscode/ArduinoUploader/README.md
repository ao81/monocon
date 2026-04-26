# ArduinoUploader C++

C++で実装されたArduinoアップロードツール。PowerShellやC#版よりも高速で、.NET Runtimeを必要としません。

## 特徴

- **高速**: ネイティブC++実装で最小限のオーバーヘッド
- **差分コンパイル**: ソースファイルの変更を検出し、必要な場合のみコンパイル
- **キャッシュ管理**: 各種パスとポート情報をキャッシュ
- **自動ポート検出**: COMポートを自動的に検出
- **エラーハンドリング**: 適切なエラーメッセージと処理

## ビルド方法

### 前提条件

- Windows 10/11
- Visual Studio 2019/2022 (C++開発ツール)
- CMake 3.15以上
- Arduino CLI
- avr-gcc
- avrdude

### ビルド手順

```bash
# ビルドスクリプトを実行
build.bat
```

または手動でビルド:

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

ビルドが成功すると、`build\bin\Release\ArduinoUploader.exe`が作成されます。

## 使用方法

### 基本的な使用方法

```bash
ArduinoUploader.exe <BuildPath> <SketchDir> [Port]
```

### 引数

- `<BuildPath>`: ビルド出力ディレクトリのパス
- `<SketchDir>`: Arduinoスケッチ（.inoファイル）があるディレクトリ
- `[Port]`: （オプション）COMポート番号（例: COM3）。指定しない場合は自動検出

### 使用例

```bash
# ポート自動検出で実行
ArduinoUploader.exe "C:\Projects\MyArduino\build" "C:\Projects\MyArduino\MySketch"

# ポート指定で実行
ArduinoUploader.exe "C:\Projects\MyArduino\build" "C:\Projects\MyArduino\MySketch" "COM3"

# カレントディレクトリで実行
ArduinoUploader.exe "build" "." "COM3"
```

## パフォーマンス

C++版の期待されるパフォーマンス:

- **差分コンパイル**: 0.3s → 0.15-0.2s（約50%改善）
- **アップロード**: 0.9s → 0.85-0.9s（わずかな改善）
- **全体のオーバーヘッド**: PowerShell/C#版よりも10-30%改善

## プロジェクト構成

```
ArduinoUploaderCpp/
├── CMakeLists.txt          # CMakeビルド設定
├── build.bat               # ビルドスクリプト
├── main.cpp                # メインエントリーポイント
├── utils.h                 # ユーティリティ関数宣言
├── utils.cpp               # ユーティリティ関数実装
├── uploader.h              # アップローダークラス宣言
├── uploader.cpp            # アップローダークラス実装
└── build/                  # ビルド出力ディレクトリ
    └── bin/
        └── Release/
            └── ArduinoUploader.exe
```

## 機能詳細

### 1. パス解決とキャッシュ

- Arduinoツールチェーンの自動検出
- パス情報のキャッシュによる高速化
- 複数バージョンのArduinoツール対応

### 2. 差分コンパイル

- ソースファイルのタイムスタンプベースのハッシュ計算
- 変更がない場合のコンパイルスキップ
- ヘッダーファイルの変更検出

### 3. 自動ポート検出

- WMIを使用したCOMポート検出
- Arduinoデバイスのパターンマッチング
- ポート情報のキャッシュ

### 4. エラーハンドリング

- 詳細なエラーメッセージ
- ポートキャッシュの自動クリア
- 適切な終了コード

## トラブルシューティング

### ビルドエラー

**CMakeが見つからない場合:**
```
Error: CMake not found. Please install CMake and add it to PATH.
```
解決策: CMakeをインストールしてPATHに追加してください。

**Visual Studioが見つからない場合:**
```
Error: Visual Studio not found
```
解決策: Visual Studio 2019/2022をインストールし、C++開発ツールを有効にしてください。

### 実行時エラー

**avr-gccが見つからない場合:**
```
Error: avr-gcc not found
```
解決策: Arduino CLIをインストールし、適切なツールチェーンをセットアップしてください。

**COMポートが見つからない場合:**
```
Error: No COM port found
```
解決策: Arduinoを接続し、ドライバが正しくインストールされていることを確認してください。

## ライセンス

このプロジェクトはMITライセンスの下で提供されています。

## 貢献

バグ報告や機能リクエストはIssueを通じてお願いします。
