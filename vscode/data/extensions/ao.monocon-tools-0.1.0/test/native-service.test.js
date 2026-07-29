"use strict";

const assert = require("node:assert/strict");
const { EventEmitter } = require("node:events");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");

const { NativeService } = require("../out/native-service");

class FakeWorker extends EventEmitter {
    constructor() {
        super();
        this.terminated = false;
        setImmediate(() => this.emit("message", {
            type: "ready",
            result: { success: true, version: "test" }
        }));
    }

    postMessage(message) {
        setImmediate(() => this.emit("message", {
            type: "response",
            id: message.id,
            result: { success: true, method: message.method }
        }));
    }

    terminate() {
        this.terminated = true;
        return Promise.resolve(0);
    }
}

test("keeps one native worker alive and correlates responses", async () => {
    const context = {
        extensionPath: path.resolve(__dirname, ".."),
        globalStorageUri: {
            fsPath: path.join(os.tmpdir(), "monocon-native-service-test")
        }
    };
    const service = new NativeService(context, { WorkerClass: FakeWorker });

    const ready = await service.warmup();
    const firstWorker = service.worker;
    const ping = await service.request("ping");
    const ports = await service.request("ports");

    assert.equal(ready.success, true);
    assert.equal(ping.method, "ping");
    assert.equal(ports.method, "ports");
    assert.equal(service.worker, firstWorker);

    service.dispose();
    assert.equal(firstWorker.terminated, true);
});

test("does not start a second native operation while a timed-out one still runs", async () => {
    class SlowWorker extends FakeWorker {
        postMessage(message) {
            this.lastMessage = message;
        }

        complete() {
            this.emit("message", {
                type: "response",
                id: this.lastMessage.id,
                result: { success: true }
            });
        }
    }
    const context = {
        extensionPath: path.resolve(__dirname, ".."),
        globalStorageUri: {
            fsPath: path.join(os.tmpdir(), "monocon-native-service-timeout-test")
        }
    };
    const service = new NativeService(context, { WorkerClass: SlowWorker });
    await service.warmup();

    await assert.rejects(service.request("upload", {}, 10), /timed out/);
    await assert.rejects(service.request("upload"), /still running/);
    assert.equal(service.worker.terminated, false);

    service.worker.complete();
    const next = service.request("ping");
    await new Promise(resolve => setImmediate(resolve));
    service.worker.complete();
    assert.equal((await next).success, true);
    service.dispose();
});

test("terminates a native worker that never finishes startup", async () => {
    class HungWorker extends EventEmitter {
        constructor() {
            super();
            this.terminated = false;
        }

        terminate() {
            this.terminated = true;
            return Promise.resolve(0);
        }
    }
    const context = {
        extensionPath: path.resolve(__dirname, ".."),
        globalStorageUri: {
            fsPath: path.join(os.tmpdir(), "monocon-native-service-startup-timeout")
        }
    };
    const service = new NativeService(context, {
        WorkerClass: HungWorker,
        startupTimeoutMs: 10
    });
    const startup = service.warmup();
    const worker = service.worker;

    await assert.rejects(startup, /did not start/);
    assert.equal(worker.terminated, true);
    assert.equal(service.worker, undefined);
    service.dispose();
});

test("retries after a transient worker construction failure", async () => {
    class InitiallyBrokenWorker extends FakeWorker {
        static attempts = 0;

        constructor() {
            if (InitiallyBrokenWorker.attempts++ === 0) {
                throw new Error("simulated transient constructor failure");
            }
            super();
        }
    }
    const context = {
        extensionPath: path.resolve(__dirname, ".."),
        globalStorageUri: {
            fsPath: path.join(
                os.tmpdir(),
                "monocon-native-service-constructor-retry"
            )
        }
    };
    const service = new NativeService(context, {
        WorkerClass: InitiallyBrokenWorker
    });

    await assert.rejects(service.warmup(), /transient constructor failure/);
    const ready = await service.warmup();
    assert.equal(ready.success, true);
    assert.equal(InitiallyBrokenWorker.attempts, 2);
    service.dispose();
});

test("ignores late events from an obsolete worker after restart", async () => {
    class ControlledWorker extends EventEmitter {
        static instances = [];

        constructor() {
            super();
            this.terminated = false;
            ControlledWorker.instances.push(this);
            setImmediate(() => this.emit("message", {
                type: "ready",
                result: { success: true, version: "test" }
            }));
        }

        postMessage(message) {
            this.lastMessage = message;
        }

        complete() {
            this.emit("message", {
                type: "response",
                id: this.lastMessage.id,
                result: { success: true, generation: ControlledWorker.instances.indexOf(this) }
            });
        }

        terminate() {
            this.terminated = true;
            return Promise.resolve(0);
        }
    }

    const context = {
        extensionPath: path.resolve(__dirname, ".."),
        globalStorageUri: {
            fsPath: path.join(os.tmpdir(), "monocon-native-service-restart-test")
        }
    };
    const service = new NativeService(context, {
        WorkerClass: ControlledWorker
    });
    await service.warmup();
    const obsoleteWorker = service.worker;

    obsoleteWorker.emit("error", new Error("simulated worker failure"));
    assert.equal(service.worker, undefined);

    const restartedRequest = service.request("ping");
    await new Promise(resolve => setImmediate(resolve));
    await new Promise(resolve => setImmediate(resolve));
    const currentWorker = service.worker;
    assert.notEqual(currentWorker, obsoleteWorker);
    assert.equal(service.pending.size, 1);

    // Node may deliver exit/message after the earlier error event. Those events
    // belong to the obsolete generation and must not reject the new request.
    obsoleteWorker.emit("exit", 1);
    obsoleteWorker.emit("message", {
        type: "response",
        id: currentWorker.lastMessage.id,
        error: "stale response"
    });
    assert.equal(service.pending.size, 1);

    currentWorker.complete();
    assert.deepEqual(await restartedRequest, {
        success: true,
        generation: 1
    });
    service.dispose();
});

test("never silently downgrades to an older native engine", async t => {
    const temporaryRoot = fs.mkdtempSync(
        path.join(os.tmpdir(), "monocon-native-no-downgrade-")
    );
    t.after(() => fs.rmSync(temporaryRoot, { recursive: true, force: true }));
    const nativeDirectory = path.join(
        temporaryRoot,
        "native",
        `${process.platform}-${process.arch}`
    );
    fs.mkdirSync(nativeDirectory, { recursive: true });
    fs.writeFileSync(
        path.join(nativeDirectory, "monocon_native_v160.node"),
        "obsolete engine must not be loaded"
    );
    const service = new NativeService({
        extensionPath: temporaryRoot,
        globalStorageUri: {
            fsPath: path.join(temporaryRoot, "cache")
        }
    });

    assert.equal(
        service.addonPath,
        path.join(nativeDirectory, "monocon_native_v170.node")
    );
    await assert.rejects(service.warmup(), /Native addon not found/);
    service.dispose();
});
