# 常駐デーモン化の詳細実装ガイド (Arduino MEGA Builder)

既存の C++ 実装をベースに、**Windows ネイティブで名前付きパイプ IPC を使う常駐デーモン**へリファクタリングする手順を、コードレベルまで掘り下げて解説します。

---

## 1. 全体アーキテクチャ

```
┌──────────────────────┐
│  VS Code tasks.json  │
└──────────┬───────────┘
           │ exec
           ▼
┌──────────────────────────────────────┐
│  arduino-build-cli.exe (~50 KB)      │  ← クライアント (薄い)
│  - 引数を JSON にして送信             │
│  - daemon が無ければ起動              │
│  - stdout/exitcode をそのまま中継     │
└──────────┬───────────────────────────┘
           │ Named Pipe \\.\pipe\arduino-build-<SID>
           ▼
┌──────────────────────────────────────┐
│  arduino-build-daemon.exe (常駐)     │
│  ┌────────────────────────────────┐  │
│  │ Hot State                       │  │
│  │  - toolchain paths              │  │
│  │  - core.a global cache index    │  │
│  │  - COM port list (cached)       │  │
│  │  - source signature cache       │  │
│  └────────────────────────────────┘  │
│  ┌────────────────────────────────┐  │
│  │ Pipe Server (main thread)       │  │
│  │  - ConnectNamedPipe loop        │  │
│  │  - 1 接続 = 1 ジョブ            │  │
│  └────────────────────────────────┘  │
│  ┌────────────────────────────────┐  │
│  │ Worker Pool                     │  │
│  │  - avr-g++ / avr-gcc / avrdude  │  │
│  │  を CreateProcess で実行         │  │
│  └────────────────────────────────┘  │
│  ┌────────────────────────────────┐  │
│  │ Idle Watchdog                   │  │
│  │  - last_request_time を監視      │  │
│  │  - 30 分無通信で self-terminate  │  │
│  └────────────────────────────────┘  │
└──────────────────────────────────────┘
```

**重要な原則**:

1. **クライアントは「JSONを投げて結果を待つ」だけ**。ビルドロジックを一切持たない。
2. **デーモンはステートフル**。ツールチェーン解決・COM ポート列挙・ハッシュ計算結果をすべてメモリに保持する。
3. **名前付きパイプ名にユーザー SID を含める**。複数ユーザー環境で衝突しない。
4. **シングルインスタンス保証は名前付き Mutex で行う**。pipe の存在チェックでは race がある。

---

## 2. プロトコル設計

### 2.1 形式

LSP スタイル（`Content-Length` ヘッダ + JSON ボディ）が一番無難です。

```
Content-Length: 142\r\n
\r\n
{"jsonrpc":"2.0","id":1,"method":"build","params":{"sketchDir":"C:\\projects\\foo","port":"COM3"}}
```

LSP スタイルにしておくと、将来 VS Code 拡張化したときに `vscode-jsonrpc` がそのまま使えます。

### 2.2 RPC メソッド一覧

| method | params | result |
|---|---|---|
| `ping` | `{}` | `{"version":"1.0.0","uptime":1234}` |
| `compile` | `{"sketchDir":"...","fqbn":"...","forceFullBuild":false}` | `{"hexFile":"...","cached":true,"buildTimeMs":120}` |
| `upload` | `{"sketchDir":"...","port":"COM3","skipCompile":false}` | `{"port":"COM3","uploadTimeMs":3500}` |
| `listPorts` | `{}` | `{"ports":["COM3","COM5"]}` |
| `invalidateCache` | `{"sketchDir":"..."}` | `{}` |
| `shutdown` | `{}` | `{}` |

通知（id なし）でログストリーミングを送るのも便利です。

```json
{"jsonrpc":"2.0","method":"log","params":{"level":"info","message":"Compiling sketch.cpp"}}
```

---

## 3. クライアント実装 (C++)

### 3.1 メインフロー

```cpp
// arduino-build-cli.cpp  (50 KB 程度の薄い exe)
#include <windows.h>
#include <sddl.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// ユーザー SID を取得して pipe 名に組み込む（マルチユーザー隔離）
static std::string makePipeName() {
    HANDLE hToken = nullptr;
    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    DWORD len = 0;
    GetTokenInformation(hToken, TokenUser, nullptr, 0, &len);
    std::vector<BYTE> buf(len);
    GetTokenInformation(hToken, TokenUser, buf.data(), len, &len);
    auto* tu = reinterpret_cast<TOKEN_USER*>(buf.data());
    LPSTR sidStr = nullptr;
    ConvertSidToStringSidA(tu->User.Sid, &sidStr);
    std::string name = std::string("\\\\.\\pipe\\arduino-build-") + sidStr;
    LocalFree(sidStr);
    CloseHandle(hToken);
    return name;
}

// pipe に接続。失敗時は daemon を起動して再試行。
static HANDLE connectOrSpawn(const std::string& pipeName,
                             const std::string& daemonExe) {
    for (int attempt = 0; attempt < 50; ++attempt) {
        HANDLE h = CreateFileA(
            pipeName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            // メッセージモードに切替（オプション）
            DWORD mode = PIPE_READMODE_BYTE;
            SetNamedPipeHandleState(h, &mode, nullptr, nullptr);
            return h;
        }
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_BUSY) {
            WaitNamedPipeA(pipeName.c_str(), 500);
            continue;
        }
        // ERROR_FILE_NOT_FOUND: daemon が無い
        if (attempt == 0) {
            // daemon を起動（DETACHED で親から完全に切り離す）
            STARTUPINFOA si{ sizeof(si) };
            PROCESS_INFORMATION pi{};
            std::string cmd = "\"" + daemonExe + "\" --daemonize";
            BOOL ok = CreateProcessA(
                nullptr, cmd.data(), nullptr, nullptr, FALSE,
                DETACHED_PROCESS | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB,
                nullptr, nullptr, &si, &pi);
            if (!ok) return INVALID_HANDLE_VALUE;
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return INVALID_HANDLE_VALUE;
}

// LSP 風フレーミングで 1 リクエスト送信 → 結果を待つ
static std::string sendRequest(HANDLE hPipe, const std::string& jsonBody) {
    std::string header = "Content-Length: " + std::to_string(jsonBody.size()) + "\r\n\r\n";
    DWORD written = 0;
    WriteFile(hPipe, header.data(), (DWORD)header.size(), &written, nullptr);
    WriteFile(hPipe, jsonBody.data(), (DWORD)jsonBody.size(), &written, nullptr);

    // ヘッダ読み込み
    std::string buf;
    char ch = 0;
    while (true) {
        DWORD n = 0;
        if (!ReadFile(hPipe, &ch, 1, &n, nullptr) || n == 0) break;
        buf.push_back(ch);
        if (buf.size() >= 4 && buf.substr(buf.size()-4) == "\r\n\r\n") break;
    }
    // Content-Length をパース
    size_t pos = buf.find("Content-Length:");
    size_t len = std::stoul(buf.substr(pos + 15));
    std::string body(len, '\0');
    DWORD totalRead = 0;
    while (totalRead < len) {
        DWORD n = 0;
        if (!ReadFile(hPipe, body.data() + totalRead, (DWORD)(len - totalRead), &n, nullptr)) break;
        totalRead += n;
    }
    return body;
}

int main(int argc, char* argv[]) {
    auto pipeName = makePipeName();
    std::string daemonPath = /* デーモン exe のフルパス */ "C:\\Tools\\arduino-build-daemon.exe";

    HANDLE hPipe = connectOrSpawn(pipeName, daemonPath);
    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << ">>> Failed to connect to daemon\n";
        return 1;
    }

    // 引数を JSON 化（実プロジェクトでは nlohmann/json を使うと楽）
    std::string req = R"({"jsonrpc":"2.0","id":1,"method":"upload","params":{...}})";
    std::string resp = sendRequest(hPipe, req);

    std::cout << resp << "\n";
    CloseHandle(hPipe);
    return 0;
}
```

**ポイント**:

- **`DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB`** で起動すると、VS Code が tasks.json の子プロセスを Job Object で kill しても daemon が巻き込まれません。`CREATE_NEW_PROCESS_GROUP` を足すのも有効。
- **`WaitNamedPipe`** は他クライアントが先客で繋いでいる時にブロックするための API。`CreateFile` の単純リトライより筋が良い。
- **クライアントは絶対にビルド処理を持たない**。フォールバックロジック（「daemon が起動できなかったら自分でビルドする」）を入れたくなりますが、それをやると 2 系統メンテになるので、daemon 起動失敗は単純エラーで返すのが正解。

---

## 4. デーモン側の骨格 (C++)

### 4.1 シングルインスタンス保証

```cpp
// daemon main の最初で
HANDLE hMutex = CreateMutexA(nullptr, FALSE, "Local\\ArduinoBuildDaemon-Mutex");
if (GetLastError() == ERROR_ALREADY_EXISTS) {
    // 別インスタンスが既に動いている
    return 0;
}
```

`Local\` プレフィックスでセッションローカルになるので、リモートデスクトップ複数セッションでも安全です。

### 4.2 名前付きパイプサーバ（シリアル処理版）

最初はシリアル処理（1 接続ずつ）で十分です。Arduino のビルドはそんなに頻繁に並列実行されません。

```cpp
// daemon.cpp
static const std::string PIPE_NAME = makePipeName();  // クライアントと同じ式

void runPipeServer() {
    while (!g_shutdownRequested) {
        HANDLE hPipe = CreateNamedPipeA(
            PIPE_NAME.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT |
                PIPE_REJECT_REMOTE_CLIENTS,   // ← セキュリティ的に重要
            PIPE_UNLIMITED_INSTANCES,
            64 * 1024,    // out buffer
            64 * 1024,    // in buffer
            0,            // default timeout
            nullptr);     // 後述の SECURITY_ATTRIBUTES を渡す

        if (hPipe == INVALID_HANDLE_VALUE) {
            Sleep(100);
            continue;
        }

        // クライアントの接続を待機
        BOOL connected = ConnectNamedPipe(hPipe, nullptr) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected) {
            CloseHandle(hPipe);
            continue;
        }

        // 1 接続を 1 リクエストとして処理
        handleRequest(hPipe);

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);

        g_lastRequestTime = std::chrono::steady_clock::now();
    }
}
```

### 4.3 セキュリティ記述子（同一ユーザーのみ ACCESS 許可）

`PIPE_REJECT_REMOTE_CLIENTS` だけでは不十分で、本番では明示的に DACL を絞ります。

```cpp
// 現在ユーザーのみが Read/Write できる SECURITY_DESCRIPTOR を作る
// SDDL: "D:(A;;GA;;;OW)" = Owner にのみ GENERIC_ALL
PSECURITY_DESCRIPTOR pSD = nullptr;
ConvertStringSecurityDescriptorToSecurityDescriptorA(
    "D:(A;;GA;;;OW)", SDDL_REVISION_1, &pSD, nullptr);

SECURITY_ATTRIBUTES sa{};
sa.nLength = sizeof(sa);
sa.lpSecurityDescriptor = pSD;
sa.bInheritHandle = FALSE;
// CreateNamedPipeA の最後の引数に &sa を渡す
```

これで他ユーザー（同一マシンの別アカウント）からのアクセスを遮断できます。

### 4.4 リクエスト処理ループ

```cpp
struct DaemonState {
    // Hot caches
    std::string avrGppPath, avrGccPath, avrObjcopyPath;
    std::string avrdudePath, avrdudeConf;
    std::string coreCacheRoot;       // %LOCALAPPDATA%\YourTool\core-cache
    std::vector<std::string> cachedComPorts;
    std::chrono::steady_clock::time_point comPortsCachedAt;

    // Per-sketch incremental state
    struct SketchState {
        std::string lastSignature;       // (size,mtime,xxh3)
        std::string hexFile;
        std::chrono::steady_clock::time_point lastBuildAt;
    };
    std::unordered_map<std::string, SketchState> sketches;

    std::mutex mtx;  // 単純化のため全体ロック (シリアル処理なら不要)
};

static DaemonState g_state;

void handleRequest(HANDLE hPipe) {
    std::string body = readLspMessage(hPipe);
    auto req = json::parse(body);

    json resp = { {"jsonrpc","2.0"}, {"id", req["id"]} };
    try {
        std::string method = req["method"];
        if (method == "ping")           resp["result"] = handlePing();
        else if (method == "compile")   resp["result"] = handleCompile(req["params"]);
        else if (method == "upload")    resp["result"] = handleUpload(req["params"]);
        else if (method == "listPorts") resp["result"] = handleListPorts();
        else if (method == "shutdown") {
            resp["result"] = json::object();
            g_shutdownRequested = true;
        }
        else throw std::runtime_error("unknown method: " + method);
    } catch (std::exception& e) {
        resp["error"] = { {"code", -32603}, {"message", e.what()} };
    }

    writeLspMessage(hPipe, resp.dump());
}
```

### 4.5 ホットキャッシュの活用

既存 `Uploader::resolveToolchainPaths()` 等を **デーモン起動時に 1 回だけ実行**して `g_state` に保存します。

```cpp
void initializeDaemon() {
    // ツールチェーン解決を起動時に 1 回だけ
    g_state.avrGppPath = resolveAvrGpp();
    g_state.avrGccPath = resolveAvrGcc();
    g_state.avrObjcopyPath = resolveAvrObjcopy();
    auto avrdude = resolveAvrdudePaths();
    g_state.avrdudePath = avrdude.first;
    g_state.avrdudeConf = avrdude.second;

    g_state.coreCacheRoot =
        getLocalAppData() + "\\ArduinoBuilderDaemon\\core-cache";
    createDirectoryRecursive(g_state.coreCacheRoot);

    // COM ポートを最初に 1 回スキャン
    refreshComPortsFromRegistry();  // WMI ではなくレジストリ直読
}
```

`handleListPorts()` はキャッシュが新しければ即返す:

```cpp
json handleListPorts() {
    auto now = std::chrono::steady_clock::now();
    if (now - g_state.comPortsCachedAt > std::chrono::seconds(5)) {
        refreshComPortsFromRegistry();
        g_state.comPortsCachedAt = now;
    }
    return { {"ports", g_state.cachedComPorts} };
}
```

5 秒以上前のキャッシュなら更新、それ以下ならメモリ即返し。これでフィットネスの高い応答（マイクロ秒）と、新規挿抜への追従を両立できます。

### 4.6 デバイス変化通知（さらに最適化）

「USB 抜き差しでだけポート再列挙」にしたい場合は別スレッドで `RegNotifyChangeKeyValue` を張る:

```cpp
void watchSerialCommRegistry() {
    HKEY hKey;
    RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_NOTIFY | KEY_READ, &hKey);

    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    while (!g_shutdownRequested) {
        ResetEvent(hEvent);
        RegNotifyChangeKeyValue(hKey, FALSE,
            REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        WaitForSingleObject(hEvent, INFINITE);
        refreshComPortsFromRegistry();
    }
    CloseHandle(hEvent);
    RegCloseKey(hKey);
}
```

これで `handleListPorts` は常にゼロレイテンシ。

### 4.7 アイドルタイムアウト（自動終了）

```cpp
void idleWatchdog() {
    using namespace std::chrono;
    constexpr auto kIdleTimeout = minutes(30);
    while (!g_shutdownRequested) {
        std::this_thread::sleep_for(seconds(60));
        auto idle = steady_clock::now() - g_state.lastRequestTime;
        if (idle > kIdleTimeout) {
            g_shutdownRequested = true;
            // pipe サーバスレッドを起こすため、自分で 1 回 connect する
            HANDLE h = CreateFileA(PIPE_NAME.c_str(), GENERIC_READ,
                0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
        }
    }
}
```

`ConnectNamedPipe` でブロック中のメインループを起こすために「自分で 1 回 connect する」のがコツ。

---

## 5. ライフサイクル全体の流れ

```
[ユーザーが VS Code でビルドボタンを押す]
        │
        ▼
[tasks.json が arduino-build-cli.exe を起動]
        │
        ▼
[CLI が pipe に CreateFile]
        │
        ├── 成功 ──→ JSON 投げる ──→ 結果受信 ──→ exit
        │
        └── ERROR_FILE_NOT_FOUND
              │
              ▼
        [CLI が daemon を DETACHED で起動]
              │
              ▼
        [50 ms x 50 = 2.5 秒以内に再接続を試みる]
              │
              ▼
        [接続成功 ──→ JSON 投げる ──→ 結果受信 ──→ exit]

[daemon 側]
        │
        起動時: ツールチェーン解決、COM 列挙、core-cache 索引
        │
        ▼
        [pipe loop で待機]
              │
              ▼
        [接続来る → handleRequest → DisconnectNamedPipe]
              │
              ▼
        [30 分無通信 → self-terminate]
```

---

## 6. 「core.a グローバルキャッシュ」との結合

デーモン化の真価は、core.a キャッシュと組み合わさったときに出ます。

### キャッシュキーの設計

```cpp
std::string buildCoreCacheKey(const BuildConfig& cfg) {
    // すべての入力をハッシュに混ぜる
    std::string raw =
        cfg.fqbn + "|" +
        cfg.mcu + "|" +
        cfg.fcpu + "|" +
        cfg.compilerVersion + "|" +     // avr-gcc --version の出力
        cfg.coreDirHash + "|" +         // cores/ 配下のツリーハッシュ
        joinAll(cfg.extraDefines, ";"); // -D... を全部
    return sha256(raw);  // hex 文字列
}

std::string coreArchivePath(const std::string& key) {
    return g_state.coreCacheRoot + "\\" + key + "\\core.a";
}
```

### compile RPC の本体

```cpp
json handleCompile(const json& params) {
    std::string sketchDir = params["sketchDir"];
    BuildConfig cfg = resolveConfig(sketchDir);

    // 1. core.a があるか?
    std::string coreKey = buildCoreCacheKey(cfg);
    std::string coreA = coreArchivePath(coreKey);
    if (!fileExists(coreA)) {
        // 初回のみ arduino-cli で core.a 生成
        generateCoreArchive(cfg, coreA);
    }

    // 2. インクリメンタルチェック
    auto& s = g_state.sketches[sketchDir];
    std::string sig = computeSourceSignature(sketchDir);
    if (sig == s.lastSignature && fileExists(s.hexFile)) {
        return { {"hexFile", s.hexFile}, {"cached", true}, {"buildTimeMs", 0} };
    }

    // 3. avr-g++ → avr-gcc → avr-objcopy を直接呼ぶ
    auto t0 = now();
    runAvrGpp(...);
    runAvrGcc(...);   // core.a を -L で参照
    runAvrObjcopy(...);

    s.lastSignature = sig;
    s.hexFile = /* ... */;
    s.lastBuildAt = now();

    return { {"hexFile", s.hexFile}, {"cached", false},
             {"buildTimeMs", elapsedMs(t0)} };
}
```

これで「同じ fqbn + コンパイラバージョンを使う全プロジェクト」で core.a が共有され、フルビルドはツールチェーン更新時しか走らなくなります。

---

## 7. 落とし穴と対策

### 7.1 デーモンの異常終了

`avr-g++` が segfault したり、デーモン本体がクラッシュしても、次回 CLI がアクセスしたときに「pipe が無い → 起動」で自動回復します。**state を全部メモリに置いて永続化していない**から、クラッシュ後の整合性も心配ない（最悪、初回ビルドが 1 回遅くなるだけ）。

### 7.2 ハング検出

無限ループバグなどで daemon が固まると、CLI 側で `WaitNamedPipe` または `ReadFile` がハングします。CLI 側で **タイムアウト付き OVERLAPPED I/O** を実装するのが正しい:

```cpp
// 30 秒で諦める
OVERLAPPED ov{};
ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
ReadFile(hPipe, buf, len, nullptr, &ov);
DWORD r = WaitForSingleObject(ov.hEvent, 30000);
if (r == WAIT_TIMEOUT) {
    CancelIoEx(hPipe, &ov);
    // エラー表示 + 「daemon が応答しません」
}
```

タイムアウトしたら CLI 側で daemon を kill して再起動するロジックも欲しい:

```cpp
// pipe 名から daemon の PID を取得（GetNamedPipeServerProcessId）
ULONG pid = 0;
GetNamedPipeServerProcessId(hPipe, &pid);
HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
TerminateProcess(hProc, 1);
CloseHandle(hProc);
// その後再起動
```

### 7.3 マルチクライアント並行

デフォルトはシリアル処理（同時 1 ジョブ）ですが、もし将来「ライブラリインストールしながら別スケッチをビルド」したくなったら、`PIPE_UNLIMITED_INSTANCES` で複数 pipe インスタンスを作り、各接続をスレッドプールに渡す形に拡張します。
そのときは `g_state.sketches` への書き込みに `std::mutex` を必須に。

### 7.4 デーモン更新時の問題

ユーザーが daemon exe を新バージョンに置き換えたとき、古い daemon が常駐していると古いコードで動き続けます。CLI 側で **バージョン整合性チェック**を入れます:

```cpp
auto resp = sendRequest(hPipe, R"({"method":"ping"})");
auto j = json::parse(resp);
std::string daemonVersion = j["result"]["version"];
if (daemonVersion != CLIENT_VERSION) {
    // shutdown を投げて再起動
    sendRequest(hPipe, R"({"method":"shutdown"})");
    CloseHandle(hPipe);
    Sleep(500);
    hPipe = connectOrSpawn(...);
}
```

### 7.5 デバッグの取り回し

デーモンの stdout は見えないので、ロギングは必須です。

```cpp
// %LOCALAPPDATA%\ArduinoBuilderDaemon\daemon.log にローテーションログ
```

`--no-daemonize` フラグで前景実行できるオプションを残しておくと、開発中の調査が楽です。

```bash
arduino-build-daemon.exe --no-daemonize  # コンソールで stdout 出力
```

### 7.6 Defender 除外との相互作用

デーモン本体（`arduino-build-daemon.exe`）と CLI（`arduino-build-cli.exe`）は **必ず Defender の ExclusionProcess に追加**してください。これらが除外されていれば、daemon が CreateProcess で呼ぶ avr-gcc 等の子プロセスのスキャンも軽量パスになる傾向があります（信頼プロセスから派生したプロセスとして扱われる）。

---

## 8. 段階的な移行ロードマップ

既存 C++ コードからこの構成に持っていく段取り:

### フェーズ 1（最小 IPC、1〜2 日）
- `main.cpp` を 2 つに分割（CLI と daemon）。
- 既存の `Uploader::initialize()` を daemon 起動時に 1 回だけ実行。
- 既存の `Uploader::upload()` を `handleUpload()` の中身にする。
- LSP フレーミング + nlohmann/json で IPC 完成。
- これだけで「2 回目以降のビルドが 1 秒程度速くなる」効果が出る。

### フェーズ 2（ホットキャッシュ、3〜5 日）
- COM ポートのレジストリ常時監視。
- core.a のグローバルキャッシュ実装（`%LOCALAPPDATA%\...\core-cache\<key>\core.a`）。
- 既存の `incrementalCompile` を daemon 内のメソッドに移植。

### フェーズ 3（堅牢化、3〜7 日）
- アイドルタイムアウト + 自動再起動。
- バージョン不整合検出。
- タイムアウト付き OVERLAPPED I/O 化。
- Defender 除外案内ダイアログ。

### フェーズ 4（オプション）
- arduino-cli daemon を子に持つ方式へ移行（gRPC ラッパ化）。
- VS Code 拡張化（LSP 互換にしておけば既存の vscode-jsonrpc が再利用可能）。

---

## 9. 計測ポイント

実装後は必ず計測します。

```cpp
// daemon 内で各フェーズの時間を JSON 結果に含める
{
  "result": {
    "hexFile": "...",
    "cached": false,
    "timing": {
      "ipcOverheadMs": 3,
      "sigCheckMs": 8,
      "compileMs": 280,
      "linkMs": 95,
      "objcopyMs": 35,
      "uploadMs": 3450
    }
  }
}
```

これでクライアントは内訳を表示でき、ボトルネックがどこにあるか一目で分かります。デスクトップとノートで内訳を比較すると、残った差分（コンパイル本体だけのはず）が見えるので、そこで戦略 ⑥ PCH や戦略 ⑦ ccache を追加投入するか判断できます。

---

## まとめ

「常駐デーモン化」の実装は次の 6 要素で構成されます。

1. **薄いクライアント exe** が pipe に接続。無ければ DETACHED で daemon を起動して再接続。
2. **名前付きパイプ + LSP フレーミング**で JSON-RPC を運ぶ。SDDL でユーザー隔離。
3. **シングルインスタンス**は名前付き Mutex で。
4. **ホット状態**（ツールチェーン、COM、core.a 索引、ソース署名）をデーモンメモリに保持。
5. **アイドルタイムアウト**で自動終了、次回 CLI が来たら自動再起動。
6. **Defender 除外**と組み合わせて、子プロセス起動コストも最小化。

実装難易度は最初の MVP までで C++ 経験者なら 1〜2 週間、そこからフェーズ 2 + 3 を 1〜2 週間、合計 3〜4 週間で「ノート PC でもデスクトップとほぼ同じ体感」になります。
