# ---------------------------------------------------------------------------------------------
#   Copyright (c) Microsoft Corporation. All rights reserved.
#   Licensed under the MIT License. See License.txt in the project root for license information.
# ---------------------------------------------------------------------------------------------

# Prevent installing more than once per session
if ((Test-Path variable:global:__VSCodeState) -and $null -ne $Global:__VSCodeState.OriginalPrompt) {
	return;
}

# Disable shell integration when the language mode is restricted
if ($ExecutionContext.SessionState.LanguageMode -ne "FullLanguage") {
	return;
}

$Global:__VSCodeState = @{
	OriginalPrompt = $function:Prompt
	LastHistoryId = -1
	IsInExecution = $false
	EnvVarsToReport = @()
	Nonce = $null
	IsStable = $null
	IsA11yMode = $null
	IsWindows10 = $false
}

# Store the nonce in a regular variable and unset the environment variable. It's by design that
# anything that can execute PowerShell code can read the nonce, as it's basically impossible to hide
# in PowerShell. The most important thing is getting it out of the environment.
$Global:__VSCodeState.Nonce = $env:VSCODE_NONCE
$env:VSCODE_NONCE = $null

$Global:__VSCodeState.IsStable = $env:VSCODE_STABLE
$env:VSCODE_STABLE = $null

$Global:__VSCodeState.IsA11yMode = $env:VSCODE_A11Y_MODE
$env:VSCODE_A11Y_MODE = $null

$__vscode_shell_env_reporting = $env:VSCODE_SHELL_ENV_REPORTING
$env:VSCODE_SHELL_ENV_REPORTING = $null
if ($__vscode_shell_env_reporting) {
	$Global:__VSCodeState.EnvVarsToReport = $__vscode_shell_env_reporting.Split(',')
}
Remove-Variable -Name __vscode_shell_env_reporting -ErrorAction SilentlyContinue

$osVersion = [System.Environment]::OSVersion.Version
$Global:__VSCodeState.IsWindows10 = $IsWindows -and $osVersion.Major -eq 10 -and $osVersion.Minor -eq 0 -and $osVersion.Build -lt 22000
Remove-Variable -Name osVersion -ErrorAction SilentlyContinue

if ($env:VSCODE_ENV_REPLACE) {
	$Split = $env:VSCODE_ENV_REPLACE.Split(":")
	foreach ($Item in $Split) {
		$Inner = $Item.Split('=', 2)
		[Environment]::SetEnvironmentVariable($Inner[0], $Inner[1].Replace('\x3a', ':'))
	}
	$env:VSCODE_ENV_REPLACE = $null
}
if ($env:VSCODE_ENV_PREPEND) {
	$Split = $env:VSCODE_ENV_PREPEND.Split(":")
	foreach ($Item in $Split) {
		$Inner = $Item.Split('=', 2)
		[Environment]::SetEnvironmentVariable($Inner[0], $Inner[1].Replace('\x3a', ':') + [Environment]::GetEnvironmentVariable($Inner[0]))
	}
	$env:VSCODE_ENV_PREPEND = $null
}
if ($env:VSCODE_ENV_APPEND) {
	$Split = $env:VSCODE_ENV_APPEND.Split(":")
	foreach ($Item in $Split) {
		$Inner = $Item.Split('=', 2)
		[Environment]::SetEnvironmentVariable($Inner[0], [Environment]::GetEnvironmentVariable($Inner[0]) + $Inner[1].Replace('\x3a', ':'))
	}
	$env:VSCODE_ENV_APPEND = $null
}

# Register Python shell activate hooks
# Prevent multiple activation with guard
if (-not $env:VSCODE_PYTHON_AUTOACTIVATE_GUARD) {
	$env:VSCODE_PYTHON_AUTOACTIVATE_GUARD = '1'
	if ($env:VSCODE_PYTHON_PWSH_ACTIVATE -and $env:TERM_PROGRAM -eq 'vscode') {
		$activateScript = $env:VSCODE_PYTHON_PWSH_ACTIVATE

		try {
			Invoke-Expression $activateScript
			$Global:__VSCodeState.OriginalPrompt = $function:Prompt
		}
		catch {
			$activationError = $_
			Write-Host "`e[0m`e[7m * `e[0;103m VS Code Python powershell activation failed with exit code $($activationError.Exception.Message) `e[0m"
		}
	}
	# Remove any leftover Python activation env vars.
	Get-ChildItem Env:VSCODE_PYTHON_*_ACTIVATE | Remove-Item -ErrorAction SilentlyContinue
}

function Global:__VSCode-Escape-Value([string]$value) {
	# NOTE: In PowerShell v6.1+, this can be written `$value -replace '…', { … }` instead of `[regex]::Replace`.
	# Replace any non-alphanumeric characters.
	[regex]::Replace($value, "[$([char]0x00)-$([char]0x1f)\\\n;]", { param($match)
			# Encode the (ascii) matches as `\x<hex>`
			-Join (
				[System.Text.Encoding]::UTF8.GetBytes($match.Value) | ForEach-Object { '\x{0:x2}' -f $_ }
			)
		})
}

function Global:Prompt() {
	$FakeCode = [int]!$global:?
	# NOTE: We disable strict mode for the scope of this function because it unhelpfully throws an
	# error when $LastHistoryEntry is null, and is not otherwise useful.
	Set-StrictMode -Off
	$LastHistoryEntry = Get-History -Count 1
	$Result = ""
	# Skip finishing the command if the first command has not yet started or an execution has not
	# yet begun
	if ($Global:__VSCodeState.LastHistoryId -ne -1 -and ($Global:__VSCodeState.HasPSReadLine -eq $false -or $Global:__VSCodeState.IsInExecution -eq $true)) {
		$Global:__VSCodeState.IsInExecution = $false
		if ($LastHistoryEntry.Id -eq $Global:__VSCodeState.LastHistoryId) {
			# Don't provide a command line or exit code if there was no history entry (eg. ctrl+c, enter on no command)
			$Result += "$([char]0x1b)]633;D`a"
		}
		else {
			# Command finished exit code
			# OSC 633 ; D [; <ExitCode>] ST
			$Result += "$([char]0x1b)]633;D;$FakeCode`a"
		}
	}
	# Prompt started
	# OSC 633 ; A ST
	$Result += "$([char]0x1b)]633;A`a"
	# Current working directory
	# OSC 633 ; <Property>=<Value> ST
	$Result += if ($pwd.Provider.Name -eq 'FileSystem') { "$([char]0x1b)]633;P;Cwd=$(__VSCode-Escape-Value $pwd.ProviderPath)`a" }

	# Send current environment variables as JSON
	# OSC 633 ; EnvJson ; <Environment> ; <Nonce>
	if ($Global:__VSCodeState.EnvVarsToReport.Count -gt 0) {
		$envMap = @{}
        foreach ($varName in $Global:__VSCodeState.EnvVarsToReport) {
            if (Test-Path "env:$varName") {
                $envMap[$varName] = (Get-Item "env:$varName").Value
            }
        }
        $envJson = $envMap | ConvertTo-Json -Compress
        $Result += "$([char]0x1b)]633;EnvJson;$(__VSCode-Escape-Value $envJson);$($Global:__VSCodeState.Nonce)`a"
	}

	# Before running the original prompt, put $? back to what it was:
	if ($FakeCode -ne 0) {
		Write-Error "failure" -ea ignore
	}
	# Run the original prompt
	$OriginalPrompt += $Global:__VSCodeState.OriginalPrompt.Invoke()
	$Result += $OriginalPrompt

	# Prompt
	# OSC 633 ; <Property>=<Value> ST
	if ($Global:__VSCodeState.IsStable -eq "0") {
		$Result += "$([char]0x1b)]633;P;Prompt=$(__VSCode-Escape-Value $OriginalPrompt)`a"
	}

	# Write command started
	$Result += "$([char]0x1b)]633;B`a"
	$Global:__VSCodeState.LastHistoryId = $LastHistoryEntry.Id
	return $Result
}

# Report prompt type
if ($env:STARSHIP_SESSION_KEY) {
	[Console]::Write("$([char]0x1b)]633;P;PromptType=starship`a")
}
elseif ($env:POSH_SESSION_ID) {
	[Console]::Write("$([char]0x1b)]633;P;PromptType=oh-my-posh`a")
}
elseif ((Test-Path variable:global:GitPromptSettings) -and $Global:GitPromptSettings) {
	[Console]::Write("$([char]0x1b)]633;P;PromptType=posh-git`a")
}

if ($Global:__VSCodeState.IsA11yMode -eq "1") {
	# Check if the loaded PSReadLine already supports EnableScreenReaderMode
	$hasScreenReaderParam = (Get-Module -Name PSReadLine) -and (Get-Command Set-PSReadLineOption).Parameters.ContainsKey('EnableScreenReaderMode')

	if (-not $hasScreenReaderParam -and $PSVersionTable.PSVersion -ge "7.0") {
		# The loaded PSReadLine lacks EnableScreenReaderMode (only available in 2.4.4-beta4+).
		# PowerShell 7.0+ skips autoloading PSReadLine when the OS reports a screen reader active.
		# When only VS Code's accessibility mode is enabled (no OS screen reader),
		# it's still loaded and must be removed to load our bundled copy.
		# Skip this on Windows PowerShell 5.1 where removing the built-in PSReadLine 2.0.0
		# and replacing it can cause input handling issues (e.g. repeated Enter key presses).
		if (Get-Module -Name PSReadLine) {
			Remove-Module PSReadLine -Force
		}

		# Import VS Code's bundled PSReadLine 2.4.3 which has EnableScreenReaderMode
		$specialPsrlPath = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) 'psreadline'
		if (Test-Path $specialPsrlPath) {
			Import-Module $specialPsrlPath
		}

		$hasScreenReaderParam = (Get-Module -Name PSReadLine) -and (Get-Command Set-PSReadLineOption).Parameters.ContainsKey('EnableScreenReaderMode')
	}

	if ($hasScreenReaderParam) {
		Set-PSReadLineOption -EnableScreenReaderMode
	}
}

# Only send the command executed sequence when PSReadLine is loaded, if not shell integration should
# still work thanks to the command line sequence
$Global:__VSCodeState.HasPSReadLine = $false
if (Get-Module -Name PSReadLine) {
	$Global:__VSCodeState.HasPSReadLine = $true
	[Console]::Write("$([char]0x1b)]633;P;HasRichCommandDetection=True`a")

	$Global:__VSCodeState.OriginalPSConsoleHostReadLine = $function:PSConsoleHostReadLine
	function Global:PSConsoleHostReadLine {
		$CommandLine = $Global:__VSCodeState.OriginalPSConsoleHostReadLine.Invoke()
		$Global:__VSCodeState.IsInExecution = $true

		# Command line
		# OSC 633 ; E [; <CommandLine> [; <Nonce>]] ST
		$Result = "$([char]0x1b)]633;E;"
		$Result += $(__VSCode-Escape-Value $CommandLine)
		# Only send the nonce if the OS is not Windows 10 as it seems to echo to the terminal
		# sometimes
		if ($Global:__VSCodeState.IsWindows10 -eq $false) {
			$Result += ";$($Global:__VSCodeState.Nonce)"
		}
		$Result += "`a"

		# Command executed
		# OSC 633 ; C ST
		$Result += "$([char]0x1b)]633;C`a"

		# Write command executed sequence directly to Console to avoid the new line from Write-Host
		[Console]::Write($Result)

		$CommandLine
	}

	# Set ContinuationPrompt property
	$Global:__VSCodeState.ContinuationPrompt = (Get-PSReadLineOption).ContinuationPrompt
	if ($Global:__VSCodeState.ContinuationPrompt) {
		[Console]::Write("$([char]0x1b)]633;P;ContinuationPrompt=$(__VSCode-Escape-Value $Global:__VSCodeState.ContinuationPrompt)`a")
	}
}

# Set IsWindows property
if ($PSVersionTable.PSVersion -lt "6.0") {
	# Windows PowerShell is only available on Windows
	[Console]::Write("$([char]0x1b)]633;P;IsWindows=$true`a")
}
else {
	[Console]::Write("$([char]0x1b)]633;P;IsWindows=$IsWindows`a")
}

# Set always on key handlers which map to default VS Code keybindings
function Set-MappedKeyHandler {
	param ([string[]] $Chord, [string[]]$Sequence)
	try {
		$Handler = Get-PSReadLineKeyHandler -Chord $Chord | Select-Object -First 1
	}
 catch [System.Management.Automation.ParameterBindingException] {
		# PowerShell 5.1 ships with PSReadLine 2.0.0 which does not have -Chord,
		# so we check what's bound and filter it.
		$Handler = Get-PSReadLineKeyHandler -Bound | Where-Object -FilterScript { $_.Key -eq $Chord } | Select-Object -First 1
	}
	if ($Handler) {
		Set-PSReadLineKeyHandler -Chord $Sequence -Function $Handler.Function
	}
}

function Set-MappedKeyHandlers {
	Set-MappedKeyHandler -Chord Ctrl+Spacebar -Sequence 'F12,a'
	Set-MappedKeyHandler -Chord Alt+Spacebar -Sequence 'F12,b'
	Set-MappedKeyHandler -Chord Shift+Enter -Sequence 'F12,c'
	Set-MappedKeyHandler -Chord Shift+End -Sequence 'F12,d'
}

if ($Global:__VSCodeState.HasPSReadLine) {
	Set-MappedKeyHandlers

	# Prevent AI-executed commands from polluting shell history
	if ($env:VSCODE_PREVENT_SHELL_HISTORY -eq "1") {
		Set-PSReadLineOption -AddToHistoryHandler {
			param([string]$line)
			return $false
		}
		$env:VSCODE_PREVENT_SHELL_HISTORY = $null
	}
}

# SIG # Begin signature block
# MIIuyAYJKoZIhvcNAQcCoIIuuTCCLrUCAQExDzANBglghkgBZQMEAgEFADB5Bgor
# BgEEAYI3AgEEoGswaTA0BgorBgEEAYI3AgEeMCYCAwEAAAQQH8w7YFlLCE63JNLG
# KX7zUQIBAAIBAAIBAAIBAAIBADAxMA0GCWCGSAFlAwQCAQUABCAfGrwqijsIoxub
# jHYHcEKgTrE2cVfuLVugyXdXVzgN8aCCFAgwggYiMIIECqADAgECAhMzAAAAOqVM
# eg/pLY5WAAEAAAA6MA0GCSqGSIb3DQEBCwUAMIGHMQswCQYDVQQGEwJVUzETMBEG
# A1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWlj
# cm9zb2Z0IENvcnBvcmF0aW9uMTEwLwYDVQQDEyhNaWNyb3NvZnQgTWFya2V0cGxh
# Y2UgUHJvZHVjdGlvbiBDQSAyMDExMB4XDTI1MDYxOTE4NTQxNVoXDTI2MDYxNzE4
# NTQxNVowdDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNV
# BAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEeMBwG
# A1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMIIBIjANBgkqhkiG9w0BAQEFAAOC
# AQ8AMIIBCgKCAQEAt+78Zgve1ubGrUDoN6b38AwwRTsG3Se8MLvV19OCgewrUcuR
# EcXHc5cdJM/anZ826GOGXAjdDRwOZVDMHROsFKj2PICU012e5Hjs+p6vwaBPnnnB
# uUuydZaIp2WNSmN/asrooD6J8uQRHGsPbHXCJ6YpJVQoYSWRRVM84NQGv4eSHs0d
# 5oV3V4YTHoZ8Fd3pCARGU+y26WKuqJZKw1QIJQ8cbeQYG3YYLDGAg7FHme8QdOU6
# lB9j8dyYQ5QKsBTcLaHipJjTOs8Xk97Vlp/UdY5AwzynG9BoPiQhpiyuL+txj+tV
# de6H/sixUoHpHkR4bwbtZ2SEmwVnQ8+RdYhWnQIDAQABo4IBlzCCAZMwEwYDVR0l
# BAwwCgYIKwYBBQUHAwMwHQYDVR0OBBYEFLGyVe1sw+70Uzk4ufV2dFPjDoVJMEUG
# A1UdEQQ+MDykOjA4MR4wHAYDVQQLExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xFjAU
# BgNVBAUTDTIyOTk3OSs1MDUyOTYwHwYDVR0jBBgwFoAUnqf5oCNwnxHFaeOhjQr6
# 8bD01YAwbAYDVR0fBGUwYzBhoF+gXYZbaHR0cDovL3d3dy5taWNyb3NvZnQuY29t
# L3BraW9wcy9jcmwvTWljcm9zb2Z0JTIwTWFya2V0cGxhY2UlMjBQcm9kdWN0aW9u
# JTIwQ0ElMjAyMDExKDEpLmNybDB5BggrBgEFBQcBAQRtMGswaQYIKwYBBQUHMAKG
# XWh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2lvcHMvY2VydHMvTWljcm9zb2Z0
# JTIwTWFya2V0cGxhY2UlMjBQcm9kdWN0aW9uJTIwQ0ElMjAyMDExKDEpLmNydDAM
# BgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4ICAQBBpSDW5NL6rmKT8ftzHOR+
# JbUg6yHKn19WmtZ6eowhPYG8m9cMpGM2+/6eXjX87Pf3UHC0Gqdg/DXnjavS3QAX
# qCLktsYiPdG877xNK3pWA25ZoP6hyIjeh/iFhqCgLDAHERBEb3hghPGep9jTQDxz
# S550721TvdZzLdYuaDKa11J1jxSgX5hKAkWrjHo/rfqSROLcP58zAXeOHPzUKrXW
# mVskEMnczZRTkFBQunCnikWTV8KKap5mNh59Go/Q8TSHvvRudMljYgOQrQZnFQAK
# /v0NOGv81z0jb5yRnK2A+T9SUviNiKtjo7zzproy3vBYdeWWontlFQqhIcSnd1Np
# MjYJEC0PHDS2JdvaJtjyYlPH5+xjAKDQztSazXte0IRyhCnz8dnmJMXzh+zd0hTk
# EuZ8l+3dphYb5CXBVvw7PhkOlAP5zOqPHi9nzuwK/iS4E4iZM5IdI+WY5H6jtzfk
# VxkoaEL6LTMs2bRBgj1eFsi2W/Eiqx0WBjoEFFPRiXTHb0rVLZOM1nbQ4lREsl8d
# pCJhQEBUYt5s6CsPRucMGHP+o4Uy/X2+IWaxxjWNXsc3PEYJGcOgQkp4gbPTQ29h
# YszDwvw9rDlA1X32AENHkJNh7V1EahIdciW/tzKQCf5BIKaYrWAY5Gefp+4iGmcN
# sIiGN7Lh/3VlyxF6dkMPFTCCBtcwggS/oAMCAQICCmESRKIAAAAAAAIwDQYJKoZI
# hvcNAQELBQAwgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
# DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
# MjAwBgNVBAMTKU1pY3Jvc29mdCBSb290IENlcnRpZmljYXRlIEF1dGhvcml0eSAy
# MDExMB4XDTExMDMyODIxMDkzOVoXDTMxMDMyODIxMTkzOVowfTELMAkGA1UEBhMC
# VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
# BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEnMCUGA1UEAxMeTWljcm9zb2Z0IE1h
# cmtldFBsYWNlIFBDQSAyMDExMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKC
# AgEAubUaSwGYVsE3MAnPfvmozUhAB3qxBABgJRW1vDp4+tVinXxD32f7k1K89JQ6
# zDOgS/iDgULC+yFK1K/1Qjac/0M7P6c8v5LSjnWGlERLa/qY32j46S7SLQcit3g2
# jgoTTO03eUG+9yHZUTGV/FJdRYB8uXhrznJBa+Y+yGwiQKF+m6XFeBH/KORoKFx+
# dmMoy9EWJ/m/o9IiUj2kzm9C691+vZ/I2w0Bj93W9SPPkV2PCNHlzgfIAoeajWpH
# mi38Wi3xZHonkzAVBHxPsCBppOoNsWvmAfUM7eBthkSPvFruekyDCPNEYhfGqgqt
# qLkoBebXLZCOVybF7wTQaLvse60//3P003icRcCoQYgY4NAqrF7j80o5U7DkeXxc
# B0xvengsaKgiAaV1DKkRbpe98wCqr1AASvm5rAJUYMU+mXmOieV2EelY2jGrenWe
# 9FQpNXYV1NoWBh0WKoFxttoWYAnF705bIWtSZsz08ZfK6WLX4GXNLcPBlgCzfTm1
# sdKYASWdBbH2haaNhPapFhQQBJHKwnVW2iXErImhuPi45W3MVTZ5D9ASshZx69cL
# YY6xAdIa+89Kf/uRrsGOVZfahDuDw+NI183iAyzC8z/QRt2P32LYxP0xrCdqVh+D
# Jo2i4NoE8Uk1usCdbVRuBMBQl/AwpOTq7IMvHGElf65CqzUCAwEAAaOCAUswggFH
# MBAGCSsGAQQBgjcVAQQDAgEAMB0GA1UdDgQWBBQPU8s/FmEl/mCJHdO5fOiQrbOU
# 0TAZBgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMCAYYwDwYDVR0T
# AQH/BAUwAwEB/zAfBgNVHSMEGDAWgBRyLToCMZBDuRQFTuHqp8cx0SOJNDBaBgNV
# HR8EUzBRME+gTaBLhklodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2NybC9w
# cm9kdWN0cy9NaWNSb29DZXJBdXQyMDExXzIwMTFfMDNfMjIuY3JsMF4GCCsGAQUF
# BwEBBFIwUDBOBggrBgEFBQcwAoZCaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3Br
# aS9jZXJ0cy9NaWNSb29DZXJBdXQyMDExXzIwMTFfMDNfMjIuY3J0MA0GCSqGSIb3
# DQEBCwUAA4ICAQCjuZmM8ZVNDgp9wHsL4RY8KJ8nLinvxFTphNGCrxaLknkYG5pm
# MhVlX+UB/tSiW8W13W60nggz9u5xwMx7v/1t/Tgm6g2brVyOKI5A7u6/2SIJwkJK
# Fw953K0YIKVT28w9zl8dSJnmRnyR0G86ncWbF6CLQ6A6lBQ9o2mTGVqDr4m35WKA
# nc6YxUUM1y74mbzFFZr63VHsCcOp3pXWnUqAY1rb6Q6NX1b3clncKqLFm0EjKHcQ
# 56grTbwuuB7pMdh/IFCJR01MQzQbDtpEisbOeZUi43YVAAHKqI1EO9bRwg3frCjw
# Abml9MmI4utMW94gWFgvrMxIX+n42RBDIjf3Ot3jkT6gt3XeTTmO9bptgblZimhE
# RdkFRUFpVtkocJeLoGuuzP93uH/Yp032wzRH+XmMgujfZv+vnfllJqxdowoQLx55
# FxLLeTeYfwi/xMSjZO2gNven3U/3KeSCd1kUOFS3AOrwZ0UNOXJeW5JQC6Vfd1Ba
# vFZ6FAta1fMLu3WFvNB+FqeHUaU3ya7rmtxJnzk29DeSqXgGNmVSywBS4NajI5jJ
# IKAA6UhNJlsg8CHYwUOKf5ej8OoQCkbadUxXygAfxCfW2YBbujtI+PoyejRFxWUj
# YFWO5LeTI62UMyqfOEiqugoYjNxmQZla2s4YHVuqIC34R85FQlg9pKQBsDCCBwMw
# ggTroAMCAQICEzMAAABVyAZrOCOXKQkAAAAAAFUwDQYJKoZIhvcNAQELBQAwfTEL
# MAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
# bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEnMCUGA1UEAxMeTWlj
# cm9zb2Z0IE1hcmtldFBsYWNlIFBDQSAyMDExMB4XDTIxMDkwOTIyNDIzMFoXDTMw
# MDkwOTIyNTIzMFowgYcxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
# MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRp
# b24xMTAvBgNVBAMTKE1pY3Jvc29mdCBNYXJrZXRwbGFjZSBQcm9kdWN0aW9uIENB
# IDIwMTEwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDHfQ3P+L0El1S6
# JNYAz70y3e1i7EZAYcCDVXde/nQdpOKtVr6H4QkBkROv7HBxY0U8lR9C3bUUZKn6
# CCcN3v3bQuYKu1Ff2G4nIIr8a1cB4iOU8i4YSN7bRr+5LvD5hyCfJHqXrJe5LRRG
# jws5aRAxYuGhQ3ypWPEZYfrIXmmYK+e+udApgxahHUPBqcbI2PT1PpkKDgqR7hyz
# W0CfWzRUwh+YoZpsVvDaEkxcHQe/yGJB5BluYyRm5K9z+YQqBvYJkNUisTE/9OIm
# naZqoujkEuhM5bBV/dNjw7YN37OcBuH0NvlQomLQo+V7PA519HVVE1kRQ8pFad6i
# 4YdRWpj/+1yFskRZ5m7y+dEdGyXAiFeIgaM6O1CFrA1LbMAvyaZpQwBkrT/etC0h
# w4BPmW70zSmSubMoHpx/UUTNo3fMUVqx6r2H1xsc4aXTpPN5IxjkGIQhPN6h3q5J
# C+JOPnsfDRg3Ive2Q22jj3tkNiOXrYpmkILk7v+4XUxDErdc/WLZ3sbF27hug7HS
# VbTCNA46scIqE7ZkgH3M7+8aP3iUBDNcYUWjO1u+P1Q6UUzFdShSbGbKf+Z3xpql
# wdxQq9kuUahACRQLMFjRUfmAqGXUdMXECRaFPTxl6SB/7IAcuK855beqNPcexVEp
# kSZxZJbnqjKWbyTk/GA1abW8zgfH2QIDAQABo4IBbzCCAWswEgYJKwYBBAGCNxUB
# BAUCAwEAATAjBgkrBgEEAYI3FQIEFgQUeBlfau2VIfkwk2K+EoAD6hZ05ccwHQYD
# VR0OBBYEFJ6n+aAjcJ8RxWnjoY0K+vGw9NWAMBkGCSsGAQQBgjcUAgQMHgoAUwB1
# AGIAQwBBMAsGA1UdDwQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMB8GA1UdIwQY
# MBaAFA9Tyz8WYSX+YIkd07l86JCts5TRMFcGA1UdHwRQME4wTKBKoEiGRmh0dHA6
# Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY01hclBDQTIw
# MTFfMjAxMS0wMy0yOC5jcmwwWwYIKwYBBQUHAQEETzBNMEsGCCsGAQUFBzAChj9o
# dHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRzL01pY01hclBDQTIwMTFf
# MjAxMS0wMy0yOC5jcnQwDQYJKoZIhvcNAQELBQADggIBACY4RaglNFzKOO+3zgaz
# CsgCvXca79D573wDc0DAj6KzBX9m4rHhAZqzBkfSWvanLFilDibWmbGUGbkuH0y2
# 9NEoLVHfY64PXmXcBWEWd1xK4QxyKx2VVDq9P9494Z/vXy9OsifTP8Gt2UkhftAQ
# McvKgGiAHtyRHda8r7oU4cc4ITZnMsgXv6GnMDVuIk+Cq0Eh93rgzKF2rJ1sJcra
# H/kgSkgawBYYdJlXXHTkOrfEPKU82BDT5h8SGsXVt5L1mwRzjVQRLs1FNPkA+Kqy
# z0L+UEXJZWldNtHC79XtYh/ysRov4Yu/wLF+c8Pm15ICn8EYJUL4ZKmk9ZM7ZcaU
# V/2XvBpufWE2rcMnS/dPHWIojQ1FTToqM+Ag2jZZ33fl8rJwnnIF/Ku4OZEN24wQ
# LYsOMHh6WKADxkXJhiYUwBe2vCMHDVLpbCY7CbPpQdtBYHEkto0MFADdyX50sNVg
# TKboPyCxPW6GLiR5R+qqzNRzpYru2pTsM6EodSTgcMbeaDZI7ssnv+NYMyWstE1I
# XQCUywLQohNDo6H7/HNwC8HtdsGd5j0j+WOIEO5PyCbjn5viNWWCUu7Ko6Qx68Nu
# xHf++swe9YQhufh0hzJnixidTRPkBUgYQ6xubG6I5g/2OO1BByOu9/jt5vMTTvct
# q2YWOhUjoOZPe53eYSzjvNydMYIaFjCCGhICAQEwgZ8wgYcxCzAJBgNVBAYTAlVT
# MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
# ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xMTAvBgNVBAMTKE1pY3Jvc29mdCBNYXJr
# ZXRwbGFjZSBQcm9kdWN0aW9uIENBIDIwMTECEzMAAAA6pUx6D+ktjlYAAQAAADow
# DQYJYIZIAWUDBAIBBQCggbAwGQYJKoZIhvcNAQkDMQwGCisGAQQBgjcCAQQwHAYK
# KwYBBAGCNwIBCzEOMAwGCisGAQQBgjcCARUwLwYJKoZIhvcNAQkEMSIEIIHqws2g
# 6cn0S7TNrQRrd8fHZs3usGlRT+9DpCUFIbPfMEQGCisGAQQBgjcCAQwxNjA0oBCA
# DgBWAFMAIABDAG8AZABloSCAHmh0dHBzOi8vY29kZS52aXN1YWxzdHVkaW8uY29t
# LzANBgkqhkiG9w0BAQEFAASCAQCoEzDxsdL/8iNmt3bKP/CD2QaVEIvLuLsajnhc
# m3CFUEWsDU2iL9Aw1/o4uvHovwWsm/gsNyke02mex6BjWVrdePi4KI4Lwq3pv6E6
# wgv6FMJgtsXqsjGyuP4f0Uo7g3071sWV6evMb0GEpSb/KQhGXlf1uw6np9y0FgO6
# bkVsLf60aPvp1Ay/19oBSAn5cjhGHw105V7UNmx1c46dmZ/8isd8GsiRW8NH7Ing
# VATtq7bG3+tPQ8k5wI8Njfz7728yUNhkxdaimXqqi2Ua0hL8jDwBu/GL5TyQZ2ug
# LgSgCff9R3N8luY6fvvbddaX0plHMq9fh9yEq4DpJoM/9AQdoYIXlDCCF5AGCisG
# AQQBgjcDAwExgheAMIIXfAYJKoZIhvcNAQcCoIIXbTCCF2kCAQMxDzANBglghkgB
# ZQMEAgEFADCCAVIGCyqGSIb3DQEJEAEEoIIBQQSCAT0wggE5AgEBBgorBgEEAYRZ
# CgMBMDEwDQYJYIZIAWUDBAIBBQAEID9uPwyG3qdwOZGlOEB9NZgPGhZiYfq1rK0x
# WvwMCCmvAgZp51uY/FAYEzIwMjYwNDI5MTUyMTU0LjcxNVowBIACAfSggdGkgc4w
# gcsxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
# ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNVBAsT
# HE1pY3Jvc29mdCBBbWVyaWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQg
# VFNTIEVTTjpBMDAwLTA1RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
# U3RhbXAgU2VydmljZaCCEeowggcgMIIFCKADAgECAhMzAAACK7sAUP9NO5qhAAEA
# AAIrMA0GCSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
# aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29y
# cG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEw
# MB4XDTI2MDIxOTE5NDAxMVoXDTI3MDUxNzE5NDAxMVowgcsxCzAJBgNVBAYTAlVT
# MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
# ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVy
# aWNhIE9wZXJhdGlvbnMxJzAlBgNVBAsTHm5TaGllbGQgVFNTIEVTTjpBMDAwLTA1
# RTAtRDk0NzElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZTCC
# AiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAJfeaLo4PezJSpCbhCqCSso9
# tywr9DHd9hy0vz5UzW45jduiiLkbHBq5OBB/okUchNOjFLuCOoqUrw4UvMvpROXS
# PEQrm3oO45yAld2+62KahOU5LQLeTIhNcEBeiP+CnqFFH3PpZGKnUq2SKVvd0lcK
# NCpP0/YK66Ov5XPyv5n6MOXT2OL+Jz/gbfiveZXCOz/8afH0+7fVXytcWJw2IDPG
# m5trClt3ymp/OVZPa+cbeQX2XoyJERu8ndcctTAdCyHS39OtIXH+z/IKqklZgnqg
# KUbvS2+wUfRpE/zAHhw/8IVrYgu+TbqLc5wkGX6moqMdNIHL2a/BM8QOWfNyjQ23
# xHqlI9NdmAGyxweGgp8LRZCY7NjaR5dsCZFNxkzJfPm/8AluagjTLTsFrO+3k2Rd
# 10b1MStBbC2wXIgqsSUOBZ8d4KhO7XC7ZyIPd0rvbPdxraDOgQPFPaP0FchQpqJP
# NN1A9GwAxo7d1TTNobAwyXC1InIOHXhgSBmhS7m9Lwy6Ayp2s2OmHIvrnIqGOkBZ
# uFiQgc7/S5mO73m0/zNk2pchGHi119Yck8BOf2v5zGTK6HbHRUt1/HWWYr1fc2Mf
# Q22ACzkkH/A6WTK653GYVN9ZXJGsvfuKyk5nxo8AWC/JHpw1OQamQWjfklGNyI2Z
# mJTipP1S3L5XmC50WTWPAgMBAAGjggFJMIIBRTAdBgNVHQ4EFgQUPhaO5BNQlu1t
# 3eOa9mS7QVnZ5TYwHwYDVR0jBBgwFoAUn6cVXQBeYl2D9OXSZacbUzUZ6XIwXwYD
# VR0fBFgwVjBUoFKgUIZOaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9j
# cmwvTWljcm9zb2Z0JTIwVGltZS1TdGFtcCUyMFBDQSUyMDIwMTAoMSkuY3JsMGwG
# CCsGAQUFBwEBBGAwXjBcBggrBgEFBQcwAoZQaHR0cDovL3d3dy5taWNyb3NvZnQu
# Y29tL3BraW9wcy9jZXJ0cy9NaWNyb3NvZnQlMjBUaW1lLVN0YW1wJTIwUENBJTIw
# MjAxMCgxKS5jcnQwDAYDVR0TAQH/BAIwADAWBgNVHSUBAf8EDDAKBggrBgEFBQcD
# CDAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcNAQELBQADggIBAM78IqxyIQzySvV+
# ofhOV9ProQ2hOPFfDzXSnISrQ92uAvB+BPfH7WDPsJAe1R16+oxDmofIMuGbqlP3
# XUJiQY37qD4xTt8xhOvp3dLGJ4nAEihaR9HiDtKK0bMwpTjrkoRh6N912hSCi8L/
# FxGloAs7mnf8DbjwHKEmIy20FA0O2xP8doIXBEUJRFvL9/xzWSTLwXzGQJcXP78y
# 1nl3WVYWPA4jaB5kdar1eKEM6B57mdLaSijlXqfxcbbbRRN69V/6mCakgfvVcoNU
# hhMYZkmzrI+V8nZperDUwTg1HqiQ2xjc/UzfUfoMxhF8kY0E16nn3mRcaHdjMDdw
# KLKD6OYnnyH99O+OeAim5QV84OkOMXHSJzVigsA3GEIXdGFL2pgzsrjQ0SEqyFi5
# oCQgbZcEpiKDev/T9vSyO+MHCznkBiicybcDypxf/qT1V9zSa/122ice5YZ8DZv6
# oTaqkKeHMZt0MeruI5JkTDTWc26kAx/VzjWT0ihNDbPeLDrDhlmgs7KDhMoxunWS
# ulPi2uKn/LfQK/mSHKoIM2ppdCkGQ5g43wuC2hDdqZU5fuLHmN2ufH+9TFNRKKBe
# +tZ0vtSTySmZLTO2jZOjLtpPmgHMJO9+P2In8E38TW+EUGSEkK9ns9W+wxKdNOaT
# HYVqXaVWTO24Ajjh8P+7Isl1oxBLMIIHcTCCBVmgAwIBAgITMwAAABXF52ueAptJ
# mQAAAAAAFTANBgkqhkiG9w0BAQsFADCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT
# Cldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29m
# dCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMpTWljcm9zb2Z0IFJvb3QgQ2VydGlmaWNh
# dGUgQXV0aG9yaXR5IDIwMTAwHhcNMjEwOTMwMTgyMjI1WhcNMzAwOTMwMTgzMjI1
# WjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
# UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQD
# Ex1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAxMDCCAiIwDQYJKoZIhvcNAQEB
# BQADggIPADCCAgoCggIBAOThpkzntHIhC3miy9ckeb0O1YLT/e6cBwfSqWxOdcjK
# NVf2AX9sSuDivbk+F2Az/1xPx2b3lVNxWuJ+Slr+uDZnhUYjDLWNE893MsAQGOhg
# fWpSg0S3po5GawcU88V29YZQ3MFEyHFcUTE3oAo4bo3t1w/YJlN8OWECesSq/XJp
# rx2rrPY2vjUmZNqYO7oaezOtgFt+jBAcnVL+tuhiJdxqD89d9P6OU8/W7IVWTe/d
# vI2k45GPsjksUZzpcGkNyjYtcI4xyDUoveO0hyTD4MmPfrVUj9z6BVWYbWg7mka9
# 7aSueik3rMvrg0XnRm7KMtXAhjBcTyziYrLNueKNiOSWrAFKu75xqRdbZ2De+JKR
# Hh09/SDPc31BmkZ1zcRfNN0Sidb9pSB9fvzZnkXftnIv231fgLrbqn427DZM9itu
# qBJR6L8FA6PRc6ZNN3SUHDSCD/AQ8rdHGO2n6Jl8P0zbr17C89XYcz1DTsEzOUyO
# ArxCaC4Q6oRRRuLRvWoYWmEBc8pnol7XKHYC4jMYctenIPDC+hIK12NvDMk2ZItb
# oKaDIV1fMHSRlJTYuVD5C4lh8zYGNRiER9vcG9H9stQcxWv2XFJRXRLbJbqvUAV6
# bMURHXLvjflSxIUXk8A8FdsaN8cIFRg/eKtFtvUeh17aj54WcmnGrnu3tz5q4i6t
# AgMBAAGjggHdMIIB2TASBgkrBgEEAYI3FQEEBQIDAQABMCMGCSsGAQQBgjcVAgQW
# BBQqp1L+ZMSavoKRPEY1Kc8Q/y8E7jAdBgNVHQ4EFgQUn6cVXQBeYl2D9OXSZacb
# UzUZ6XIwXAYDVR0gBFUwUzBRBgwrBgEEAYI3TIN9AQEwQTA/BggrBgEFBQcCARYz
# aHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9wcy9Eb2NzL1JlcG9zaXRvcnku
# aHRtMBMGA1UdJQQMMAoGCCsGAQUFBwMIMBkGCSsGAQQBgjcUAgQMHgoAUwB1AGIA
# QwBBMAsGA1UdDwQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB8GA1UdIwQYMBaAFNX2
# VsuP6KJcYmjRPZSQW9fOmhjEMFYGA1UdHwRPME0wS6BJoEeGRWh0dHA6Ly9jcmwu
# bWljcm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY1Jvb0NlckF1dF8yMDEw
# LTA2LTIzLmNybDBaBggrBgEFBQcBAQROMEwwSgYIKwYBBQUHMAKGPmh0dHA6Ly93
# d3cubWljcm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYt
# MjMuY3J0MA0GCSqGSIb3DQEBCwUAA4ICAQCdVX38Kq3hLB9nATEkW+Geckv8qW/q
# XBS2Pk5HZHixBpOXPTEztTnXwnE2P9pkbHzQdTltuw8x5MKP+2zRoZQYIu7pZmc6
# U03dmLq2HnjYNi6cqYJWAAOwBb6J6Gngugnue99qb74py27YP0h1AdkY3m2CDPVt
# I1TkeFN1JFe53Z/zjj3G82jfZfakVqr3lbYoVSfQJL1AoL8ZthISEV09J+BAljis
# 9/kpicO8F7BUhUKz/AyeixmJ5/ALaoHCgRlCGVJ1ijbCHcNhcy4sa3tuPywJeBTp
# kbKpW99Jo3QMvOyRgNI95ko+ZjtPu4b6MhrZlvSP9pEB9s7GdP32THJvEKt1MMU0
# sHrYUP4KWN1APMdUbZ1jdEgssU5HLcEUBHG/ZPkkvnNtyo4JvbMBV0lUZNlz138e
# W0QBjloZkWsNn6Qo3GcZKCS6OEuabvshVGtqRRFHqfG3rsjoiV5PndLQTHa1V1QJ
# sWkBRH58oWFsc/4Ku+xBZj1p/cvBQUl+fpO+y/g75LcVv7TOPqUxUYS8vwLBgqJ7
# Fx0ViY1w/ue10CgaiQuPNtq6TPmb/wrpNPgkNWcr4A245oyZ1uEi6vAnQj0llOZ0
# dFtq0Z4+7X6gMTN9vMvpe784cETRkPHIqzqKOghif9lwY1NNje6CbaUFEMFxBmoQ
# tB1VM1izoXBm8qGCA00wggI1AgEBMIH5oYHRpIHOMIHLMQswCQYDVQQGEwJVUzET
# MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMV
# TWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQLExxNaWNyb3NvZnQgQW1lcmlj
# YSBPcGVyYXRpb25zMScwJQYDVQQLEx5uU2hpZWxkIFRTUyBFU046QTAwMC0wNUUw
# LUQ5NDcxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2WiIwoB
# ATAHBgUrDgMCGgMVAAmsP3TKQemj/QAZvuWbC+wK2pE5oIGDMIGApH4wfDELMAkG
# A1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
# HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9z
# b2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAwDQYJKoZIhvcNAQELBQACBQDtnGTNMCIY
# DzIwMjYwNDI5MTEwNjUzWhgPMjAyNjA0MzAxMTA2NTNaMHQwOgYKKwYBBAGEWQoE
# ATEsMCowCgIFAO2cZM0CAQAwBwIBAAICIgUwBwIBAAICE3swCgIFAO2dtk0CAQAw
# NgYKKwYBBAGEWQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQACAwehIKEKMAgC
# AQACAwGGoDANBgkqhkiG9w0BAQsFAAOCAQEAFX85ivkN88JllyFwVO/ytpWI6kP5
# 228nT3/smjEY2QITO1QkHA/DdPXiOyp3BDvbvwdfphtYdhgBYL3+0KlKOIM9e95c
# KsUhwfdsxqVZLE1Qjl8QFHQuxGieNm2IfXvwBjKsJ8VxQaqKjzEW+2moBAIJe1d/
# AW0g/Ji4PSgNmKF3I2qkaKqkNWqymXls9qeMhZJTDRbau2kF3Yn2allFAos1Q0fc
# d+/LaKYq06Aw29oBjA1wPoYy/lk1RXbkDN2HIWwOpYNjTDg7JSv+9eoi44F0AlJT
# e5zaxvs0IL5dLCN+4npUmYGZ3aoBHOBnaG0gPZh411/FQaH3C15m1SqU7TGCBA0w
# ggQJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
# DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24x
# JjAkBgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwAhMzAAACK7sA
# UP9NO5qhAAEAAAIrMA0GCWCGSAFlAwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYL
# KoZIhvcNAQkQAQQwLwYJKoZIhvcNAQkEMSIEIGpp/BKqK0dGbqVUNSB5umbeUogQ
# QZGMy93fb23E6u/GMIH6BgsqhkiG9w0BCRACLzGB6jCB5zCB5DCBvQQgcg4j9D+Q
# V+1gD4zY5j7UHHdqMEPr9YMC09Pa8WS/blIwgZgwgYCkfjB8MQswCQYDVQQGEwJV
# UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
# ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGlt
# ZS1TdGFtcCBQQ0EgMjAxMAITMwAAAiu7AFD/TTuaoQABAAACKzAiBCBl1VFaUGQh
# z8szzJdhN0AUL4TE2BM13XDahjK8QZ4AGDANBgkqhkiG9w0BAQsFAASCAgB1Z0+c
# k10zjCCaonr+GqnN75sy/eIFkFdqeAELs9hFPV+lNyQPzZDw3bEX9mXMOB2UV8nB
# jv1bsaF4/QLMHei2h2BRjIAHxcMiOeC3ww/f0yP6fVFv2aTyhgD19p0E+BB0jfve
# 2S9sYuszWv1Eo0GeRG8klFm79g231KJ1WLFQ4KllYHLACF3xDjwPVpG8goBUNMoT
# WSZVFLYyGAC/TbUKB230jOwqciMq44f00j1cNikxWYuEydbo/6UdOrhm6mJyyLCb
# rH9OaozUaXPyv4uTDkSd3wWH6u9bXUiNpXGhKcfIKxeIzQbUbipH29pcu8qY7s18
# wXeUvgJRWrvgAz/5eQlic2g9CBI3c6isuZzzdmLWd+2MJhxoaUWRvxYRGE0w7aDe
# jiP482LeZiuSZyS3nKbydwU+HKTBjwSVga/0OcyuxebOdMDh0jpA24bBSLtEH/as
# WnUXk0TKJP/K++dvAN0bbHkVXJkvcSOvDq+PNLQzHcFW6a6x8ePgzch/LynA4nCi
# ZjT47DPvb5aOrOO6SByMWEfyaEf2amCi9cJ0Z7N+wnttPp1zDdDqju3Pft1MUMTj
# cCe8Sh4KzI5IdJQdhMuHyip7TWnWHEEr42KWH1OQL2rcDa/4V/S+14AXkz4m2NZr
# d29dG2cVwwfFEAMKQfyi2QD6z5qhcxU62hzFCQ==
# SIG # End signature block
