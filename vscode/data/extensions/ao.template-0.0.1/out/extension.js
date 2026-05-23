"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = require("vscode");
const path = require("path");
const fs = require("fs");
function activate(context) {
    const disposable = vscode.commands.registerCommand('template.generate', async (clickedUri, selectedUris) => {
        try {
            // エクスプローラで複数選択された場合、selectedUris に全て入る
            // 単独クリックや コマンドパレットから呼ばれた場合は clickedUri のみ
            const uris = selectedUris && selectedUris.length > 0
                ? selectedUris
                : clickedUri ? [clickedUri] : [];
            // ヘッダファイルと親ディレクトリを振り分ける
            let headerUri;
            let parentDirUri;
            for (const uri of uris) {
                const stat = await fs.promises.stat(uri.fsPath);
                if (stat.isDirectory()) {
                    parentDirUri = uri;
                }
                else if (stat.isFile()) {
                    headerUri = uri;
                }
            }
            // 不足分はダイアログで補完
            if (!headerUri) {
                const picked = await vscode.window.showOpenDialog({
                    canSelectFiles: true,
                    canSelectFolders: false,
                    canSelectMany: false,
                    title: 'コピー元のヘッダファイル (.h) を選択',
                    filters: { 'Header': ['h', 'hpp'], 'All Files': ['*'] }
                });
                if (!picked || picked.length === 0) {
                    vscode.window.showInformationMessage('ヘッダファイルが選択されなかったため処理を中止しました。');
                    return;
                }
                headerUri = picked[0];
            }
            if (!parentDirUri) {
                const picked = await vscode.window.showOpenDialog({
                    canSelectFiles: false,
                    canSelectFolders: true,
                    canSelectMany: false,
                    title: '生成先の親ディレクトリを選択'
                });
                if (!picked || picked.length === 0) {
                    vscode.window.showInformationMessage('親ディレクトリが選択されなかったため処理を中止しました。');
                    return;
                }
                parentDirUri = picked[0];
            }
            // 個数を入力で取得
            const input = await vscode.window.showInputBox({
                title: '生成する個数',
                prompt: '何個のフォルダを生成しますか?',
                value: '5',
                validateInput: (value) => {
                    const n = Number(value);
                    if (!Number.isInteger(n))
                        return '整数を入力してください';
                    if (n < 1)
                        return '1以上を入力してください';
                    if (n > 100)
                        return '100以下を入力してください';
                    return null;
                }
            });
            if (input === undefined) {
                // キャンセル
                return;
            }
            const count = Number(input);
            // 設定からベース名を取得
            const config = vscode.workspace.getConfiguration('template');
            const baseName = config.get('baseName', 'kadai');
            const headerFileName = path.basename(headerUri.fsPath);
            const parentDir = parentDirUri.fsPath;
            // 既存ディレクトリがあれば事前確認
            const existing = [];
            for (let i = 1; i <= count; i++) {
                const target = path.join(parentDir, `${baseName}${i}`);
                if (fs.existsSync(target)) {
                    existing.push(`${baseName}${i}`);
                }
            }
            if (existing.length > 0) {
                const choice = await vscode.window.showWarningMessage(`次のフォルダが既に存在します: ${existing.join(', ')}。中身を上書き(ヘッダコピー・空 .ino 生成)しますか?`, { modal: true }, '続行', 'キャンセル');
                if (choice !== '続行') {
                    return;
                }
            }
            // 生成処理
            let created = 0;
            for (let i = 1; i <= count; i++) {
                const folderName = `${baseName}${i}`;
                const folderPath = path.join(parentDir, folderName);
                await fs.promises.mkdir(folderPath, { recursive: true });
                // ヘッダファイルをコピー (元のファイル名のまま)
                const headerDest = path.join(folderPath, headerFileName);
                await fs.promises.copyFile(headerUri.fsPath, headerDest);
                // 空の .ino ファイルを作成
                const inoPath = path.join(folderPath, `${folderName}.ino`);
                await fs.promises.writeFile(inoPath, '', { flag: 'w' });
                created++;
            }
            vscode.window.showInformationMessage(`${created}個のフォルダを生成しました (${baseName}1 〜 ${baseName}${count})`);
        }
        catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            vscode.window.showErrorMessage(`生成に失敗しました: ${message}`);
        }
    });
    context.subscriptions.push(disposable);
}
function deactivate() { }
//# sourceMappingURL=extension.js.map