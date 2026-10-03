Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$cwd = (Get-Location).Path
$remainingArgs = @($args)

if ($remainingArgs.Count -gt 0 -and $remainingArgs[0] -eq "--dir") {
    if ($remainingArgs.Count -lt 2) {
        Write-Error "Missing directory after --dir."
        exit 1
    }

    $cwd = $remainingArgs[1]
    $remainingArgs = @($remainingArgs[2..($remainingArgs.Count - 1)])

    Set-Location -LiteralPath $cwd
}

$gurdDir = Join-Path (Get-Location).Path ".gurd"
$buildExe = Join-Path $gurdDir "build.exe"
$buildFile = Join-Path (Get-Location).Path "build.c"

if (-not (Test-Path $buildFile -PathType Leaf)) {
    Write-Error "No gurd build file found in directory $((Get-Location).Path)"
    exit 1
}

New-Item -ItemType Directory -Force -Path $gurdDir | Out-Null

if (-not (Get-Command clang -ErrorAction SilentlyContinue)) {
    Write-Error "clang was not found in PATH."
    exit 1
}

$includeDir = Join-Path $env:LOCALAPPDATA "Programs\Gurd\Include"

& clang $buildFile "-I$includeDir" -o $buildExe

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $buildExe @remainingArgs

exit $LASTEXITCODE