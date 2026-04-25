Param(
	[string]$BuildPath,
	[string]$SketchDir,
	[string]$Port = '',
	[switch]$ForceUpload
)

$utf8EncodingNoBom = [System.Text.UTF8Encoding]::new($false)

Write-Host ">>> Uploading to Arduino...`n"

$totalSw = [System.Diagnostics.Stopwatch]::StartNew()
$sw = [System.Diagnostics.Stopwatch]::StartNew()

function Exit-Error {
	param(
		[string]$Message,
		[int]$Code = 1
	)
	Write-Host ">>> Error: $Message" -ForegroundColor Red
	exit $Code
}

function Get-SubdirectoryWithLatestWriteTime {
	param([string]$BaseDir)
	if (-not [System.IO.Directory]::Exists($BaseDir)) { return "" }

	$dirs = [System.IO.Directory]::GetDirectories($BaseDir)
	if ($dirs.Count -eq 0) { return "" }

	$latestDir = $dirs[0]
	$latestTicks = [System.IO.Directory]::GetLastWriteTimeUtc($latestDir).Ticks
	for ($i = 1; $i -lt $dirs.Count; $i++) {
		$ticks = [System.IO.Directory]::GetLastWriteTimeUtc($dirs[$i]).Ticks
		if ($ticks -gt $latestTicks) {
			$latestTicks = $ticks
			$latestDir = $dirs[$i]
		}
	}
	return $latestDir
}

function Resolve-CachedDirectory {
	param(
		[string]$CacheFile,
		[string]$BaseDir,
		[string]$ErrorMessage,
		[string]$Suffix = ''
	)

	$cached = ''
	try { $cached = [System.IO.File]::ReadAllText($CacheFile).Trim() } catch {}
	if ($cached -and [System.IO.Directory]::Exists($cached)) { return $cached }

	$latest = Get-SubdirectoryWithLatestWriteTime -BaseDir $BaseDir
	if (-not $latest) { Exit-Error $ErrorMessage }

	$result = "$latest$Suffix"
	[System.IO.File]::WriteAllText($CacheFile, $result, $utf8EncodingNoBom)
	return $result
}

function Resolve-AvrdudePaths {
	param(
		[string]$CacheFile,
		[string]$RootDir
	)

	try {
		$lines = [System.IO.File]::ReadAllLines($CacheFile)
		if ($lines.Count -ge 2 -and [System.IO.File]::Exists($lines[0]) -and [System.IO.File]::Exists($lines[1])) {
			return [string[]]@($lines[0], $lines[1])
		}
	}
	catch {}

	$latest = Get-SubdirectoryWithLatestWriteTime -BaseDir $RootDir
	if (-not $latest) { Exit-Error "avrdude not found under $RootDir" }

	$exe = [System.IO.Path]::Combine($latest, 'bin', 'avrdude.exe')
	$conf = [System.IO.Path]::Combine($latest, 'etc', 'avrdude.conf')
	if (-not [System.IO.File]::Exists($exe) -or -not [System.IO.File]::Exists($conf)) {
		Exit-Error "avrdude files not found under $latest"
	}

	[System.IO.File]::WriteAllLines($CacheFile, [string[]]@($exe, $conf), $utf8EncodingNoBom)
	return [string[]]@($exe, $conf)
}

function Resolve-Port {
	param(
		[string]$ProvidedPort,
		[string]$CacheFile
	)

	if ($ProvidedPort) { return $ProvidedPort }

	try {
		$cached = [System.IO.File]::ReadAllText($CacheFile).Trim()
		if ($cached) { return $cached }
	}
	catch {}

	Write-Host "Port cache miss, scanning..." -ForegroundColor Yellow

	$ports = [System.IO.Ports.SerialPort]::GetPortNames()
	if ($ports -and $ports.Count -gt 0) {
		$resolved = $ports[-1]
		[System.IO.File]::WriteAllText($CacheFile, $resolved, $utf8EncodingNoBom)
		return $resolved
	}

	try {
		$pnpDevices = Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%(COM%)'" -ErrorAction Stop
		$devicePattern = 'Arduino|Mega|USB Serial|CH340|CP210'
		for ($i = 0; $i -lt $pnpDevices.Count; $i++) {
			$caption = $pnpDevices[$i].Caption
			if ($caption -and $caption -match $devicePattern -and $caption -match '\((COM\d+)\)') {
				$resolved = $matches[1]
				[System.IO.File]::WriteAllText($CacheFile, $resolved, $utf8EncodingNoBom)
				return $resolved
			}
		}
	}
	catch {}

	Exit-Error 'No COM port found.'
}

function Get-SourceHash {
	param([string[]]$Files)

	$initialCapacity = 256
	$sb = [System.Text.StringBuilder]::new($initialCapacity)
	for ($i = 0; $i -lt $Files.Count; $i++) {
		if ($i -gt 0) { [void]$sb.Append(';') }
		[void]$sb.Append([System.IO.File]::GetLastWriteTimeUtc($Files[$i]).Ticks)
	}
	return $sb.ToString()
}

$globalCacheDir = [System.IO.Path]::Combine($env:LOCALAPPDATA, 'ArduinoCLI_Cache')
[System.IO.Directory]::CreateDirectory($globalCacheDir) | Out-Null

$portCacheFile = [System.IO.Path]::Combine($globalCacheDir, 'port_cache.txt')
$avrGccCacheFile = [System.IO.Path]::Combine($globalCacheDir, 'avr_gcc_path_cache.txt')
$hwCacheFile = [System.IO.Path]::Combine($globalCacheDir, 'hw_path_cache.txt')
$avrdudeCacheFile = [System.IO.Path]::Combine($globalCacheDir, 'avrdude_path_cache.txt')

$avrGccRoot = Resolve-CachedDirectory `
	-CacheFile $avrGccCacheFile `
	-BaseDir ([System.IO.Path]::Combine($env:LOCALAPPDATA, 'Arduino15\packages\arduino\tools\avr-gcc')) `
	-ErrorMessage "avr-gcc not found under $env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avr-gcc" `
	-Suffix '\bin'

$avrGpp = [System.IO.Path]::Combine($avrGccRoot, 'avr-g++.exe')
$avrGcc = [System.IO.Path]::Combine($avrGccRoot, 'avr-gcc.exe')
$avrObjcopy = [System.IO.Path]::Combine($avrGccRoot, 'avr-objcopy.exe')

$hwRoot = Resolve-CachedDirectory `
	-CacheFile $hwCacheFile `
	-BaseDir ([System.IO.Path]::Combine($env:LOCALAPPDATA, 'Arduino15\packages\arduino\hardware\avr')) `
	-ErrorMessage "arduino hardware not found under $env:LOCALAPPDATA\Arduino15\packages\arduino\hardware\avr"

$coresInc = [System.IO.Path]::Combine($hwRoot, 'cores\arduino')
$variantsInc = [System.IO.Path]::Combine($hwRoot, 'variants\mega')
$TargetPort = Resolve-Port -ProvidedPort $Port -CacheFile $portCacheFile

Write-Host "Using Port: $TargetPort`n"
Write-Host "Find port: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 2. avrdudeのパス解決
# ---------------------------------------------------------
$sw.Restart()
$avrdudePaths = Resolve-AvrdudePaths `
	-CacheFile $avrdudeCacheFile `
	-RootDir ([System.IO.Path]::Combine($env:LOCALAPPDATA, 'Arduino15\packages\arduino\tools\avrdude'))
$avrdude = $avrdudePaths[0]
$avrdudeConf = $avrdudePaths[1]
Write-Host "Avrdude path resolve: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 3. パス構築
# ---------------------------------------------------------
$sw.Restart()
$sketchName = [System.IO.Path]::GetFileName($SketchDir)
$inoFile = [System.IO.Path]::Combine($SketchDir, "$sketchName.ino")
$sketchBuild = [System.IO.Path]::Combine($BuildPath, 'sketch')
$cppFile = [System.IO.Path]::Combine($sketchBuild, "$sketchName.ino.cpp")
$objFile = [System.IO.Path]::Combine($sketchBuild, "$sketchName.ino.cpp.o")
$elfFile = [System.IO.Path]::Combine($BuildPath, "$sketchName.ino.elf")
$hexFile = [System.IO.Path]::Combine($BuildPath, "$sketchName.ino.hex")
$coreA = [System.IO.Path]::Combine($BuildPath, 'core\core.a')
$hashCacheFile = [System.IO.Path]::Combine($BuildPath, "$sketchName.ino.hash")
$uploadHashCacheFile = [System.IO.Path]::Combine($BuildPath, "$sketchName.ino.uploaded.hash")

if (-not [System.IO.File]::Exists($inoFile)) { Exit-Error "Sketch file not found: $inoFile" }

$headerFiles = [System.IO.Directory]::GetFiles($SketchDir, '*.h')
$srcFiles = [string[]]@($inoFile) + $headerFiles
$currentHash = Get-SourceHash -Files $srcFiles

$needCompile = $true
if ([System.IO.File]::Exists($hashCacheFile) -and [System.IO.File]::Exists($hexFile)) {
	try {
		if ([System.IO.File]::ReadAllText($hashCacheFile).Trim() -eq $currentHash) {
			$needCompile = $false
		}
	}
	catch {}
}

Write-Host "Cache check: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 4. コンパイル
# ---------------------------------------------------------
$buildTime = 0.0
$sw.Restart()
if ($needCompile) {
	if (-not [System.IO.File]::Exists($coreA)) {
		Write-Host "First build: generating core.a via arduino-cli..." -ForegroundColor Yellow
		$buildOut = & arduino-cli compile -b arduino:avr:megaADK -j 0 --build-path $BuildPath --warnings none $SketchDir 2>&1
		if ($LASTEXITCODE -ne 0) {
			$buildOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Build failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}
		[System.IO.File]::WriteAllText($hashCacheFile, $currentHash, $utf8EncodingNoBom)
		$buildTime = $sw.Elapsed.TotalSeconds
		Write-Host "Compile (full): $($buildTime)s"
	}
	else {
		[System.IO.Directory]::CreateDirectory($sketchBuild) | Out-Null
		$staleHeaders = [System.IO.Directory]::GetFiles($sketchBuild, '*.h')
		for ($i = 0; $i -lt $staleHeaders.Count; $i++) {
			try { [System.IO.File]::Delete($staleHeaders[$i]) }
			catch {
				Exit-Error "Failed to clean stale header: $($staleHeaders[$i]) ($($_.Exception.Message))"
			}
		}

		$inoContent = [System.IO.File]::ReadAllText($inoFile, [System.Text.Encoding]::UTF8)
		$cppContent = "#include <Arduino.h>`r`n$inoContent"
		[System.IO.File]::WriteAllText($cppFile, $cppContent, $utf8EncodingNoBom)

		$compileArgs = @(
			'-c', '-g', '-Os', '-w',
			'-std=gnu++11', '-fpermissive', '-fno-exceptions',
			'-ffunction-sections', '-fdata-sections',
			'-fno-threadsafe-statics', '-Wno-error=narrowing',
			'-MMD', '-flto',
			'-mmcu=atmega2560',
			'-DF_CPU=16000000L', '-DARDUINO=10607',
			'-DARDUINO_AVR_ADK', '-DARDUINO_ARCH_AVR',
			"-I$coresInc", "-I$variantsInc", "-I$SketchDir",
			$cppFile, '-o', $objFile
		)
		$compileOut = & $avrGpp @compileArgs 2>&1
		if ($LASTEXITCODE -ne 0) {
			$compileOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Compile failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}

		$linkArgs = @(
			'-w', '-Os', '-g', '-flto', '-fuse-linker-plugin',
			'-Wl,--gc-sections',
			'-mmcu=atmega2560',
			'-o', $elfFile,
			$objFile, $coreA,
			"-L$BuildPath", '-lm'
		)
		$linkOut = & $avrGcc @linkArgs 2>&1
		if ($LASTEXITCODE -ne 0) {
			$linkOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Link failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}

		$objcopyOut = & $avrObjcopy -O ihex -R .eeprom $elfFile $hexFile 2>&1
		if ($LASTEXITCODE -ne 0) {
			$objcopyOut | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			Write-Host "`n>>> Objcopy failed.`n" -ForegroundColor Red; exit $LASTEXITCODE
		}

		[System.IO.File]::WriteAllText($hashCacheFile, $currentHash, $utf8EncodingNoBom)
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
$uploadCacheKey = "$currentHash|$TargetPort"
$skipUpload = $false
if (-not $ForceUpload -and [System.IO.File]::Exists($uploadHashCacheFile)) {
	try {
		$lastUploaded = [System.IO.File]::ReadAllText($uploadHashCacheFile).Trim()
		if ($lastUploaded -eq $uploadCacheKey) { $skipUpload = $true }
	}
	catch {}
}

$uploadTime = 0.0
$uploadDisplay = 'skipped'
if ($skipUpload) {
	Write-Host 'Upload: skipped (no changes detected since last upload; use -ForceUpload to override)'
}
else {
	$avrdudeArgs = @(
		'-C', $avrdudeConf,
		'-q', '-q',
		'-V',
		'-p', 'm2560',
		'-c', 'wiring',
		'-P', $TargetPort,
		'-b', '115200',
		'-D',
		"-U", "flash:w:${hexFile}:i"
	)

	$null = & $avrdude @avrdudeArgs 2>&1
	$avrdudeExit = $LASTEXITCODE

	$uploadTime = $sw.Elapsed.TotalSeconds
	$uploadDisplay = "{0:F2}s" -f $uploadTime
	Write-Host "Upload: $($uploadTime)s"

	if ($avrdudeExit -ne 0) {
		try { [System.IO.File]::Delete($portCacheFile) } catch {}
		Write-Host "`n>>> Upload failed. Port cache cleared.`n" -ForegroundColor Red
		exit $avrdudeExit
	}

	try {
		[System.IO.File]::WriteAllText($uploadHashCacheFile, $uploadCacheKey, $utf8EncodingNoBom)
	}
	catch {
		Write-Host "Warning: failed to update upload cache: $($_.Exception.Message)" -ForegroundColor Yellow
	}
}

# ---------------------------------------------------------
# 6. トータルタイム表示
# ---------------------------------------------------------
$totalTime = $totalSw.Elapsed.TotalSeconds
$line = "`nBuild: {0:F2}s  Upload: {1}  Total: {2:F2}s`n" -f $buildTime, $uploadDisplay, $totalTime
[Console]::WriteLine($line)
exit 0
