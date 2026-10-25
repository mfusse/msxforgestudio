param(
  [string]$BuildDir = "build-win",
  [string]$OutputDir = "release",
  [string]$QtBin = ""
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $root $BuildDir
$target = Join-Path $root $OutputDir
$portable = Join-Path $target 'MSXForgeStudio_0_6_0_Portable_Beta1'
$exeCandidates = @((Join-Path $build 'Release/MSXForgeStudio.exe'), (Join-Path $build 'MSXForgeStudio.exe'))
$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) { throw 'No se encontro MSXForgeStudio.exe compilado en Release.' }
if (-not $QtBin) {
  $qmake = Get-Command qmake6 -ErrorAction SilentlyContinue
  if (-not $qmake) { $qmake = Get-Command qmake -ErrorAction SilentlyContinue }
  if (-not $qmake) { throw 'Indica -QtBin con la carpeta bin de la misma version de Qt usada al compilar.' }
  $QtBin = (& $qmake.Source -query QT_INSTALL_BINS).Trim()
}
$deploy = Join-Path $QtBin 'windeployqt.exe'
if (-not (Test-Path $deploy)) { throw "No existe windeployqt: $deploy" }
New-Item -ItemType Directory -Force -Path $target | Out-Null
if (Test-Path $portable) { Remove-Item $portable -Recurse -Force }
New-Item -ItemType Directory -Force -Path $portable | Out-Null
Copy-Item $exe (Join-Path $portable 'MSXForgeStudio.exe')
& $deploy --release --compiler-runtime --no-translations (Join-Path $portable 'MSXForgeStudio.exe')
if ($LASTEXITCODE -ne 0) { throw 'Fallo windeployqt.' }
Copy-Item (Join-Path $root 'examples') (Join-Path $portable 'examples') -Recurse
@'
MSX Forge Studio 0.6.0 - Windows Portable Beta 1

1. Extrae todo el ZIP a una carpeta local.
2. Ejecuta MSXForgeStudio.exe.
3. No es necesario instalar Qt, Qt Creator, Visual Studio ni CMake.
4. Guarda una copia de tus proyectos .msxf antes de usarlos con esta beta.
5. Si Windows pide confirmacion de seguridad para un EXE sin firma digital,
   verifica primero que el archivo provenga de la distribucion oficial.

La ejecucion en un PC Windows limpio debe validarse antes de publicar la beta.
'@ | Set-Content -Encoding UTF8 (Join-Path $portable 'LEEME.txt')
$zip = Join-Path $target 'MSXForgeStudio_0_6_0_Windows_Portable_Beta1.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $portable '*') -DestinationPath $zip -CompressionLevel Optimal
Write-Host "Paquete generado: $zip"
