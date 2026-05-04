/*!--------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
/******************************************************************************
Copyright (c) Microsoft Corporation.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
***************************************************************************** */
/* global Reflect, Promise, SuppressedError, Symbol */

var extendStatics = function(d, b) {
    extendStatics = Object.setPrototypeOf ||
        ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
        function (d, b) { for (var p in b) if (Object.prototype.hasOwnProperty.call(b, p)) d[p] = b[p]; };
    return extendStatics(d, b);
};

export function __extends(d, b) {
    if (typeof b !== "function" && b !== null)
        throw new TypeError("Class extends value " + String(b) + " is not a constructor or null");
    extendStatics(d, b);
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
}

export var __assign = function() {
    __assign = Object.assign || function __assign(t) {
        for (var s, i = 1, n = arguments.length; i < n; i++) {
            s = arguments[i];
            for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p)) t[p] = s[p];
        }
        return t;
    }
    return __assign.apply(this, arguments);
}

export function __rest(s, e) {
    var t = {};
    for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p) && e.indexOf(p) < 0)
        t[p] = s[p];
    if (s != null && typeof Object.getOwnPropertySymbols === "function")
        for (var i = 0, p = Object.getOwnPropertySymbols(s); i < p.length; i++) {
            if (e.indexOf(p[i]) < 0 && Object.prototype.propertyIsEnumerable.call(s, p[i]))
                t[p[i]] = s[p[i]];
        }
    return t;
}

export function __decorate(decorators, target, key, desc) {
    var c = arguments.length, r = c < 3 ? target : desc === null ? desc = Object.getOwnPropertyDescriptor(target, key) : desc, d;
    if (typeof Reflect === "object" && typeof Reflect.decorate === "function") r = Reflect.decorate(decorators, target, key, desc);
    else for (var i = decorators.length - 1; i >= 0; i--) if (d = decorators[i]) r = (c < 3 ? d(r) : c > 3 ? d(target, key, r) : d(target, key)) || r;
    return c > 3 && r && Object.defineProperty(target, key, r), r;
}

export function __param(paramIndex, decorator) {
    return function (target, key) { decorator(target, key, paramIndex); }
}

export function __esDecorate(ctor, descriptorIn, decorators, contextIn, initializers, extraInitializers) {
    function accept(f) { if (f !== void 0 && typeof f !== "function") throw new TypeError("Function expected"); return f; }
    var kind = contextIn.kind, key = kind === "getter" ? "get" : kind === "setter" ? "set" : "value";
    var target = !descriptorIn && ctor ? contextIn["static"] ? ctor : ctor.prototype : null;
    var descriptor = descriptorIn || (target ? Object.getOwnPropertyDescriptor(target, contextIn.name) : {});
    var _, done = false;
    for (var i = decorators.length - 1; i >= 0; i--) {
        var context = {};
        for (var p in contextIn) context[p] = p === "access" ? {} : contextIn[p];
        for (var p in contextIn.access) context.access[p] = contextIn.access[p];
        context.addInitializer = function (f) { if (done) throw new TypeError("Cannot add initializers after decoration has completed"); extraInitializers.push(accept(f || null)); };
        var result = (0, decorators[i])(kind === "accessor" ? { get: descriptor.get, set: descriptor.set } : descriptor[key], context);
        if (kind === "accessor") {
            if (result === void 0) continue;
            if (result === null || typeof result !== "object") throw new TypeError("Object expected");
            if (_ = accept(result.get)) descriptor.get = _;
            if (_ = accept(result.set)) descriptor.set = _;
            if (_ = accept(result.init)) initializers.unshift(_);
        }
        else if (_ = accept(result)) {
            if (kind === "field") initializers.unshift(_);
            else descriptor[key] = _;
        }
    }
    if (target) Object.defineProperty(target, contextIn.name, descriptor);
    done = true;
};

export function __runInitializers(thisArg, initializers, value) {
    var useValue = arguments.length > 2;
    for (var i = 0; i < initializers.length; i++) {
        value = useValue ? initializers[i].call(thisArg, value) : initializers[i].call(thisArg);
    }
    return useValue ? value : void 0;
};

export function __propKey(x) {
    return typeof x === "symbol" ? x : "".concat(x);
};

export function __setFunctionName(f, name, prefix) {
    if (typeof name === "symbol") name = name.description ? "[".concat(name.description, "]") : "";
    return Object.defineProperty(f, "name", { configurable: true, value: prefix ? "".concat(prefix, " ", name) : name });
};

export function __metadata(metadataKey, metadataValue) {
    if (typeof Reflect === "object" && typeof Reflect.metadata === "function") return Reflect.metadata(metadataKey, metadataValue);
}

export function __awaiter(thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
}

export function __generator(thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
    return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (g && (g = 0, op[0] && (_ = 0)), _) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
}

export var __createBinding = Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
        desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
});

export function __exportStar(m, o) {
    for (var p in m) if (p !== "default" && !Object.prototype.hasOwnProperty.call(o, p)) __createBinding(o, m, p);
}

export function __values(o) {
    var s = typeof Symbol === "function" && Symbol.iterator, m = s && o[s], i = 0;
    if (m) return m.call(o);
    if (o && typeof o.length === "number") return {
        next: function () {
            if (o && i >= o.length) o = void 0;
            return { value: o && o[i++], done: !o };
        }
    };
    throw new TypeError(s ? "Object is not iterable." : "Symbol.iterator is not defined.");
}

export function __read(o, n) {
    var m = typeof Symbol === "function" && o[Symbol.iterator];
    if (!m) return o;
    var i = m.call(o), r, ar = [], e;
    try {
        while ((n === void 0 || n-- > 0) && !(r = i.next()).done) ar.push(r.value);
    }
    catch (error) { e = { error: error }; }
    finally {
        try {
            if (r && !r.done && (m = i["return"])) m.call(i);
        }
        finally { if (e) throw e.error; }
    }
    return ar;
}

/** @deprecated */
export function __spread() {
    for (var ar = [], i = 0; i < arguments.length; i++)
        ar = ar.concat(__read(arguments[i]));
    return ar;
}

/** @deprecated */
export function __spreadArrays() {
    for (var s = 0, i = 0, il = arguments.length; i < il; i++) s += arguments[i].length;
    for (var r = Array(s), k = 0, i = 0; i < il; i++)
        for (var a = arguments[i], j = 0, jl = a.length; j < jl; j++, k++)
            r[k] = a[j];
    return r;
}

export function __spreadArray(to, from, pack) {
    if (pack || arguments.length === 2) for (var i = 0, l = from.length, ar; i < l; i++) {
        if (ar || !(i in from)) {
            if (!ar) ar = Array.prototype.slice.call(from, 0, i);
            ar[i] = from[i];
        }
    }
    return to.concat(ar || Array.prototype.slice.call(from));
}

export function __await(v) {
    return this instanceof __await ? (this.v = v, this) : new __await(v);
}

export function __asyncGenerator(thisArg, _arguments, generator) {
    if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
    var g = generator.apply(thisArg, _arguments || []), i, q = [];
    return i = {}, verb("next"), verb("throw"), verb("return", awaitReturn), i[Symbol.asyncIterator] = function () { return this; }, i;
    function awaitReturn(f) { return function (v) { return Promise.resolve(v).then(f, reject); }; }
    function verb(n, f) { if (g[n]) { i[n] = function (v) { return new Promise(function (a, b) { q.push([n, v, a, b]) > 1 || resume(n, v); }); }; if (f) i[n] = f(i[n]); } }
    function resume(n, v) { try { step(g[n](v)); } catch (e) { settle(q[0][3], e); } }
    function step(r) { r.value instanceof __await ? Promise.resolve(r.value.v).then(fulfill, reject) : settle(q[0][2], r); }
    function fulfill(value) { resume("next", value); }
    function reject(value) { resume("throw", value); }
    function settle(f, v) { if (f(v), q.shift(), q.length) resume(q[0][0], q[0][1]); }
}

export function __asyncDelegator(o) {
    var i, p;
    return i = {}, verb("next"), verb("throw", function (e) { throw e; }), verb("return"), i[Symbol.iterator] = function () { return this; }, i;
    function verb(n, f) { i[n] = o[n] ? function (v) { return (p = !p) ? { value: __await(o[n](v)), done: false } : f ? f(v) : v; } : f; }
}

export function __asyncValues(o) {
    if (!Symbol.asyncIterator) throw new TypeError("Symbol.asyncIterator is not defined.");
    var m = o[Symbol.asyncIterator], i;
    return m ? m.call(o) : (o = typeof __values === "function" ? __values(o) : o[Symbol.iterator](), i = {}, verb("next"), verb("throw"), verb("return"), i[Symbol.asyncIterator] = function () { return this; }, i);
    function verb(n) { i[n] = o[n] && function (v) { return new Promise(function (resolve, reject) { v = o[n](v), settle(resolve, reject, v.done, v.value); }); }; }
    function settle(resolve, reject, d, v) { Promise.resolve(v).then(function(v) { resolve({ value: v, done: d }); }, reject); }
}

export function __makeTemplateObject(cooked, raw) {
    if (Object.defineProperty) { Object.defineProperty(cooked, "raw", { value: raw }); } else { cooked.raw = raw; }
    return cooked;
};

var __setModuleDefault = Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
};

export function __importStar(mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
}

export function __importDefault(mod) {
    return (mod && mod.__esModule) ? mod : { default: mod };
}

export function __classPrivateFieldGet(receiver, state, kind, f) {
    if (kind === "a" && !f) throw new TypeError("Private accessor was defined without a getter");
    if (typeof state === "function" ? receiver !== state || !f : !state.has(receiver)) throw new TypeError("Cannot read private member from an object whose class did not declare it");
    return kind === "m" ? f : kind === "a" ? f.call(receiver) : f ? f.value : state.get(receiver);
}

export function __classPrivateFieldSet(receiver, state, value, kind, f) {
    if (kind === "m") throw new TypeError("Private method is not writable");
    if (kind === "a" && !f) throw new TypeError("Private accessor was defined without a setter");
    if (typeof state === "function" ? receiver !== state || !f : !state.has(receiver)) throw new TypeError("Cannot write private member to an object whose class did not declare it");
    return (kind === "a" ? f.call(receiver, value) : f ? f.value = value : state.set(receiver, value)), value;
}

export function __classPrivateFieldIn(state, receiver) {
    if (receiver === null || (typeof receiver !== "object" && typeof receiver !== "function")) throw new TypeError("Cannot use 'in' operator on non-object");
    return typeof state === "function" ? receiver === state : state.has(receiver);
}

export function __addDisposableResource(env, value, async) {
    if (value !== null && value !== void 0) {
        if (typeof value !== "object" && typeof value !== "function") throw new TypeError("Object expected.");
        var dispose, inner;
        if (async) {
            if (!Symbol.asyncDispose) throw new TypeError("Symbol.asyncDispose is not defined.");
            dispose = value[Symbol.asyncDispose];
        }
        if (dispose === void 0) {
            if (!Symbol.dispose) throw new TypeError("Symbol.dispose is not defined.");
            dispose = value[Symbol.dispose];
            if (async) inner = dispose;
        }
        if (typeof dispose !== "function") throw new TypeError("Object not disposable.");
        if (inner) dispose = function() { try { inner.call(this); } catch (e) { return Promise.reject(e); } };
        env.stack.push({ value: value, dispose: dispose, async: async });
    }
    else if (async) {
        env.stack.push({ async: true });
    }
    return value;

}

var _SuppressedError = typeof SuppressedError === "function" ? SuppressedError : function (error, suppressed, message) {
    var e = new Error(message);
    return e.name = "SuppressedError", e.error = error, e.suppressed = suppressed, e;
};

export function __disposeResources(env) {
    function fail(e) {
        env.error = env.hasError ? new _SuppressedError(e, env.error, "An error was suppressed during disposal.") : e;
        env.hasError = true;
    }
    function next() {
        while (env.stack.length) {
            var rec = env.stack.pop();
            try {
                var result = rec.dispose && rec.dispose.call(rec.value);
                if (rec.async) return Promise.resolve(result).then(next, function(e) { fail(e); return next(); });
            }
            catch (e) {
                fail(e);
            }
        }
        if (env.hasError) throw env.error;
    }
    return next();
}

export default {
    __extends: __extends,
    __assign: __assign,
    __rest: __rest,
    __decorate: __decorate,
    __param: __param,
    __metadata: __metadata,
    __awaiter: __awaiter,
    __generator: __generator,
    __createBinding: __createBinding,
    __exportStar: __exportStar,
    __values: __values,
    __read: __read,
    __spread: __spread,
    __spreadArrays: __spreadArrays,
    __spreadArray: __spreadArray,
    __await: __await,
    __asyncGenerator: __asyncGenerator,
    __asyncDelegator: __asyncDelegator,
    __asyncValues: __asyncValues,
    __makeTemplateObject: __makeTemplateObject,
    __importStar: __importStar,
    __importDefault: __importDefault,
    __classPrivateFieldGet: __classPrivateFieldGet,
    __classPrivateFieldSet: __classPrivateFieldSet,
    __classPrivateFieldIn: __classPrivateFieldIn,
    __addDisposableResource: __addDisposableResource,
    __disposeResources: __disposeResources,
};

function S(e){let r=[];typeof e=="number"&&r.push("code/timeOrigin",e);function c(n,t){r.push(n,t?.startTime??Date.now())}function u(){let n=[];for(let t=0;t<r.length;t+=2)n.push({name:r[t],startTime:r[t+1]});return n}function m(n){if(typeof n>"u"){let t=r.length>=2&&r[0]==="code/timeOrigin",i=t?r[1]:void 0;r.length=0,t&&r.push("code/timeOrigin",i)}else for(let t=r.length-2;t>=0;t-=2)r[t]===n&&r.splice(t,2)}return{mark:c,getMarks:u,clearMarks:m}}function A(){if(typeof performance=="object"&&typeof performance.mark=="function"&&!performance.nodeTiming)return typeof performance.timeOrigin!="number"&&!performance.timing?S():{mark(e,r){performance.mark(e,r)},clearMarks(e){performance.clearMarks(e)},getMarks(){let e=performance.timeOrigin;typeof e!="number"&&(e=(performance.timing.navigationStart||performance.timing.redirectStart||performance.timing.fetchStart)??0);let r=[{name:"code/timeOrigin",startTime:Math.round(e)}];for(let c of performance.getEntriesByType("mark"))r.push({name:c.name,startTime:Math.round(e+c.startTime)});return r}};if(typeof process=="object"){let e=performance?.timeOrigin;return S(e)}else return console.trace("perf-util loaded in UNKNOWN environment"),S()}function w(e){return e.MonacoPerformanceMarks||(e.MonacoPerformanceMarks=A()),e.MonacoPerformanceMarks}var N=w(globalThis),E=N.mark,W=N.clearMarks,q=N.getMarks;import*as d from"node:path";import{createRequire as V}from"node:module";var I=V(import.meta.url),U=process.platform==="win32";Error.stackTraceLimit=100;if(!process.env.VSCODE_HANDLES_SIGPIPE){let e=!1;process.on("SIGPIPE",()=>{e||(e=!0,console.error(new Error("Unexpected SIGPIPE")))})}function j(){try{typeof process.env.VSCODE_CWD!="string"&&(process.env.VSCODE_CWD=process.cwd()),process.platform==="win32"&&process.chdir(d.dirname(process.execPath))}catch(e){console.error(e)}}j();function C(e){if(!process.env.VSCODE_DEV)return;if(!e)throw new Error("Missing injectPath");I("node:module").register("./bootstrap-import.js",{parentURL:import.meta.url,data:e})}function D(){if(typeof process?.versions?.electron=="string")return;let e=I("module"),r=e.globalPaths,c=e._resolveLookupPaths;e._resolveLookupPaths=function(m,n){let t=c(m,n);if(Array.isArray(t)){let i=0;for(;i<t.length&&t[t.length-1-i]===r[r.length-1-i];)i++;return t.slice(0,t.length-i)}return t};let u=e._nodeModulePaths;e._nodeModulePaths=function(m){let n=u(m);if(!U)return n;let t=i=>i.length>=3&&i.endsWith(":\\");if(t(m)||(n=n.filter(i=>!t(d.dirname(i)))),process.env.HOMEDRIVE&&process.env.HOMEPATH){let i=d.dirname(d.join(process.env.HOMEDRIVE,process.env.HOMEPATH)),o=a=>d.relative(a,i).length===0;o(m)||(n=n.filter(a=>!o(d.dirname(a))))}return n}}import*as _ from"node:fs";import{register as x}from"node:module";import{createRequire as G}from"node:module";var O=G(import.meta.url),p={BUILD_INSERT_PRODUCT_CONFIGURATION:"BUILD_INSERT_PRODUCT_CONFIGURATION"};p.BUILD_INSERT_PRODUCT_CONFIGURATION&&(p=O("../product.json"));var h={"name":"Code","version":"1.118.1","distro":"b85ea484c93395a77703478557a49e35ac7c0841","author":{"name":"Microsoft Corporation"},"license":"MIT","main":"./out/main.js","type":"module","private":true,"scripts":{"test":"echo Please run any of the test scripts from the scripts folder.","test-browser":"npx playwright install && node test/unit/browser/index.js","test-browser-no-install":"node test/unit/browser/index.js","test-node":"mocha test/unit/node/index.js --delay --ui=tdd --timeout=5000 --exit","test-extension":"vscode-test","test-build-scripts":"cd build && npm run test","check-cyclic-dependencies":"node build/lib/checkCyclicDependencies.ts out","preinstall":"node build/npm/preinstall.ts","postinstall":"node build/npm/postinstall.ts","compile":"npm run gulp compile","compile-check-ts-native":"tsgo --project ./src/tsconfig.json --noEmit --skipLibCheck","watch":"npm-run-all2 -lp watch-client-transpile watch-client watch-extensions watch-copilot","watchd":"deemon npm run watch","watch-webd":"deemon npm run watch-web","kill-watchd":"deemon --kill npm run watch","kill-watch-webd":"deemon --kill npm run watch-web","restart-watchd":"deemon --restart npm run watch","restart-watch-webd":"deemon --restart npm run watch-web","watch-client":"npm run gulp watch-client","watch-clientd":"deemon npm run watch-client","kill-watch-clientd":"deemon --kill npm run watch-client","transpile-client":"node build/next/index.ts transpile","watch-client-transpile":"node build/next/index.ts transpile --watch","watch-client-transpiled":"deemon npm run watch-client-transpile","kill-watch-client-transpiled":"deemon --kill npm run watch-client-transpile","watch-extensions":"npm run gulp watch-extensions watch-extension-media","watch-extensionsd":"deemon npm run watch-extensions","kill-watch-extensionsd":"deemon --kill npm run watch-extensions","watch-copilot":"npm --prefix extensions/copilot run watch","watch-copilotd":"deemon npm run watch-copilot","kill-watch-copilotd":"deemon --kill npm run watch-copilot","precommit":"node --experimental-strip-types build/hygiene.ts","gulp":"node --max-old-space-size=8192 ./node_modules/gulp/bin/gulp.js","electron":"node build/lib/electron.ts","7z":"7z","update-grammars":"node build/npm/update-all-grammars.ts","update-localization-extension":"node build/npm/update-localization-extension.ts","mixin-telemetry-docs":"node build/npm/mixin-telemetry-docs.ts","smoketest":"node build/lib/preLaunch.ts && cd test/smoke && npm run compile && node test/index.js","smoketest-no-compile":"cd test/smoke && node test/index.js","download-builtin-extensions":"node build/lib/builtInExtensions.ts","download-builtin-extensions-cg":"node build/lib/builtInExtensionsCG.ts","monaco-compile-check":"tsgo --project src/tsconfig.monaco.json --noEmit","tsec-compile-check":"node node_modules/tsec/bin/tsec -p src/tsconfig.tsec.json","vscode-dts-compile-check":"tsgo --project src/tsconfig.vscode-dts.json && tsgo --project src/tsconfig.vscode-proposed-dts.json","valid-layers-check":"node build/checker/layersChecker.ts && tsgo --project build/checker/tsconfig.browser.json && tsgo --project build/checker/tsconfig.worker.json && tsgo --project build/checker/tsconfig.node.json && tsgo --project build/checker/tsconfig.electron-browser.json && tsgo --project build/checker/tsconfig.electron-main.json && tsgo --project build/checker/tsconfig.electron-utility.json","define-class-fields-check":"node build/lib/propertyInitOrderChecker.ts && tsgo --project src/tsconfig.defineClassFields.json","update-distro":"node build/npm/update-distro.ts","export-policy-data":"node build/lib/policies/exportPolicyData.ts","web":"echo 'npm run web' is replaced by './scripts/code-server' or './scripts/code-web'","compile-cli":"npm run gulp compile-cli","compile-web":"npm run gulp compile-web","serve-out-rspack":"cd build/rspack && npx rspack serve --config rspack.serve-out.config.mts","watch-web":"npm run gulp watch-web","watch-cli":"npm run gulp watch-cli","eslint":"node build/eslint.ts","stylelint":"node build/stylelint.ts","playwright-install":"npm exec playwright install","compile-build":"npm run gulp compile-build-with-mangling","compile-extensions-build":"npm run gulp compile-extensions-build","minify-vscode":"npm run gulp minify-vscode","minify-vscode-reh":"npm run gulp minify-vscode-reh","minify-vscode-reh-web":"npm run gulp minify-vscode-reh-web","hygiene":"npm run gulp hygiene","core-ci":"npm run gulp core-ci","core-ci-pr":"npm run gulp core-ci-pr","extensions-ci":"npm run gulp extensions-ci","extensions-ci-pr":"npm run gulp extensions-ci-pr","perf":"node scripts/code-perf.js","perf:chat":"node scripts/chat-simulation/test-chat-perf-regression.js","perf:chat-leak":"node scripts/chat-simulation/test-chat-mem-leaks.js","copilot:setup":"npm --prefix extensions/copilot run setup","copilot:get_token":"npm --prefix extensions/copilot run get_token","update-build-ts-version":"npm install -D typescript@next && npm install -D @typescript/native-preview && (cd build && npm run typecheck)","install-local-component-explorer":"npm install ../vscode-packages/js-component-explorer/dist/vscode-component-explorer-0.1.0.tgz ../vscode-packages/js-component-explorer/dist/vscode-component-explorer-cli-0.1.0.tgz --no-save && cd build/rspack && npm install ../../../vscode-packages/js-component-explorer/dist/vscode-component-explorer-webpack-plugin-0.1.0.tgz --no-save && cd ../vite && npm install ../../../vscode-packages/js-component-explorer/dist/vscode-component-explorer-vite-plugin-0.1.0.tgz --no-save","symlink-local-component-explorer":"npm install ../vscode-packages/js-component-explorer/packages/explorer ../vscode-packages/js-component-explorer/packages/cli --no-save && cd build/rspack && npm install ../../../vscode-packages/js-component-explorer/packages/webpack-plugin ../../../vscode-packages/js-component-explorer/packages/explorer --no-save && cd ../vite && npm install ../../../vscode-packages/js-component-explorer/packages/vite-plugin ../../../vscode-packages/js-component-explorer/packages/explorer --no-save","install-latest-component-explorer":"npm install @vscode/component-explorer@next @vscode/component-explorer-cli@next && cd build/rspack && npm install @vscode/component-explorer-webpack-plugin@next @vscode/component-explorer@next && cd ../vite && npm install @vscode/component-explorer-vite-plugin@next @vscode/component-explorer@next"},"dependencies":{"@anthropic-ai/sandbox-runtime":"0.0.49","@github/copilot":"^1.0.34","@github/copilot-sdk":"^0.2.2","@microsoft/1ds-core-js":"^3.2.13","@microsoft/1ds-post-js":"^3.2.13","@microsoft/dev-tunnels-connections":"^1.3.41","@microsoft/dev-tunnels-contracts":"^1.3.41","@microsoft/dev-tunnels-management":"^1.3.41","@microsoft/dev-tunnels-ssh":"^3.12.22","@microsoft/dev-tunnels-ssh-tcp":"^3.12.22","@parcel/watcher":"^2.5.6","@types/semver":"^7.5.8","@vscode/codicons":"^0.0.46-5","@vscode/deviceid":"^0.1.1","@vscode/iconv-lite-umd":"0.7.1","@vscode/native-watchdog":"^1.4.6","@vscode/policy-watcher":"^1.3.2","@vscode/proxy-agent":"^0.41.0","@vscode/ripgrep":"^1.17.1","@vscode/spdlog":"^0.15.8","@vscode/sqlite3":"5.1.12-vscode","@vscode/sudo-prompt":"9.3.2","@vscode/tree-sitter-wasm":"^0.3.1","@vscode/vscode-languagedetection":"1.0.23","@vscode/windows-mutex":"^0.5.0","@vscode/windows-process-tree":"^0.7.0","@vscode/windows-registry":"^1.2.0","@xterm/addon-clipboard":"^0.3.0-beta.197","@xterm/addon-image":"^0.10.0-beta.197","@xterm/addon-ligatures":"^0.11.0-beta.197","@xterm/addon-progress":"^0.3.0-beta.197","@xterm/addon-search":"^0.17.0-beta.197","@xterm/addon-serialize":"^0.15.0-beta.197","@xterm/addon-unicode11":"^0.10.0-beta.197","@xterm/addon-webgl":"^0.20.0-beta.196","@xterm/headless":"^6.1.0-beta.197","@xterm/xterm":"^6.1.0-beta.197","http-proxy-agent":"^7.0.0","https-proxy-agent":"^7.0.2","jschardet":"3.1.4","katex":"^0.16.22","kerberos":"2.1.1","minimist":"^1.2.8","native-is-elevated":"0.9.0","native-keymap":"^3.3.5","node-pty":"^1.2.0-beta.12","open":"^10.1.2","playwright-core":"1.59.1","ssh2":"^1.16.0","tas-client":"0.3.1","undici":"^7.24.0","v8-inspect-profiler":"^0.1.1","vscode-oniguruma":"1.7.0","vscode-regexpp":"^3.1.0","vscode-textmate":"^9.3.2","ws":"^8.19.0","yauzl":"^3.0.0","yazl":"^2.4.3"},"devDependencies":{"@playwright/cli":"^0.1.1","@playwright/test":"^1.56.1","@stylistic/eslint-plugin-ts":"^2.8.0","@types/cookie":"^0.3.3","@types/debug":"^4.1.5","@types/eslint":"^9.6.1","@types/gulp-svgmin":"^1.2.1","@types/http-proxy-agent":"^2.0.1","@types/kerberos":"^1.1.2","@types/minimist":"^1.2.1","@types/mocha":"^10.0.10","@types/node":"^22.18.10","@types/sinon":"^10.0.2","@types/sinon-test":"^2.4.2","@types/ssh2":"^1.15.4","@types/trusted-types":"^2.0.7","@types/vscode-notebook-renderer":"^1.72.0","@types/wicg-file-system-access":"^2023.10.7","@types/windows-foreground-love":"^0.3.0","@types/winreg":"^1.2.30","@types/ws":"^8.18.1","@types/yauzl":"^2.10.0","@types/yazl":"^2.4.2","@typescript-eslint/utils":"^8.45.0","@typescript/native-preview":"^7.0.0-dev.20260408","@vscode/component-explorer":"^0.2.1-17","@vscode/component-explorer-cli":"^0.2.1-16","@vscode/gulp-electron":"1.41.2","@vscode/l10n-dev":"0.0.35","@vscode/telemetry-extractor":"^1.20.2","@vscode/test-cli":"^0.0.6","@vscode/test-electron":"^2.4.0","@vscode/test-web":"^0.0.76","@vscode/v8-heap-parser":"^0.1.0","@vscode/vscode-perf":"^0.0.19","@webgpu/types":"^0.1.66","agent-browser":"^0.16.3","ansi-colors":"^3.2.3","asar":"^3.0.3","chromium-pickle-js":"^0.2.0","cookie":"^0.7.2","debounce":"^1.0.0","deemon":"^1.13.6","electron":"39.8.8","eslint":"^9.36.0","eslint-formatter-compact":"^8.40.0","eslint-plugin-header":"3.1.1","eslint-plugin-jsdoc":"^50.3.1","event-stream":"3.3.4","fancy-log":"^1.3.3","glob":"^5.0.13","gulp":"^4.0.0","gulp-azure-storage":"^0.12.1","gulp-bom":"^3.0.0","gulp-buffer":"0.0.2","gulp-filter":"^5.1.0","gulp-flatmap":"^1.0.2","gulp-gunzip":"^1.0.0","gulp-gzip":"^1.4.2","gulp-json-editor":"^2.5.0","gulp-plumber":"^1.2.0","gulp-rename":"^1.2.0","gulp-replace":"^0.5.4","gulp-sourcemaps":"^3.0.0","gulp-svgmin":"^4.1.0","husky":"^0.13.1","innosetup":"^6.4.1","istanbul-lib-coverage":"^3.2.0","istanbul-lib-instrument":"^6.0.1","istanbul-lib-report":"^3.0.0","istanbul-lib-source-maps":"^4.0.1","istanbul-reports":"^3.1.5","lazy.js":"^0.4.2","merge-options":"^1.0.1","mime":"^1.4.1","minimatch":"^3.1.5","mocha":"^10.8.2","mocha-junit-reporter":"^2.2.1","mocha-multi-reporters":"^1.5.1","npm-run-all2":"^8.0.4","os-browserify":"^0.3.0","p-all":"^1.0.0","path-browserify":"^1.0.1","pump":"^1.0.1","rcedit":"^1.1.0","rimraf":"^2.2.8","sinon":"^12.0.1","sinon-test":"^3.1.3","source-map":"0.6.1","source-map-support":"^0.5.21","@types/source-map-support":"^0.5.10","tar":"^7.5.9","tsec":"0.2.7","tslib":"^2.6.3","typescript":"^6.0.0-dev.20260401","typescript-eslint":"^8.45.0","util":"^0.12.4","xml2js":"^0.5.0","yaserver":"^0.4.0"},"overrides":{"node-gyp-build":"4.8.1","kerberos@2.1.1":{"node-addon-api":"7.1.0"},"serialize-javascript":"^7.0.3","ssh2":{"cpu-features":"0.0.0"}},"repository":{"type":"git","url":"https://github.com/microsoft/vscode.git"},"bugs":{"url":"https://github.com/microsoft/vscode/issues"},"optionalDependencies":{"windows-foreground-love":"0.6.1"}};h.BUILD_INSERT_PACKAGE_CONFIGURATION&&(h=O("../package.json"));if(process.isEmbeddedApp){p.parentPolicyConfig={win32RegValueName:p.win32RegValueName,darwinBundleIdentifier:p.darwinBundleIdentifier,urlProtocol:p.urlProtocol};try{let e=O("../product.sub.json");p.embedded&&e.embedded&&(Object.assign(p.embedded,e.embedded),delete e.embedded),Object.assign(p,e)}catch{}try{let e=O("../package.sub.json");h=Object.assign(h,e)}catch{}}var v={};if(process.env.VSCODE_DEV)try{v=O("../product.overrides.json"),p=Object.assign(p,v)}catch{}var k=p,R=h;(process.env.ELECTRON_RUN_AS_NODE||process.versions.electron)&&x(`data:text/javascript;base64,${Buffer.from(`
	export async function resolve(specifier, context, nextResolve) {
		if (specifier === 'fs') {
			return {
				format: 'builtin',
				shortCircuit: true,
				url: 'node:original-fs'
			};
		}

		// Defer to the next hook in the chain, which would be the
		// Node.js default resolve if this is the last user-specified loader.
		return nextResolve(specifier, context);
	}`).toString("base64")}`,import.meta.url);globalThis._VSCODE_PRODUCT_JSON={...k};globalThis._VSCODE_PACKAGE_JSON={...R};globalThis._VSCODE_FILE_ROOT=import.meta.dirname;var T;function F(){return T||(T=B()),T}async function B(){E("code/willLoadNls");let e,r;if(process.env.VSCODE_NLS_CONFIG)try{e=JSON.parse(process.env.VSCODE_NLS_CONFIG),e?.languagePack?.messagesFile?r=e.languagePack.messagesFile:e?.defaultMessagesFile&&(r=e.defaultMessagesFile),globalThis._VSCODE_NLS_LANGUAGE=e?.resolvedLanguage}catch(c){console.error(`Error reading VSCODE_NLS_CONFIG from environment: ${c}`)}if(!(process.env.VSCODE_DEV||!r)){try{globalThis._VSCODE_NLS_MESSAGES=JSON.parse((await _.promises.readFile(r)).toString())}catch(c){if(console.error(`Error reading NLS messages file ${r}: ${c}`),e?.languagePack?.corruptMarkerFile)try{await _.promises.writeFile(e.languagePack.corruptMarkerFile,"corrupted")}catch(u){console.error(`Error writing corrupted NLS marker file: ${u}`)}if(e?.defaultMessagesFile&&e.defaultMessagesFile!==r)try{globalThis._VSCODE_NLS_MESSAGES=JSON.parse((await _.promises.readFile(e.defaultMessagesFile)).toString())}catch(u){console.error(`Error reading default NLS messages file ${e.defaultMessagesFile}: ${u}`)}}return E("code/didLoadNls"),e}}async function M(){await F()}E("code/fork/start");function H(){function c(o){let a=[],g=[];if(o.length)for(let l=0;l<o.length;l++){let s=o[l];if(typeof s>"u")s="undefined";else if(s instanceof Error){let f=s;f.stack?s=f.stack:s=f.toString()}g.push(s)}try{let l=JSON.stringify(g,function(s,f){if(m(f)||Array.isArray(f)){if(a.indexOf(f)!==-1)return"[Circular]";a.push(f)}return f});return l.length>1e5?"Output omitted for a large object that exceeds the limits":l}catch(l){return`Output omitted for an object that cannot be inspected ('${l.toString()}')`}}function u(o){try{process.send&&process.send(o)}catch{}}function m(o){return typeof o=="object"&&o!==null&&!Array.isArray(o)&&!(o instanceof RegExp)&&!(o instanceof Date)}function n(o,a){u({type:"__$console",severity:o,arguments:a})}function t(o,a){Object.defineProperty(console,o,{set:()=>{},get:()=>function(){n(a,c(arguments))}})}function i(o,a){let g=process[o],l=g.write,s="";Object.defineProperty(g,"write",{set:()=>{},get:()=>(f,b,L)=>{s+=f.toString(b);let P=s.length>1048576?s.length:s.lastIndexOf(`
`);P!==-1&&(console[a](s.slice(0,P)),s=s.slice(P+1)),l.call(g,f,b,L)}})}process.env.VSCODE_VERBOSE_LOGGING==="true"?(t("info","log"),t("log","log"),t("warn","warn"),t("error","error")):(console.log=function(){},console.warn=function(){},console.info=function(){},t("error","error")),i("stderr","error"),i("stdout","log")}function $(){process.on("uncaughtException",function(e){console.error("Uncaught Exception: ",e)}),process.on("unhandledRejection",function(e){console.error("Unhandled Promise Rejection: ",e)})}function J(){let e=Number(process.env.VSCODE_PARENT_PID);typeof e=="number"&&!isNaN(e)&&setInterval(function(){try{process.kill(e,0)}catch{process.exit()}},5e3)}function K(){let e=process.env.VSCODE_CRASH_REPORTER_PROCESS_TYPE;if(e)try{process.crashReporter&&typeof process.crashReporter.addExtraParameter=="function"&&process.crashReporter.addExtraParameter("processType",e)}catch(r){console.error(r)}}K();D();process.env.VSCODE_DEV_INJECT_NODE_MODULE_LOOKUP_PATH&&C(process.env.VSCODE_DEV_INJECT_NODE_MODULE_LOOKUP_PATH);process.send&&process.env.VSCODE_PIPE_LOGGING==="true"&&H();process.env.VSCODE_HANDLES_UNCAUGHT_ERRORS||$();process.env.VSCODE_PARENT_PID&&J();await M();await import([`./${process.env.VSCODE_ESM_ENTRYPOINT}.js`].join("/"));
//# sourceMappingURL=https://main.vscode-cdn.net/sourcemaps/034f571df509819cc10b0c8129f66ef77a542f0e/core/bootstrap-fork.js.map
