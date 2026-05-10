/******/ (() => { // webpackBootstrap
/******/ 	"use strict";
/******/ 	var __webpack_modules__ = ([
/* 0 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {


var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.deactivate = exports.activate = void 0;
const vscode = __webpack_require__(1);
let timerStatusBarItem;
let resetStatusBarItem;
function activate(context) {
    let time = 0;
    let isTimerRunning = false;
    let timer;
    timerStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    timerStatusBarItem.text = 'Begin Timer';
    timerStatusBarItem.show();
    timerStatusBarItem.command = 'helloworld.statusBarClick';
    context.subscriptions.push(timerStatusBarItem);
    resetStatusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    resetStatusBarItem.text = `$(stop-circle)`;
    resetStatusBarItem.show();
    resetStatusBarItem.command = 'helloworld.resetTimer';
    context.subscriptions.push(resetStatusBarItem);
    context.subscriptions.push(vscode.commands.registerCommand('helloworld.statusBarClick', () => {
        if (isTimerRunning) {
            isTimerRunning = false;
            clearInterval(timer);
        }
        else {
            isTimerRunning = true;
            timer = setInterval(function () {
                time += 1;
                timerStatusBarItem.text = formatTimer(time);
            }, 1000);
        }
    }));
    context.subscriptions.push(vscode.commands.registerCommand('helloworld.resetTimer', () => __awaiter(this, void 0, void 0, function* () {
        isTimerRunning = false;
        clearInterval(timer);
        time = 0;
        timerStatusBarItem.text = 'Begin Timer';
    })));
}
exports.activate = activate;
// todo keep track of timer var here
function deactivate() { }
exports.deactivate = deactivate;
function formatTimer(time) {
    let hour = Math.floor(time / 3600);
    let minute = Math.floor((time % 3600) / 60);
    let second = Math.floor(time % 60);
    const hh = hour.toString().padStart(2, '0');
    const mm = minute.toString().padStart(2, '0');
    const ss = second.toString().padStart(2, '0');
    return `${hh}:${mm}:${ss}`;
}


/***/ }),
/* 1 */
/***/ ((module) => {

module.exports = require("vscode");;

/***/ })
/******/ 	]);
/************************************************************************/
/******/ 	// The module cache
/******/ 	var __webpack_module_cache__ = {};
/******/ 	
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/ 		// Check if module is in cache
/******/ 		var cachedModule = __webpack_module_cache__[moduleId];
/******/ 		if (cachedModule !== undefined) {
/******/ 			return cachedModule.exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = __webpack_module_cache__[moduleId] = {
/******/ 			// no module.id needed
/******/ 			// no module.loaded needed
/******/ 			exports: {}
/******/ 		};
/******/ 	
/******/ 		// Execute the module function
/******/ 		__webpack_modules__[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/ 	
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/ 	
/************************************************************************/
/******/ 	
/******/ 	// startup
/******/ 	// Load entry module and return exports
/******/ 	// This entry module is referenced by other modules so it can't be inlined
/******/ 	var __webpack_exports__ = __webpack_require__(0);
/******/ 	module.exports = __webpack_exports__;
/******/ 	
/******/ })()
;
//# sourceMappingURL=extension.js.map