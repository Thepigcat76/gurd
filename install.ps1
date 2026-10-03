$programsDir = Join-Path $env:LOCALAPPDATA "Programs"
$gurdDir = Join-Path $programsDir "Gurd"
$gurdIncludeDir = Join-Path $gurdDir "Include"

New-Item -ItemType Directory -Force -Path $gurdDir | Out-Null
New-Item -ItemType Directory -Force -Path $gurdIncludeDir | Out-Null
Copy-Item ".\scripts\windows\gurd.ps1" (Join-Path $gurdDir "gurd.ps1") -Force
Copy-Item ".\gurd.h" (Join-Path $gurdIncludeDir "gurd.h") -Force

$userPath = [Environment]::GetEnvironmentVariable("Path", "User")

if (($userPath -split ";") -notcontains $gurdDir) {
    [Environment]::SetEnvironmentVariable(
        "Path",
        (($userPath.TrimEnd(";") + ";" + $gurdDir).Trim(";")),
        "User"
    )
}