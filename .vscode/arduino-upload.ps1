Param(
	[string]$BuildPath,
	[string]$SketchDir,
	[string]$Port = 'COM3'
)
Write-Host ">>> Uploading to Arduino...`n" -ForegroundColor Cyan

$totalSw = [System.Diagnostics.Stopwatch]::StartNew()

$sw = [System.Diagnostics.Stopwatch]::StartNew()
$availablePorts = [System.IO.Ports.SerialPort]::GetPortNames()
Write-Host "GetPortNames: $($sw.Elapsed.TotalMilliseconds)ms"

if ($availablePorts -notcontains $Port) {
	Write-Host ">>> Port $Port not found. Available ports: $($availablePorts -join ', ')" -ForegroundColor Red
	exit 1
}

$sw.Restart()
$avrdudeRoot = "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avrdude"
$dirs = [System.IO.Directory]::GetDirectories($avrdudeRoot)
$avrdudeDir = $dirs[$dirs.Length - 1]
$avrdude = "$avrdudeDir\bin\avrdude.exe"
$avrdudeConf = "$avrdudeDir\etc\avrdude.conf"
Write-Host "avrdude path resolve: $($sw.Elapsed.TotalMilliseconds)ms"

$sketchName = [System.IO.Path]::GetFileName($SketchDir)
$inoFile = "$SketchDir\$sketchName.ino"
$hexFile = "$BuildPath\$sketchName.ino.hex"
$hashCacheFile = "$BuildPath\$sketchName.ino.sha256"

$sw.Restart()
$inoInfo = [System.IO.FileInfo]::new($inoFile)
$hashCacheInfo = [System.IO.FileInfo]::new($hashCacheFile)
$needCompile = $true
if ($inoInfo.Exists -and $hashCacheInfo.Exists) {
	$currentHash = $inoInfo.LastWriteTime.Ticks.ToString()
	$cachedHash = [System.IO.File]::ReadAllText($hashCacheFile).Trim()
	if ($currentHash -eq $cachedHash) { $needCompile = $false }
}
Write-Host "Cache check: $($sw.Elapsed.TotalMilliseconds)ms"

$buildTime = 0.0
$uploadTime = 0.0

if ($needCompile) {
	$currentHash = $inoInfo.LastWriteTime.Ticks.ToString()

	$sw.Restart()
	arduino-cli compile -b arduino:avr:megaADK -j 0 --build-path $BuildPath --warnings none --quiet $SketchDir
	
	$buildTime = $sw.Elapsed.TotalSeconds
	Write-Host "Compile: $($buildTime)s"

	if ($LASTEXITCODE -ne 0) {
		Write-Host "`n>>> Build failed.`n" -ForegroundColor Red
		exit $LASTEXITCODE
	}

	[System.IO.File]::WriteAllText($hashCacheFile, $currentHash)

	$sw.Restart()
	& $avrdude -C $avrdudeConf -q -q -V -p m2560 -c wiring -P $Port -b 115200 -D -U "flash:w:${hexFile}:i"
	$avrdudeExit = $LASTEXITCODE
	$uploadTime = $sw.Elapsed.TotalSeconds
	Write-Host "Upload: $($uploadTime)s"

	if ($avrdudeExit -ne 0) {
		Write-Host "`n>>> Upload failed.`n" -ForegroundColor Red
		exit $avrdudeExit
	}
}
else {
	Write-Host ">> Skip compiling and uploading (No changes detected).`n" -ForegroundColor Yellow
}

$totalTime = $totalSw.Elapsed.TotalSeconds
$line = "`nBuild: {0:F2}s  Upload: {1:F2}s  Total: {2:F2}s`n" -f $buildTime, $uploadTime, $totalTime
[Console]::WriteLine($line)
exit 0