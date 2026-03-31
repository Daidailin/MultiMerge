# 容差测试脚本

$executable = "../../build/MultiMerge.exe"
$testDir = ".."
$inputDir = "$testDir/input"
$outputDir = "$testDir/output"

# 创建输出目录
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# 测试用例
$testCases = @(
    @{
        Name = "tol_0ms"
        Files = @("tol_file1.txt", "tol_file2.txt")
        Tolerance = 0
    },
    @{
        Name = "tol_100ms"
        Files = @("tol_file1.txt", "tol_file2.txt")
        Tolerance = 100
    },
    @{
        Name = "tol_500ms"
        Files = @("tol_file1.txt", "tol_file2.txt")
        Tolerance = 500
    }
)

# 运行测试
foreach ($testCase in $testCases) {
    $outputFile = "$outputDir/$($testCase.Name).txt"
    
    # 构建命令参数
    $files = $testCase.Files | ForEach-Object { "$inputDir/$_" }
    $arguments = @(
        "-o", $outputFile,
        "-t", $testCase.Tolerance,
        "-v"
    ) + $files
    
    # 运行命令
    Write-Host "Running test: $($testCase.Name) with tolerance $($testCase.Tolerance)ms"
    & $executable @arguments
    
    # 验证结果
    if (Test-Path $outputFile) {
        Write-Host "Test $($testCase.Name): PASSED - Output file created"
    } else {
        Write-Host "Test $($testCase.Name): FAILED - Output file not created"
    }
}

Write-Host "All tests completed!"