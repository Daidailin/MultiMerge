# MultiMerge Day4 Mixed Mode Test Script

$ErrorActionPreference = "Continue"
$ProjectRoot = "E:\TraeCN\TraeCN_project\test\QT_Project\MultiMerge"
$BuildDir = "$ProjectRoot\build"
$InputDir = "$ProjectRoot\test_data\input"
$OutputDir = "$ProjectRoot\test_data\output_day4"
$ExpectedDir = "$ProjectRoot\test_data\expected"
$ExePath = "$BuildDir\MultiMerge.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MultiMerge Day4 Mixed Mode Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$testName = "mixed_mode"
$outputFile = "$OutputDir\$testName.txt"
$input1 = "$InputDir\mixed_a1_file1.txt"
$input2 = "$InputDir\mixed_b1_file2.txt"
$input3 = "$InputDir\mixed_c1_file3.txt"

$modes = @("nearest", "linear")

foreach ($mode in $modes) {
    $outFile = "$OutputDir\$($testName)_$($mode).txt"
    $expectedFile = "$ExpectedDir\$($testName)_$($mode).txt"

    Write-Host "Running: $testName with $mode mode" -NoNewline

    $argList = "-E", "stream", "-i", $mode, "`"$input1`"", "`"$input2`"", "`"$input3`"", "-o", "`"$outFile`""
    $processInfo = Start-Process -FilePath $ExePath -ArgumentList $argList -Wait -PassThru -NoNewWindow -RedirectStandardOutput "$OutputDir\$($testName)_$($mode).stdout.log" -RedirectStandardError "$OutputDir\$($testName)_$($mode).stderr.log"

    if ($processInfo.ExitCode -eq 0) {
        Write-Host " [OK]" -ForegroundColor Green
    } else {
        Write-Host " [FAIL - Exit: $($processInfo.ExitCode)]" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Mixed Mode Test completed" -ForegroundColor Cyan
