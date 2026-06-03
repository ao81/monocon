@echo off
setlocal

rem === ポータブル nvim を PATH に通す ===
set "PATH=%~dp0data\nvim\bin;%PATH%"

rem === Neovim の設定をポータブル内から読む ===
set "XDG_CONFIG_HOME=%~dp0data\nvim-config"

rem === 既定で開くパスを書いておくファイル（bat と同じ場所） ===
set "PATHFILE=%~dp0default-path.txt"

rem === ファイルがあれば 1 行目を読む。無ければ TARGET は空のまま ===
set "TARGET="
if exist "%PATHFILE%" set /p TARGET=<"%PATHFILE%"

rem === パス指定が無ければ空で起動 ===
if not defined TARGET (
    start "" "%~dp0Code.exe"
    goto :eof
)

rem === bat からの相対パスとして存在すれば、それを開く（持ち運び向け） ===
if exist "%~dp0%TARGET%" (
    start "" "%~dp0Code.exe" "%~dp0%TARGET%"
    goto :eof
)

rem === それ以外は絶対パスとしてそのまま開く ===
start "" "%~dp0Code.exe" "%TARGET%"
