# Clitboin All-In-One Launcher
# This script assumes you have compiled bitcoind.exe and bitcoin-cli.exe
# and they are located in src/ or build/src/

$ErrorActionPreference = "Stop"

# 1. Locate Binaries
$BitcoindPath = ".\src\clitboind.exe"
$CliPath = ".\src\clitboin-cli.exe"

if (-not (Test-Path $BitcoindPath)) {
    # Check Debug build
    $BitcoindPath = ".\build\src\Debug\clitboind.exe"
    $CliPath = ".\build\src\Debug\clitboin-cli.exe"
}

if (-not (Test-Path $BitcoindPath)) {
    # Check Release build
    $BitcoindPath = ".\build\src\Release\clitboind.exe"
    $CliPath = ".\build\src\Release\clitboin-cli.exe"
}

if (-not (Test-Path $BitcoindPath)) {
    Write-Host "ERROR: Could not find clitboind.exe. Please compile the project first!" -ForegroundColor Red
    exit 1
}

# 2. Start Node
Write-Host "Starting Clitboin Node..." -ForegroundColor Green
$NodeProcess = Start-Process -FilePath $BitcoindPath -ArgumentList "-conf=$(Get-Location)\clitboin.conf" -PassThru
Start-Sleep -Seconds 5

# 3. Create Wallet
Write-Host "Creating Wallet 'mywallet'..." -ForegroundColor Green
try {
    & $CliPath -conf=$(Get-Location)\clitboin.conf createwallet "mywallet"
} catch {
    Write-Host "Wallet might already exist, continuing..." -ForegroundColor Yellow
}

# 4. Mine Genesis Blocks (Allocate Tokens)
Write-Host "Mining 101 Blocks to unlock first 50 coins..." -ForegroundColor Green
& $CliPath -conf=$(Get-Location)\clitboin.conf -generate 101

# 5. Show Balance
Write-Host "Checking Balance..." -ForegroundColor Green
& $CliPath -conf=$(Get-Location)\clitboin.conf getbalance

# 6. Launch Explorer & Wallet UI
Write-Host "Starting Explorer (http://localhost:5000)..." -ForegroundColor Cyan
Start-Process -FilePath "python" -ArgumentList "simple_explorer.py"

Write-Host "Starting Web Wallet (http://localhost:5001)..." -ForegroundColor Cyan
Start-Process -FilePath "python" -ArgumentList "simple_wallet.py"

Write-Host "Clitboin is running! Press Enter to exit this launcher (Node will keep running)." -ForegroundColor Green
Read-Host
