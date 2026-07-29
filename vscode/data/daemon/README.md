# arduino-build-daemon

Arduino MEGA / ADK 向けの**常駐ビルドデーモン + 軽量クライアント**実装。
PC スペック (とくに HDD ノート PC) によるビルド時間のばらつきを構造的に
解消することを目的としています。

リリース判定に必要な信頼性・可用性・保守性・完全性・セキュリティの証跡は
[`RASIS.md`](RASIS.md)に定義しています。

---

## 1. 構成

```
[VS Code tasks.json]
				│
				▼  exec
[arduino-build-cli.exe]   ← Unicode対応の薄いクライアント
				│
				▼  Named Pipe (\\.\pipe\arduino-build-v170-<SID>)
[arduino-build-daemon.exe] ← 常駐
	 ├ 配布物内の同梱ツールチェーンを自動検出
	 ├ ツールチェーンパス (起動時 1 回解決)
	 ├ COM ポートリスト (レジストリ通知で更新)
	 ├ core.a グローバルキャッシュ
	 ├ HEX解析のメモリキャッシュ
	 └ スケッチ単位のインクリメンタル状態
```

クライアントは pipe に JSON を投げて結果を待つだけ。
daemon が居なければ自動で `DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB`
で起動してから再接続します。

パイプと多重起動防止Mutexにはエンジン世代と現在ユーザーのSIDを含めます。
更新直後のCLIが起動中の旧daemonへ接続することはありません。パイプは
同一ユーザーだけが利用でき、リモート接続を拒否し、要求サイズを1 MiBに制限します。

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

- `build/bin/arduino-build-daemon.exe`
- `build/bin/arduino-build-cli.exe`

**重要**: クライアントは `arduino-build-daemon.exe` を「自分と同じディレクトリ」
から探します。両 exe を必ず同じフォルダに配置してください。MSVCビルドは
Visual C++ Runtimeを静的リンクするため、追加のVC/UCRT DLLや再頒布可能
パッケージは不要です。

---

## 4. 配置

ポータブル版では`data/daemon/build/bin/`へ両exeを配置します。daemonは自身の
場所から上向きに`data/extensions/ao.monocon-tools-*`を探索し、完全なArduino
リソースを持つ最新バージョンを選ぶため、Arduino IDEやarduino-cliを別途導入せず
オフラインで動作します。

任意フォルダーへ両exeだけをコピーして使う場合は、従来どおり
`%LOCALAPPDATA%\Arduino15`のAVRツールチェーンとコアを利用します。セキュリティ
ソフトの除外設定は通常不要です。組織管理PCで性能問題が確認された場合だけ、
管理者の方針に従って最小範囲を検討してください。

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
			"type": "process",
			"command": "C:/Tools/ArduinoBuilder/arduino-build-cli.exe",
			"args": [
				"upload",
				"${fileDirname}",
				"${config:monoconTools.upload.port}",
				"--workspace",
				"${workspaceFolder}"
			],
			"problemMatcher": [],
			"group": { "kind": "build", "isDefault": true }
		},
		{
			"label": "Arduino: Build only",
			"type": "process",
			"command": "C:/Tools/ArduinoBuilder/arduino-build-cli.exe",
			"args": ["build", "${fileDirname}", "--workspace", "${workspaceFolder}"],
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

`%LOCALAPPDATA%\ArduinoBuildDaemon\daemon.log` にログが残ります。無操作終了は30分で、コンテスト中のツールチェーン・HEXメモリキャッシュを維持します。

前景実行 (デバッグ用):

```cmd
arduino-build-daemon.exe --no-daemonize
```

---

## 7. キャッシュ

- **core.a グローバルキャッシュ**: `%LOCALAPPDATA%\ArduinoBuildDaemon\core-cache\<key>\core.a`
	- 同梱`core.a`の内容ハッシュを含むキーとsidecarを検証
	- 欠落・破損時は安全に再生成または再配置
- **インクリメンタル**: `.ino/.cpp/.c/.S`、ヘッダー、標準ライブラリ、生成物を
	内容ハッシュで検証。更新日時が同じでも内容が変われば再ビルド。
- **保持上限**: 30日以上未使用、または64スケッチを超えた古いキャッシュを
	プロセス間ロック下で整理。削除・改名済みソースの中間物も回収。
- **HEXメモリキャッシュ**: 同一HEXの再解析を省略。
- **入力検証**: ソース、合計テキスト、HEX、メタデータに上限を設け、
	Intel HEXの長さ・チェックサム・アドレス重複・範囲を厳密に検証。
- **ブートローダー保護**: Mega 2560のアプリケーション領域
	（先頭253,952バイト）を超えるHEXを拒否し、末尾8 KiBへ書き込まない。
- **確実な書き込み**: Mega 2560公式ブートローダーの消去順序に合わせて、
	先頭から全ページを連続して書き込む。書き込み後は全ページを読み戻し、
	HEXイメージと1バイト単位で一致した場合だけ成功とする。
- **誤書き込み防止**: フラッシュ操作前に対象MCU署名がATmega2560
	 (`1E 98 01`)であることを確認。不一致時は書き込まず停止。
- **安全上の理由で使わないキャッシュ**: 書き込みページの省略と「同一HEXならボードへ
	接続しない」最適化は、実機の状態を保証できないため使用しない。
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
2. **同梱ライブラリ**: EEPROM、Ethernet、HID、LiquidCrystal、SD、Servo、
	 SoftwareSerial、SPI、Stepper、TFT、Wireに対応。
	 任意のサードパーティライブラリはまだ同梱していません。
3. **単一ジョブ**: daemonはコンパイル・書き込みを直列化します。同じキャッシュは
	 プロセス間Mutex、同じCOMポートは専用のプロセス間Mutexでも保護します。

---

## 9. トラブルシュート

| 症状 | 原因/対処 |
|---|---|
| `Toolchain not initialized` | ポータブル版では`data/extensions/ao.monocon-tools-*/resources/arduino`が完全か確認。単独配置では`%LOCALAPPDATA%\Arduino15`を確認 |
| `Resources: system` | 同梱リソースを発見できず、PC側Arduino環境を使っています。ポータブル版の配置を確認 |
| `No COM port detected` | USB ドライバ未インストール / ケーブル未接続。`arduino-build-cli ports` で確認 |
| 複数ポートで書き込み先不明 | `upload <SketchDir> COM3`のように明示。ポータブル版では`monoconTools.upload.port`を設定 |
| キャッシュ破損 | 通常は内容検証で自動修復。継続する場合は`shutdown`後にポータブル版の`data/cache/build`（単独配置ではexe隣接の`data/cache/build`）から対象`sketch_*`を退避 |
| daemon が応答しない / 大量にプロセスが残る | `arduino-build-cli kill` で全 daemon を強制終了。次回クライアント呼び出しで自動再起動 |

---

## 10. パフォーマンス目安 (HDD ノート PC 想定)

| 操作 | 旧 C++ 版 | 本 daemon 版 |
|---|---|---|
| 初回起動 | 3–5 秒 | 0.3–1 秒 (1 回だけ) |
| ping | 200–500 ms | 数十ms程度 |
| ports | 100–2000 ms (WMI) | 数ms程度 (registry) |
| 無変更ビルド | 1.5–3 秒 | 数十ms程度 |
| 1 ファイル変更 | 2–4 秒 | 内容とPC性能に依存 |
| フルビルド (同梱core.a) | 8–25 秒 | おおむね1秒未満～数秒 |
| 同一HEXの再アップロード | 約1.5秒 | 約2.1秒（全ページ書き込み + 全ページ検証、現行スケッチ実測） |
| 一部変更後の再アップロード | 約1.5秒 | HEXサイズに比例（全ページ書き込み + 全ページ検証） |
