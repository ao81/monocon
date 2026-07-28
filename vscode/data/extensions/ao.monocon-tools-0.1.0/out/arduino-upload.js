"use strict";

const path = require("node:path");
const fs = require("node:fs");
const vscode = require("vscode");
const {
    NativeUnavailableError,
    NativeOperationTimeoutError
} = require("./native-service");
const { createCompilerReport } = require("./compiler-report");

const UPLOAD_COMMAND = "monoconTools.uploadArduino";
const BUILD_COMMAND = "monoconTools.compileArduino";
const LEGACY_UPLOAD_COMMAND = "monocon.upload";
const UPLOAD_TASK_NAME = "Arduino: Upload";

let activeUpload;
let activeBuild;
let serialMonitorApiPromise;
let uploadTaskPromise;
let outputChannel;
let compilerDiagnostics;

function containsIno(directory) {
    try {
        return fs.readdirSync(directory, { withFileTypes: true })
            .some(entry => entry.isFile() && entry.name.toLowerCase().endsWith(".ino"));
    }
    catch {
        return false;
    }
}

function findSketchDirectory(filePath, workspacePath) {
    let directory = path.dirname(filePath);
    const workspaceRoot = workspacePath ? path.resolve(workspacePath) : undefined;
    while (directory) {
        if (containsIno(directory)) return directory;
        if (workspaceRoot && path.resolve(directory) === workspaceRoot) break;
        const parent = path.dirname(directory);
        if (parent === directory) break;
        if (workspaceRoot) {
            const relative = path.relative(workspaceRoot, parent);
            if (relative.startsWith("..") || path.isAbsolute(relative)) break;
        }
        directory = parent;
    }
    return undefined;
}

function resolveSketchContext() {
    const editorUri = vscode.window.activeTextEditor?.document?.uri;
    if (editorUri?.scheme === "file" && editorUri.fsPath) {
        const folder = vscode.workspace.getWorkspaceFolder?.(editorUri);
        const workspaceDir = folder?.uri?.fsPath || path.dirname(editorUri.fsPath);
        const sketchDir = editorUri.fsPath.toLowerCase().endsWith(".ino")
            ? path.dirname(editorUri.fsPath)
            : findSketchDirectory(editorUri.fsPath, workspaceDir);
        if (!sketchDir) return undefined;
        return {
            sketchDir,
            workspaceDir
        };
    }
    const folder = vscode.workspace.workspaceFolders?.[0];
    if (folder?.uri?.fsPath && containsIno(folder.uri.fsPath)) {
        return {
            sketchDir: folder.uri.fsPath,
            workspaceDir: folder.uri.fsPath
        };
    }
    return undefined;
}

function formatCompileLabel(compile) {
    if (compile?.cached) return "(cached)";
    const recompiled = compile?.recompiledFiles || 0;
    const total = compile?.totalFiles || 0;
    return recompiled === total && total > 0
        ? "(full)"
        : `(diff ${recompiled}/${total} files)`;
}

function reportNativeResult(result, totalMs) {
    const compile = result.compile || {};
    outputChannel?.appendLine(
        `Compile ${formatCompileLabel(compile)} in ${compile.buildTimeMs || 0} ms`
    );
    outputChannel?.appendLine(
        `Upload to ${result.port || ""} in ${result.uploadTimeMs || 0} ms`
    );
    outputChannel?.appendLine(`Total client time: ${totalMs.toFixed(4)} ms`);
    if (result.avrdudeOutput) {
        log(result.avrdudeOutput.trimEnd());
    }
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function withTimeout(promise, timeoutMs, timeoutMessage) {
    return new Promise((resolve, reject) => {
        const timer = setTimeout(() => reject(new Error(timeoutMessage)), timeoutMs);
        Promise.resolve(promise).then(
            value => {
                clearTimeout(timer);
                resolve(value);
            },
            error => {
                clearTimeout(timer);
                reject(error);
            }
        );
    });
}

function log(message) {
    outputChannel?.appendLine(`[${new Date().toLocaleTimeString()}] ${message}`);
}

function diagnosticSeverity(severity) {
    if (severity === "error") return vscode.DiagnosticSeverity?.Error ?? 0;
    if (severity === "warning") return vscode.DiagnosticSeverity?.Warning ?? 1;
    return vscode.DiagnosticSeverity?.Information ?? 2;
}

function publishCompilerDiagnostics(report) {
    if (!compilerDiagnostics || !vscode.Uri?.file
        || typeof vscode.Range !== "function"
        || typeof vscode.Diagnostic !== "function") {
        return;
    }
    compilerDiagnostics.clear();
    const byFile = new Map();
    for (const item of report.diagnostics) {
        const line = Math.max(0, item.line - 1);
        const column = Math.max(0, item.column - 1);
        const range = new vscode.Range(line, column, line, column + 1);
        const diagnostic = new vscode.Diagnostic(
            range,
            item.message,
            diagnosticSeverity(item.severity)
        );
        diagnostic.source = "Monocon Tools";
        diagnostic.code = "arduino-compile";
        const fileDiagnostics = byFile.get(item.absolutePath) || [];
        fileDiagnostics.push(diagnostic);
        byFile.set(item.absolutePath, fileDiagnostics);
    }
    for (const [filePath, diagnostics] of byFile) {
        compilerDiagnostics.set(vscode.Uri.file(filePath), diagnostics);
    }
}

function createCompileFailureError(compileResult, sketch) {
    const report = createCompilerReport(
        compileResult?.compilerOutput || compileResult?.errorMessage || "",
        sketch?.sketchDir
    );
    publishCompilerDiagnostics(report);
    outputChannel?.appendLine(report.text);
    outputChannel?.show?.(true);
    const error = new Error(report.summary);
    error.isCompilerFailure = true;
    error.detailsAlreadyReported = true;
    return error;
}

function showOperationFailure(operationName, error) {
    const message = error instanceof Error ? error.message : String(error);
    if (!error?.detailsAlreadyReported) {
        log(`${operationName}に失敗しました: ${message}`);
    }
    const prefix = error?.isCompilerFailure ? "コンパイルエラー" : `${operationName}失敗`;
    const action = error?.isCompilerFailure ? "問題を表示" : undefined;
    const notification = action
        ? vscode.window.showErrorMessage(`${prefix}: ${message}`, action)
        : vscode.window.showErrorMessage(`${prefix}: ${message}`);
    if (action && notification?.then) {
        notification.then(selection => {
            if (selection === action) {
                vscode.commands.executeCommand("workbench.actions.view.problems");
            }
        });
    }
    return message;
}

async function getSerialMonitorApi(context) {
    if (!serialMonitorApiPromise) {
        serialMonitorApiPromise = (async () => {
            const extension = vscode.extensions.getExtension("ms-vscode.vscode-serial-monitor");
            if (!extension) {
                return undefined;
            }
            const exports = extension.isActive ? extension.exports : await extension.activate();
            if (!exports || typeof exports.getApi !== "function") {
                return undefined;
            }
            const api = exports.getApi(0, context);
            if (api && typeof api.dispose === "function") {
                context.subscriptions.push(api);
            }
            return api;
        })().catch(error => {
            serialMonitorApiPromise = undefined;
            throw error;
        });
    }
    return serialMonitorApiPromise;
}

function getPortName(port) {
    if (typeof port === "string") {
        return port;
    }
    if (!port || typeof port !== "object") {
        return undefined;
    }
    return port.portName || port.port || port.path;
}

async function stopActiveSerialMonitors(api, operationTimeoutMs) {
    if (!api) {
        return [];
    }

    const stopped = [];
    let ports;
    try {
        ports = await withTimeout(
            api.listAvailablePorts(),
            operationTimeoutMs,
            "シリアルポート一覧の取得がタイムアウトしました。"
        );
    }
    catch (error) {
        log(`シリアルポート一覧を取得できませんでした: ${error instanceof Error ? error.message : String(error)}`);
        vscode.window.showWarningMessage("シリアルモニターの状態を確認できませんでした。停止せずに書き込みを続行します。");
        return stopped;
    }

    for (const port of ports) {
        const portName = getPortName(port);
        if (!portName) {
            continue;
        }
        try {
            if (await withTimeout(
                api.stopMonitoringPort(portName),
                operationTimeoutMs,
                `${portName} の停止がタイムアウトしました。`
            )) {
                stopped.push(portName);
                log(`シリアルモニターを停止: ${portName}`);
            }
        }
        catch (error) {
            log(`${portName} のシリアルモニターを停止できませんでした: ${error instanceof Error ? error.message : String(error)}`);
        }
    }
    return stopped;
}

async function findUploadTask() {
    if (!uploadTaskPromise) {
        uploadTaskPromise = vscode.tasks.fetchTasks().then(tasks => {
            const task = tasks.find(candidate => candidate.name === UPLOAD_TASK_NAME);
            if (!task) {
                uploadTaskPromise = undefined;
            }
            return task;
        }, error => {
            uploadTaskPromise = undefined;
            throw error;
        });
    }
    return uploadTaskPromise;
}

async function executeTaskAndWait(task, timeoutMs) {
    const alreadyRunning = vscode.tasks.taskExecutions.some(execution => execution.task.name === task.name);
    if (alreadyRunning) {
        throw new Error("Arduinoの書き込みタスクが実際に実行中です。完了後にもう一度お試しください。");
    }

    let execution;
    let exitCode;
    let endedBeforeAssignment;
    let endedProcessBeforeAssignment;
    let timeout;
    let endGraceTimer;
    let settled = false;
    let resolveCompletion;
    let rejectCompletion;
    const completion = new Promise((resolve, reject) => {
        resolveCompletion = resolve;
        rejectCompletion = reject;
    });
    const cleanup = () => {
        clearTimeout(timeout);
        clearTimeout(endGraceTimer);
        processDisposable.dispose();
        taskDisposable.dispose();
    };
    const finish = (error, code) => {
        if (settled) {
            return;
        }
        settled = true;
        cleanup();
        if (error) {
            rejectCompletion(error);
        }
        else {
            resolveCompletion(code);
        }
    };
    const isCandidate = eventExecution => execution
        ? eventExecution === execution
        : eventExecution.task.name === task.name;
    const processDisposable = vscode.tasks.onDidEndTaskProcess(event => {
        if (!isCandidate(event.execution)) {
            return;
        }
        if (!execution) {
            endedProcessBeforeAssignment = event.execution;
        }
        exitCode = event.exitCode;
        if (endedBeforeAssignment === event.execution || execution === event.execution) {
            finish(undefined, exitCode);
        }
    });
    const taskDisposable = vscode.tasks.onDidEndTask(event => {
        if (!isCandidate(event.execution)) {
            return;
        }
        if (!execution) {
            endedBeforeAssignment = event.execution;
            return;
        }
        endGraceTimer = setTimeout(() => finish(undefined, exitCode), 100);
    });

    try {
        execution = await vscode.tasks.executeTask(task);
        log(`タスクを開始: ${task.name}`);
        if (endedProcessBeforeAssignment === execution) {
            finish(undefined, exitCode);
        }
        else if (endedBeforeAssignment === execution) {
            endGraceTimer = setTimeout(() => finish(undefined, exitCode), 100);
        }
        if (!settled) {
            timeout = setTimeout(() => {
                execution.terminate();
                finish(new Error(`書き込みが ${Math.round(timeoutMs / 1000)} 秒以内に完了しなかったため中止しました。`));
            }, timeoutMs);
        }
    }
    catch (error) {
        finish(error);
    }

    return completion;
}

async function reopenSerialMonitors(api, ports, baudRate, delayMs, operationTimeoutMs) {
    if (!api || ports.length === 0) {
        return;
    }
    await sleep(delayMs);
    for (const port of ports) {
        try {
            await withTimeout(api.startMonitoringPort({
                port,
                baudRate,
                lineEnding: "none",
                dataBits: 8,
                stopBits: "one",
                parity: "none",
                dtr: false,
                rts: false
            }), operationTimeoutMs, `${port} の再開がタイムアウトしました。`);
            log(`シリアルモニターを再開: ${port} (${baudRate} baud)`);
        }
        catch (error) {
            const message = error instanceof Error ? error.message : String(error);
            log(`シリアルモニターを再開できませんでした (${port}): ${message}`);
            vscode.window.showWarningMessage(`シリアルモニターを再開できませんでした (${port}): ${message}`);
        }
    }
}

async function runUpload(context, uploadStatus, nativeService) {
    let api;
    let stoppedPorts = [];
    let allowMonitorReopen = true;
    const config = vscode.workspace.getConfiguration("monoconTools.upload");
    const baudRate = config.get("baudRate", 9600);
    const reopenDelayMs = config.get("reopenDelayMs", 0);
    const portReleaseDelayMs = config.get("portReleaseDelayMs", 0);
    const taskTimeoutMs = config.get("taskTimeoutMs", 180000);
    const operationTimeoutMs = config.get("serialOperationTimeoutMs", 5000);
    const reopenMonitor = config.get("reopenMonitor", true);

    return vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: "Arduinoへ書き込み中",
        cancellable: false
    }, async progress => {
        try {
            progress.report({ message: "ファイルを保存しています…" });
            if (!await vscode.workspace.saveAll(false)) {
                throw new Error("ファイルを保存できませんでした。");
            }

            progress.report({ message: "シリアルモニターを停止しています…" });
            try {
                api = await withTimeout(
                    getSerialMonitorApi(context),
                    operationTimeoutMs,
                    "シリアルモニター拡張の起動がタイムアウトしました。"
                );
                stoppedPorts = await stopActiveSerialMonitors(api, operationTimeoutMs);
            }
            catch (error) {
                log(`シリアルモニター連携を開始できませんでした: ${error instanceof Error ? error.message : String(error)}`);
                vscode.window.showWarningMessage("シリアルモニターと連携できませんでした。停止せずに書き込みを続行します。");
            }
            if (stoppedPorts.length > 0) {
                await sleep(portReleaseDelayMs);
            }

            progress.report({ message: "コンパイル・書き込みを実行しています…" });
            const sketch = resolveSketchContext();
            if (!sketch) {
                throw new Error("Arduinoスケッチを特定できません。.inoファイルを開いてから実行してください。");
            }
            compilerDiagnostics?.clear();
            let nativeCompleted = false;
            if (nativeService) {
                const started = performance.now();
                try {
                    uploadStatus?.start();
                    const result = await nativeService.request(
                        "upload",
                        sketch,
                        taskTimeoutMs
                    );
                    if (!result.success) {
                        if (result.compile && !result.compile.success
                            && (result.compile.compilerOutput || result.compile.errorMessage)) {
                            throw createCompileFailureError(result.compile, sketch);
                        }
                        const details = result.compile?.compilerOutput
                            || result.avrdudeOutput
                            || "";
                        throw new Error(
                            `${result.errorMessage || "ネイティブ書き込みに失敗しました。"}`
                            + (details ? `\n${details}` : "")
                        );
                    }
                    reportNativeResult(result, performance.now() - started);
                    uploadStatus?.succeed();
                    nativeCompleted = true;
                    log("ネイティブエンジンでArduinoへの書き込みが完了しました。");
                }
                catch (error) {
                    if (!(error instanceof NativeUnavailableError)) {
                        throw error;
                    }
                    log(`ネイティブエンジンを利用できないためCLIへ切り替えます: ${error.message}`);
                }
            }
            if (!nativeCompleted) {
                const task = await findUploadTask();
                if (!task) {
                    throw new Error(`${UPLOAD_TASK_NAME} タスクが見つかりません。`);
                }
                const exitCode = await executeTaskAndWait(task, taskTimeoutMs);
                if (exitCode !== 0) {
                    const code = exitCode === undefined ? "不明" : exitCode;
                    throw new Error(`書き込みタスクが終了コード ${code} で失敗しました。`);
                }
                log("CLI互換エンジンでArduinoへの書き込みが完了しました。");
            }
        }
        catch (error) {
            const message = error instanceof Error ? error.message : String(error);
            if (error instanceof NativeOperationTimeoutError) {
                // C++同期処理はタイムアウト通知後も終了処理中の可能性がある。
                // 同じCOMポートを開き直して書き込みを妨害しない。
                allowMonitorReopen = false;
                log("安全のためシリアルモニターを自動再開しません。ネイティブ処理の完了後に手動で再開してください。");
            }
            uploadStatus?.fail(message);
            showOperationFailure("Arduinoへの書き込み", error);
        }
        finally {
            if (reopenMonitor && allowMonitorReopen && stoppedPorts.length > 0) {
                progress.report({ message: "シリアルモニターを再開しています…" });
                await reopenSerialMonitors(api, stoppedPorts, baudRate, reopenDelayMs, operationTimeoutMs);
            }
        }
    });
}

function registerArduinoUploadCommands(context, uploadStatus, nativeService) {
    outputChannel = vscode.window.createOutputChannel("Monocon Tools");
    context.subscriptions.push(outputChannel);
    if (vscode.languages?.createDiagnosticCollection) {
        compilerDiagnostics = vscode.languages.createDiagnosticCollection("monoconTools");
        context.subscriptions.push(compilerDiagnostics);
    }

    const handler = async () => {
        if (activeUpload || activeBuild) {
            vscode.window.showInformationMessage("コンパイルまたはアップロードは既に実行中です。");
            return;
        }
        outputChannel?.clear();
        compilerDiagnostics?.clear();
        const thisUpload = runUpload(context, uploadStatus, nativeService);
        activeUpload = thisUpload;
        try {
            await thisUpload;
        }
        finally {
            if (activeUpload === thisUpload) {
                activeUpload = undefined;
            }
        }
    };

    const buildHandler = async () => {
        if (activeBuild || activeUpload) {
            vscode.window.showInformationMessage("コンパイルまたはアップロードは既に実行中です。");
            return;
        }
        outputChannel?.clear();
        compilerDiagnostics?.clear();
        const operation = (async () => {
            try {
                if (!await vscode.workspace.saveAll(false)) {
                    throw new Error("ファイルを保存できませんでした。");
                }
                const sketch = resolveSketchContext();
                if (!sketch) {
                    throw new Error("コンパイルするArduinoスケッチを特定できません。");
                }
                compilerDiagnostics?.clear();
                const result = await nativeService.request("compile", sketch, 180000);
                if (!result.success) {
                    throw createCompileFailureError(result, sketch);
                }
                outputChannel?.appendLine(
                    `Compile ${formatCompileLabel(result)} in ${result.buildTimeMs || 0} ms`
                );
                vscode.window.showInformationMessage("Arduinoのコンパイルが完了しました。");
            }
            catch (error) {
                showOperationFailure("Arduinoのコンパイル", error);
            }
        })();
        activeBuild = operation;
        try {
            await operation;
        }
        finally {
            if (activeBuild === operation) activeBuild = undefined;
        }
    };

    context.subscriptions.push(
        vscode.commands.registerCommand(UPLOAD_COMMAND, handler),
        vscode.commands.registerCommand(BUILD_COMMAND, buildHandler),
        vscode.commands.registerCommand(LEGACY_UPLOAD_COMMAND, handler)
    );
}

module.exports = {
    registerArduinoUploadCommands,
    UPLOAD_COMMAND,
    BUILD_COMMAND
};
