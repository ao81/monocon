"use strict";

const assert = require('node:assert/strict');
const Module = require('node:module');
const path = require('node:path');
const test = require('node:test');

function disposable(remove) {
    return { dispose: remove || (() => undefined) };
}

function loadExtension(vscode, net) {
    const extensionPath = path.resolve(__dirname, '../out/extension.js');
    const uploadPath = path.resolve(__dirname, '../out/arduino-upload.js');
    const foldersPath = path.resolve(__dirname, '../out/task-folders.js');
    const statusPath = path.resolve(__dirname, '../out/upload-status.js');
    const originalLoad = Module._load;
    Module._load = function (request, parent, isMain) {
        if (request === 'vscode') {
            return vscode;
        }
        if (request === 'node:net' && net) {
            return net;
        }
        return originalLoad.call(this, request, parent, isMain);
    };
    delete require.cache[extensionPath];
    delete require.cache[uploadPath];
    delete require.cache[foldersPath];
    delete require.cache[statusPath];
    try {
        return require(extensionPath);
    }
    finally {
        Module._load = originalLoad;
    }
}

function createVscodeMock(options = {}) {
    const commands = new Map();
    const processListeners = new Set();
    const startListeners = new Set();
    const taskListeners = new Set();
    const messages = { info: [], warning: [], error: [] };
    const statusItems = [];
    let executeCount = 0;
    let fetchCount = 0;
    let terminateCount = 0;
    let processEnded = false;
    const monitorStartSettings = [];
    const completionServers = new Set();

    const net = {
        createServer(connectionListener) {
            const server = {
                on() { return server; },
                listen() {
                    completionServers.add(server);
                    return server;
                },
                close() {
                    completionServers.delete(server);
                },
                signal(message) {
                    connectionListener({
                        on(event, listener) {
                            if (event === 'data') {
                                const chunks = Array.isArray(message) ? message : [message];
                                for (const chunk of chunks) {
                                    listener(Buffer.from(chunk));
                                }
                            }
                        }
                    });
                }
            };
            return server;
        }
    };

    const signalUploadCompletion = () => {
        for (const server of [...completionServers]) {
            server.signal(options.splitCompletionSignal
                ? ['upload-', 'verified\n']
                : 'upload-verified\n');
        }
    };

    const task = { name: 'Arduino: Upload' };
    const api = {
        dispose() {},
        async listAvailablePorts() {
            return options.monitorActive ? [{ portName: 'COM3' }] : [];
        },
        async stopMonitoringPort() {
            return true;
        },
        startMonitoringPort(settings) {
            monitorStartSettings.push(settings);
            return options.hangingMonitorRestart
                ? new Promise(() => undefined)
                : Promise.resolve({ dispose() {} });
        }
    };
    const vscode = {
        ProgressLocation: { Notification: 15 },
        StatusBarAlignment: { Left: 1, Right: 2 },
        ThemeColor: class ThemeColor {
            constructor(id) {
                this.id = id;
            }
        },
        commands: {
            registerCommand(name, callback) {
                commands.set(name, callback);
                return disposable(() => commands.delete(name));
            }
        },
        extensions: {
            getExtension() {
                return { isActive: true, exports: { getApi: () => api } };
            }
        },
        tasks: {
            taskExecutions: [],
            async fetchTasks() {
                fetchCount++;
                return [task];
            },
            onDidEndTaskProcess(listener) {
                processListeners.add(listener);
                return disposable(() => processListeners.delete(listener));
            },
            onDidStartTask(listener) {
                startListeners.add(listener);
                return disposable(() => startListeners.delete(listener));
            },
            onDidEndTask(listener) {
                taskListeners.add(listener);
                return disposable(() => taskListeners.delete(listener));
            },
            async executeTask(startedTask) {
                executeCount++;
                const execution = {
                    task: startedTask,
                    terminate() {
                        terminateCount++;
                    }
                };
                for (const listener of startListeners) {
                    listener({ execution });
                }
                if (!options.hangingTask) {
                    if (options.completionSignalDelayMs !== undefined) {
                        setTimeout(signalUploadCompletion, options.completionSignalDelayMs);
                    }
                    setTimeout(() => {
                        if (!options.taskEndWithoutProcess) {
                            processEnded = true;
                            for (const listener of processListeners) {
                                listener({ execution, exitCode: options.exitCode ?? 0 });
                            }
                        }
                        for (const listener of taskListeners) {
                            listener({ execution });
                        }
                    }, options.processEndDelayMs || 0);
                }
                return execution;
            }
        },
        window: {
            createOutputChannel() {
                return { appendLine() {}, dispose() {} };
            },
            createStatusBarItem() {
                const item = {
                    text: '',
                    tooltip: '',
                    color: undefined,
                    backgroundColor: undefined,
                    visible: false,
                    show() { this.visible = true; },
                    hide() { this.visible = false; },
                    dispose() { this.visible = false; }
                };
                statusItems.push(item);
                return item;
            },
            async withProgress(_settings, callback) {
                return callback({ report() {} });
            },
            showInformationMessage(message) {
                messages.info.push(message);
            },
            showWarningMessage(message) {
                messages.warning.push(message);
            },
            showErrorMessage(message) {
                messages.error.push(message);
            }
        },
        workspace: {
            async saveAll() {
                return true;
            },
            getConfiguration(section) {
                if (section !== 'monoconTools.upload') {
                    return { get: (_name, fallback) => fallback };
                }
                const values = {
                    baudRate: 115200,
                    reopenMonitor: true,
                    reopenDelayMs: 0,
                    portReleaseDelayMs: 0,
                    taskTimeoutMs: options.timeoutMs || 30,
                    serialOperationTimeoutMs: options.timeoutMs || 30
                };
                return { get: (name, fallback) => values[name] ?? fallback };
            }
        }
    };

    return {
        vscode,
        net,
        commands,
        messages,
        monitorStartSettings,
        statusItems,
        get executeCount() { return executeCount; },
        get fetchCount() { return fetchCount; },
        get terminateCount() { return terminateCount; },
        get processEnded() { return processEnded; }
    };
}

async function activateAndGetUpload(mock) {
    const extension = loadExtension(mock.vscode, mock.net);
    extension.activate({ subscriptions: [] });
    const upload = mock.commands.get('monoconTools.uploadArduino');
    assert.equal(typeof upload, 'function');
    return upload;
}

test('registers descriptive command IDs and compatibility aliases', async () => {
    const mock = createVscodeMock();
    await activateAndGetUpload(mock);

    assert.equal(typeof mock.commands.get('monoconTools.uploadArduino'), 'function');
    assert.equal(typeof mock.commands.get('monoconTools.createTaskFolders'), 'function');
    assert.equal(typeof mock.commands.get('monocon.upload'), 'function');
    assert.equal(typeof mock.commands.get('template.generate'), 'function');
});

test('monitor restart timeout always releases the upload lock', async () => {
    const mock = createVscodeMock({
        monitorActive: true,
        hangingMonitorRestart: true
    });
    const upload = await activateAndGetUpload(mock);

    await upload();
    await upload();

    assert.equal(mock.executeCount, 2);
    assert.equal(mock.fetchCount, 1);
    assert.equal(mock.messages.info.includes('アップロードは既に実行中です。'), false);
    assert.equal(mock.messages.warning.length, 2);
});

test('task timeout terminates the task and releases the upload lock', async () => {
    const mock = createVscodeMock({ hangingTask: true });
    const upload = await activateAndGetUpload(mock);

    await upload();
    await upload();

    assert.equal(mock.executeCount, 2);
    assert.equal(mock.terminateCount, 2);
    assert.equal(mock.messages.info.includes('アップロードは既に実行中です。'), false);
    assert.equal(mock.messages.error.length, 2);
});

test('reopens the serial monitor with reset control lines disabled', async () => {
    const mock = createVscodeMock({ monitorActive: true });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.monitorStartSettings.length, 1);
    assert.equal(mock.monitorStartSettings[0].port, 'COM3');
    assert.equal(mock.monitorStartSettings[0].dtr, false);
    assert.equal(mock.monitorStartSettings[0].rts, false);
});

test('shows a completion popup and keeps the result in the status bar', async () => {
    const mock = createVscodeMock();
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.messages.info.includes('Arduinoへの書き込みが完了しました。'), true);
    assert.equal(mock.statusItems.length, 1);
    assert.equal(mock.statusItems[0].text, '$(check) Arduino: 書き込み完了');
    assert.equal(mock.statusItems[0].color, '#22c55e');
    assert.equal(mock.statusItems[0].backgroundColor, undefined);
    assert.equal(mock.statusItems[0].visible, true);
});

test('uses yellow text while uploading', async () => {
    const mock = createVscodeMock({ processEndDelayMs: 50 });
    const upload = await activateAndGetUpload(mock);

    const pending = upload();
    await new Promise(resolve => setTimeout(resolve, 10));

    assert.equal(mock.statusItems[0].text, '$(sync~spin) Arduino: 書き込み中…');
    assert.equal(mock.statusItems[0].color, '#eab308');
    assert.equal(mock.statusItems[0].backgroundColor, undefined);

    await pending;
});

test('uses red text when uploading fails', async () => {
    const mock = createVscodeMock({ exitCode: 1 });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.statusItems[0].text, '$(error) Arduino: 書き込み失敗');
    assert.equal(mock.statusItems[0].color, '#ef4444');
    assert.equal(mock.statusItems[0].backgroundColor, undefined);
});

test('shows completion from the CLI signal before VS Code reports process end', async () => {
    const mock = createVscodeMock({
        completionSignalDelayMs: 0,
        processEndDelayMs: 50,
        splitCompletionSignal: true
    });
    const upload = await activateAndGetUpload(mock);

    const pending = upload();
    await new Promise(resolve => setTimeout(resolve, 10));

    assert.equal(mock.processEnded, false);
    assert.equal(mock.messages.info.includes('Arduinoへの書き込みが完了しました。'), true);

    await pending;
    assert.equal(mock.messages.info.filter(
        message => message === 'Arduinoへの書き込みが完了しました。'
    ).length, 1);
});

test('does not report success when VS Code omits the process exit code', async () => {
    const mock = createVscodeMock({
        taskEndWithoutProcess: true,
        timeoutMs: 300
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.messages.info.includes('Arduinoへの書き込みが完了しました。'), false);
    assert.equal(mock.messages.error.length, 1);
    assert.match(mock.messages.error[0], /終了コード 不明/);
    assert.equal(mock.statusItems[0].text, '$(error) Arduino: 書き込み結果不明');
    assert.equal(mock.statusItems[0].color, '#ef4444');
    assert.equal(mock.statusItems[0].backgroundColor, undefined);
});
