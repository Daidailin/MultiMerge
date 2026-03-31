# 混合模式测试脚本

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
        Name = "mixed_nearest"
        Files = @("mixed_a1_file1.txt", "mixed_b1_file2.txt", "mixed_c1_file3.txt")
        Interpolation = "nearest"
    },
    @{
        Name = "mixed_linear"
        Files = @("mixed_a1_file1.txt", "mixed_b1_file2.txt", "mixed_c1_file3.txt")
        Interpolation = "linear"
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