# 重复列测试脚本

$executable = "../../build/MultiMerge.exe"
$testDir = ".."
$inputDir = "$testDir/input"
$outputDir = "$testDir/output"

# 创建输出目录
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

# 测试用例
$outputFile = "$outputDir/duplicate_columns_test.txt"
$files = @("duplicate_columns_file1.txt", "duplicate_columns_file2.txt") | ForEach-Object { "$inputDir/$_" }

# 构建命令参数
$arguments = @(
    "-o", $outputFile,
    "-v"
) + $files

# 运行命令
Write-Host "Running duplicate columns test"
& $executable @arguments

# 验证结果
if (Test-Path $outputFile) {
    Write-Host "Test: PASSED - Output file created"
    Write-Host "Output file: $outputFile"
} else {
    Write-Host "Test: FAILED - Output file not created"
}

Write-Host "Test completed!"