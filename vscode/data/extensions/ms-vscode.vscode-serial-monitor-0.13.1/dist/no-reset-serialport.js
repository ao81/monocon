"use strict";

// Arduino Mega 2560 は DTR の変化を自動リセット信号として使用する。
// Serial Monitor がポートを開く前に hupcl を無効化し、監視中も
// DTR / RTS を常に非アクティブにして、出力ピンが一瞬途切れるのを防ぐ。
const PATCH_MARK = Symbol.for("monocon.serial-monitor.no-reset");

function install(serialPortModule = require("serialport")) {
    const SerialPort = serialPortModule && serialPortModule.SerialPort;
    if (!SerialPort || !SerialPort.prototype) {
        throw new Error("SerialPort implementation was not found.");
    }

    const prototype = SerialPort.prototype;
    if (prototype[PATCH_MARK]) {
        return;
    }

    const originalOpen = prototype.open;
    const originalSet = prototype.set;
    if (typeof originalOpen !== "function" || typeof originalSet !== "function") {
        throw new Error("SerialPort open/set API is not compatible.");
    }

    Object.defineProperty(prototype, PATCH_MARK, {
        value: true,
        configurable: false,
        enumerable: false,
        writable: false
    });

    prototype.open = function openWithoutArduinoReset(callback) {
        // bindings-cpp は hupcl=true の場合、SetCommState時にDTRを有効化する。
        // binding.open()より前にfalseへ変更することで、短いDTRパルスも防止する。
        if (this.settings && typeof this.settings === "object") {
            this.settings.hupcl = false;
            this.settings.rtscts = false;
        }
        return originalOpen.call(this, callback);
    };

    prototype.set = function keepResetLinesInactive(options, callback) {
        return originalSet.call(this, {
            ...(options || {}),
            dtr: false,
            rts: false
        }, callback);
    };
}

module.exports = { install };
