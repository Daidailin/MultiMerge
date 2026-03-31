# 格式测试脚本

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
        Name = "test01_dotms_colonus"
        Files = @("01_dot_ms.txt", "04_colon_us.txt")
    },
    @{
        Name = "test02_dotus_colonms"
        Files = @("02_dot_us.txt", "03_colon_ms.txt")
    },
    @{
        Name = "test03_colonms_dotus"
        Files = @("03_colon_ms.txt", "02_dot_us.txt")
    },
    @{
        Name = "test04_colonus_dotms"
        Files = @("04_colon_us.txt", "01_dot_ms.txt")
    }
)

# 运行测试
foreach ($testCase in $testCases) {
    $outputFile = "$outputDir/$($testCase.Name).txt"
    
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
    if (Test-Path $outputFile) {
        Write-Host "Test $($testCase.Name): PASSED - Output file created"
    } else {
        Write-Host "Test $($testCase.Name): FAILED - Output file not created"
    }
}

Write-Host "All tests completed!"