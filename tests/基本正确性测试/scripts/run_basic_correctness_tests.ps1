# 基本正确性测试脚本

$executable = "../../build/MultiMerge.exe"
$testDir = ".."
$inputDir = "$testDir/input"
$expectedDir = "$testDir/expected"
$outputDir = "$testDir/output"

# 创建输出目录
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# 测试用例
$testCases = @(
    @{
        Name = "gold_a_nearest"
        Files = @("abc_a_file.txt", "abc_b_file.txt")
        Interpolation = "nearest"
    },
    @{
        Name = "gold_a_linear"
        Files = @("abc_a_file.txt", "abc_b_file.txt")
        Interpolation = "linear"
    },
    @{
        Name = "gold_a_exact"
        Files = @("abc_a_file.txt", "abc_b_file.txt")
        Interpolation = "none"
    }
)

# 运行测试
foreach ($testCase in $testCases) {
    $outputFile = "$outputDir/$($testCase.Name).txt"
    $expectedFile = "$expectedDir/$($testCase.Name).txt"
    
    # 构建命令参数
    $files = $testCase.Files | ForEach-Object { "$inputDir/$_" }
    $arguments = @(
        "-o", $outputFile,
        "-i", $testCase.Interpolation,
        "-v"
    ) + $files
    
    # 运行命令
    Write-Host "Running test: $($testCase.Name)"
    & $executable @arguments
    
    # 验证结果
    if (Test-Path $outputFile -and Test-Path $expectedFile) {
        $outputContent = Get-Content $outputFile
        $expectedContent = Get-Content $expectedFile
        
        if (Compare-Object $outputContent $expectedContent) {
            Write-Host "Test $($testCase.Name): FAILED - Output does not match expected"
        } else {
            Write-Host "Test $($testCase.Name): PASSED"
        }
    } else {
        Write-Host "Test $($testCase.Name): FAILED - Output or expected file not found"
    }
}

Write-Host "All tests completed!"