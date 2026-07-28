"use strict";

const fs = require("node:fs");
const path = require("node:path");
const { Worker } = require("node:worker_threads");

class NativeUnavailableError extends Error {
    constructor(message) {
        super(message);
        this.name = "NativeUnavailableError";
    }
}

class NativeOperationTimeoutError extends Error {
    constructor(message) {
        super(message);
        this.name = "NativeOperationTimeoutError";
    }
}

class NativeService {
    constructor(context, options = {}) {
        this.context = context;
        this.WorkerClass = options.WorkerClass || Worker;
        this.worker = undefined;
        this.readyPromise = undefined;
        this.pending = new Map();
        this.timedOutOperation = undefined;
        this.nextId = 1;
        this.disposed = false;
    }

    get extensionRoot() {
        return this.context.extensionPath || path.resolve(__dirname, "..");
    }

    get cacheRoot() {
        return this.context.globalStorageUri?.fsPath
            || path.join(this.extensionRoot, ".cache");
    }

    get addonPath() {
        return path.join(
            this.extensionRoot,
            "native",
            `${process.platform}-${process.arch}`,
            "monocon_native.node"
        );
    }

    async warmup() {
        return this.ensureReady();
    }

    ensureReady() {
        if (this.readyPromise) {
            return this.readyPromise;
        }
        this.readyPromise = new Promise((resolve, reject) => {
            if (this.disposed) {
                reject(new NativeUnavailableError("Native service is disposed"));
                return;
            }
            if (process.platform !== "win32" || process.arch !== "x64") {
                reject(new NativeUnavailableError(
                    `Unsupported platform: ${process.platform}-${process.arch}`
                ));
                return;
            }
            if (!fs.existsSync(this.addonPath)) {
                reject(new NativeUnavailableError(
                    `Native addon not found: ${this.addonPath}`
                ));
                return;
            }
            let worker;
            try {
                fs.mkdirSync(this.cacheRoot, { recursive: true });
                const workerPath = path.join(__dirname, "native-worker.js");
                worker = new this.WorkerClass(workerPath, {
                    workerData: {
                        addonPath: this.addonPath,
                        extensionRoot: this.extensionRoot,
                        cacheRoot: this.cacheRoot
                    }
                });
            }
            catch (error) {
                reject(new NativeUnavailableError(
                    error instanceof Error ? error.message : String(error)
                ));
                return;
            }
            this.worker = worker;
            let readySettled = false;
            worker.on("message", message => {
                if (message?.type === "ready" && !readySettled) {
                    readySettled = true;
                    if (message.error) {
                        reject(new NativeUnavailableError(message.error));
                        this.resetWorker(worker);
                        worker.terminate();
                    }
                    else {
                        resolve(message.result);
                    }
                    return;
                }
                if (message?.type === "response") {
                    this.handleResponse(message);
                }
            });
            worker.on("error", error => {
                if (!readySettled) {
                    readySettled = true;
                    reject(new NativeUnavailableError(error.message));
                }
                this.failPending(error);
                this.resetWorker(worker);
            });
            worker.on("exit", code => {
                if (!readySettled) {
                    readySettled = true;
                    reject(new NativeUnavailableError(
                        `Native worker exited during startup (${code})`
                    ));
                }
                if (this.pending.size > 0) {
                    this.failPending(new Error(`Native worker exited (${code})`));
                }
                this.resetWorker(worker);
            });
        });
        return this.readyPromise;
    }

    async request(method, params = {}, timeoutMs = 180000) {
        await this.ensureReady();
        if (!this.worker) {
            throw new NativeUnavailableError("Native worker is not running");
        }
        if (this.timedOutOperation) {
            throw new Error(
                `Timed-out native ${this.timedOutOperation.method} is still running; `
                + "reload VS Code if it does not finish"
            );
        }
        if (this.pending.size > 0) {
            throw new Error("Another native compile or upload is already running");
        }
        const id = this.nextId++;
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                this.pending.delete(id);
                // 同期C++処理はWorker.terminate()では安全に中断できない。
                // 完了応答が返るまでワーカーを残し、二重書き込みだけを禁止する。
                this.timedOutOperation = { id, method };
                reject(new NativeOperationTimeoutError(
                    `Native ${method} timed out after ${Math.round(timeoutMs / 1000)} seconds`
                ));
            }, timeoutMs);
            this.pending.set(id, { resolve, reject, timeout });
            try {
                this.worker.postMessage({ id, method, params });
            }
            catch (error) {
                clearTimeout(timeout);
                this.pending.delete(id);
                reject(new NativeUnavailableError(
                    error instanceof Error ? error.message : String(error)
                ));
            }
        });
    }

    handleResponse(message) {
        if (this.timedOutOperation?.id === message.id) {
            this.timedOutOperation = undefined;
            return;
        }
        const pending = this.pending.get(message.id);
        if (!pending) return;
        this.pending.delete(message.id);
        clearTimeout(pending.timeout);
        if (message.error) pending.reject(new Error(message.error));
        else pending.resolve(message.result);
    }

    failPending(error) {
        for (const pending of this.pending.values()) {
            clearTimeout(pending.timeout);
            pending.reject(error);
        }
        this.pending.clear();
    }

    resetWorker(worker) {
        if (this.worker !== worker) return;
        this.worker = undefined;
        this.readyPromise = undefined;
        this.timedOutOperation = undefined;
    }

    dispose() {
        this.disposed = true;
        this.failPending(new Error("Native service disposed"));
        const worker = this.worker;
        this.worker = undefined;
        this.readyPromise = undefined;
        worker?.terminate();
    }
}

function createNativeService(context, options) {
    const service = new NativeService(context, options);
    context.subscriptions.push(service);
    return service;
}

module.exports = {
    NativeService,
    NativeUnavailableError,
    NativeOperationTimeoutError,
    createNativeService
};
