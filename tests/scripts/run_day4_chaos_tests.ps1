# MultiMerge Day4 Chaos Test Script
# Tests IndexedFallback (C-type file handling)

$ErrorActionPreference = "Continue"
$ProjectRoot = "E:\TraeCN\TraeCN_project\test\QT_Project\MultiMerge"
$BuildDir = "$ProjectRoot\build"
$InputDir = "$ProjectRoot\test_data\input"
$OutputDir = "$ProjectRoot\test_data\output_day4"
$ExpectedDir = "$ProjectRoot\test_data\expected"
$ExePath = "$BuildDir\MultiMerge.exe"

if (Test-Path $OutputDir) {
    Remove-Item "$OutputDir\*" -Force
} else {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "MultiMerge Day4 Chaos Tests" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$testSets = @(
    @{ Name = "chaos_c1"; Desc = "Fully chaotic"; File1 = "chaos_c1_file1.txt"; File2 = "chaos_c1_file2.txt"; Modes = @("nearest", "linear") },
    @{ Name = "chaos_c2"; Desc = "Partial backtrack"; File1 = "chaos_c2_file1.txt"; File2 = "chaos_c2_file2.txt"; Modes = @("nearest", "linear") },
    @{ Name = "chaos_c3"; Desc = "Chaos+non-numeric"; File1 = "chaos_c3_file1.txt"; File2 = "chaos_c3_file2.txt"; Modes = @("nearest", "linear") }
)

$engines = @("stream")

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
        $cols1 = $lines1[$i] -split "\s+"
        $cols2 = $lines2[$i] -split "\s+"

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
    Write-Host "Test Set: $($testSet.Name) ($($testSet.Desc))" -ForegroundColor Yellow
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
                $results += @{ Name = $testName; Status = "SKIP"; Reason = $errMsg; Output = $outputFile }
                continue
            }

            if (-not (Test-Path $outputFile)) {
                Write-Host " [SKIP]" -ForegroundColor Yellow -NoNewline
                $errMsg = "output not generated"
                Write-Host " ($errMsg)" -ForegroundColor Yellow
                $skipCount++
                $results += @{ Name = $testName; Status = "SKIP"; Reason = $errMsg; Output = "" }
                continue
            }

            $comparison = Test-FileEquality -File1 $outputFile -File2 $expectedFile -TestName $testName -IgnoreTimeFormat $true

            if ($comparison.Pass) {
                Write-Host " [PASS]" -ForegroundColor Green
                $passCount++
                $results += @{ Name = $testName; Status = "PASS"; Reason = ""; Output = $outputFile }
            } else {
                Write-Host " [FAIL]" -ForegroundColor Red
                Write-Host "    Reason: $($comparison.Message)" -ForegroundColor Red
                $failCount++
                $results += @{ Name = $testName; Status = "FAIL"; Reason = $comparison.Message; Output = $outputFile }
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
Write-Host "Failed: $failCount" -ForegroundColor Red
Write-Host "Skipped: $skipCount" -ForegroundColor Yellow
Write-Host ""

$failedTests = $results | Where-Object { $_.Status -eq "FAIL" }
if ($failedTests.Count -gt 0) {
    Write-Host "Failed Tests:" -ForegroundColor Red
    foreach ($test in $failedTests) {
        Write-Host "  - $($test.Name): $($test.Reason)" -ForegroundColor Red
        Write-Host "    Output: $($test.Output)" -ForegroundColor Gray
    }
    Write-Host ""
}

$reportPath = "$ProjectRoot\test_data\day4_chaos_report.md"
$report = "Day4 Chaos Test Report`n===================`n`n"
$report += "Test Summary: Total=$totalTests Passed=$passCount Failed=$failCount Skipped=$skipCount`n`n"
$report += "Results:`n"
foreach ($test in $results) {
    $report += "| $($test.Name) | $($test.Status) | $($test.Reason) |`n"
}
$report += "`nGenerated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')`n"
$report | Out-File -FilePath $reportPath -Encoding UTF8
Write-Host "Report saved to: $reportPath" -ForegroundColor Gray
Write-Host ""
