"use strict";

const net = require("node:net");
const vscode = require("vscode");

const UPLOAD_TASK_NAME = "Arduino: Upload";
const UPLOAD_STATUS_PIPE = "\\\\.\\pipe\\monocon-upload-status-v1";
const SUCCESS_FOREGROUND = "#22c55e";
const WARNING_FOREGROUND = "#eab308";
const ERROR_FOREGROUND = "#ef4444";

function isUploadTask(execution) {
    return execution?.task?.name === UPLOAD_TASK_NAME;
}

/**
 * Arduino書き込みタスクを、コマンドの呼び出し方に関係なく監視する。
 * CLIの検証完了通知を直接受け、タスク終了イベントはフォールバックに使う。
 */
function registerUploadStatus(context) {
    const status = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        100
    );
    status.name = "Monocon Arduino Upload";
    status.text = "$(plug) Arduino: 待機中";
    status.tooltip = "Arduinoへの書き込み準備ができています";
    status.color = undefined;
    status.backgroundColor = undefined;
    status.show();

    let completionServer;
    let completionShown = false;
    let processResultReceived = false;
    let taskEndFallbackTimer;

    const showCompletion = () => {
        if (completionShown) {
            return;
        }
        completionShown = true;
        status.text = "$(check) Arduino: 書き込み完了";
        status.tooltip = "Arduinoへの書き込みが正常に完了しました";
        status.color = SUCCESS_FOREGROUND;
        status.backgroundColor = undefined;
        status.show();
        vscode.window.showInformationMessage("Arduinoへの書き込みが完了しました。");
    };

    const closeCompletionServer = () => {
        const server = completionServer;
        completionServer = undefined;
        if (!server) {
            return;
        }
        try {
            server.close();
        }
        catch {
            // listen完了前・既にclose済みの場合は何もしない。
        }
    };

    const startCompletionServer = () => {
        closeCompletionServer();
        clearTimeout(taskEndFallbackTimer);
        completionShown = false;
        processResultReceived = false;
        const server = net.createServer(socket => {
            let message = "";
            socket.on("data", chunk => {
                message += String(chunk);
                if (message.includes("upload-verified")) {
                    showCompletion();
                }
                if (message.length > 256) {
                    message = message.slice(-256);
                }
            });
        });
        completionServer = server;
        server.on("error", () => {
            // パイプを作れない場合も、タスク終了イベントによる従来経路を使える。
            if (completionServer === server) {
                completionServer = undefined;
            }
        });
        server.listen(UPLOAD_STATUS_PIPE);
    };

    const startDisposable = vscode.tasks.onDidStartTask(event => {
        if (!isUploadTask(event.execution)) {
            return;
        }
        startCompletionServer();
        status.text = "$(sync~spin) Arduino: 書き込み中…";
        status.tooltip = "コンパイル・書き込みを実行しています";
        status.color = WARNING_FOREGROUND;
        status.backgroundColor = undefined;
        status.show();
    });

    const endDisposable = vscode.tasks.onDidEndTaskProcess(event => {
        if (!isUploadTask(event.execution)) {
            return;
        }
        processResultReceived = true;
        clearTimeout(taskEndFallbackTimer);
        closeCompletionServer();
        if (event.exitCode === 0) {
            // CLIからの即時通知が届かなかった場合のフォールバック。
            showCompletion();
        }
        else {
            const code = event.exitCode === undefined ? "不明" : event.exitCode;
            status.text = "$(error) Arduino: 書き込み失敗";
            status.tooltip = `Arduinoへの書き込みに失敗しました（終了コード: ${code}）`;
            status.color = ERROR_FOREGROUND;
            status.backgroundColor = undefined;
        }
        status.show();
    });

    const taskEndDisposable = vscode.tasks.onDidEndTask(event => {
        if (!isUploadTask(event.execution) || processResultReceived) {
            return;
        }
        if (completionShown) {
            closeCompletionServer();
            return;
        }
        // process終了コードが通知されない異常系で「書き込み中」のまま残さない。
        taskEndFallbackTimer = setTimeout(() => {
            closeCompletionServer();
            if (!completionShown && !processResultReceived) {
                status.text = "$(error) Arduino: 書き込み結果不明";
                status.tooltip = "書き込みタスクの終了コードを取得できませんでした";
                status.color = ERROR_FOREGROUND;
                status.backgroundColor = undefined;
                status.show();
            }
        }, 100);
    });

    const serverDisposable = {
        dispose() {
            clearTimeout(taskEndFallbackTimer);
            closeCompletionServer();
        }
    };
    context.subscriptions.push(
        status,
        startDisposable,
        endDisposable,
        taskEndDisposable,
        serverDisposable
    );
}

module.exports = { registerUploadStatus, UPLOAD_STATUS_PIPE };
