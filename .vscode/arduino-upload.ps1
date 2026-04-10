Param(
	[string]$BuildPath,
	[string]$SketchDir,
	[string]$Port = ''
)
Write-Host ">>> Uploading to Arduino...`n"

$totalSw = [System.Diagnostics.Stopwatch]::StartNew()
$sw = [System.Diagnostics.Stopwatch]::StartNew()

# ---------------------------------------------------------
# 共通定数（環境に依存する部分）
# ---------------------------------------------------------
$globalCacheDir = "$env:LOCALAPPDATA\ArduinoCLI_Cache"
$portCacheFile = "$globalCacheDir\port_cache.txt"

$avrGccCacheFile = "$globalCacheDir\avr_gcc_path_cache.txt"
$avrGccRoot = ""
try {
	$avrGccRoot = [System.IO.File]::ReadAllText($avrGccCacheFile).Trim()
	if (-not (Test-Path $avrGccRoot)) { $avrGccRoot = "" }
}
catch {}

if ($avrGccRoot -eq "") {
	$avrGccBase = "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avr-gcc"
	$avrGccDirs = [System.IO.Directory]::GetDirectories($avrGccBase)
	if ($avrGccDirs.Count -eq 0) {
		Write-Host ">>> Error: avr-gcc not found under $avrGccBase" -ForegroundColor Red; exit 1
	}
	$avrGccRoot = "$($avrGccDirs[-1])\bin"
	[System.IO.File]::WriteAllText($avrGccCacheFile, $avrGccRoot)
}

$avrGpp = "$avrGccRoot\avr-g++.exe"
$avrGcc = "$avrGccRoot\avr-gcc.exe"
$avrObjcopy = "$avrGccRoot\avr-objcopy.exe"

$hwCacheFile = "$globalCacheDir\hw_path_cache.txt"
$hwRoot = ""
try {
	$hwRoot = [System.IO.File]::ReadAllText($hwCacheFile).Trim()
	if (-not (Test-Path $hwRoot)) { $hwRoot = "" }
}
catch {}

if ($hwRoot -eq "") {
	$hwBase = "$env:LOCALAPPDATA\Arduino15\packages\arduino\hardware\avr"
	$hwDirs = [System.IO.Directory]::GetDirectories($hwBase)
	if ($hwDirs.Count -eq 0) {
		Write-Host ">>> Error: arduino hardware not found under $hwBase" -ForegroundColor Red; exit 1
	}
	$hwRoot = $hwDirs[-1]
	[System.IO.File]::WriteAllText($hwCacheFile, $hwRoot)
}

$coresInc = "$hwRoot\cores\arduino"
$variantsInc = "$hwRoot\variants\mega"

# ---------------------------------------------------------
# 1. ポート検出
# ---------------------------------------------------------
$TargetPort = ""
try { $TargetPort = [System.IO.File]::ReadAllText($portCacheFile).Trim() } catch {}

if ($TargetPort -eq "") {
	Write-Host "Port cache miss, scanning..." -ForegroundColor Yellow
	try {
		$pnpDevices = Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%(COM%)'" -ErrorAction Stop
		$target = $pnpDevices | Where-Object { $_.Caption -match 'Arduino|Mega|USB Serial|CH340|CP210' } | Select-Object -First 1
		if ($target -and $target.Caption -match '\((COM\d+)\)') { $TargetPort = $matches[1] }
	}
 catch {}

	if ($TargetPort -eq "") {
		$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
		if ($availablePorts.Count -gt 0) { $TargetPort = $availablePorts[-1] }
		else { Write-Host ">>> Error: No COM port found." -ForegroundColor Red; exit 1 }
	}

	if (-not (Test-Path $globalCacheDir)) { New-Item -ItemType Directory -Force -Path $globalCacheDir | Out-Null }
	[System.IO.File]::WriteAllText($portCacheFile, $TargetPort)
}

Write-Host "Using Port: $TargetPort`n"
Write-Host "Find port: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 2. avrdudeのパス解決
# ---------------------------------------------------------
$sw.Restart()
$avrdudeCacheFile = "$globalCacheDir\avrdude_path_cache.txt"
$avrdude = ""; $avrdudeConf = ""
try {
	$lines = [System.IO.File]::ReadAllLines($avrdudeCacheFile)
	if ((Test-Path $lines[0]) -and (Test-Path $lines[1])) {
		$avrdude = $lines[0]; $avrdudeConf = $lines[1]
	}
}
catch {}

if ($avrdude -eq "") {
	$avrdudeRoot = "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avrdude"
	$avrdudeDirs = [System.IO.Directory]::GetDirectories($avrdudeRoot)
	if ($avrdudeDirs.Count -eq 0) {
		Write-Host ">>> Error: avrdude not found under $avrdudeRoot" -ForegroundColor Red; exit 1
	}
	$avrdudeDir = $avrdudeDirs[-1]
	$avrdude = "$avrdudeDir\bin\avrdude.exe"
	$avrdudeConf = "$avrdudeDir\etc\avrdude.conf"
	[System.IO.File]::WriteAllLines($avrdudeCacheFile, [string[]]@($avrdude, $avrdudeConf))
}
Write-Host "Avrdude path resolve: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 3. パス構築
# ---------------------------------------------------------
$sw.Restart()
$sketchName = [System.IO.Path]::GetFileName($SketchDir)
$inoFile = "$SketchDir\$sketchName.ino"
$sketchBuild = "$BuildPath\sketch"
$cppFile = "$sketchBuild\$sketchName.ino.cpp"
$objFile = "$sketchBuild\$sketchName.ino.cpp.o"
$elfFile = "$BuildPath\$sketchName.ino.elf"
$hexFile = "$BuildPath\$sketchName.ino.hex"
$eepFile = "$BuildPath\$sketchName.ino.eep"
$coreA = "$BuildPath\core\core.a"
$hashCacheFile = "$BuildPath\$sketchName.ino.sha256"

$inoInfo = [System.IO.FileInfo]::new($inoFile)
$needCompile = $true

$allSrcFiles = @($inoFile) + @([System.IO.Directory]::GetFiles($SketchDir, "*.h"))
$currentHash = ($allSrcFiles | ForEach-Object { ([System.IO.FileInfo]::new($_)).LastWriteTime.Ticks }) -join ","

if ($inoInfo.Exists -and [System.IO.File]::Exists($hashCacheFile) -and [System.IO.File]::Exists($hexFile)) {
	$cachedHash = [System.IO.File]::ReadAllText($hashCacheFile).Trim()
	if ($currentHash -eq $cachedHash) { $needCompile = $false }
}
Write-Host "Cache check: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 4. コンパイル（arduino-cliを完全バイパス）
# ---------------------------------------------------------
$buildTime = 0.0
$sw.Restart()
if ($needCompile) {
	# core.aが存在しない場合はarduino-cliで一度だけフルビルド（初回のみ）
	if (-not (Test-Path $coreA)) {
		Write-Host "First build: generating core.a via arduino-cli..." -ForegroundColor Yellow
		$buildOut = & arduino-cli compile -b arduino:avr:megaADK -j 0 --build-path $BuildPath --warnings none $SketchDir 2>&1
		if ($LASTEXITCODE -ne 0) {
			$buildOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Build failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}
		[System.IO.File]::WriteAllText($hashCacheFile, $currentHash)
		$buildTime = $sw.Elapsed.TotalSeconds
		Write-Host "Compile (full): $($buildTime)s"
	}
 else {
		# -------------------------------------------------------
		# 差分ビルド: スケッチ1ファイルのみ avr-g++ → リンク → objcopy
		# -------------------------------------------------------

		# Step0: .ino → .cpp を再生成
		# .ino ファイルの内容が変わっているため、必ず .cpp を書き直す。
		# arduino-cli が付加する #include <Arduino.h> をヘッダとして先頭に追加する。
		# （関数プロトタイプの自動挿入は avr-g++ の -fpermissive で吸収できる）
		$inoContent = [System.IO.File]::ReadAllText($inoFile, [System.Text.Encoding]::UTF8)
		$cppContent = "#include <Arduino.h>`r`n" + $inoContent
		if (-not (Test-Path $sketchBuild)) { New-Item -ItemType Directory -Force -Path $sketchBuild | Out-Null }
		[System.IO.File]::WriteAllText($cppFile, $cppContent, [System.Text.Encoding]::UTF8)

		# Step1: コンパイル
		$compileArgs = @(
			"-c", "-g", "-Os", "-w",
			"-std=gnu++11", "-fpermissive", "-fno-exceptions",
			"-ffunction-sections", "-fdata-sections",
			"-fno-threadsafe-statics", "-Wno-error=narrowing",
			"-MMD", "-flto",
			"-mmcu=atmega2560",
			"-DF_CPU=16000000L", "-DARDUINO=10607",
			"-DARDUINO_AVR_ADK", "-DARDUINO_ARCH_AVR",
			"-I$coresInc", "-I$variantsInc",
			$cppFile, "-o", $objFile
		)
		$compileOut = & $avrGpp @compileArgs 2>&1
		if ($LASTEXITCODE -ne 0) {
			$compileOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Compile failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}

		# Step2: リンク
		$linkArgs = @(
			"-w", "-Os", "-g", "-flto", "-fuse-linker-plugin",
			"-Wl,--gc-sections",
			"-mmcu=atmega2560",
			"-o", $elfFile,
			$objFile, $coreA,
			"-L$BuildPath", "-lm"
		)
		$linkOut = & $avrGcc @linkArgs 2>&1
		if ($LASTEXITCODE -ne 0) {
			$linkOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Link failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}

		# Step3: eep生成
		$null = & $avrObjcopy -O ihex -j .eeprom `
			--set-section-flags=.eeprom=alloc, load `
			--no-change-warnings --change-section-lma .eeprom=0 `
			$elfFile $eepFile 2>&1

		# Step4: hex生成
		$objcopyOut = & $avrObjcopy -O ihex -R .eeprom $elfFile $hexFile 2>&1
		if ($LASTEXITCODE -ne 0) {
			$objcopyOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Objcopy failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}

		[System.IO.File]::WriteAllText($hashCacheFile, $currentHash)
		$buildTime = $sw.Elapsed.TotalSeconds
		Write-Host "Compile (incremental): $($buildTime)s"
	}
}
else {
	Write-Host "Compile: skipped (no changes)"
}

# ---------------------------------------------------------
# 5. 書き込み
# ---------------------------------------------------------
$sw.Restart()
$avrdudeArgs = @("-C", $avrdudeConf, "-q", "-q", "-V", "-p", "m2560", "-c", "wiring", "-P", $TargetPort, "-b", "115200", "-D", "-U", "flash:w:${hexFile}:i")
$null = & $avrdude @avrdudeArgs 2>&1
$avrdudeExit = $LASTEXITCODE

$uploadTime = $sw.Elapsed.TotalSeconds
Write-Host "Upload: $($uploadTime)s"

if ($avrdudeExit -ne 0) {
	[System.IO.File]::Delete($portCacheFile)
	Write-Host "`n>>> Upload failed. Port cache cleared.`n" -ForegroundColor Red
	exit $avrdudeExit
}

# ---------------------------------------------------------
# 6. トータルタイム表示
# ---------------------------------------------------------
$totalTime = $totalSw.Elapsed.TotalSeconds
$line = "`nBuild: {0:F2}s  Upload: {1:F2}s  Total: {2:F2}s`n" -f $buildTime, $uploadTime, $totalTime
[Console]::WriteLine($line)
exit 0
