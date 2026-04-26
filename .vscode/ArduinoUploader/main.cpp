#include <windows.h>

#include <clocale>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

#include "uploader.h"
#include "utils.h"

namespace {

	void printUsage() {
		std::cout << "Usage: ArduinoUploader.exe [build] <SketchDir> [Port]\n"
			<< "  SketchDir: Arduino sketch directory\n"
			<< "  Port:      (optional) COM port (e.g., COM3)\n";
	}

	void printError(const std::string& message) {
		std::cout << ">>> Error: " << message << "\n";
	}

	// コンソールを UTF-8 設定する
	void setupConsole() {
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
		std::setlocale(LC_ALL, "");
	}

	// 引数解析: "build" サブコマンドにも対応
	struct Args {
		std::string sketchDir;
		std::string port;
		bool valid = false;
	};

	Args parseArgs(int argc, char* argv[]) {
		Args a;
		if (argc < 2) return a;

		int idx = 1;
		if (std::string(argv[1]) == "build") {
			if (argc < 3) return a;
			idx = 2;
		}

		a.sketchDir = argv[idx];
		if (argc > idx + 1) a.port = argv[idx + 1];
		a.valid = true;
		return a;
	}

} // namespace

int main(int argc, char* argv[]) {
	setupConsole();

	Args args = parseArgs(argc, argv);
	if (!args.valid) {
		printUsage();
		return 1;
	}

	std::cout << ">>> Uploading to Arduino...\n\n";

	Stopwatch totalSw;
	Stopwatch sw;

	try {
		Uploader uploader(args.sketchDir, args.port);

		if (!uploader.initialize()) {
			printError(uploader.getLastError());
			return 1;
		}
		std::cout << "Find port: "
			<< std::fixed << std::setprecision(0)
			<< sw.elapsedMilliseconds() << "ms\n";

		if (!uploader.upload()) {
			printError(uploader.getLastError());
			return 1;
		}

		std::cout << "\nTotal: "
			<< std::fixed << std::setprecision(2)
			<< totalSw.elapsedSeconds() << "s\n";
		return 0;
	} catch (const std::exception& e) {
		printError(std::string("Exception: ") + e.what());
		return 1;
	}
}