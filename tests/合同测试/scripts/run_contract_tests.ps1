# 合同测试脚本

$executable = "../../build/MultiMerge.exe"
$testDir = ".."
$outputDir = "$testDir/output"

# 创建输出目录
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# 测试用例
$testCases = @(
    @{
        Name = "contract_basic"
        Description = "基本功能测试"
        Command = "$executable ../../tests/基本正确性测试/input/abc_a_file.txt ../../tests/基本正确性测试/input/abc_b_file.txt -o $outputDir/contract_basic.txt -v"
    },
    @{
        Name = "contract_linear"
        Description = "线性插值测试"
        Command = "$executable ../../tests/基本正确性测试/input/abc_a_file.txt ../../tests/基本正确性测试/input/abc_b_file.txt -o $outputDir/contract_linear.txt -i linear -v"
    },
    @{
        Name = "contract_tolerance"
        Description = "时间容差测试"
        Command = "$executable ../../tests/容差测试/input/tol_file1.txt ../../tests/容差测试/input/tol_file2.txt -o $outputDir/contract_tolerance.txt -t 100 -v"
    }
)

# 运行测试
foreach ($testCase in $testCases) {
    Write-Host "Running contract test: $($testCase.Name) - $($testCase.Description)"
    Invoke-Expression $testCase.Command
    
    # 验证结果
    $outputFile = "$outputDir/$($testCase.Name).txt"
    if (Test-Path $outputFile) {
        Write-Host "Test $($testCase.Name): PASSED - Output file created"
    } else {
        Write-Host "Test $($testCase.Name): FAILED - Output file not created"
    }
}

Write-Host "All contract tests completed!"