"use strict";

// Serial Monitor本体より先にSerialPortを補正する必要がある。
require("./no-reset-serialport").install();
module.exports = require("./extension");
