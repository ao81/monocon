#pragma once

#include <windows.h>
#include <string>

// =============================================================================
// PipeIO: LSP 形式 (Content-Length: N\r\n\r\n + body) で名前付きパイプを
//         読み書きするヘルパ。クライアント / デーモン共通で使う。
// =============================================================================
namespace PipeIO {

	// 世代・SID付きのパイプ名
	// (\\.\pipe\arduino-build-v170-<SID>[-<test-instance>]) を生成
	std::string makePipeName();

	// Local\ プレフィックス付きの Mutex 名
	std::string makeMutexName();

	// LSP 風メッセージを 1 つ読む。EOF / エラー / 1 MiB超過時は空文字。
	std::string readMessage(HANDLE hPipe);

	// LSP 風メッセージを 1 つ書く。
	bool writeMessage(HANDLE hPipe, const std::string& body);

} // namespace PipeIO
