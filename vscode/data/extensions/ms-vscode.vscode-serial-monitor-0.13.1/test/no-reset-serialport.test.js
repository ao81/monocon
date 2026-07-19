"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");
const { install } = require("../dist/no-reset-serialport");

test("disables reset control lines before opening the port", () => {
    class FakeSerialPort {
        constructor() {
            this.settings = { hupcl: true, rtscts: true };
        }

        open(callback) {
            this.openSettings = { ...this.settings };
            callback?.();
            return "opened";
        }

        set(options, callback) {
            this.setOptions = options;
            callback?.();
            return "set";
        }
    }

    install({ SerialPort: FakeSerialPort });
    const port = new FakeSerialPort();

    assert.equal(port.open(), "opened");
    assert.equal(port.openSettings.hupcl, false);
    assert.equal(port.openSettings.rtscts, false);

    assert.equal(port.set({ dtr: true, rts: true, brk: true }), "set");
    assert.deepEqual(port.setOptions, { dtr: false, rts: false, brk: true });
});

test("install is idempotent", () => {
    class FakeSerialPort {
        open() {}
        set() {}
    }

    install({ SerialPort: FakeSerialPort });
    const firstOpen = FakeSerialPort.prototype.open;
    const firstSet = FakeSerialPort.prototype.set;
    install({ SerialPort: FakeSerialPort });

    assert.equal(FakeSerialPort.prototype.open, firstOpen);
    assert.equal(FakeSerialPort.prototype.set, firstSet);
});
