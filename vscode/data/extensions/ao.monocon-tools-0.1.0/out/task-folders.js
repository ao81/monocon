"use strict";

const fs = require("fs");
const path = require("path");
const vscode = require("vscode");

const CREATE_FOLDERS_COMMAND = "monoconTools.createTaskFolders";
const LEGACY_CREATE_FOLDERS_COMMAND = "template.generate";

async function chooseHeaderFile() {
    const picked = await vscode.window.showOpenDialog({
        canSelectFiles: true,
        canSelectFolders: false,
        canSelectMany: false,
        title: "コピー元のヘッダファイル（.h/.hpp）を選択",
        filters: { "Header files": ["h", "hpp"], "All files": ["*"] }
    });
    return picked?.[0];
}

async function chooseDestinationFolder() {
    const picked = await vscode.window.showOpenDialog({
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: false,
        title: "課題フォルダーの作成先を選択"
    });
    return picked?.[0];
}

async function askFolderCount() {
    const input = await vscode.window.showInputBox({
        title: "作成する課題フォルダー数",
        prompt: "1～100の整数を入力してください",
        value: "5",
        validateInput: value => {
            const count = Number(value);
            if (!Number.isInteger(count)) {
                return "整数を入力してください";
            }
            if (count < 1 || count > 100) {
                return "1～100の範囲で入力してください";
            }
            return null;
        }
    });
    return input === undefined ? undefined : Number(input);
}

async function resolveSelection(clickedUri, selectedUris) {
    const uris = selectedUris?.length > 0
        ? selectedUris
        : clickedUri ? [clickedUri] : [];
    let headerUri;
    let destinationUri;

    for (const uri of uris) {
        const stat = await fs.promises.stat(uri.fsPath);
        if (stat.isDirectory()) {
            destinationUri = uri;
        }
        else if (stat.isFile()) {
            headerUri = uri;
        }
    }

    headerUri ??= await chooseHeaderFile();
    if (!headerUri) {
        return undefined;
    }
    destinationUri ??= await chooseDestinationFolder();
    if (!destinationUri) {
        return undefined;
    }
    return { headerUri, destinationUri };
}

async function confirmOverwrite(existingNames) {
    if (existingNames.length === 0) {
        return true;
    }
    const choice = await vscode.window.showWarningMessage(
        `次のフォルダーは既に存在します: ${existingNames.join(", ")}。ヘッダーと.inoを上書きしますか？`,
        { modal: true },
        "上書きして続行",
        "キャンセル"
    );
    return choice === "上書きして続行";
}

async function createTaskFolders(clickedUri, selectedUris) {
    try {
        const selection = await resolveSelection(clickedUri, selectedUris);
        if (!selection) {
            return;
        }
        const count = await askFolderCount();
        if (count === undefined) {
            return;
        }

        const config = vscode.workspace.getConfiguration("monoconTools.taskFolders");
        const baseName = config.get("baseName", "kadai");
        const headerFileName = path.basename(selection.headerUri.fsPath);
        const destination = selection.destinationUri.fsPath;
        const folderNames = Array.from({ length: count }, (_, index) => `${baseName}${index + 1}`);
        const existingNames = folderNames.filter(name => fs.existsSync(path.join(destination, name)));

        if (!await confirmOverwrite(existingNames)) {
            return;
        }

        for (const folderName of folderNames) {
            const folderPath = path.join(destination, folderName);
            await fs.promises.mkdir(folderPath, { recursive: true });
            await fs.promises.copyFile(
                selection.headerUri.fsPath,
                path.join(folderPath, headerFileName)
            );
            await fs.promises.writeFile(
                path.join(folderPath, `${folderName}.ino`),
                "",
                { flag: "w" }
            );
        }

        vscode.window.showInformationMessage(
            `${count}個の課題フォルダーを作成しました（${folderNames[0]}～${folderNames.at(-1)}）`
        );
    }
    catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        vscode.window.showErrorMessage(`課題フォルダーの作成に失敗しました: ${message}`);
    }
}

function registerTaskFolderCommands(context) {
    const handler = (clickedUri, selectedUris) => createTaskFolders(clickedUri, selectedUris);
    context.subscriptions.push(
        vscode.commands.registerCommand(CREATE_FOLDERS_COMMAND, handler),
        vscode.commands.registerCommand(LEGACY_CREATE_FOLDERS_COMMAND, handler)
    );
}

module.exports = {
    registerTaskFolderCommands,
    CREATE_FOLDERS_COMMAND
};
