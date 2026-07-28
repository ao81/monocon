"use strict";

const path = require("node:path");

const ANSI_ESCAPE = /\x1B\[[0-?]*[ -/]*[@-~]/g;

function stripAnsi(value) {
    return String(value || "").replace(ANSI_ESCAPE, "");
}

function translateDiagnostic(message) {
    let match;
    if ((match = message.match(/^'([^']+)' was not declared in this scope$/))) {
        return {
            message: `「${match[1]}」が宣言されていません。`,
            hint: "変数名のスペル、変数を宣言した位置、必要なヘッダーファイルを確認してください。"
        };
    }
    if ((match = message.match(/^suggested alternative: '([^']+)'$/))) {
        return {
            message: `似た名前の「${match[1]}」があります。`,
            hint: "入力した名前が正しいか確認してください。",
            suggestion: match[1]
        };
    }
    if ((match = message.match(/(?:fatal error: )?([^:]+): No such file or directory/))) {
        return {
            message: `ヘッダーファイル「${match[1]}」が見つかりません。`,
            hint: "スケッチと同じフォルダーにあるか、#includeの名前が一致しているか確認してください。"
        };
    }
    if (message.includes("expected ';'")) {
        return {
            message: "セミコロン「;」が必要です。",
            hint: "表示された行と、その1つ前の行の末尾を確認してください。"
        };
    }
    if (message.includes("expected '}'")) {
        return {
            message: "閉じ波括弧「}」が必要です。",
            hint: "if、for、関数などの「{」と「}」の数を確認してください。"
        };
    }
    if (message.includes("expected ')'")) {
        return {
            message: "閉じ丸括弧「)」が必要です。",
            hint: "関数呼び出しやif文の「(」と「)」の数を確認してください。"
        };
    }
    if ((match = message.match(/^redefinition of ['‘]([^'’]+)['’]/))) {
        return {
            message: `「${match[1]}」が二重に定義されています。`,
            hint: "同じ名前の変数や関数が複数ないか確認してください。"
        };
    }
    if ((match = message.match(/^['‘]([^'’]+)['’] does not name a type/))) {
        return {
            message: `「${match[1]}」は型名として認識できません。`,
            hint: "型名のスペルと、必要なヘッダーファイルを確認してください。"
        };
    }
    if (message.includes("no matching function for call to")) {
        return {
            message: "指定した引数で呼び出せる関数がありません。",
            hint: "関数名、引数の数、引数の型を確認してください。"
        };
    }
    if (message.includes("too few arguments")) {
        return {
            message: "関数に渡す引数が足りません。",
            hint: "関数の定義と呼び出し側の引数の数を確認してください。"
        };
    }
    if (message.includes("too many arguments")) {
        return {
            message: "関数に渡す引数が多すぎます。",
            hint: "関数の定義と呼び出し側の引数の数を確認してください。"
        };
    }
    if (message.includes("invalid conversion") || message.includes("cannot convert")) {
        return {
            message: "値の型を変換できません。",
            hint: "代入先や関数の引数が要求する型を確認してください。"
        };
    }
    if (message.includes("expected primary-expression")) {
        return {
            message: "式の書き方が正しくありません。",
            hint: "演算子、括弧、カンマの位置と、直前の行を確認してください。"
        };
    }
    return { message };
}

function normalizeFile(file) {
    const cleaned = file.trim();
    return cleaned.endsWith(".ino.cpp") ? cleaned.slice(0, -4) : cleaned;
}

function resolveDiagnosticPath(file, sketchDir) {
    const normalized = normalizeFile(file);
    return path.isAbsolute(normalized)
        ? path.normalize(normalized)
        : path.resolve(sketchDir || ".", normalized);
}

function parseCompilerOutput(output, sketchDir) {
    const cleanOutput = stripAnsi(output).replace(/\r/g, "");
    const diagnostics = [];
    const contexts = [];
    const unparsed = [];
    const diagnosticPattern = /^(.+):(\d+):(\d+):\s*(fatal error|error|warning|note):\s*(.+)$/;
    const diagnosticWithoutColumn = /^(.+):(\d+):\s*(fatal error|error|warning|note):\s*(.+)$/;
    const contextPattern = /^(.+): In (?:function|member function) ['‘](.+)['’]:$/;

    for (const rawLine of cleanOutput.split("\n")) {
        const line = rawLine.trimEnd();
        if (!line.trim()) continue;
        let match = line.match(diagnosticPattern);
        let column = 1;
        if (!match) {
            match = line.match(diagnosticWithoutColumn);
            if (match) {
                match = [match[0], match[1], match[2], String(column), match[3], match[4]];
            }
        }
        if (match) {
            const translated = translateDiagnostic(match[5]);
            diagnostics.push({
                file: normalizeFile(match[1]),
                absolutePath: resolveDiagnosticPath(match[1], sketchDir),
                line: Math.max(1, Number(match[2]) || 1),
                column: Math.max(1, Number(match[3]) || 1),
                severity: match[4] === "fatal error" ? "error" : match[4],
                originalMessage: match[5],
                message: translated.message,
                hint: translated.hint,
                suggestion: translated.suggestion
            });
            continue;
        }
        const context = line.match(contextPattern);
        if (context) {
            contexts.push({ file: normalizeFile(context[1]), functionName: context[2] });
            continue;
        }
        // ソース引用行やキャレットだけの行は、整理後の表示では重複するため省く。
        if (/^\s*[\^~]+\s*$/.test(line)) continue;
        unparsed.push(line);
    }
    return { cleanOutput, diagnostics, contexts, unparsed };
}

function severityMark(severity) {
    if (severity === "error") return "エラー";
    if (severity === "warning") return "警告";
    return "参考";
}

function createCompilerReport(output, sketchDir) {
    const parsed = parseCompilerOutput(output, sketchDir);
    const errors = parsed.diagnostics.filter(item => item.severity === "error");
    const warnings = parsed.diagnostics.filter(item => item.severity === "warning");
    const notes = parsed.diagnostics.filter(item => item.severity === "note");
    const lines = [];

    lines.push("");
    lines.push("============================================================");
    lines.push(` Arduino コンパイル失敗  エラー ${errors.length}件 / 警告 ${warnings.length}件`);
    lines.push("============================================================");

    if (parsed.contexts.length > 0) {
        const context = parsed.contexts[0];
        lines.push(`対象関数 : ${context.functionName}`);
        lines.push("");
    }

    const primary = [...errors, ...warnings];
    primary.forEach((item, index) => {
        lines.push(`[${severityMark(item.severity)} ${index + 1}] ${item.file}  ${item.line}行 ${item.column}列`);
        lines.push(`  内容     : ${item.message}`);
        lines.push(`  原文     : ${item.originalMessage}`);
        // VS Codeの出力パネルでクリックできる絶対パス形式。
        lines.push(`  場所     : ${item.absolutePath}:${item.line}:${item.column}`);

        const relatedNotes = notes.filter(note =>
            note.file === item.file && note.line === item.line
        );
        for (const note of relatedNotes) {
            if (note.suggestion) lines.push(`  候補     : 「${note.suggestion}」`);
            else lines.push(`  参考     : ${note.message}`);
        }
        lines.push("------------------------------------------------------------");
    });

    if (primary.length === 0) {
        lines.push("コンパイラの診断位置を解析できませんでした。以下の原文を確認してください。");
        lines.push(parsed.cleanOutput || "詳細情報はありません。");
        lines.push("------------------------------------------------------------");
    }
    else {
        const unmatchedNotes = notes.filter(note => !primary.some(item =>
            note.file === item.file && note.line === item.line
        ));
        for (const note of unmatchedNotes) {
            lines.push(`[参考] ${note.file}  ${note.line}行 ${note.column}列`);
            lines.push(`  ${note.message}`);
        }
    }

    lines.push("修正後、F2 または Ctrl+Space でもう一度書き込んでください。");
    lines.push("エディターの波線、または「表示 > 問題」から該当箇所を開けます。");
    lines.push("============================================================");

    const first = errors[0] || warnings[0];
    const summary = first
        ? `${first.file} ${first.line}行目: ${first.message}`
        : "コンパイルに失敗しました。出力パネルを確認してください。";

    return {
        ...parsed,
        errors,
        warnings,
        notes,
        summary,
        text: lines.join("\n")
    };
}

module.exports = {
    stripAnsi,
    translateDiagnostic,
    parseCompilerOutput,
    createCompilerReport
};
