"use strict";

const assert = require("node:assert/strict");
const { EventEmitter } = require("node:events");
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
