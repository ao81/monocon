"use strict";

const vscode = require("vscode");

const UPLOAD_TASK_NAME = "Arduino: Upload";

function isUploadTask(execution) {
    return execution?.task?.name === UPLOAD_TASK_NAME;
}

/**
 * Arduino書き込みタスクを、コマンドの呼び出し方に関係なく監視する。
 * プロセス終了イベントはCLIの最終出力直後に届くため、完了表示とのずれが少ない。
 */
function registerUploadStatus(context) {
    const status = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left,
        100
    );
    status.name = "Monocon Arduino Upload";
    status.text = "$(plug) Arduino: 待機中";
    status.tooltip = "Arduinoへの書き込み準備ができています";
    status.show();

    const startDisposable = vscode.tasks.onDidStartTask(event => {
        if (!isUploadTask(event.execution)) {
            return;
        }
        status.text = "$(sync~spin) Arduino: 書き込み中…";
        status.tooltip = "コンパイル・書き込みを実行しています";
        status.show();
    });

    const endDisposable = vscode.tasks.onDidEndTaskProcess(event => {
        if (!isUploadTask(event.execution)) {
            return;
        }
        if (event.exitCode === 0) {
            status.text = "$(check) Arduino: 書き込み完了";
            status.tooltip = "Arduinoへの書き込みが正常に完了しました";
            vscode.window.showInformationMessage("Arduinoへの書き込みが完了しました。");
        }
        else {
            const code = event.exitCode === undefined ? "不明" : event.exitCode;
            status.text = "$(error) Arduino: 書き込み失敗";
            status.tooltip = `Arduinoへの書き込みに失敗しました（終了コード: ${code}）`;
        }
        status.show();
    });

    context.subscriptions.push(status, startDisposable, endDisposable);
}

module.exports = { registerUploadStatus };
