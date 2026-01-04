# Test script for aab2apk installer
# Run this AFTER building the installer to verify it works correctly

Write-Host "=== aab2apk Installer Test Script ===" -ForegroundColor Cyan
Write-Host ""

# Check if installer exists
$installerPath = "installer\aab2apk-setup-1.0.0.exe"
if (-not (Test-Path $installerPath)) {
    Write-Host "ERROR: Installer not found at $installerPath" -ForegroundColor Red
    Write-Host "Please build the installer first using Inno Setup Compiler" -ForegroundColor Yellow
    exit 1
}

Write-Host "Installer found: $installerPath" -ForegroundColor Green
Write-Host ""

# Check if already installed
$installPath = "C:\Program Files\aab2apk\aab2apk.exe"
$isInstalled = Test-Path $installPath

if ($isInstalled) {
    Write-Host "WARNING: aab2apk appears to be already installed" -ForegroundColor Yellow
    Write-Host "Install path: $installPath" -ForegroundColor Yellow
    Write-Host ""
    $response = Read-Host "Do you want to test uninstall first? (Y/N)"
    if ($response -eq "Y" -or $response -eq "y") {
        Write-Host "Please uninstall aab2apk from Control Panel first" -ForegroundColor Yellow
        Write-Host "Then run this script again" -ForegroundColor Yellow
        exit 0
    }
}

Write-Host "=== Installation Test ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "To test the installer:" -ForegroundColor Yellow
Write-Host "1. Run the installer as Administrator: $installerPath" -ForegroundColor White
Write-Host "2. Follow the installation wizard" -ForegroundColor White
Write-Host "3. After installation, open a NEW Command Prompt window" -ForegroundColor White
Write-Host "4. Run: aab2apk --version" -ForegroundColor White
Write-Host "5. Expected output: aab2apk version 1.0.0" -ForegroundColor White
Write-Host ""

# Check current PATH
Write-Host "=== PATH Verification ===" -ForegroundColor Cyan
Write-Host ""
$currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
if ($currentPath -match "aab2apk") {
    Write-Host "aab2apk is already in system PATH" -ForegroundColor Yellow
} else {
    Write-Host "aab2apk is NOT in system PATH (this is expected before installation)" -ForegroundColor White
}
Write-Host ""

Write-Host "=== Post-Installation Verification ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "After installation, verify:" -ForegroundColor Yellow
Write-Host "1. Files installed to: C:\Program Files\aab2apk\" -ForegroundColor White
Write-Host "2. PATH contains: C:\Program Files\aab2apk" -ForegroundColor White
Write-Host "3. Command works: aab2apk --version" -ForegroundColor White
Write-Host ""

Write-Host "=== Uninstallation Test ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "To test uninstall:" -ForegroundColor Yellow
Write-Host "1. Go to Control Panel > Programs > Uninstall a program" -ForegroundColor White
Write-Host "2. Find 'aab2apk' and uninstall" -ForegroundColor White
Write-Host "3. Verify files are removed" -ForegroundColor White
Write-Host "4. Verify PATH entry is removed" -ForegroundColor White
Write-Host "5. Open new CMD and verify 'aab2apk' command fails (not found)" -ForegroundColor White
Write-Host ""

Write-Host "Press any key to open installer location..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

# Open installer directory
if (Test-Path "installer") {
    explorer.exe installer
}

