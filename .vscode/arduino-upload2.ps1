Param(
	[string]$BuildPath,
	[string]$SketchDir,
	[string]$Port = ''
)
Write-Host ">>> Uploading to Arduino...`n"

$totalSw = [System.Diagnostics.Stopwatch]::StartNew()
$sw = [System.Diagnostics.Stopwatch]::StartNew()

# ---------------------------------------------------------
# 1. ポート検出
# ---------------------------------------------------------
$globalCacheDir = "$env:LOCALAPPDATA\ArduinoCLI_Cache"
$portCacheFile = "$globalCacheDir\port_cache.txt"

$TargetPort = ""
try {
	$TargetPort = [System.IO.File]::ReadAllText($portCacheFile).Trim()
} catch {}

if ($TargetPort -eq "") {
	Write-Host "Port cache miss, scanning..." -ForegroundColor Yellow
	try {
		$pnpDevices = Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%(COM%)'" -ErrorAction Stop
		$target = $pnpDevices | Where-Object { $_.Caption -match 'Arduino|Mega|USB Serial|CH340|CP210' } | Select-Object -First 1
		if ($target -and $target.Caption -match '\((COM\d+)\)') {
			$TargetPort = $matches[1]
		}
	} catch {}

	if ($TargetPort -eq "") {
		$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
		if ($availablePorts.Count -gt 0) {
			$TargetPort = $availablePorts[-1]
		} else {
			Write-Host ">>> Error: No COM port found." -ForegroundColor Red
			exit 1
		}
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
$avrdude = ""
$avrdudeConf = ""

try {
	$lines = [System.IO.File]::ReadAllLines($avrdudeCacheFile)
	$avrdude     = $lines[0]
	$avrdudeConf = $lines[1]
} catch {}

if ($avrdude -eq "") {
	$avrdudeRoot = "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avrdude"
	$avrdudeDir  = [System.IO.Directory]::GetDirectories($avrdudeRoot)[-1]
	$avrdude     = "$avrdudeDir\bin\avrdude.exe"
	$avrdudeConf = "$avrdudeDir\etc\avrdude.conf"
	[System.IO.File]::WriteAllLines($avrdudeCacheFile, [string[]]@($avrdude, $avrdudeConf))
}

Write-Host "Avrdude path resolve: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 3. 変更検知
# ---------------------------------------------------------
$sw.Restart()
$sketchName    = [System.IO.Path]::GetFileName($SketchDir)
$inoFile       = "$SketchDir\$sketchName.ino"
$hexFile       = "$BuildPath\$sketchName.ino.hex"
$hashCacheFile = "$BuildPath\$sketchName.ino.sha256"

$inoInfo       = [System.IO.FileInfo]::new($inoFile)
$hashCacheInfo = [System.IO.FileInfo]::new($hashCacheFile)
$needCompile   = $true

if ($inoInfo.Exists -and $hashCacheInfo.Exists -and [System.IO.File]::Exists($hexFile)) {
	$currentHash = $inoInfo.LastWriteTime.Ticks.ToString()
	$cachedHash  = [System.IO.File]::ReadAllText($hashCacheFile).Trim()
	if ($currentHash -eq $cachedHash) { $needCompile = $false }
}
Write-Host "Cache check: $($sw.Elapsed.TotalMilliseconds)ms"

# ---------------------------------------------------------
# 4. コンパイル（$null = & ... 2>&1 でシンプルに破棄）
# ---------------------------------------------------------
$buildTime = 0.0
$sw.Restart()
if ($needCompile) {
	$currentHash = $inoInfo.LastWriteTime.Ticks.ToString()
	$null = & arduino-cli compile -b arduino:avr:megaADK -j 0 --build-path $BuildPath --warnings none --quiet $SketchDir 2>&1
	$buildTime = $sw.Elapsed.TotalSeconds
	if ($LASTEXITCODE -ne 0) {
		Write-Host "`n>>> Build failed.`n" -ForegroundColor Red
		exit $LASTEXITCODE
	}
	[System.IO.File]::WriteAllText($hashCacheFile, $currentHash)
	Write-Host "Compile: $($buildTime)s"
} else {
	Write-Host "Compile: skipped (no changes)"
}

# ---------------------------------------------------------
# 5. 書き込み（同様に $null = & ... 2>&1）
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
$line = "`nBuild: {0:F2}s  Upload: {1:F2}s  Total: {2:F2}s" -f $buildTime, $uploadTime, $totalTime
[Console]::WriteLine($line)
exit 0