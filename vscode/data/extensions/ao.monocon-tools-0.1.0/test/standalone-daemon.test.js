"use strict";

const assert = require("node:assert/strict");
const { spawnSync } = require("node:child_process");
const fs = require("node:fs");
const net = require("node:net");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");

const extensionRoot = path.resolve(__dirname, "..");
const daemonBin = path.resolve(
    extensionRoot,
    "..",
    "..",
    "daemon",
    "build",
    "bin"
);
const cliPath = path.join(daemonBin, "arduino-build-cli.exe");
const daemonPath = path.join(daemonBin, "arduino-build-daemon.exe");
const portableTasksPath = path.resolve(
    extensionRoot,
    "..",
    "..",
    "user-data",
    "User",
    "tasks.json"
);
const supported = process.platform === "win32"
    && process.arch === "x64"
    && fs.existsSync(cliPath)
    && fs.existsSync(daemonPath);

function runCli(args, environment, timeout = 30000) {
    return spawnSync(cliPath, args, {
        encoding: "utf8",
        env: environment,
        maxBuffer: 4 * 1024 * 1024,
        timeout,
        windowsHide: true
    });
}

function diagnostic(result) {
    return [
        result.error ? String(result.error.stack || result.error) : "",
        `exit=${String(result.status)} signal=${String(result.signal)}`,
        result.stdout || "",
        result.stderr || ""
    ].filter(Boolean).join("\n");
}

function currentUserSid() {
    const result = spawnSync(
        path.join(process.env.SystemRoot || "C:\\Windows", "System32", "whoami.exe"),
        ["/user", "/fo", "csv", "/nh"],
        { encoding: "utf8", windowsHide: true }
    );
    const match = result.stdout?.match(/\bS-\d+(?:-\d+)+\b/i);
    assert.ok(match, diagnostic(result));
    return match[0];
}

function sendRawRpc(pipeName, body) {
    return new Promise((resolve, reject) => {
        const chunks = [];
        const socket = net.connect(pipeName);
        let transportError;
        const timeout = setTimeout(() => {
            socket.destroy();
            reject(new Error("Raw daemon RPC timed out"));
        }, 5000);
        socket.on("connect", () => {
            socket.write(
                `Content-Length: ${Buffer.byteLength(body)}\r\n\r\n${body}`
            );
        });
        socket.on("data", chunk => chunks.push(chunk));
        socket.on("error", error => {
            // Windows named pipes report the server's normal DisconnectNamedPipe
            // as EPIPE after all response bytes have already arrived.
            if (error.code !== "EPIPE") transportError = error;
        });
        socket.on("close", () => {
            clearTimeout(timeout);
            if (transportError) reject(transportError);
            else resolve(Buffer.concat(chunks).toString("utf8"));
        });
    });
}

test("portable upload task forwards the configured COM port", () => {
    const tasks = JSON.parse(fs.readFileSync(portableTasksPath, "utf8"));
    const uploadTask = tasks.tasks.find(task => task.label === "Arduino: Upload");
    assert.ok(uploadTask);
    assert.equal(
        uploadTask.command,
        "${execPath}/../data/daemon/build/bin/arduino-build-cli.exe"
    );
    assert.deepEqual(uploadTask.args, [
        "upload",
        "${fileDirname}",
        "${config:monoconTools.upload.port}",
        "--workspace",
        "${workspaceFolder}"
    ]);
});

test("standalone compatibility daemon is isolated, offline, and Unicode-safe", {
    skip: supported ? false : "Windows x64 standalone daemon is not available",
    timeout: 60000
}, async t => {
    const temporaryRoot = fs.mkdtempSync(
        path.join(os.tmpdir(), "monocon-standalone-test-")
    );
    const instance = `test-${process.pid}-${Date.now().toString(36)}`;
    const environment = {
        ...process.env,
        MONOCON_DAEMON_INSTANCE: instance
    };

    t.after(() => {
        runCli(["shutdown"], environment, 10000);
        fs.rmSync(temporaryRoot, { recursive: true, force: true });
    });

    const ping = runCli(["ping"], environment);
    assert.equal(ping.status, 0, diagnostic(ping));
    assert.match(ping.stdout, /Daemon version: 1\.7\.0\b/);
    assert.match(ping.stdout, /Resources: bundled\b/);

    const pipeName = `\\\\.\\pipe\\arduino-build-v170-${currentUserSid()}-${instance}`;
    const invalidResponse = await sendRawRpc(pipeName, "[]");
    const separator = invalidResponse.indexOf("\r\n\r\n");
    assert.notEqual(separator, -1, invalidResponse);
    const invalidJson = JSON.parse(invalidResponse.slice(separator + 4));
    assert.equal(invalidJson.error.code, -32600);

    const sketchName = `互換 経路 🚀${String.fromCodePoint(0x20000)}`;
    const sketchDir = path.join(temporaryRoot, sketchName);
    fs.mkdirSync(sketchDir);
    fs.writeFileSync(
        path.join(sketchDir, `${sketchName}.ino`),
        [
            "#include <Wire.h>",
            "void setup() {",
            "  Wire.begin();",
            "  consume(readValue());",
            "}",
            "void loop() {}",
            "int readValue(int value = 7) { return value; }",
            "void consume(int value) { (void)value; }",
            ""
        ].join("\n")
    );

    const buildArguments = [
        "build",
        sketchDir,
        "--workspace",
        temporaryRoot
    ];
    const first = runCli(buildArguments, environment);
    assert.equal(first.status, 0, diagnostic(first));
    assert.match(first.stdout, /Compile \(diff \d+\/\d+ files\)/);

    const cached = runCli(buildArguments, environment);
    assert.equal(cached.status, 0, diagnostic(cached));
    assert.match(cached.stdout, /Compile \(cached\)/);

    const shutdown = runCli(["shutdown"], environment, 10000);
    assert.equal(shutdown.status, 0, diagnostic(shutdown));
});
