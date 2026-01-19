# PowerShell test runner for Solana C++ SDK
param(
    [string]$TestName = ""
)

$ErrorActionPreference = "Stop"

# Get script directory and root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

# Compiler detection
$Compiler = ""
if ($env:CXX) {
    $Compiler = $env:CXX
} elseif (Get-Command "clang++" -ErrorAction SilentlyContinue) {
    $Compiler = "clang++"
} elseif (Get-Command "g++" -ErrorAction SilentlyContinue) {
    $Compiler = "g++"
} else {
    Write-Host "No C++ compiler found. Set `$env:CXX or install clang++/g++." -ForegroundColor Red
    exit 1
}

$CxxFlags = @("-std=c++17", "-I$RootDir/include", "-Wall", "-Wextra")
$LdFlags = @("-L$RootDir/lib", "-lsodium")
$BuildDir = "$RootDir/build"

# Create build directory
if (!(Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Find all test files
$TestFiles = Get-ChildItem -Path "$RootDir/tests/solana" -Filter "test_*.cpp"

function Build-And-Run {
    param($TestFile)
    
    $TestName = [System.IO.Path]::GetFileNameWithoutExtension($TestFile.Name)
    $ExecutableName = "$TestName.exe"
    
    Write-Host "Building $TestName..." -ForegroundColor Yellow
    
    # Get all source files
    $SourceFiles = Get-ChildItem -Path "$RootDir/src/solana" -Filter "*.cpp"
    $SourcePaths = $SourceFiles | ForEach-Object { $_.FullName }
    
    # Build command
    $BuildArgs = $CxxFlags + $SourcePaths + $TestFile.FullName + $LdFlags + @("-o", "$BuildDir/$ExecutableName")
    
    try {
        & $Compiler $BuildArgs
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Build failed for $TestName" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "Build failed for ${TestName}: $_" -ForegroundColor Red
        return $false
    }
    
    Write-Host "Running $TestName..." -ForegroundColor Green
    
    # Set PATH to include DLL location and run
    $env:PATH = "$RootDir;$env:PATH"
    try {
        & "$BuildDir\$ExecutableName"
        if ($LASTEXITCODE -eq 0) {
            Write-Host "$TestName passed" -ForegroundColor Green
        } else {
            Write-Host "$TestName failed with exit code $LASTEXITCODE" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "${TestName} failed: $_" -ForegroundColor Red
        return $false
    }
    
    Write-Host ""
    return $true
}

if ($TestName -eq "") {
    Write-Host "Running all tests..." -ForegroundColor Cyan
    $AllPassed = $true
    foreach ($TestFile in $TestFiles) {
        $Result = Build-And-Run $TestFile
        if (-not $Result) {
            $AllPassed = $false
        }
    }
    
    if ($AllPassed) {
        Write-Host "All tests passed!" -ForegroundColor Green
    } else {
        Write-Host "Some tests failed" -ForegroundColor Red
        exit 1
    }
} else {
    $TestFile = Get-ChildItem -Path "$RootDir/tests/solana" -Filter "$TestName.cpp" -ErrorAction SilentlyContinue
    if ($TestFile) {
        $Result = Build-And-Run $TestFile
        if (-not $Result) {
            exit 1
        }
    } else {
        Write-Host "Test '$TestName' not found." -ForegroundColor Red
        Write-Host "Available tests:" -ForegroundColor Yellow
        foreach ($File in $TestFiles) {
            $Name = [System.IO.Path]::GetFileNameWithoutExtension($File.Name)
            Write-Host "  - $Name" -ForegroundColor Yellow
        }
        exit 1
    }
}