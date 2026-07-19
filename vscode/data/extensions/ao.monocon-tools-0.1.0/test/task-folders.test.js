"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const Module = require("node:module");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");

function loadTaskFolders(vscode) {
    const modulePath = path.resolve(__dirname, "../out/task-folders.js");
    const originalLoad = Module._load;
    Module._load = function (request, parent, isMain) {
        if (request === "vscode") {
            return vscode;
        }
        return originalLoad.call(this, request, parent, isMain);
    };
    delete require.cache[modulePath];
    try {
        return require(modulePath);
    }
    finally {
        Module._load = originalLoad;
    }
}

test("creates clearly named task folders with matching ino files", async () => {
    const temporaryRoot = fs.mkdtempSync(path.join(os.tmpdir(), "monocon-tools-test-"));
    try {
        const headerPath = path.join(temporaryRoot, "mono_con.h");
        const destinationPath = path.join(temporaryRoot, "tasks");
        fs.writeFileSync(headerPath, "#pragma once\n");
        fs.mkdirSync(destinationPath);

        const commands = new Map();
        const messages = [];
        const vscode = {
            commands: {
                registerCommand(id, handler) {
                    commands.set(id, handler);
                    return { dispose() {} };
                }
            },
            workspace: {
                getConfiguration(section) {
                    assert.equal(section, "monoconTools.taskFolders");
                    return { get: () => "task" };
                }
            },
            window: {
                async showInputBox() { return "2"; },
                showInformationMessage(message) { messages.push(message); },
                showErrorMessage(message) { assert.fail(message); },
                async showWarningMessage() { return "上書きして続行"; }
            }
        };

        const { registerTaskFolderCommands } = loadTaskFolders(vscode);
        registerTaskFolderCommands({ subscriptions: [] });
        const createFolders = commands.get("monoconTools.createTaskFolders");
        assert.equal(typeof createFolders, "function");

        await createFolders(undefined, [
            { fsPath: headerPath },
            { fsPath: destinationPath }
        ]);

        for (const name of ["task1", "task2"]) {
            assert.equal(fs.readFileSync(path.join(destinationPath, name, "mono_con.h"), "utf8"), "#pragma once\n");
            assert.equal(fs.readFileSync(path.join(destinationPath, name, `${name}.ino`), "utf8"), "");
        }
        assert.match(messages[0], /2個の課題フォルダー/);
    }
    finally {
        fs.rmSync(temporaryRoot, { recursive: true, force: true });
    }
});
