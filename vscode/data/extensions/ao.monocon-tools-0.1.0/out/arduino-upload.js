"use strict";

const vscode = require("vscode");

const UPLOAD_COMMAND = "monoconTools.uploadArduino";
const LEGACY_UPLOAD_COMMAND = "monocon.upload";
const UPLOAD_TASK_NAME = "Arduino: Upload";

let activeUpload;
let serialMonitorApiPromise;
let uploadTaskPromise;
let outputChannel;

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

async function runUpload(context) {
    let api;
    let stoppedPorts = [];
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
            const task = await findUploadTask();
            if (!task) {
                throw new Error(`${UPLOAD_TASK_NAME} タスクが見つかりません。`);
            }
            const exitCode = await executeTaskAndWait(task, taskTimeoutMs);
            if (exitCode !== undefined && exitCode !== 0) {
                throw new Error(`書き込みタスクが終了コード ${exitCode} で失敗しました。`);
            }
            log("Arduinoへの書き込みが完了しました。");
            vscode.window.showInformationMessage("Arduinoへの書き込みが完了しました。");
        }
        catch (error) {
            const message = error instanceof Error ? error.message : String(error);
            log(`Arduinoへの書き込みに失敗しました: ${message}`);
            vscode.window.showErrorMessage(`Arduinoへの書き込みに失敗しました: ${message}`);
        }
        finally {
            if (reopenMonitor && stoppedPorts.length > 0) {
                progress.report({ message: "シリアルモニターを再開しています…" });
                await reopenSerialMonitors(api, stoppedPorts, baudRate, reopenDelayMs, operationTimeoutMs);
            }
        }
    });
}

function registerArduinoUploadCommands(context) {
    outputChannel = vscode.window.createOutputChannel("Monocon Tools");
    context.subscriptions.push(outputChannel);

    const handler = async () => {
        if (activeUpload) {
            vscode.window.showInformationMessage("アップロードは既に実行中です。");
            return;
        }
        const thisUpload = runUpload(context);
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

    context.subscriptions.push(
        vscode.commands.registerCommand(UPLOAD_COMMAND, handler),
        vscode.commands.registerCommand(LEGACY_UPLOAD_COMMAND, handler)
    );
}

module.exports = {
    registerArduinoUploadCommands,
    UPLOAD_COMMAND
};
