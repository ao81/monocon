# arduino-build-daemon

Arduino MEGA / ADK 向けの**常駐ビルドデーモン + 軽量クライアント**実装。
PC スペック (とくに HDD ノート PC) によるビルド時間のばらつきを構造的に
解消することを目的としています。

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
	 ├ utils.h / utils.cpp           汎用ユーティリティ + プロセス実行
	 ├ pipe_io.h / pipe_io.cpp       LSP 形式の名前付きパイプ I/O
	 ├ port_scanner.h / port_scanner.cpp  COM 列挙 (WMI 不使用)
	 ├ daemon_state.h / daemon_state.cpp  グローバル状態
	 ├ builder.h / builder.cpp       コンパイル/アップロードロジック
	 ├ daemon_main.cpp               デーモン本体 (パイプサーバ)
	 └ client_main.cpp               クライアント
```

---

## 3. ビルド

要件:

- Windows 10 / 11
- CMake 3.16+
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

任意のフォルダ (例: `C:\Tools\ArduinoBuilder\`) に両 exe をコピー。

### Windows Defender 除外の追加 (強く推奨)

管理者 PowerShell で:

```powershell
Add-MpPreference -ExclusionProcess "C:\Tools\ArduinoBuilder\arduino-build-daemon.exe"
Add-MpPreference -ExclusionProcess "C:\Tools\ArduinoBuilder\arduino-build-cli.exe"
Add-MpPreference -ExclusionPath    "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools"
Add-MpPreference -ExclusionPath    "$env:LOCALAPPDATA\ArduinoBuildDaemon"
```

ノート PC で「avr-gcc 起動が遅い」「フルビルドが遅い」原因の大半はこれで消えます。

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

## 6. ライフサイクル

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

## 7. キャッシュ

- **core.a グローバルキャッシュ**: `%LOCALAPPDATA%\ArduinoBuildDaemon\core-cache\<key>\core.a`
	- キー: MCU + F_CPU + variant + コンパイラバージョン + cores ディレクトリ mtime
	- 一度生成すれば以降の全プロジェクトで再利用
- **インクリメンタル**: スケッチディレクトリ内の `.ino/.cpp/.c/.h` の
	`(filename, mtime)` シグネチャ。変化なしなら `.hex` をそのまま再利用。
- **キャッシュクリア**:

	```cmd
	arduino-build-cli.exe shutdown
	rmdir /S /Q %LOCALAPPDATA%\ArduinoBuildDaemon\core-cache
	```

---

## 8. 既知の制約

1. **対応ボード**: 現状 ATmega2560 (MEGA / ADK) のみハードコード。
	 他の AVR ボードに広げる場合は `builder.cpp` の `resolveBoardConfig`
	 を拡張してください。
2. **ライブラリ依存**: スケッチディレクトリ内の `.cpp/.c` のみコンパイル。
	 `Arduino15\libraries\` のサードパーティライブラリは現状フルビルドに
	 含まれません。必要なら fqbn 経由で arduino-cli にフルビルドさせる
	 フォールバックパス (forceFullBuild=true) を使ってください。
3. **arduino-cli の依存**: 初回 core.a 生成のために `arduino-cli.exe` が
	 PATH または `%LOCALAPPDATA%\Programs\arduino-cli\` に必要です。
4. **シリアル処理**: 現実装は同時 1 ジョブのみ。並列ビルドが必要なら
	 `runPipeServer` をワーカープール化してください。

---

## 9. トラブルシュート

| 症状 | 原因/対処 |
|---|---|
| `Toolchain not initialized` | Arduino IDE で MEGA 用ボードを 1 回インストールする必要あり (`%LOCALAPPDATA%\Arduino15\packages\arduino\tools\avr-gcc\` が空のとき発生) |
| `arduino-cli not found` | arduino-cli を PATH に通す or `%LOCALAPPDATA%\Programs\arduino-cli\arduino-cli.exe` に置く |
| `No COM port detected` | USB ドライバ未インストール / ケーブル未接続。`arduino-build-cli ports` で確認 |
| ビルドが古いまま | `arduino-build-cli shutdown` してからもう一度ビルド (またはキャッシュディレクトリを消す) |
| daemon が応答しない / 大量にプロセスが残る | `arduino-build-cli kill` で全 daemon を強制終了。次回クライアント呼び出しで自動再起動 |

---

## 10. パフォーマンス目安 (HDD ノート PC 想定)

| 操作 | 旧 C++ 版 | 本 daemon 版 |
|---|---|---|
| 初回起動 | 3–5 秒 | 0.3–1 秒 (1 回だけ) |
| ping | 200–500 ms | 5–10 ms |
| ports | 100–2000 ms (WMI) | 1–5 ms (registry) |
| 無変更ビルド | 1.5–3 秒 | 50–150 ms |
| 1 ファイル変更 | 2–4 秒 | 300–800 ms |
| フルビルド (core.a 再利用) | 8–25 秒 | 0.6–2 秒 |
| フルビルド (core.a 再生成) | 8–25 秒 | 8–25 秒 (初回のみ) |
