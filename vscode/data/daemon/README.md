# arduino-build-daemon (UNO R4 WiFi 専用版)

Arduino **UNO R4 WiFi (Renesas RA4M1 / Cortex-M4F)** 向けの
**常駐ビルドデーモン + 軽量クライアント** 実装。
PC スペック (とくに HDD ノート PC) によるビルド時間のばらつきを構造的に
解消することを目的としています。

旧 ATmega2560 (MEGA / ADK) 版から、ツールチェーンを
`avr-gcc + avrdude (STK500v2)` から `arm-none-eabi-gcc + dfu-util (USB DFU 1.1)` に
完全に置き換えた **2.0.0-unor4wifi** ブランチです。

---

## 1. 構成

```
[VS Code tasks.json]
				│
				▼  exec
[arduino-build-cli.exe]   ← 50KB 程度の薄いクライアント
				│
				▼  Named Pipe (\\.\pipe\arduino-build-<SID>)
[arduino-build-daemon.exe] ← 常駐
	 ├ ツールチェーンパス (起動時 1 回解決)
	 │   - arm-none-eabi-g++ / gcc / objcopy / ar
	 │   - dfu-util.exe (USB DFU 1.1 クライアント)
	 │   - renesas_uno core / variants/UNOWIFIR4 / FSP include
	 ├ COM ポートリスト (レジストリ通知で更新)
	 ├ core.a グローバルキャッシュ
	 └ スケッチ単位のインクリメンタル状態
```

クライアントは pipe に JSON を投げて結果を待つだけ。
daemon が居なければ自動で `DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB`
で起動してから再接続します。

---

## 2. ファイル構成

```
arduino-build-daemon/
├ CMakeLists.txt
├ README.md (このファイル)
└ src/
	 ├ utils.h / utils.cpp                 汎用ユーティリティ + プロセス実行
	 ├ pipe_io.h / pipe_io.cpp             LSP 形式の名前付きパイプ I/O
	 ├ port_scanner.h / port_scanner.cpp   COM 列挙 (WMI 不使用) + UNO R4 VID:PID 判定
	 ├ daemon_state.h / daemon_state.cpp   グローバル状態 (ARM ツールチェーン解決)
	 ├ builder.h / builder.cpp             コンパイル/アップロードロジック (ARM 用)
	 ├ ra4m1_uploader.h / ra4m1_uploader.cpp
	 │                                     1200bps タッチ → DFU 出現待ち → dfu-util
	 ├ daemon_main.cpp                     デーモン本体 (パイプサーバ)
	 └ client_main.cpp                     クライアント
```

旧 `stk500v2.cpp/.h` (ATmega2560 専用 STK500v2 自前実装) は削除されています。

---

## 3. ビルド

要件:

- Windows 10 / 11
- CMake 3.21+
- Visual Studio 2019 以降 (MSVC) または MinGW-w64 (GCC 9+)
- インターネット接続 (初回ビルド時に nlohmann/json を FetchContent で取得)

```cmd
cd arduino-build-daemon
cmake -B build -A x64
cmake --build build --config Release
```

成果物:

- `build/bin/Release/arduino-build-daemon.exe`
- `build/bin/Release/arduino-build-cli.exe`

**重要**: クライアントは `arduino-build-daemon.exe` を「自分と同じディレクトリ」
から探します。両 exe を必ず同じフォルダに配置してください。

---

## 4. インストール

### 4.1 Arduino IDE 側の事前準備

UNO R4 WiFi のツールチェーンと DFU ブートローダドライバが必要です:

1. **Arduino IDE** で **Boards Manager** から
   **"Arduino UNO R4 Boards"** をインストール
   (=パッケージ `arduino:renesas_uno`)
   - これで `arm-none-eabi-gcc`, `dfu-util`, `fsp`, `cores/arduino`,
     `variants/UNOWIFIR4` がすべて `%LOCALAPPDATA%\Arduino15\packages\arduino\` 配下に展開されます
2. **UNO R4 WiFi を一度 PC に接続**してドライバインストールを待つ
   (アプリモード USB CDC ドライバが自動で当たる)
3. **DFU モード用 WinUSB ドライバが必要**な場合があります。Arduino IDE 経由で
   1 度書き込みを成功させると Windows の自動署名で当たります。

### 4.2 daemon と CLI の配置

任意のフォルダ (例: `C:\Tools\ArduinoBuilder\`) に両 exe をコピー。

### 4.3 Windows Defender 除外の追加 (強く推奨)

管理者 PowerShell で:

```powershell
Add-MpPreference -ExclusionProcess "C:\Tools\ArduinoBuilder\arduino-build-daemon.exe"
Add-MpPreference -ExclusionProcess "C:\Tools\ArduinoBuilder\arduino-build-cli.exe"
Add-MpPreference -ExclusionPath    "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools"
Add-MpPreference -ExclusionPath    "$env:LOCALAPPDATA\ArduinoBuildDaemon"
```

ノート PC で「arm-none-eabi-gcc 起動が遅い」「フルビルドが遅い」原因の大半は
これで消えます。

---

## 5. 使い方

### コマンドラインから

```cmd
arduino-build-cli.exe ping
arduino-build-cli.exe ports
arduino-build-cli.exe build  C:\Path\To\Sketch
arduino-build-cli.exe upload C:\Path\To\Sketch
arduino-build-cli.exe upload C:\Path\To\Sketch COM3
arduino-build-cli.exe shutdown
```

### VS Code tasks.json

```jsonc
{
	"version": "2.0.0",
	"tasks": [
		{
			"label": "Arduino: Upload",
			"type": "shell",
			"command": "C:/Tools/ArduinoBuilder/arduino-build-cli.exe",
			"args": ["upload", "${workspaceFolder}"],
			"problemMatcher": [],
			"group": { "kind": "build", "isDefault": true }
		},
		{
			"label": "Arduino: Build only",
			"type": "shell",
			"command": "C:/Tools/ArduinoBuilder/arduino-build-cli.exe",
			"args": ["build", "${workspaceFolder}"],
			"problemMatcher": []
		}
	]
}
```

---

## 6. アップロードシーケンス (1200bps タッチ → DFU)

UNO R4 WiFi の Renesas RA4M1 は工場出荷時に Arduino LLC が用意した USB DFU
ブートローダが ROM 領域 (0x0000-0x3FFF) に焼かれています。
書き込みフローは以下の通り:

1. **1200bps タッチ**: アプリ側 COM (VID:PID = 2341:1002) を 1200bps で open→close
   → MCU 内のアプリが DFU 遷移コードを実行して USB 再列挙
2. **アプリ COM 消失待ち** (最短検出、最大 3 秒)
3. **DFU デバイス出現待ち** (USB VID:PID = 2341:006D、最大 5 秒)
4. **dfu-util** を最小引数で実行:
   ```
   dfu-util -d 2341:006D -a 0 -s 0x4000:leave -D <sketch>.bin
   ```
5. `:leave` で書き込み終了直後にアプリへ自動復帰

`.elf → .bin` 変換は `arm-none-eabi-objcopy -O binary` で行い、
`.hex` を経由しないので速度が出ます。

---

## 7. ライフサイクル

| イベント | 挙動 |
|---|---|
| 初回 build/upload | クライアントが daemon を `DETACHED_PROCESS` で起動 |
| 2 回目以降 | 既存 daemon に pipe 接続 (起動コスト 0) |
| 30 分無リクエスト | daemon が自動 self-terminate |
| Ctrl-C / コンソール close | daemon クリーン終了 (前景実行時のみ) |
| `arduino-build-cli shutdown` | 任意のタイミングで daemon 終了 |
| daemon クラッシュ | 次回クライアント呼び出しで自動再起動 |

`%LOCALAPPDATA%\ArduinoBuildDaemon\daemon.log` にログが残ります。

前景実行 (デバッグ用):

```cmd
arduino-build-daemon.exe --no-daemonize
```

---

## 8. キャッシュ

- **core.a グローバルキャッシュ**: `%LOCALAPPDATA%\ArduinoBuildDaemon\core-cache\<key>\core.a`
	- キー: MCU (cortex-m4) + FPU + float-abi + F_CPU + variant +
		コンパイラバージョン + renesas_uno パッケージバージョン +
		cores ディレクトリ mtime
	- 一度生成すれば以降の全プロジェクトで再利用
- **インクリメンタル**: スケッチディレクトリ内の `.ino/.cpp/.c/.h` の
	`(filename, mtime, size)` シグネチャ。変化なしなら `.bin` をそのまま再利用。
- **キャッシュクリア**:

	```cmd
	arduino-build-cli.exe shutdown
	rmdir /S /Q %LOCALAPPDATA%\ArduinoBuildDaemon\core-cache
	```

---

## 9. 既知の制約

1. **対応ボード**: 現状 UNO R4 WiFi (`UNOWIFIR4` variant) のみハードコード。
	 UNO R4 Minima を追加する場合は `builder.cpp` の `resolveBoardConfig` を拡張し、
	 `daemon_state.cpp` で `MINIMA` variant ディレクトリも解決してください。
2. **ライブラリ依存**: スケッチディレクトリ内の `.cpp/.c` のみコンパイル。
	 `Arduino15\libraries\` のサードパーティライブラリは現状フルビルドに
	 含まれません。必要なら fqbn 経由で arduino-cli にフルビルドさせる
	 フォールバックパス (forceFullBuild=true) を使ってください。
3. **arduino-cli の依存**: 初回 core.a 生成のために `arduino-cli.exe` が
	 PATH または `%LOCALAPPDATA%\Programs\arduino-cli\` に必要です。
4. **DFU ドライバ**: UNO R4 WiFi の DFU モードには WinUSB ドライバが必要です。
	 Arduino IDE 経由で 1 度書き込みを成功させると自動で当たります。
5. **シリアル処理**: 現実装は同時 1 ジョブのみ。並列ビルドが必要なら
	 `runPipeServer` をワーカープール化してください。
6. **ESP32-S3 (WiFi モジュール) ファームウェア**: 本デーモンは
	 RA4M1 (メイン MCU) の書き込みのみを行います。
	 ESP32-S3 のファームウェアアップデートは Arduino IDE の専用機能を使ってください。

---

## 10. トラブルシュート

| 症状 | 原因/対処 |
|---|---|
| `arm-none-eabi-gcc not found` | Arduino IDE で **"Arduino UNO R4 Boards"** を Boards Manager からインストール |
| `dfu-util not found` | 上と同じ。`%LOCALAPPDATA%\Arduino15\packages\arduino\tools\dfu-util\<ver>\dfu-util.exe` を確認 |
| `arduino-cli not found` | arduino-cli を PATH に通す or `%LOCALAPPDATA%\Programs\arduino-cli\arduino-cli.exe` に置く |
| `No COM port detected` | USB ドライバ未インストール / ケーブル未接続。`arduino-build-cli ports` で確認 |
| `DFU device 2341:006D not detected` | WinUSB ドライバ未インストール。Arduino IDE で 1 度書き込んで自動署名 |
| ビルドが古いまま | `arduino-build-cli shutdown` してからもう一度ビルド (またはキャッシュディレクトリを消す) |
| daemon が応答しない | `arduino-build-cli kill` で全 daemon を強制終了。次回クライアント呼び出しで自動再起動 |

---

## 11. パフォーマンス目安 (HDD ノート PC 想定 / UNO R4 WiFi)

| 操作 | Arduino IDE | 本 daemon 版 |
|---|---|---|
| 初回起動 | 3–5 秒 | 0.3–1 秒 (1 回だけ) |
| ping | -- | 5–10 ms |
| ports | 100–2000 ms (WMI) | 1–5 ms (registry) |
| 無変更ビルド | 3–6 秒 | 50–200 ms |
| 1 ファイル変更 | 4–8 秒 | 500–1200 ms |
| フルビルド (core.a 再利用) | 15–35 秒 | 1.0–3.0 秒 |
| アップロード (1200bps + DFU) | 3–5 秒 | 1.5–2.5 秒 |
