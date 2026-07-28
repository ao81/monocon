"use strict";

Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;

const { registerArduinoUploadCommands } = require("./arduino-upload");
const { registerTaskFolderCommands } = require("./task-folders");
const { registerUploadStatus } = require("./upload-status");
const { createNativeService } = require("./native-service");

/**
 * Monocon Tools entry point.
 * 各機能の実装は専用モジュールへ分け、ここでは登録だけを行う。
 */
function activate(context) {
    const uploadStatus = registerUploadStatus(context);
    const nativeService = createNativeService(context);
    registerArduinoUploadCommands(context, uploadStatus, nativeService);
    registerTaskFolderCommands(context);

    // 起動直後にワーカーとツールチェーンを準備し、最初の書き込みにも
    // 初期化コストを載せない。失敗時は書き込み時にCLIへフォールバックする。
    if (context.extensionPath && context.globalStorageUri) {
        nativeService.warmup().catch(() => undefined);
    }
}

function deactivate() {}
