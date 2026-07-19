"use strict";

Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;

const { registerArduinoUploadCommands } = require("./arduino-upload");
const { registerTaskFolderCommands } = require("./task-folders");
const { registerUploadStatus } = require("./upload-status");

/**
 * Monocon Tools entry point.
 * 各機能の実装は専用モジュールへ分け、ここでは登録だけを行う。
 */
function activate(context) {
    registerUploadStatus(context);
    registerArduinoUploadCommands(context);
    registerTaskFolderCommands(context);
}

function deactivate() {}
