"use strict";

const assert = require('node:assert/strict');
const Module = require('node:module');
const path = require('node:path');
const test = require('node:test');

function disposable(remove) {
    return { dispose: remove || (() => undefined) };
}

function loadExtension(vscode) {
    const extensionPath = path.resolve(__dirname, '../out/extension.js');
    const uploadPath = path.resolve(__dirname, '../out/arduino-upload.js');
    const foldersPath = path.resolve(__dirname, '../out/task-folders.js');
    const originalLoad = Module._load;
    Module._load = function (request, parent, isMain) {
        if (request === 'vscode') {
            return vscode;
        }
        return originalLoad.call(this, request, parent, isMain);
    };
    delete require.cache[extensionPath];
    delete require.cache[uploadPath];
    delete require.cache[foldersPath];
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
    const taskListeners = new Set();
    const messages = { info: [], warning: [], error: [] };
    let executeCount = 0;
    let fetchCount = 0;
    let terminateCount = 0;
    const monitorStartSettings = [];

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
                if (!options.hangingTask) {
                    setTimeout(() => {
                        for (const listener of processListeners) {
                            listener({ execution, exitCode: 0 });
                        }
                        for (const listener of taskListeners) {
                            listener({ execution });
                        }
                    }, 0);
                }
                return execution;
            }
        },
        window: {
            createOutputChannel() {
                return { appendLine() {}, dispose() {} };
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
        commands,
        messages,
        monitorStartSettings,
        get executeCount() { return executeCount; },
        get fetchCount() { return fetchCount; },
        get terminateCount() { return terminateCount; }
    };
}

async function activateAndGetUpload(mock) {
    const extension = loadExtension(mock.vscode);
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
