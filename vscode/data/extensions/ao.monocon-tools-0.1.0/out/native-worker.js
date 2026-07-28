"use strict";

const { parentPort, workerData } = require("node:worker_threads");

if (!parentPort) {
    throw new Error("Monocon native worker requires a parent port");
}

let addon;

function invoke(method, params = {}) {
    const response = addon.invoke(JSON.stringify({ method, params }));
    return JSON.parse(response);
}

try {
    addon = require(workerData.addonPath);
    const initialized = invoke("initialize", {
        extensionRoot: workerData.extensionRoot,
        cacheRoot: workerData.cacheRoot
    });
    if (!initialized.success) {
        throw new Error(initialized.errorMessage || "Native initialization failed");
    }
    parentPort.postMessage({ type: "ready", result: initialized });
}
catch (error) {
    parentPort.postMessage({
        type: "ready",
        error: error instanceof Error ? error.message : String(error)
    });
}

parentPort.on("message", message => {
    if (!addon || !message || typeof message.id !== "number") {
        return;
    }
    try {
        parentPort.postMessage({
            type: "response",
            id: message.id,
            result: invoke(message.method, message.params)
        });
    }
    catch (error) {
        parentPort.postMessage({
            type: "response",
            id: message.id,
            error: error instanceof Error ? error.message : String(error)
        });
    }
});
