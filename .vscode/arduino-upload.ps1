param(
    [string]$BuildPath,
    [string]$SketchDir,
    [string]$Port = 'COM3'
)

Write-Host ">>> Uploading to Arduino...`n" -ForegroundColor Cyan

# path resolution
$avrdudeRoot = "$env:LOCALAPPDATA\Arduino15\packages\arduino\tools\avrdude"
$dirs = [System.IO.Directory]::GetDirectories($avrdudeRoot)
$avrdudeDir = $dirs[$dirs.Length - 1] 

$avrdude     = "$avrdudeDir\bin\avrdude.exe"
$avrdudeConf = "$avrdudeDir\etc\avrdude.conf"
$sketchName  = [System.IO.Path]::GetFileName($SketchDir)
$hexFile     = "$BuildPath\$sketchName.ino.hex"

# timer start
$sw = [System.Diagnostics.Stopwatch]::StartNew()

# build
arduino-cli compile -b arduino:avr:megaADK -j 0 --build-path $BuildPath $SketchDir
$buildTime = $sw.Elapsed.TotalSeconds

# write
$sw.Restart()
& $avrdude -C $avrdudeConf -q -q -V -p m2560 -c wiring -P $Port -b 115200 -D -U "flash:w:${hexFile}:i"
$uploadTime = $sw.Elapsed.TotalSeconds

# output
$totalTime = $buildTime + $uploadTime
[Console]::WriteLine( ("`nBuild: {0:F2}s  Upload: {1:F2}s  Total: {2:F2}s`n" -f $buildTime, $uploadTime, $totalTime) )