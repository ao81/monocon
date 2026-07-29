"use strict";

const assert = require('node:assert/strict');
const Module = require('node:module');
const path = require('node:path');
const test = require('node:test');

function disposable(remove) {
    return { dispose: remove || (() => undefined) };
}

async function waitUntil(predicate, advance = () => undefined) {
    for (let attempt = 0; attempt < 100; attempt++) {
        advance();
        if (predicate()) return;
        await new Promise(resolve => setImmediate(resolve));
    }
    throw new Error("Condition was not reached");
}

function loadExtension(vscode, net, nativeModule) {
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
        if (request === './native-service' && nativeModule) {
            return nativeModule;
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
    const outputLines = [];
    const diagnosticEntries = [];
    const monitorStopPorts = [];
    let executeCount = 0;
    let fetchCount = 0;
    let terminateCount = 0;
    let processEnded = false;
    let nativeRequestCount = 0;
    const nativeRequests = [];
    let outputClearCount = 0;
    const monitorStartSettings = [];
    const completionServers = new Set();
    const listenedPipeNames = [];
    const executedTasks = [];

    const net = {
        createServer(connectionListener) {
            const server = {
                on() { return server; },
                listen(pipeName) {
                    listenedPipeNames.push(pipeName);
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

    class NativeUnavailableError extends Error {}
    class NativeOperationTimeoutError extends Error {}
    const nativeService = {
        warmup: async () => ({ success: true }),
        async request(method, params, timeoutMs) {
            nativeRequestCount++;
            nativeRequests.push({ method, params, timeoutMs });
            if (options.nativeError) throw options.nativeError;
            if (method === 'ports') {
                return options.nativePortsResult || {
                    success: true,
                    ports: ['COM3'],
                    arduinoPort: 'COM3'
                };
            }
            if (!options.nativeResult) {
                throw new NativeUnavailableError("native test fallback");
            }
            return options.nativeResult;
        },
        dispose() {}
    };
    const nativeModule = {
        NativeUnavailableError,
        NativeOperationTimeoutError,
        createNativeService() {
            return nativeService;
        }
    };

    const task = { name: 'Arduino: Upload' };
    class ProcessExecution {
        constructor(process, args = [], options) {
            Object.assign(this, { process, args, options });
        }
    }
    class Task {
        constructor(definition, scope, name, source, execution, problemMatchers) {
            Object.assign(this, {
                definition,
                scope,
                name,
                source,
                execution,
                problemMatchers
            });
        }
    }
    const api = {
        dispose() {},
        async listAvailablePorts() {
            if (options.monitorPorts) {
                return options.monitorPorts.map(portName => ({ portName }));
            }
            return options.monitorActive ? [{ portName: 'COM3' }] : [];
        },
        async stopMonitoringPort(portName) {
            monitorStopPorts.push(portName);
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
        DiagnosticSeverity: { Error: 0, Warning: 1, Information: 2 },
        Task,
        ProcessExecution,
        TaskScope: { Global: 1, Workspace: 2 },
        TaskRevealKind: { Silent: 3 },
        TaskPanelKind: { Shared: 4 },
        Uri: {
            file(fsPath) {
                return { scheme: 'file', fsPath };
            }
        },
        Range: class Range {
            constructor(startLine, startColumn, endLine, endColumn) {
                Object.assign(this, { startLine, startColumn, endLine, endColumn });
            }
        },
        Diagnostic: class Diagnostic {
            constructor(range, message, severity) {
                Object.assign(this, { range, message, severity });
            }
        },
        commands: {
            registerCommand(name, callback) {
                commands.set(name, callback);
                return disposable(() => commands.delete(name));
            },
            executeCommand() {}
        },
        languages: {
            createDiagnosticCollection() {
                return {
                    clear() { diagnosticEntries.length = 0; },
                    set(uri, diagnostics) { diagnosticEntries.push({ uri, diagnostics }); },
                    dispose() { diagnosticEntries.length = 0; }
                };
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
                executedTasks.push(startedTask);
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
            activeTextEditor: {
                document: {
                    uri: {
                        scheme: 'file',
                        fsPath: 'C:\\sketch\\test.ino'
                    }
                }
            },
            createOutputChannel() {
                return {
                    appendLine(line) { outputLines.push(line); },
                    clear() {
                        outputClearCount += 1;
                        outputLines.length = 0;
                    },
                    show() {},
                    dispose() {}
                };
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
            },
            showQuickPick(items) {
                return Promise.resolve(options.quickPickSelection ?? items[0]);
            }
        },
        workspace: {
            workspaceFolders: [{
                uri: { fsPath: 'C:\\workspace' }
            }],
            getWorkspaceFolder() {
                return { uri: { fsPath: 'C:\\workspace' } };
            },
            async saveAll() {
                return true;
            },
            getConfiguration(section) {
                if (section !== 'monoconTools.upload') {
                    return { get: (_name, fallback) => fallback };
                }
                const values = {
                    baudRate: 115200,
                    port: options.configuredPort || '',
                    reopenMonitor: true,
                    reopenDelayMs: 0,
                    portReleaseDelayMs: 0,
                    taskTimeoutMs: options.timeoutMs || 30,
                    serialOperationTimeoutMs: options.timeoutMs || 30,
                    ...options.configValues
                };
                return { get: (name, fallback) => values[name] ?? fallback };
            }
        }
    };

    return {
        vscode,
        net,
        nativeModule,
        NativeOperationTimeoutError,
        commands,
        messages,
        monitorStopPorts,
        monitorStartSettings,
        statusItems,
        outputLines,
        diagnosticEntries,
        nativeRequests,
        listenedPipeNames,
        executedTasks,
        activationContext: {
            subscriptions: [],
            ...(options.extensionPath
                ? { extensionPath: options.extensionPath }
                : {})
        },
        get executeCount() { return executeCount; },
        get fetchCount() { return fetchCount; },
        get terminateCount() { return terminateCount; },
        get processEnded() { return processEnded; },
        get nativeRequestCount() { return nativeRequestCount; },
        get outputClearCount() { return outputClearCount; }
    };
}

async function activateAndGetUpload(mock) {
    const extension = loadExtension(mock.vscode, mock.net, mock.nativeModule);
    extension.activate(mock.activationContext);
    const upload = mock.commands.get('monoconTools.uploadArduino');
    assert.equal(typeof upload, 'function');
    return upload;
}

test('registers descriptive command IDs and compatibility aliases', async () => {
    const mock = createVscodeMock();
    await activateAndGetUpload(mock);

    assert.equal(typeof mock.commands.get('monoconTools.uploadArduino'), 'function');
    assert.equal(typeof mock.commands.get('monoconTools.compileArduino'), 'function');
    assert.equal(typeof mock.commands.get('monoconTools.createTaskFolders'), 'function');
    assert.equal(typeof mock.commands.get('monocon.upload'), 'function');
    assert.equal(typeof mock.commands.get('template.generate'), 'function');
});

test('uses the native worker path without starting the compatibility task', async () => {
    const mock = createVscodeMock({
        nativeResult: {
            success: true,
            port: 'COM3',
            uploadTimeMs: 2000,
            compile: {
                success: true,
                cached: true,
                buildTimeMs: 5,
                recompiledFiles: 0,
                totalFiles: 1
            }
        }
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.nativeRequestCount, 2);
    assert.equal(mock.executeCount, 0);
    assert.equal(mock.statusItems[0].text, '$(check) Arduino: 書き込み完了');
    assert.equal(mock.messages.info.includes('Arduinoへの書き込みが完了しました。'), true);
});

test('uses a private completion pipe and forwards the selected port to portable CLI', async () => {
    const extensionPath = path.resolve(__dirname, '..');
    const mock = createVscodeMock({
        extensionPath,
        completionSignalDelayMs: 0
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.fetchCount, 0);
    assert.equal(mock.executedTasks.length, 1);
    const execution = mock.executedTasks[0].execution;
    assert.match(
        execution.process,
        /[\\/]daemon[\\/]build[\\/]bin[\\/]arduino-build-cli\.exe$/i
    );
    assert.deepEqual(execution.args.slice(0, 3), [
        'upload',
        'C:\\sketch',
        'COM3'
    ]);
    const pipeIndex = execution.args.indexOf('--status-pipe');
    assert.notEqual(pipeIndex, -1);
    const pipeName = execution.args[pipeIndex + 1];
    assert.match(
        pipeName,
        /^\\\\\.\\pipe\\monocon-upload-status-v2-\d+-[0-9a-f]{32}$/
    );
    assert.equal(mock.listenedPipeNames.includes(pipeName), true);
    assert.equal(mock.messages.info.filter(
        message => message === 'Arduinoへの書き込みが完了しました。'
    ).length, 1);
});

test('falls back to the portable CLI for compile-only requests', async () => {
    const extensionPath = path.resolve(__dirname, '..');
    const mock = createVscodeMock({ extensionPath });
    await activateAndGetUpload(mock);
    const compile = mock.commands.get('monoconTools.compileArduino');

    await compile();

    assert.equal(mock.nativeRequestCount, 1);
    assert.equal(mock.nativeRequests[0].method, 'compile');
    assert.equal(mock.fetchCount, 0);
    assert.equal(mock.executedTasks.length, 1);
    assert.equal(mock.executedTasks[0].name, 'Arduino: Compile');
    assert.deepEqual(mock.executedTasks[0].execution.args, [
        'build',
        'C:\\sketch',
        '--workspace',
        'C:\\workspace'
    ]);
    assert.equal(
        mock.messages.info.includes('Arduinoのコンパイルが完了しました。'),
        true
    );
});

test('asks for the target port when multiple serial devices are connected', async () => {
    const mock = createVscodeMock({
        nativePortsResult: {
            success: true,
            ports: ['COM3', 'COM8'],
            arduinoPort: ''
        },
        quickPickSelection: 'COM8',
        monitorPorts: ['COM3', 'COM8'],
        nativeResult: {
            success: true,
            port: 'COM8',
            compile: { success: true }
        }
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.nativeRequests[1].method, 'upload');
    assert.equal(mock.nativeRequests[1].params.port, 'COM8');
    assert.deepEqual(mock.monitorStopPorts, ['COM8']);
});

test('clears previous output whenever an upload starts', async () => {
    const mock = createVscodeMock({
        nativeResult: {
            success: true,
            port: 'COM3',
            uploadTimeMs: 1,
            compile: {
                success: true,
                cached: true,
                buildTimeMs: 1,
                recompiledFiles: 0,
                totalFiles: 1
            }
        }
    });
    const upload = await activateAndGetUpload(mock);

    await upload();
    const firstOutputLineCount = mock.outputLines.length;
    await upload();

    assert.equal(mock.outputClearCount, 2);
    assert.equal(mock.outputLines.length, firstOutputLineCount);
    assert.equal(mock.outputLines.filter(line => line.startsWith('Compile ')).length, 1);
    assert.equal(mock.outputLines.filter(line => line.startsWith('Upload to ')).length, 1);
});

test('shows native compiler errors as a readable report and editor diagnostic', async () => {
    const mock = createVscodeMock({
        nativeResult: {
            success: false,
            errorMessage: 'Compile failed: test.ino.cpp',
            compile: {
                success: false,
                errorMessage: 'Compile failed: test.ino.cpp',
                compilerOutput: [
                    "test.ino: In function 'void userLoop()':",
                    "\x1b[1;37mtest.ino\x1b[0m:\x1b[1;96m4\x1b[0m:\x1b[1;37m2\x1b[0m: \x1b[1;31merror:\x1b[0m 'a' was not declared in this scope",
                    "test.ino:4:2: note: suggested alternative: 'ar'"
                ].join('\n')
            }
        }
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    const output = mock.outputLines.join('\n');
    assert.match(output, /Arduino コンパイル失敗/);
    assert.match(output, /内容     : 「a」が宣言されていません。/);
    assert.match(output, /候補     : 「ar」/);
    assert.doesNotMatch(output, /確認事項/);
    assert.doesNotMatch(output, /\x1b/);
    assert.equal(mock.diagnosticEntries.length, 1);
    assert.equal(mock.diagnosticEntries[0].diagnostics[0].severity, 0);
    assert.match(mock.messages.error[0], /test\.ino 4行目/);
    assert.doesNotMatch(mock.messages.error[0], /\x1b/);
});

test('monitor restart timeout always releases the upload lock', async t => {
    t.mock.timers.enable({ apis: ['setTimeout'] });
    const mock = createVscodeMock({
        monitorActive: true,
        hangingMonitorRestart: true,
        nativeResult: {
            success: true,
            port: 'COM3',
            compile: { success: true }
        }
    });
    const upload = await activateAndGetUpload(mock);

    const first = upload();
    await waitUntil(
        () => mock.monitorStartSettings.length === 1,
        () => t.mock.timers.tick(0)
    );
    t.mock.timers.tick(1000);
    await first;

    const second = upload();
    await waitUntil(
        () => mock.monitorStartSettings.length === 2,
        () => t.mock.timers.tick(0)
    );
    t.mock.timers.tick(1000);
    await second;

    assert.equal(mock.nativeRequestCount, 4);
    assert.equal(mock.executeCount, 0);
    assert.equal(mock.messages.info.includes('アップロードは既に実行中です。'), false);
    assert.equal(mock.messages.warning.length, 2);
});

test('task timeout terminates the task and releases the upload lock', async t => {
    t.mock.timers.enable({ apis: ['setTimeout'] });
    const mock = createVscodeMock({ hangingTask: true });
    const upload = await activateAndGetUpload(mock);

    const first = upload();
    await waitUntil(() => mock.executeCount === 1);
    await new Promise(resolve => setImmediate(resolve));
    t.mock.timers.tick(10000);
    await first;

    const second = upload();
    await waitUntil(() => mock.executeCount === 2);
    await new Promise(resolve => setImmediate(resolve));
    t.mock.timers.tick(10000);
    await second;

    assert.equal(mock.executeCount, 2);
    assert.equal(mock.terminateCount, 2);
    assert.equal(mock.messages.info.includes('アップロードは既に実行中です。'), false);
    assert.equal(mock.messages.error.length, 2);
});

test('normalizes invalid numeric and boolean upload settings', async () => {
    const mock = createVscodeMock({
        monitorActive: true,
        configValues: {
            baudRate: Number.POSITIVE_INFINITY,
            reopenMonitor: 'false',
            reopenDelayMs: -100,
            portReleaseDelayMs: -100,
            taskTimeoutMs: Number.NaN,
            serialOperationTimeoutMs: -100
        },
        nativeResult: {
            success: true,
            port: 'COM3',
            compile: { success: true }
        }
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.nativeRequests[0].timeoutMs, 1000);
    assert.equal(mock.nativeRequests[1].timeoutMs, 180000);
    assert.equal(mock.monitorStartSettings[0].baudRate, 9600);
    assert.equal(mock.monitorStartSettings.length, 1);
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

test('does not reopen the serial monitor while a timed-out native upload may still run', async () => {
    const mock = createVscodeMock({ monitorActive: true, nativeResult: { success: true } });
    mock.nativeModule.createNativeService = () => ({
        warmup: async () => ({ success: true }),
        request: async () => {
            throw new mock.NativeOperationTimeoutError('native upload timed out');
        },
        dispose() {}
    });
    const upload = await activateAndGetUpload(mock);

    await upload();

    assert.equal(mock.monitorStartSettings.length, 0);
    assert.equal(mock.messages.error.length, 1);
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
    assert.equal(mock.statusItems[0].text, '$(error) Arduino: 書き込み失敗');
    assert.equal(mock.statusItems[0].color, '#ef4444');
    assert.equal(mock.statusItems[0].backgroundColor, undefined);
});
