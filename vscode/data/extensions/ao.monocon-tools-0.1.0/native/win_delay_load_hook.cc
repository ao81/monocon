// Windows版Node/Electron用の遅延ロードフック。
// node-gypの同名フックと同じ方式で、node.exeへのインポートを現在の
// ホスト実行ファイル（VS CodeではCode.exe）へ解決する。

#ifdef _MSC_VER

#pragma managed(push, off)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <delayimp.h>
#include <cstring>

static FARPROC WINAPI loadHostBinary(unsigned int event, DelayLoadInfo* info) {
	if (event != dliNotePreLoadLibrary
		|| _stricmp(info->szDll, HOST_BINARY) != 0) {
		return nullptr;
	}

	// DLL版Nodeにも対応し、通常のNode/Electronでは現在のEXEを返す。
	HMODULE host = GetModuleHandleW(L"libnode.dll");
	if (host == nullptr) host = GetModuleHandleW(nullptr);
	return reinterpret_cast<FARPROC>(host);
}

decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = loadHostBinary;

#pragma managed(pop)

#endif
