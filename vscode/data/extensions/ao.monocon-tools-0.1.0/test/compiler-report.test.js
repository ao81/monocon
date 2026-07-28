"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");

const {
    stripAnsi,
    parseCompilerOutput,
    createCompilerReport
} = require("../out/compiler-report");

const SAMPLE = [
    "mon2.ino: In function 'void userLoop()':",
    "\x1b[1;37mmon2.ino\x1b[0m:\x1b[1;96m4\x1b[0m:\x1b[1;37m2\x1b[0m: \x1b[1;31merror:\x1b[0m 'a' was not declared in this scope",
    "\x1b[1;37mmon2.ino\x1b[0m:\x1b[1;96m4\x1b[0m:\x1b[1;37m2\x1b[0m: \x1b[1;32mnote:\x1b[0m suggested alternative: 'ar'"
].join("\n");

test("removes terminal color escape sequences", () => {
    const clean = stripAnsi(SAMPLE);
    assert.doesNotMatch(clean, /\x1b/);
    assert.match(clean, /mon2\.ino:4:2: error:/);
});

test("parses compiler locations and translates common diagnostics", () => {
    const parsed = parseCompilerOutput(SAMPLE, "C:\\sketch");

    assert.equal(parsed.diagnostics.length, 2);
    assert.equal(parsed.diagnostics[0].file, "mon2.ino");
    assert.equal(parsed.diagnostics[0].line, 4);
    assert.equal(parsed.diagnostics[0].column, 2);
    assert.equal(parsed.diagnostics[0].message, "「a」が宣言されていません。");
    assert.equal(parsed.diagnostics[1].suggestion, "ar");
    assert.equal(parsed.contexts[0].functionName, "void userLoop()");
});

test("creates a readable Japanese compiler report without ANSI codes", () => {
    const report = createCompilerReport(SAMPLE, "C:\\sketch");

    assert.equal(report.errors.length, 1);
    assert.match(report.text, /Arduino コンパイル失敗/);
    assert.match(report.text, /mon2\.ino  4行 2列/);
    assert.match(report.text, /内容     : 「a」が宣言されていません。/);
    assert.doesNotMatch(report.text, /確認事項/);
    assert.match(report.text, /候補     : 「ar」/);
    assert.match(report.text, /C:\\sketch\\mon2\.ino:4:2/);
    assert.doesNotMatch(report.text, /\x1b/);
});
