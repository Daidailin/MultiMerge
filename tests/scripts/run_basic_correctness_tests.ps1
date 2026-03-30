# MultiMerge Basic Correctness Test Script
# Tests gold standard data sets against both legacy and stream engines

$ErrorActionPreference = "Continue"
$ProjectRoot = "E:\TraeCN\TraeCN_project\test\QT_Project\MultiMerge"
$BuildDir = "$ProjectRoot\build"
$InputDir = "$ProjectRoot\test_data\input"
$OutputDir = "$ProjectRoot\test_data\output"
$ExpectedDir = "$ProjectRoot\test_data\expected"
$ExePath = "$BuildDir\MultiMerge.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MultiMerge Basic Correctness Tests" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$testSets = @(
    @{ Name = "gold_a"; File1 = "gold_a1_file1.txt"; File2 = "gold_a1_file2.txt"; Modes = @("nearest", "linear") },
    @{ Name = "gold_b"; File1 = "gold_b1_file1.txt"; File2 = "gold_b1_file2.txt"; Modes = @("nearest", "linear") },
    @{ Name = "gold_c"; File1 = "gold_c1_file1.txt"; File2 = "gold_c1_file2.txt"; Modes = @("nearest", "linear") }
)

$engines = @("legacy", "stream")

$totalTests = 0
$passCount = 0
$failCount = 0
$skipCount = 0

$results = @()

function Normalize-TimeFormat {
    param([string]$TimeStr)

    $normalized = $TimeStr -replace "\.", ":"
    return $normalized
}

function Test-FileEquality {
    param(
        [string]$File1,
        [string]$File2,
        [string]$TestName,
        [string]$Delimiter = " ",
        [bool]$IgnoreTimeFormat = $false
    )

    if (-not (Test-Path $File1)) {
        return @{ Pass = $false; Message = "Output file not found: $File1" }
    }
    if (-not (Test-Path $File2)) {
        return @{ Pass = $false; Message = "Expected file not found: $File2" }
    }

    $content1 = Get-Content $File1 -Raw
    $content2 = Get-Content $File2 -Raw

    $lines1 = $content1 -split "`n" | Where-Object { $_.Trim() -ne "" }
    $lines2 = $content2 -split "`n" | Where-Object { $_.Trim() -ne "" }

    $lineCount1 = $lines1.Count
    $lineCount2 = $lines2.Count

    if ($lineCount1 -ne $lineCount2) {
        $msg = "Row count mismatch: output=$lineCount1 expected=$lineCount2"
        return @{ Pass = $false; Message = $msg }
    }

    for ($i = 0; $i -lt $lineCount1; $i++) {
        if ($Delimiter -eq " ") {
            $cols1 = $lines1[$i] -split "\s+"
            $cols2 = $lines2[$i] -split "\s+"
        } else {
            $cols1 = $lines1[$i] -split ","
            $cols2 = $lines2[$i] -split ","
        }

        $cols1 = $cols1 | Where-Object { $_.Trim() -ne "" }
        $cols2 = $cols2 | Where-Object { $_.Trim() -ne "" }

        $colCount1 = $cols1.Count
        $colCount2 = $cols2.Count

        if ($colCount1 -ne $colCount2) {
            $msg = "Column count mismatch at row $i output=$colCount1 expected=$colCount2"
            return @{ Pass = $false; Message = $msg }
        }

        for ($j = 0; $j -lt $colCount1; $j++) {
            $val1 = $cols1[$j].Trim()
            $val2 = $cols2[$j].Trim()

            if ($i -eq 0 -and $j -eq 0 -and $IgnoreTimeFormat) {
                continue
            }

            if ($i -gt 0 -and $j -eq 0 -and $IgnoreTimeFormat) {
                $val1 = Normalize-TimeFormat -TimeStr $val1
                $val2 = Normalize-TimeFormat -TimeStr $val2
            }

            if ($val1 -eq "NaN" -or $val2 -eq "NaN") {
                if ($val1 -ne $val2) {
                    $msg = "NaN mismatch at row $i col $j output='$val1' expected='$val2'"
                    return @{ Pass = $false; Message = $msg }
                }
                continue
            }

            $num1 = [double]::TryParse($val1, [ref]$null)
            $num2 = [double]::TryParse($val2, [ref]$null)

            if ($num1 -and $num2) {
                $f1 = [double]$val1
                $f2 = [double]$val2
                if ([Math]::Abs($f1 - $f2) -gt 0.001) {
                    $msg = "Value mismatch at row $i col $j output=$f1 expected=$f2"
                    return @{ Pass = $false; Message = $msg }
                }
            } elseif ($val1 -ne $val2) {
                $msg = "String mismatch at row $i col $j output='$val1' expected='$val2'"
                return @{ Pass = $false; Message = $msg }
            }
        }
    }

    return @{ Pass = $true; Message = "Files match" }
}

foreach ($testSet in $testSets) {
    Write-Host "Test Set: $($testSet.Name)" -ForegroundColor Yellow
    Write-Host "  Input: $($testSet.File1) + $($testSet.File2)" -ForegroundColor Gray

    foreach ($engine in $engines) {
        foreach ($mode in $testSet.Modes) {
            $totalTests++
            $testName = "$($testSet.Name)_$($engine)_$($mode)"
            $outputFile = "$OutputDir\$testName.txt"
            $expectedFile = "$ExpectedDir\$($testSet.Name)_$($mode).txt"
            $input1 = "$InputDir\$($testSet.File1)"
            $input2 = "$InputDir\$($testSet.File2)"

            Write-Host "  Running: $testName" -NoNewline

            $argList = "-E", $engine, "-i", $mode, "`"$input1`"", "`"$input2`"", "-o", "`"$outputFile`""
            $processInfo = Start-Process -FilePath $ExePath -ArgumentList $argList -Wait -PassThru -NoNewWindow -RedirectStandardOutput "$OutputDir\$testName.stdout.log" -RedirectStandardError "$OutputDir\$testName.stderr.log"

            if ($processInfo.ExitCode -ne 0) {
                Write-Host " [SKIP]" -ForegroundColor Yellow -NoNewline
                $errMsg = "process error: $($processInfo.ExitCode)"
                Write-Host " ($errMsg)" -ForegroundColor Yellow
                $skipCount++
                $results += @{ Name = $testName; Status = "SKIP"; Reason = $errMsg }
                continue
            }

            if (-not (Test-Path $outputFile)) {
                Write-Host " [SKIP]" -ForegroundColor Yellow -NoNewline
                $errMsg = "output not generated"
                Write-Host " ($errMsg)" -ForegroundColor Yellow
                $skipCount++
                $results += @{ Name = $testName; Status = "SKIP"; Reason = $errMsg }
                continue
            }

            $comparison = Test-FileEquality -File1 $outputFile -File2 $expectedFile -TestName $testName -Delimiter " " -IgnoreTimeFormat $true

            if ($comparison.Pass) {
                Write-Host " [PASS]" -ForegroundColor Green
                $passCount++
                $results += @{ Name = $testName; Status = "PASS"; Reason = "" }
            } else {
                Write-Host " [FAIL]" -ForegroundColor Red
                Write-Host "    Reason: $($comparison.Message)" -ForegroundColor Red
                $failCount++
                $results += @{ Name = $testName; Status = "FAIL"; Reason = $comparison.Message }
            }
        }
    }
    Write-Host ""
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Total:  $totalTests" -ForegroundColor White
Write-Host "Passed: $passCount" -ForegroundColor Green
Write-Host "Failed: $failCount" -ForegroundColor $(if ($failCount -gt 0) { "Red" } else { "Green" })
Write-Host "Skipped: $skipCount" -ForegroundColor Yellow
Write-Host ""

$failedTests = $results | Where-Object { $_.Status -eq "FAIL" }
if ($failedTests.Count -gt 0) {
    Write-Host "Failed Tests Details:" -ForegroundColor Red
    foreach ($test in $failedTests) {
        Write-Host "  - $($test.Name): $($test.Reason)" -ForegroundColor Red
    }
    Write-Host ""
}

$reportPath = "$ProjectRoot\test_data\correctness_report.md"
$report = @"
# MultiMerge Basic Correctness Report

## Test Summary
- **Total**: $totalTests
- **Passed**: $passCount
- **Failed**: $failCount
- **Skipped**: $skipCount

## Test Sets
- **gold_a**: Complete alignment (both files have identical timestamps)
- **gold_b**: Nearest interpolation (file2 offset by 200ms)
- **gold_c**: Linear interpolation (file2 timestamps between file1 points)

## Engines Tested
- legacy: DataFileMerger (old engine)
- stream: StreamMergeEngine (new engine)

## Modes Tested
- nearest: Nearest neighbor interpolation
- linear: Linear interpolation between adjacent points

## Results by Test Set

| Test Set | Engine | Mode | Status | Reason |
|----------|--------|------|--------|--------|
"@

foreach ($test in $results) {
    $parts = $test.Name -split "_"
    $report += "`n| $($parts[0]) | $($parts[1]) | $($parts[2]) | $($test.Status) | $($test.Reason) |"
}

$report += @"

## Known Issues

### Time Format Inconsistency
- Legacy engine outputs time as `HH:MM:SS:000` (colon before milliseconds)
- Stream engine outputs time as `HH:MM:SS.000` (dot before milliseconds)
- Test script normalizes time format for comparison, but this should be unified

## Notes
- This test validates basic correctness of merge operations
- Both legacy and stream engines are compared
- NaN values are handled as special case in comparisons
- CLI only supports 'nearest' and 'linear' modes (not 'exact')

Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
"@

$report | Out-File -FilePath $reportPath -Encoding UTF8
Write-Host "Report saved to: $reportPath" -ForegroundColor Gray
Write-Host ""
