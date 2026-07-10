@echo off
setlocal EnableExtensions

rem ============================================================
rem Monocon VSCode Portable Launcher
rem ------------------------------------------------------------
rem このスクリプトは Code.exe を起動する前に、Arduino
rem 開発に必要なポータブル環境を必ずセットアップします。
rem  1. ポータブル nvim を PATH に通す (vscode-neovim 拡張機能用)
rem  2. XDG_CONFIG_HOME を portable 内に固定
rem  3. arduino-cli / arduino-build-{cli,daemon} の場所を PATH に追加
rem  4. default-path.txt があれば、そのパスで Code.exe を開く
rem
rem 二重起動対策: Code.exe が既に走っていても start.bat を
rem 実行して問題が無いよう、setlocal で変更をこのプロセスに閉じる。
rem ============================================================

set "MONOCON_ROOT=%~dp0"

rem === (1) ポータブル nvim を PATH に通す ===
if exist "%MONOCON_ROOT%data\nvim\bin\nvim.exe" (
    set "PATH=%MONOCON_ROOT%data\nvim\bin;%PATH%"
)

rem === (2) Neovim の設定をポータブル内から読む ===
if exist "%MONOCON_ROOT%data\nvim-config\nvim\init.lua" (
    set "XDG_CONFIG_HOME=%MONOCON_ROOT%data\nvim-config"
)

rem === (3) arduino-build ツール群を PATH に通す (arr.bat から呼べるように) ===
if exist "%MONOCON_ROOT%data\daemon\build\bin\arduino-build-cli.exe" (
    set "PATH=%MONOCON_ROOT%data\daemon\build\bin;%PATH%"
)

rem === (4) VSCODE_PORTABLE を明示 (Code.exe が自動設定するがサブプロセスにも継承させる) ===
set "VSCODE_PORTABLE=%MONOCON_ROOT%data"

rem === 既定で開くパスを書いておくファイル (bat と同じ場所) ===
set "PATHFILE=%MONOCON_ROOT%default-path.txt"

rem === ファイルがあれば 1 行目を読む。無ければ TARGET は空のまま ===
set "TARGET="
if exist "%PATHFILE%" set /p TARGET=<"%PATHFILE%"

rem === パス指定が無ければ空で起動 ===
if not defined TARGET (
    start "" "%MONOCON_ROOT%Code.exe"
    goto :eof
)

rem === bat からの相対パスとして存在すれば、それを開く (持ち運び向け) ===
if exist "%MONOCON_ROOT%%TARGET%" (
    start "" "%MONOCON_ROOT%Code.exe" "%MONOCON_ROOT%%TARGET%"
    goto :eof
)

rem === それ以外は絶対パスとしてそのまま開く ===
start "" "%MONOCON_ROOT%Code.exe" "%TARGET%"
