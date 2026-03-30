# MultiMerge Format Compatibility Test Script
# Runs all format combination tests

$ErrorActionPreference = "Continue"
$ProjectRoot = "E:\TraeCN\TraeCN_project\test\QT_Project\MultiMerge"
$BuildDir = "$ProjectRoot\build"
$InputDir = "$ProjectRoot\test_data\input"
$OutputDir = "$ProjectRoot\test_data\output"
$ExePath = "$BuildDir\MultiMerge.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MultiMerge Format Compatibility Tests" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$tests = @(
    @{ Name = "test01_dotms_colonus"; File1 = "01_dot_ms.txt"; File2 = "04_colon_us.txt" },
    @{ Name = "test02_dotus_colonms"; File1 = "02_dot_us.txt"; File2 = "03_colon_ms.txt" },
    @{ Name = "test03_colonms_dotus"; File1 = "03_colon_ms.txt"; File2 = "02_dot_us.txt" },
    @{ Name = "test04_colonus_dotms"; File1 = "04_colon_us.txt"; File2 = "01_dot_ms.txt" },
    @{ Name = "test05_sensor_merge"; File1 = "sensor1.txt"; File2 = "sensor2.txt" }
)

$passCount = 0
$failCount = 0

foreach ($test in $tests) {
    $outputFile = "$OutputDir\$($test.Name).txt"
    $input1 = "$InputDir\$($test.File1)"
    $input2 = "$InputDir\$($test.File2)"

    Write-Host "Running: $($test.Name)" -NoNewline
    Write-Host " ($($test.File1) + $($test.File2))" -ForegroundColor Gray

    $processInfo = Start-Process -FilePath $ExePath -ArgumentList "-E", "stream", "-i", "nearest", "`"$input1`"", "`"$input2`"", "-o", "`"$outputFile`"" -Wait -PassThru -NoNewWindow

    if ($processInfo.ExitCode -eq 0) {
        Write-Host "  [PASS]" -ForegroundColor Green
        $passCount++
    } else {
        Write-Host "  [FAIL] ExitCode: $($processInfo.ExitCode)" -ForegroundColor Red
        $failCount++
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Passed: $passCount" -ForegroundColor Green
Write-Host "Failed: $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Green" })
Write-Host ""
Write-Host "Output files: $OutputDir" -ForegroundColor Gray
