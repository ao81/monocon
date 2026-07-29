"use strict";

const assert = require("node:assert/strict");
const { spawn } = require("node:child_process");
const crypto = require("node:crypto");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");

const extensionRoot = path.resolve(__dirname, "..");
const addonPath = path.join(
    extensionRoot,
    "native",
    "win32-x64",
    "monocon_native_v170.node"
);
const supported = process.platform === "win32"
    && process.arch === "x64"
    && fs.existsSync(addonPath);

async function holdNamedMutex(key) {
    const mutexName = "Local\\Monocon_"
        + crypto.createHash("sha1").update(key).digest("hex").slice(0, 32);
    const script = [
        `$mutex = [System.Threading.Mutex]::new($false, '${mutexName}')`,
        "try {",
        "  if (-not $mutex.WaitOne(5000)) { exit 2 }",
        "  [Console]::Out.WriteLine('READY')",
        "  [Console]::Out.Flush()",
        "  [Console]::In.ReadLine() | Out-Null",
        "}",
        "finally {",
        "  try { $mutex.ReleaseMutex() } catch {}",
        "  $mutex.Dispose()",
        "}"
    ].join("\n");
    const child = spawn(
        "powershell.exe",
        ["-NoProfile", "-NonInteractive", "-Command", script],
        { stdio: ["pipe", "pipe", "pipe"], windowsHide: true }
    );
    let stderr = "";
    child.stderr.on("data", chunk => {
        stderr += String(chunk);
    });
    await new Promise((resolve, reject) => {
        const timeout = setTimeout(() => {
            child.kill();
            reject(new Error(`Named mutex helper timed out\n${stderr}`));
        }, 10000);
        child.stdout.on("data", chunk => {
            if (String(chunk).includes("READY")) {
                clearTimeout(timeout);
                resolve();
            }
        });
        child.on("error", error => {
            clearTimeout(timeout);
            reject(error);
        });
        child.on("exit", code => {
            if (code !== null && code !== 0) {
                clearTimeout(timeout);
                reject(new Error(
                    `Named mutex helper exited with ${code}\n${stderr}`
                ));
            }
        });
    });
    return async () => {
        child.stdin.end("\n");
        await new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                child.kill();
                reject(new Error("Named mutex helper did not exit"));
            }, 5000);
            child.once("exit", code => {
                clearTimeout(timeout);
                if (code === 0) resolve();
                else reject(new Error(
                    `Named mutex helper exited with ${code}\n${stderr}`
                ));
            });
        });
    };
}

test("native builder preserves Arduino semantics and cache integrity", {
    skip: supported ? false : "Windows x64 native addon is not available"
}, async t => {
    const addon = require(addonPath);
    const temporaryRoot = fs.mkdtempSync(
        path.join(os.tmpdir(), "monocon-native-builder-test-")
    );
    t.after(() => fs.rmSync(temporaryRoot, { recursive: true, force: true }));

    const invoke = (method, params = {}) => JSON.parse(addon.invoke(JSON.stringify({
        method,
        params
    })));
    const cacheRoot = path.join(temporaryRoot, "日本語 cache 🚀");
    const initialized = invoke("initialize", {
        extensionRoot,
        cacheRoot
    });
    assert.equal(initialized.success, true);
    assert.equal(initialized.version, "1.7.0");

    const sketch = path.join(temporaryRoot, "課題 1");
    fs.mkdirSync(sketch);
    fs.writeFileSync(
        path.join(sketch, "課題 1.ino"),
        [
            "struct Reading { int value; };",
            "void* rawPointer = 0;",
            "byte* permissivePointer = rawPointer;",
            "void setup() { readValue(); }",
            "void loop() {}",
            "Reading readValue(int value = 1) { return {value}; }",
            ""
        ].join("\n")
    );
    const first = invoke("compile", {
        sketchDir: sketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(first.success, true, first.compilerOutput || first.errorMessage);
    const aliasedSketchPath = `${sketch}${path.sep}..${path.sep}${path.basename(sketch)}`;
    const aliasedBuild = invoke("compile", {
        sketchDir: aliasedSketchPath,
        workspaceDir: temporaryRoot
    });
    assert.equal(aliasedBuild.success, true);
    assert.equal(aliasedBuild.cached, true);
    assert.equal(aliasedBuild.hexFile, first.hexFile);

    const releaseUploadMutex = await holdNamedMutex("upload-port:COM9999");
    try {
        const concurrentUpload = invoke("upload", {
            sketchDir: sketch,
            workspaceDir: temporaryRoot,
            port: "com9999"
        });
        assert.equal(concurrentUpload.success, false);
        assert.equal(concurrentUpload.port, "COM9999");
        assert.match(
            concurrentUpload.errorMessage,
            /exclusive upload access for COM9999/
        );
        assert.equal(concurrentUpload.compile.success, false);
    }
    finally {
        await releaseUploadMutex();
    }

    const multiSketch = path.join(temporaryRoot, "zmain");
    fs.mkdirSync(multiSketch);
    fs.writeFileSync(
        path.join(multiSketch, "zmain.ino"),
        "const int sharedValue = 7;\nvoid setup() { helper(); }\nvoid loop() {}\n"
    );
    fs.writeFileSync(
        path.join(multiSketch, "aaa.ino"),
        "int helper() { return sharedValue; }\n"
    );
    const multiResult = invoke("compile", {
        sketchDir: multiSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(
        multiResult.success,
        true,
        multiResult.compilerOutput || multiResult.errorMessage
    );

    const advancedName = `🚀高度한글${String.fromCodePoint(0x20000)}`;
    const advancedSketch = path.join(temporaryRoot, advancedName);
    const advancedSource = path.join(advancedSketch, "src", "内部🚀");
    fs.mkdirSync(advancedSource, { recursive: true });
    fs.writeFileSync(
        path.join(advancedSource, "補助🚀.h"),
        "\uFEFF#pragma once\n#define FEATURE_FROM_HEADER 1\n"
            + "int helper();\nextern \"C\" int asm_helper();\n"
    );
    fs.writeFileSync(
        path.join(advancedSource, "補助🚀.cpp"),
        "#include \"補助🚀.h\"\nint helper() { return 4; }\n"
    );
    fs.writeFileSync(
        path.join(advancedSource, "helper.S"),
        [
            ".global asm_helper",
            ".type asm_helper, @function",
            "asm_helper:",
            "  ldi r24, 7",
            "  ldi r25, 0",
            "  ret",
            ""
        ].join("\n")
    );
    fs.writeFileSync(
        path.join(advancedSketch, `${advancedName}.ino`),
        [
            "\uFEFF#include <Wire.h>",
            "#include <src/内部🚀/補助🚀.h>",
            "#if 0",
            "#include <SPI.h>",
            "Unknown disabled() { return {}; }",
            "#endif",
            "#define EXECUTE(body) \\",
            "  do { body; } while (0)",
            "#define ENABLED(value) ((value) > 0)",
            "const char* rawText = R\"TAG(a \" quoted { text)TAG\";",
            "struct Number { int value; };",
            "void setup() {",
            "  Wire.begin();",
            "  Number sum = Number{1} + Number{2};",
            "  int value = helper() + asm_helper() + cFunction() + later()"
                + " + conditional() + headerConditional();",
            "  EXECUTE(value += sum.value);",
            "}",
            "void loop() {}",
            "Number operator+(Number left, Number right) { return {left.value + right.value}; }",
            "extern \"C\" { int cFunction() { return rawText[0] != 0; } }",
            "int later(int value = 1) { return value; }",
            "#if ENABLED(1)",
            "int conditional() { return 3; }",
            "#endif",
            "#if FEATURE_FROM_HEADER",
            "int headerConditional() { return 5; }",
            "#endif",
            ""
        ].join("\n")
    );
    const advanced = invoke("compile", {
        sketchDir: advancedSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(
        advanced.success,
        true,
        advanced.compilerOutput || advanced.errorMessage
    );
    assert.equal(advanced.totalFiles, 5);
    const advancedCached = invoke("compile", {
        sketchDir: advancedSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(advancedCached.success, true);
    assert.equal(advancedCached.cached, true);

    const standardLibrarySketch = path.join(
        temporaryRoot,
        "公式標準ライブラリ"
    );
    fs.mkdirSync(standardLibrarySketch);
    fs.writeFileSync(
        path.join(standardLibrarySketch, "公式標準ライブラリ.ino"),
        [
            "#include <Ethernet.h>",
            "#include <LiquidCrystal.h>",
            "#include <SD.h>",
            "#include <Servo.h>",
            "#include <Stepper.h>",
            "#include <TFT.h>",
            "EthernetClient networkClient;",
            "LiquidCrystal lcd(2, 3, 4, 5, 6, 7);",
            "Servo servo;",
            "Stepper stepper(200, 22, 23, 24, 25);",
            "TFT display(10, 9, 8);",
            "void setup() {",
            "  Ethernet.init(53);",
            "  lcd.begin(16, 2);",
            "  servo.attach(11);",
            "  stepper.setSpeed(60);",
            "  bool storageReady = SD.begin(53);",
            "  display.begin();",
            "  (void)storageReady;",
            "  (void)networkClient;",
            "}",
            "void loop() {}",
            ""
        ].join("\n")
    );
    const standardLibraries = invoke("compile", {
        sketchDir: standardLibrarySketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(
        standardLibraries.success,
        true,
        standardLibraries.compilerOutput || standardLibraries.errorMessage
    );
    assert.ok(standardLibraries.totalFiles > 10);
    const standardLibrariesCached = invoke("compile", {
        sketchDir: standardLibrarySketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(standardLibrariesCached.success, true);
    assert.equal(standardLibrariesCached.cached, true);

    const advancedHex = fs.readFileSync(advanced.hexFile);
    const makeHexRecord = (address, type, data = []) => {
        const bytes = [
            data.length,
            (address >> 8) & 0xff,
            address & 0xff,
            type,
            ...data
        ];
        const checksum = (-bytes.reduce((sum, byte) => sum + byte, 0)) & 0xff;
        return `:${[...bytes, checksum]
            .map(byte => byte.toString(16).padStart(2, "0").toUpperCase())
            .join("")}`;
    };
    const malformedHexCases = [
        {
            content: "garbage\n",
            expected: /record start/
        },
        {
            content: [
                makeHexRecord(0, 0x02),
                makeHexRecord(0, 0x01)
            ].join("\n") + "\n",
            expected: /segment record/
        },
        {
            content: [
                makeHexRecord(0, 0x04, [0xff, 0xff]),
                makeHexRecord(0xffff, 0x00, [0x00, 0x01]),
                makeHexRecord(0, 0x01)
            ].join("\n") + "\n",
            expected: /application region/
        },
        {
            content: [
                makeHexRecord(0, 0x04, [0x00, 0x03]),
                makeHexRecord(0xe000, 0x00, [0x00]),
                makeHexRecord(0, 0x01)
            ].join("\n") + "\n",
            expected: /bootloader is protected/
        },
        {
            content: [
                makeHexRecord(0, 0x00, [0x00]),
                makeHexRecord(0, 0x00, [0x00]),
                makeHexRecord(0, 0x01)
            ].join("\n") + "\n",
            expected: /Overlapping HEX data/
        },
        {
            content: [
                makeHexRecord(0, 0x00, [0x00]),
                makeHexRecord(0, 0x01),
                makeHexRecord(1, 0x00, [0x01])
            ].join("\n") + "\n",
            expected: /after HEX EOF/
        }
    ];
    for (const malformed of malformedHexCases) {
        fs.writeFileSync(advanced.hexFile, malformed.content);
        const upload = invoke("upload", {
            sketchDir: advancedSketch,
            workspaceDir: temporaryRoot,
            port: "COM999",
            skipCompile: true
        });
        assert.equal(upload.success, false);
        assert.match(upload.errorMessage, /^Hex parse failed:/);
        assert.match(upload.errorMessage, malformed.expected);
    }
    fs.writeFileSync(advanced.hexFile, "");
    fs.truncateSync(advanced.hexFile, 4 * 1024 * 1024 + 1);
    const oversizedHexUpload = invoke("upload", {
        sketchDir: advancedSketch,
        workspaceDir: temporaryRoot,
        port: "COM999",
        skipCompile: true
    });
    assert.equal(oversizedHexUpload.success, false);
    assert.match(oversizedHexUpload.errorMessage, /4 MiB safety limit/);
    fs.writeFileSync(advanced.hexFile, advancedHex);

    const cacheSketch = path.join(temporaryRoot, "課題 2");
    fs.mkdirSync(cacheSketch);
    const headerPath = path.join(cacheSketch, "config.h");
    const inoPath = path.join(cacheSketch, "課題 2.ino");
    fs.writeFileSync(headerPath, "#define VALUE 1\n");
    fs.writeFileSync(
        inoPath,
        "#include \"config.h\"\nvoid setup() {}\nvoid loop() { int x = VALUE; (void)x; }\n"
    );
    const compileCacheSketch = () => invoke("compile", {
        sketchDir: cacheSketch,
        workspaceDir: temporaryRoot
    });

    assert.equal(compileCacheSketch().success, true);
    const cached = compileCacheSketch();
    assert.equal(cached.success, true);
    assert.equal(cached.cached, true);
    assert.equal(cached.recompiledFiles, 0);

    const sourceStat = fs.statSync(inoPath);
    const originalSource = fs.readFileSync(inoPath, "utf8");
    const changedSource = originalSource.replace("int x", "int y")
        .replace("(void)x", "(void)y");
    assert.equal(changedSource.length, originalSource.length);
    fs.writeFileSync(inoPath, changedSource);
    fs.utimesSync(inoPath, sourceStat.atime, sourceStat.mtime);
    const sourceChanged = compileCacheSketch();
    assert.equal(sourceChanged.success, true);
    assert.equal(sourceChanged.cached, false);
    assert.equal(sourceChanged.recompiledFiles, 1);

    const headerStat = fs.statSync(headerPath);
    fs.writeFileSync(headerPath, "#define VALUE 2\n");
    fs.utimesSync(headerPath, headerStat.atime, headerStat.mtime);
    const headerChanged = compileCacheSketch();
    assert.equal(headerChanged.success, true);
    assert.equal(headerChanged.cached, false);
    assert.equal(headerChanged.recompiledFiles, 1);
    assert.equal(compileCacheSketch().cached, true);

    const buildDir = path.dirname(headerChanged.elfFile);
    const hexPath = headerChanged.hexFile;
    const originalHex = fs.readFileSync(hexPath);
    const hexStat = fs.statSync(hexPath);
    const corruptedHex = Buffer.from(originalHex);
    corruptedHex[Math.floor(corruptedHex.length / 2)] ^= 0x01;
    fs.writeFileSync(hexPath, corruptedHex);
    fs.utimesSync(hexPath, hexStat.atime, hexStat.mtime);
    const repairedArtifact = compileCacheSketch();
    assert.equal(repairedArtifact.success, true);
    assert.equal(repairedArtifact.cached, false);
    assert.equal(repairedArtifact.recompiledFiles, 0);
    assert.deepEqual(fs.readFileSync(hexPath), originalHex);

    const ambiguousHex = [
        makeHexRecord(0, 0x00, [0x00]),
        makeHexRecord(0, 0x00, [0x00]),
        makeHexRecord(0, 0x01)
    ].join("\n") + "\n";
    fs.writeFileSync(hexPath, ambiguousHex);
    const sha1File = filePath => crypto.createHash("sha1")
        .update(fs.readFileSync(filePath))
        .digest("hex");
    const forgedArtifactSignature = crypto.createHash("sha1")
        .update(`elf${sha1File(headerChanged.elfFile)}\nhex${sha1File(hexPath)}`)
        .digest("hex");
    fs.writeFileSync(
        path.join(buildDir, ".artifact-signature"),
        `${forgedArtifactSignature}\n`
    );
    const repairedForgedArtifact = compileCacheSketch();
    assert.equal(repairedForgedArtifact.success, true);
    assert.equal(repairedForgedArtifact.cached, false);
    assert.equal(repairedForgedArtifact.recompiledFiles, 0);
    assert.deepEqual(fs.readFileSync(hexPath), originalHex);

    const objectPath = fs.readdirSync(buildDir)
        .filter(fileName => fileName.endsWith(".o"))
        .map(fileName => path.join(buildDir, fileName))
        .find(candidate => fs.statSync(candidate).isFile());
    assert.ok(objectPath, "compiled object file should exist");
    const objectStat = fs.statSync(objectPath);
    const corruptedObject = fs.readFileSync(objectPath);
    corruptedObject[Math.floor(corruptedObject.length / 2)] ^= 0x01;
    fs.writeFileSync(objectPath, corruptedObject);
    fs.utimesSync(objectPath, objectStat.atime, objectStat.mtime);
    const repairedObject = compileCacheSketch();
    assert.equal(repairedObject.success, true);
    assert.equal(repairedObject.cached, false);
    assert.equal(repairedObject.recompiledFiles, 1);
    assert.equal(compileCacheSketch().cached, true);

    const validSource = fs.readFileSync(inoPath, "utf8");
    fs.writeFileSync(inoPath, validSource.replace("void loop()", "void loop("));
    const failed = compileCacheSketch();
    assert.equal(failed.success, false);
    assert.match(failed.compilerOutput, /課題 2\.ino/);
    assert.equal(compileCacheSketch().success, false);

    fs.writeFileSync(inoPath, validSource);
    const recovered = compileCacheSketch();
    assert.equal(recovered.success, true);
    assert.equal(recovered.cached, false);
    assert.equal(compileCacheSketch().cached, true);

    const stampPath = path.join(buildDir, ".stamps");
    fs.writeFileSync(stampPath, "");
    fs.truncateSync(stampPath, 8 * 1024 * 1024 + 1);
    const repairedOversizedMetadata = compileCacheSketch();
    assert.equal(repairedOversizedMetadata.success, true);
    assert.equal(repairedOversizedMetadata.cached, false);
    assert.ok(fs.statSync(stampPath).size < 8 * 1024 * 1024);
    assert.equal(compileCacheSketch().cached, true);

    const coreCacheDirectory = path.join(
        cacheRoot,
        "cores",
        fs.readdirSync(path.join(cacheRoot, "cores"))[0]
    );
    const cachedCore = path.join(coreCacheDirectory, "core.a");
    const validCore = fs.readFileSync(cachedCore);
    const corruptedCore = Buffer.from(validCore);
    corruptedCore[Math.floor(corruptedCore.length / 2)] ^= 0x01;
    const cachedCoreStat = fs.statSync(cachedCore);
    fs.writeFileSync(cachedCore, corruptedCore);
    fs.utimesSync(cachedCore, cachedCoreStat.atime, cachedCoreStat.mtime);
    const repairedCore = compileCacheSketch();
    assert.equal(repairedCore.success, true);
    assert.deepEqual(fs.readFileSync(cachedCore), validCore);

    const staleSketch = path.join(temporaryRoot, "名前変更");
    const staleSourceDir = path.join(staleSketch, "src");
    fs.mkdirSync(staleSourceDir, { recursive: true });
    fs.writeFileSync(
        path.join(staleSketch, "名前変更.ino"),
        "void setup() {}\nvoid loop() {}\n"
    );
    const oldSource = path.join(staleSourceDir, "old.cpp");
    const newSource = path.join(staleSourceDir, "new.cpp");
    fs.writeFileSync(oldSource, "int renamedSource() { return 1; }\n");
    const staleFirst = invoke("compile", {
        sketchDir: staleSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(staleFirst.success, true);
    const staleBuildDir = path.dirname(staleFirst.hexFile);
    const oldStageFiles = new Set(fs.readdirSync(path.join(staleBuildDir, "staged")));
    const oldObject = fs.readdirSync(staleBuildDir)
        .find(name => /^cpp_[0-9a-f]{16}\.o$/.test(name));
    assert.ok(oldObject);
    fs.renameSync(oldSource, newSource);
    const staleSecond = invoke("compile", {
        sketchDir: staleSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(staleSecond.success, true);
    const newStageFiles = fs.readdirSync(path.join(staleBuildDir, "staged"));
    assert.equal(newStageFiles.length, oldStageFiles.size);
    assert.equal(
        newStageFiles.some(name => !oldStageFiles.has(name)),
        true
    );
    assert.equal(fs.existsSync(path.join(staleBuildDir, oldObject)), false);
    assert.equal(
        fs.readdirSync(staleBuildDir)
            .filter(name => /^cpp_[0-9a-f]{16}\.o$/.test(name)).length,
        1
    );

    const oversizedSketch = path.join(temporaryRoot, "巨大入力");
    const oversizedSourceDir = path.join(oversizedSketch, "src");
    fs.mkdirSync(oversizedSourceDir, { recursive: true });
    fs.writeFileSync(
        path.join(oversizedSketch, "巨大入力.ino"),
        "void setup() {}\nvoid loop() {}\n"
    );
    const oversizedAsset = path.join(oversizedSourceDir, "accidental-video.bin");
    fs.writeFileSync(oversizedAsset, "");
    fs.truncateSync(oversizedAsset, 32 * 1024 * 1024 + 1);
    const oversized = invoke("compile", {
        sketchDir: oversizedSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(oversized.success, false);
    assert.match(oversized.errorMessage, /exceeds 32 MiB limit/);
    assert.equal(compileCacheSketch().success, true);

    const adversarialSketch = path.join(temporaryRoot, "構文耐性");
    fs.mkdirSync(adversarialSketch);
    const adversarialIno = path.join(adversarialSketch, "構文耐性.ino");
    const macroChain = Array.from(
        { length: 80 },
        (_, index) => `#define M${index} ${index === 79 ? "1" : `M${index + 1}`}`
    ).join("\n");
    const adversarialSources = [
        [
            `#if ${"(".repeat(512)}1${")".repeat(512)}`,
            "void setup() {}",
            "#else",
            "void setup() {}",
            "#endif",
            "void loop() {}",
            ""
        ].join("\n"),
        [
            `#if ${"!".repeat(512)}1`,
            "void setup() {}",
            "#else",
            "void setup() {}",
            "#endif",
            "void loop() {}",
            ""
        ].join("\n"),
        [
            macroChain,
            "#if M0",
            "void setup() {}",
            "#endif",
            "void loop() {}",
            ""
        ].join("\n"),
        [
            "#define F(x) G(x)",
            "#define G(x) F(x)",
            "#if F(1)",
            "int disabled;",
            "#endif",
            "void setup() {}",
            "void loop() {}",
            ""
        ].join("\n"),
        "void setup() { const char* value = R\"tag(unterminated; }\nvoid loop() {}\n",
        Buffer.from([
            ...Buffer.from("void setup() {}\nvoid loop() {}\n"),
            0xff,
            0xfe
        ])
    ];
    for (const source of adversarialSources) {
        fs.writeFileSync(adversarialIno, source);
        const result = invoke("compile", {
            sketchDir: adversarialSketch,
            workspaceDir: temporaryRoot
        });
        assert.equal(typeof result.success, "boolean");
        assert.doesNotMatch(result.errorMessage || "", /^Native exception:/);
        assert.equal(invoke("ping").success, true);
    }
    fs.writeFileSync(
        adversarialIno,
        "void setup() {}\nvoid loop() {}\n"
    );
    const adversarialRecovered = invoke("compile", {
        sketchDir: adversarialSketch,
        workspaceDir: temporaryRoot
    });
    assert.equal(adversarialRecovered.success, true);

    const buildCacheRoot = path.join(cacheRoot, "build");
    const oldTime = new Date(Date.now() - 40 * 24 * 60 * 60 * 1000);
    for (let index = 0; index < 66; index += 1) {
        const cacheName = `sketch_${(0xff00000000000000n + BigInt(index))
            .toString(16)}`;
        const fakeCache = path.join(buildCacheRoot, cacheName);
        fs.mkdirSync(fakeCache, { recursive: true });
        const marker = path.join(fakeCache, ".last-used");
        fs.writeFileSync(marker, "old\n");
        fs.utimesSync(marker, oldTime, oldTime);
    }
    const unmanagedCache = path.join(buildCacheRoot, "do-not-delete");
    fs.mkdirSync(unmanagedCache);
    for (let index = 0; index < 16; index += 1) {
        const result = compileCacheSketch();
        assert.equal(result.success, true);
        assert.equal(result.cached, true);
    }
    const managedCaches = fs.readdirSync(buildCacheRoot)
        .filter(name => /^sketch_[0-9a-f]{16}$/.test(name));
    assert.ok(managedCaches.length <= 64);
    assert.equal(fs.existsSync(unmanagedCache), true);

    const concurrentSketch = path.join(temporaryRoot, "同時ビルド");
    const concurrentCache = path.join(temporaryRoot, "同時 cache");
    fs.mkdirSync(concurrentSketch);
    fs.writeFileSync(
        path.join(concurrentSketch, "同時ビルド.ino"),
        "void setup() {}\nvoid loop() {}\n"
    );
    for (let index = 0; index < 12; index += 1) {
        fs.writeFileSync(
            path.join(concurrentSketch, `source-${index}.cpp`),
            `#include <Arduino.h>\nint helper${index}() { return ${index}; }\n`
        );
    }
    const childSource = [
        "const addon=require(process.argv[1]);",
        "const invoke=(method,params)=>JSON.parse(addon.invoke(JSON.stringify({method,params})));",
        "const initialized=invoke('initialize',{extensionRoot:process.argv[2],cacheRoot:process.argv[3]});",
        "if(!initialized.success)throw new Error(initialized.errorMessage);",
        "process.stdout.write(JSON.stringify(invoke('compile',{sketchDir:process.argv[4],workspaceDir:process.argv[5]})));"
    ].join("");
    const compileInChild = () => new Promise((resolve, reject) => {
        const child = spawn(process.execPath, [
            "-e",
            childSource,
            addonPath,
            extensionRoot,
            concurrentCache,
            concurrentSketch,
            temporaryRoot
        ], {
            windowsHide: true,
            stdio: ["ignore", "pipe", "pipe"]
        });
        let stdout = "";
        let stderr = "";
        child.stdout.setEncoding("utf8");
        child.stderr.setEncoding("utf8");
        child.stdout.on("data", chunk => { stdout += chunk; });
        child.stderr.on("data", chunk => { stderr += chunk; });
        child.once("error", reject);
        child.once("close", code => {
            if (code !== 0) {
                reject(new Error(`concurrent build exited ${code}: ${stderr}`));
                return;
            }
            try {
                resolve(JSON.parse(stdout));
            } catch (error) {
                reject(new Error(`invalid concurrent build response: ${stdout}`, {
                    cause: error
                }));
            }
        });
    });
    const concurrentResults = await Promise.all([
        compileInChild(),
        compileInChild()
    ]);
    for (const result of concurrentResults) {
        assert.equal(result.success, true, result.compilerOutput || result.errorMessage);
    }
    assert.deepEqual(
        concurrentResults.map(result => result.cached).sort(),
        [false, true]
    );
});
