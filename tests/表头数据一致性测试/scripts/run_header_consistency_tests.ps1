# 表头数据一致性测试脚本

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
        Name = "result_2files"
        Files = @("file1.txt", "file2.txt")
    },
    @{
        Name = "result_3files"
        Files = @("file1.txt", "file2.txt", "file3.txt")
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