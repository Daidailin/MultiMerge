# Tolerance 功能测试脚本
# 测试修复后的 tolerance 参数是否正确工作

$ErrorActionPreference = "Stop"
$exePath = "..\build-MultiMerge-Desktop_Qt_5_9_1_MSVC2015_64bit-Debug\MultiMerge.exe"
$inputDir = ".\input"
$outputDir = ".\output_tolerance"

# 创建输出目录
if (!(Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

function Run-Test {
    param(
        [string]$TestName,
        [string]$BaseFile,
        [string]$SrcFile,
        [string]$Tolerance,
        [string]$Engine,
        [string]$ExpectedBehavior
    )
    
    $outputFile = "$outputDir\${TestName}_${Engine}.txt"
    
    Write-Host "`n========================================" -ForegroundColor Cyan
    Write-Host "测试: $TestName ($Engine)" -ForegroundColor Cyan
    Write-Host "容差: $Tolerance us" -ForegroundColor Cyan
    Write-Host "期望: $ExpectedBehavior" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    
    $args = @(
        "$inputDir\$BaseFile",
        "$inputDir\$SrcFile",
        "-o", $outputFile,
        "-t", $Tolerance,
        "-E", $Engine,
        "-v"
    )
    
    & $exePath @args 2>&1 | Tee-Object -Variable output
    
    if (Test-Path $outputFile) {
        Write-Host "`n输出内容:" -ForegroundColor Green
        Get-Content $outputFile | ForEach-Object { Write-Host "  $_" }
        
        # 检查结果是否包含 NaN（当 tolerance=0 且时间差 50ms 时应该出现 NaN）
        $content = Get-Content $outputFile -Raw
        if ($content -match "NaN") {
            Write-Host "`n[结果] 包含 NaN - tolerance 生效" -ForegroundColor Green
        } else {
            Write-Host "`n[结果] 无 NaN - 值在容差范围内被插值" -ForegroundColor Yellow
        }
    } else {
        Write-Host "`n[错误] 未生成输出文件" -ForegroundColor Red
    }
}

Write-Host "========================================" -ForegroundColor Magenta
Write-Host "Tolerance 功能测试套件" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta

# 测试数据说明:
# tol_t1_base.txt: 时间点 0.0s, 1.0s, 2.0s
# tol_t1_src.txt: 时间点 0.05s, 1.05s, 2.05s (偏移 50ms = 50000us)

# Test 1: tolerance = 0 (精确匹配)
Run-Test -TestName "tol_exact" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "0" -Engine "stream" -ExpectedBehavior "应该输出 NaN (50ms > 0)"

Run-Test -TestName "tol_exact" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "0" -Engine "legacy" -ExpectedBehavior "应该输出 NaN (50ms > 0)"

# Test 2: tolerance = 100000 (100ms > 50ms)
Run-Test -TestName "tol_100ms" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "100000" -Engine "stream" -ExpectedBehavior "应该成功插值 (50ms < 100ms)"

Run-Test -TestName "tol_100ms" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "100000" -Engine "legacy" -ExpectedBehavior "应该成功插值 (50ms < 100ms)"

# Test 3: tolerance = 50000 (50ms = 50ms)
Run-Test -TestName "tol_50ms" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "50000" -Engine "stream" -ExpectedBehavior "应该成功插值 (50ms <= 50ms)"

Run-Test -TestName "tol_50ms" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "50000" -Engine "legacy" -ExpectedBehavior "应该成功插值 (50ms <= 50ms)"

# Test 4: tolerance = -1 (无限制)
Run-Test -TestName "tol_unlimited" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "-1" -Engine "stream" -ExpectedBehavior "应该成功插值 (无限制)"

Run-Test -TestName "tol_unlimited" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "-1" -Engine "legacy" -ExpectedBehavior "应该成功插值 (无限制)"

# Test 5: tolerance = 40000 (40ms < 50ms)
Run-Test -TestName "tol_40ms" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "40000" -Engine "stream" -ExpectedBehavior "应该输出 NaN (50ms > 40ms)"

Run-Test -TestName "tol_40ms" -BaseFile "tol_t1_base.txt" -SrcFile "tol_t1_src.txt" `
         -Tolerance "40000" -Engine "legacy" -ExpectedBehavior "应该输出 NaN (50ms > 40ms)"

Write-Host "`n========================================" -ForegroundColor Magenta
Write-Host "Tolerance 测试完成" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
