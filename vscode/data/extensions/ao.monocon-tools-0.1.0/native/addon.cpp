#include <node_api.h>

#include "builder.h"
#include "daemon_state.h"

#include <chrono>
#include <mutex>
#include <string>

using json = nlohmann::json;

namespace {
	std::mutex invokeMutex;

	std::string readUtf8(napi_env env, napi_value value) {
		size_t length = 0;
		if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
			throw std::runtime_error("Native request must be a UTF-8 string");
		}
		// N-APIは終端NULも書き込むため、その1バイトを明示的に確保する。
		// std::string(size=length) の末尾へ書かせる未定義動作を避ける。
		std::string result(length + 1, '\0');
		if (napi_get_value_string_utf8(env, value, result.data(), result.size(),
			&length) != napi_ok) {
			throw std::runtime_error("Cannot read native request");
		}
		result.resize(length);
		return result;
	}

	napi_value makeUtf8(napi_env env, const std::string& value) {
		napi_value result;
		if (napi_create_string_utf8(env, value.data(), value.size(), &result) != napi_ok) {
			napi_throw_error(env, nullptr, "Cannot create native response");
			return nullptr;
		}
		return result;
	}

	json requireReady() {
		if (g_state.toolchain.valid) return json();
		return {
			{"success", false},
			{"errorMessage", "Native builder is not initialized: " +
				g_state.toolchain.errorMessage}
		};
	}

	json dispatch(const json& request) {
		const std::string method = request.value("method", "");
		const json params = request.value("params", json::object());

		if (method == "initialize") {
			const bool ok = initializeDaemonState(
				params.value("extensionRoot", ""),
				params.value("cacheRoot", ""));
			return {
				{"success", ok},
				{"version", g_state.version},
				{"compilerVersion", g_state.toolchain.compilerVersion},
				{"errorMessage", ok ? "" : g_state.toolchain.errorMessage}
			};
		}

		json notReady = requireReady();
		if (!notReady.is_null()) return notReady;
		{
			std::lock_guard<std::mutex> lock(g_state.activityMtx);
			g_state.lastRequestAt = std::chrono::steady_clock::now();
		}
		g_state.requestCount++;

		if (method == "upload") {
			refreshComPorts();
			Builder::UploadRequest req;
			req.sketchDir = params.value("sketchDir", "");
			req.workspaceDir = params.value("workspaceDir", "");
			req.port = params.value("port", "");
			req.skipCompile = params.value("skipCompile", false);
			return Builder::toJson(Builder::upload(req));
		}
		if (method == "compile") {
			Builder::CompileRequest req;
			req.sketchDir = params.value("sketchDir", "");
			req.workspaceDir = params.value("workspaceDir", "");
			req.fqbn = "arduino:avr:mega:cpu=atmega2560";
			req.forceFullBuild = params.value("forceFullBuild", false);
			return Builder::toJson(Builder::compile(req));
		}
		if (method == "ports") {
			refreshComPorts();
			std::lock_guard<std::mutex> lock(g_state.portMtx);
			return {
				{"success", true},
				{"ports", g_state.cachedPorts},
				{"arduinoPort", g_state.cachedArduinoPort}
			};
		}
		if (method == "invalidateCache") {
			const std::string sketchDir = params.value("sketchDir", "");
			std::lock_guard<std::mutex> lock(g_state.sketchMtx);
			if (sketchDir.empty()) g_state.sketches.clear();
			else g_state.sketches.erase(sketchDir);
			return {{"success", true}};
		}
		if (method == "ping") {
			return {
				{"success", true},
				{"version", g_state.version},
				{"compilerVersion", g_state.toolchain.compilerVersion},
				{"requestCount", g_state.requestCount.load()}
			};
		}
		return {
			{"success", false},
			{"errorMessage", "Unknown native method: " + method}
		};
	}

	napi_value invoke(napi_env env, napi_callback_info info) {
		size_t argc = 1;
		napi_value args[1];
		napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
		try {
			if (argc != 1) throw std::runtime_error("invoke expects one JSON string");
			std::lock_guard<std::mutex> lock(invokeMutex);
			const json request = json::parse(readUtf8(env, args[0]));
			return makeUtf8(env, dispatch(request).dump());
		}
		catch (const std::exception& error) {
			return makeUtf8(env, json({
				{"success", false},
				{"errorMessage", std::string("Native exception: ") + error.what()}
			}).dump());
		}
	}
}

NAPI_MODULE_INIT() {
	napi_property_descriptor property = {
		"invoke", nullptr, invoke, nullptr, nullptr, nullptr, napi_default, nullptr
	};
	if (napi_define_properties(env, exports, 1, &property) != napi_ok) {
		napi_throw_error(env, nullptr, "Cannot export native invoke function");
	}
	return exports;
}
