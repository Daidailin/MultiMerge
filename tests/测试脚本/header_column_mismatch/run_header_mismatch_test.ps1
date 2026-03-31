# 表头列不匹配测试脚本

$executable = "../../../build/MultiMerge.exe"
$testDir = ".."
$outputDir = "$testDir/output"

# 创建输出目录
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# 测试用例
$testCases = @(
    @{
        Name = "header_mismatch_2files"
        Files = @("../../../tests/表头数据一致性测试/input/file1.txt", "../../../tests/表头数据一致性测试/input/file2.txt")
    },
    @{
        Name = "header_mismatch_3files"
        Files = @("../../../tests/表头数据一致性测试/input/file1.txt", "../../../tests/表头数据一致性测试/input/file2.txt", "../../../tests/表头数据一致性测试/input/file3.txt")
    }
)

# 运行测试
foreach ($testCase in $testCases) {
    $outputFile = "$outputDir/$($testCase.Name).txt"
    
    # 构建命令参数
    $arguments = @(
        "-o", $outputFile,
        "-v"
    ) + $testCase.Files
    
    # 运行命令
    Write-Host "Running test: $($testCase.Name)"
    & $executable @arguments
    
    # 验证结果
    if (Test-Path $outputFile) {
        Write-Host "Test $($testCase.Name): PASSED - Output file created"
    } else {
        Write-Host "Test $($testCase.Name): FAILED - Output file not created"
    }
}

Write-Host "All header mismatch tests completed!"