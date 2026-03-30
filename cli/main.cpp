﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿#include "core/DataFileMerger.h"
#include "core/StreamMergeEngine.h"
#include "TimePoint.h"
#include "TimeParser.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTextCodec>
#include <QDir>


#pragma execution_character_set("utf-8")
using namespace TimeMerge;

void test_timeFormats() {
    qDebug() << "\n========== 测试时间格式（简化版）==========";
    
    // 测试 1: 冒号分隔格式的解析（自动精度推断）
    qDebug() << "\n--- 测试冒号分隔格式解析（自动精度推断） ---";
    
    auto testParse = [](const QString& input) {
        auto& parser = TimeParser::instance();
        ParseResult result = parser.parse(input);
        if (result.isSuccess()) {
            TimePoint tp = result.timePoint();
            QString resolutionStr;
            switch (tp.resolution()) {
                case TimeResolution::Milliseconds: resolutionStr = "毫秒"; break;
                case TimeResolution::Microseconds: resolutionStr = "微秒"; break;
                default: resolutionStr = "未知"; break;
            }
            qDebug() << "输入：" << input << " → 输出：" << tp.toString() << " [" << resolutionStr << "]";
        } else {
            qDebug() << "解析失败：" << input << " 错误：" << result.errorMessage();
        }
    };
    
    // 测试自动精度推断
    qDebug() << "\n[自动精度推断测试]";
    qDebug() << "规则：1-3 位=毫秒，4-6 位=微秒";
    qDebug() << "";
    
    // 毫秒精度（1-3 位）
    qDebug() << "--- 毫秒精度（1-3 位）---";
    testParse("1:2:3:5");      // 1 位 → 毫秒 → 01:02:03:005
    testParse("1:2:3:50");     // 2 位 → 毫秒 → 01:02:03:050
    testParse("1:2:3:123");    // 3 位 → 毫秒 → 01:02:03:123
    testParse("12:30:45:123");  // 3 位 → 毫秒 → 12:30:45:123
    testParse("0:0:0:000");    // 3 位 → 毫秒 → 00:00:00:000
    
    // 微秒精度（4-6 位）
    qDebug() << "\n--- 微秒精度（4-6 位）---";
    testParse("1:2:3:1234");     // 4 位 → 微秒 → 01:02:03:001234
    testParse("1:2:3:12345");    // 5 位 → 微秒 → 01:02:03:012345
    testParse("1:2:3:123456");   // 6 位 → 微秒 → 01:02:03:123456
    testParse("12:30:45:123456"); // 6 位 → 微秒 → 12:30:45:123456
    
    // 测试 2: TimePoint 输出格式（统一补零风格）
    qDebug() << "\n--- 测试 TimePoint 输出格式（统一补零） ---";
    
    TimePoint tp1(12, 30, 45, 123, TimeResolution::Milliseconds);
    qDebug() << "毫秒时间点（补零）：" << tp1.toString();  // 应为 "12:30:45:123"
    
    TimePoint tp2(12, 30, 45, 123456, TimeResolution::Microseconds);
    qDebug() << "微秒时间点（补零）：" << tp2.toString();  // 应为 "12:30:45:123456"
    
    // 测试 3: 自定义格式
    qDebug() << "\n--- 测试自定义格式 ---";
    // 清理版已移除纳秒支持，使用微秒精度测试
    TimePoint tp(12, 30, 45, 500000, TimeResolution::Microseconds);
    
    qDebug() << "默认格式（补零）：" << tp.toString();
    qDebug() << "自定义 (hh:mm:ss)：" << tp.toString("hh:mm:ss");
    qDebug() << "自定义 (hh:mm:ss:fff)：" << tp.toString("hh:mm:ss:fff");
    qDebug() << "自定义 (hh:mm:ss:u)：" << tp.toString("hh:mm:ss:u");
    
    qDebug() << "\n========== 测试完成 ==========\n";
}

void printResult(const MergeResult& result, bool verbose) {
    if (result.success) {
        // 合并成功
        qDebug() << "合并成功!";
        qDebug() << "  输出文件:" << result.outputFile;
        qDebug() << "  合并行数:" << result.mergedRows;
        
        // 如果有警告，显示警告数量
        if (result.warningCount > 0) {
            qDebug() << "  警告数量:" << result.warningCount;
            
            // 详细模式下显示所有警告的具体内容
            if (verbose) {
                qDebug() << "\n警告详情:";
                for (const QString& warning : result.warnings) {
                    qDebug() << "  -" << warning;
                }
            }
        }
    } else {
        // 合并失败
        qWarning() << "合并失败!";
        for (const QString& warning : result.warnings) {
            qWarning() << "  -" << warning;
        }
    }
}


int main(int argc, char *argv[]) {
    // ==================== 程序初始化 ====================
    // 创建 Qt 应用程序对象（命令行工具不需要 GUI）
    QCoreApplication app(argc, argv);

    // 设置应用程序基本信息
    QCoreApplication::setApplicationName("MultiMerge");
    QCoreApplication::setApplicationVersion("1.0");
    
    // ==================== 命令行解析器配置 ====================
    QCommandLineParser parser;
    parser.setApplicationDescription("数据文件合并工具 - 基于时间戳对齐合并多个数据文件");
    parser.addHelpOption();    // 添加 -h/--help 选项
    parser.addVersionOption(); // 添加 --version 选项
    // 添加位置参数：输入文件列表
    parser.addPositionalArgument("files", "输入文件列表（至少 2 个）", "<file1> <file2> [file3] ...");
    


    // 添加 --output 选项（短选项：-o）
    QCommandLineOption outputOption(
        QStringList() << "o" << "output",
        "输出文件名（默认：merged_result.txt）",
        "filename",
        "merged_result.txt"  // 默认值
    );
    parser.addOption(outputOption);
    
    // 添加 --interpolation 选项（短选项：-i）
    QCommandLineOption interpolationOption(
        QStringList() << "i" << "interpolation",
        "插值方法：nearest（最近邻）, linear（线性）（默认：nearest）",
        "method",
        "nearest"  // 默认使用最近邻插值
    );
    parser.addOption(interpolationOption);
    
    // 添加 --delimiter 选项（短选项：-d）
    QCommandLineOption delimiterOption(
        QStringList() << "d" << "delimiter",
        "输出分隔符：space（空格）, comma（逗号）, tab（制表符）（默认：space）",
        "delimiter",
        "space"  // 默认使用空格分隔
    );
    parser.addOption(delimiterOption);
    
    // 添加 --encoding 选项（短选项：-e）
    QCommandLineOption encodingOption(
        QStringList() << "e" << "encoding",
        "输出编码：UTF-8, GBK（默认：UTF-8）",
        "encoding",
        "UTF-8"  // 默认使用 UTF-8 编码
    );
    parser.addOption(encodingOption);
    
    // 添加 --tolerance 选项（短选项：-t）
    QCommandLineOption toleranceOption(
        QStringList() << "t" << "tolerance",
        "时间容差（微秒），-1 表示不限制，0 表示精确匹配（默认：-1）",
        "microseconds",
        "-1"  // 默认不限制时间容差
    );
    parser.addOption(toleranceOption);
    
    // 添加 --engine 选项（短选项：-E）
    QCommandLineOption engineOption(
        QStringList() << "E" << "engine",
        "合并引擎：legacy（旧引擎）, stream（新流式引擎）（默认：stream）",
        "engine",
        "stream"  // 默认使用新引擎
    );
    parser.addOption(engineOption);
    
    // 添加 --verbose 选项（短选项：-v）
    QCommandLineOption verboseOption(
        QStringList() << "v" << "verbose",
        "详细输出（显示所有警告信息）"
    );
    parser.addOption(verboseOption);
    
    // ==================== 手动解析命令行参数 ====================
    
    // 重要：使用手动解析而非 parser.process()
    // 原因：parser.process() 在遇到 --help 时会直接退出程序，无法进行后续处理
    // 我们采用手动解析，完全控制程序流程
    QStringList args = QCoreApplication::arguments();
    
    // 检查帮助选项
    if (args.contains("-h") || args.contains("--help") || args.contains("-?")) {
        parser.showHelp(0);  // 显示帮助并退出
    }
    
    // 检查版本选项
    // 注意：只响应 --version，避免与 -v（verbose）冲突
    if (args.contains("--version")) {
        qDebug() << "MultiMerge 1.0";
        return 0;
    }
    
    // 初始化配置变量（使用默认值）
    QString outputFile = "merged_result.txt";
    QString interpolationMethod = "nearest";
    QString delimiterMethod = "space";
    QString encoding = "UTF-8";
    qint64 toleranceUs = -1;  // -1 表示无限制
    bool verbose = false;
    QString engineType = "stream";
    
    // 存储输入文件的向量
    QVector<QString> inputFiles;
    
    // 遍历所有参数（从索引 1 开始，跳过程序名）
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];
        
        // 解析各个选项
        if (arg == "-o" || arg == "--output") {
            if (i + 1 < args.size()) {
                outputFile = args[++i];  // 读取下一个参数作为输出文件名
            }
        } else if (arg == "-i" || arg == "--interpolation") {
            if (i + 1 < args.size()) {
                interpolationMethod = args[++i];  // 读取插值方法
            }
        } else if (arg == "-d" || arg == "--delimiter") {
            if (i + 1 < args.size()) {
                delimiterMethod = args[++i];  // 读取分隔符
            }
        } else if (arg == "-e" || arg == "--encoding") {
            if (i + 1 < args.size()) {
                encoding = args[++i];  // 读取编码
            }
        } else if (arg == "-t" || arg == "--tolerance") {
            if (i + 1 < args.size()) {
                bool ok = false;
                toleranceUs = args[++i].toLongLong(&ok);  // 读取时间容差（微秒）
                if (!ok || toleranceUs < -1) {
                    qWarning() << "错误：--tolerance 参数必须是 -1 或 >= 0 的整数（单位：微秒）";
                    return 1;
                }
            }
        } else if (arg == "-E" || arg == "--engine") {
            if (i + 1 < args.size()) {
                engineType = args[++i];
            }
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;  // 启用详细模式
        } else if (!arg.startsWith("-")) {
            // 非选项参数（不以 - 开头）视为输入文件
            inputFiles << arg;
        }
    }
    

    
    // ==================== 参数验证 ====================
    
    // 检查是否至少有两个输入文件
    if (inputFiles.size() < 2) {
        qWarning() << "错误：请至少指定两个输入文件";
        qWarning() << "使用 --help 查看使用说明";
        return 1;
    }
    
    // 检查所有输入文件是否存在
    for (const QString& file : inputFiles) {
        QFile f(file);
        if (!f.exists()) {
            qWarning() << "错误：文件不存在:" << file;
            qWarning() << "当前工作目录:" << QDir::currentPath();
            return 1;
        }
    }
    
    // ==================== 配置转换 ====================
    MergeConfig config;  // 合并配置结构体
    
    // 1. 插值方法转换（字符串 → 枚举）
    interpolationMethod = interpolationMethod.toLower();
    if (interpolationMethod == "nearest") {
        config.interpolation = InterpolationMethod::Nearest;
    } else if (interpolationMethod == "linear") {
        config.interpolation = InterpolationMethod::Linear;
    } else {
        qWarning() << "警告：未知的插值方法" << interpolationMethod << "，使用默认值 nearest";
        config.interpolation = InterpolationMethod::Nearest;
    }
    
    // 2. 输出分隔符转换
    delimiterMethod = delimiterMethod.toLower();
    if (delimiterMethod == "space") {
        config.outputDelimiter = " ";    // 空格
    } else if (delimiterMethod == "comma") {
        config.outputDelimiter = ",";    // 逗号
    } else if (delimiterMethod == "tab") {
        config.outputDelimiter = "\t";   // 制表符
    } else {
        // 允许用户使用自定义分隔符
        config.outputDelimiter = delimiterMethod;
    }
    
    // 3. 输出编码设置
    config.outputEncoding = encoding.toUpper();  // 转换为大写
    
    // 4. 时间容差设置（直接使用微秒）
    config.timeToleranceUs = toleranceUs;
    
    // 5. 详细模式设置
    config.verbose = verbose;
    
    // ==================== StreamMergeEngine 配置 ====================
    StreamMergeEngine::Config streamConfig;
    streamConfig.verbose = verbose;
    
    // 插值方法转换
    if (interpolationMethod == "nearest") {
        streamConfig.interpolationMethod = StreamMergeEngine::InterpolationMethod::Nearest;
    } else if (interpolationMethod == "linear") {
        streamConfig.interpolationMethod = StreamMergeEngine::InterpolationMethod::Linear;
    }
    
    // 分隔符转换
    if (delimiterMethod == "space") {
        streamConfig.outputDelimiter = StreamMergeEngine::OutputDelimiter::Space;
    } else if (delimiterMethod == "comma") {
        streamConfig.outputDelimiter = StreamMergeEngine::OutputDelimiter::Comma;
    } else if (delimiterMethod == "tab") {
        streamConfig.outputDelimiter = StreamMergeEngine::OutputDelimiter::Tab;
    }
    
    // 编码转换
    if (encoding.toUpper() == "UTF-8") {
        streamConfig.outputEncoding = StreamMergeEngine::OutputEncoding::Utf8;
    } else {
        streamConfig.outputEncoding = StreamMergeEngine::OutputEncoding::Gbk;
    }
    
    // 时间容差设置（直接使用微秒）
    streamConfig.timeToleranceUs = toleranceUs;
    
    // ==================== 打印配置信息 ====================
    qDebug() << "========================================";
    qDebug() << "MultiMerge 数据文件合并工具 v1.0";
    qDebug() << "========================================";
    qDebug() << "输入文件:" << inputFiles.size();
    for (const QString& file : inputFiles) {
        qDebug() << "  -" << file;
    }
    qDebug() << "输出文件:" << outputFile;
    qDebug() << "插值方法:" << interpolationMethod;
    qDebug() << "输出分隔符:" << delimiterMethod;
    qDebug() << "输出编码:" << config.outputEncoding;
    // 显示时间容差设置
    if (config.timeToleranceUs < 0) {
        qDebug() << "时间容差: unlimited";
    } else {
        qDebug() << "时间容差:" << config.timeToleranceUs << "us";
    }
    qDebug() << "========================================";
    qDebug();
    
    // ==================== 创建合并器并执行 ====================
    qDebug() << "========================================";
    qDebug() << "开始执行合并...";
    qDebug() << "========================================";

    bool useStreamEngine = (engineType.toLower() == "stream");
    bool mergeSuccess = false;
    
    if (useStreamEngine) {
        // 使用新的流式引擎
        qDebug() << "使用 StreamMergeEngine（流式引擎）";
        
        StreamMergeEngine merger;
        merger.setConfig(streamConfig);
        
        // 连接信号用于显示进度
        QObject::connect(&merger, &StreamMergeEngine::progressChanged,
                         [](int percent, const QString& message) {
                             qDebug() << "进度:" << percent << "% -" << message;
                         });
        QObject::connect(&merger, &StreamMergeEngine::statusMessage,
                         [](const QString& message) {
                             qDebug() << "状态:" << message;
                         });
        QObject::connect(&merger, &StreamMergeEngine::warningOccurred,
                         [](const QString& warning) {
                             qWarning() << "警告:" << warning;
                         });
        
        StreamMergeEngine::MergeResult result = merger.merge(inputFiles, outputFile);
        mergeSuccess = result.success;
        
        // 输出统计信息
        qDebug() << "========================================";
        qDebug() << "合并结果:";
        qDebug() << "  成功:" << (result.success ? "是" : "否");
        qDebug() << "  取消:" << (result.cancelled ? "是" : "否");
        if (!result.errorMessage.isEmpty()) {
            qDebug() << "  错误:" << result.errorMessage;
        }
        qDebug() << "  输入文件数:" << result.stats.totalInputFiles;
        qDebug() << "  输出行数:" << result.stats.totalOutputRows;
        qDebug() << "  扫描耗时:" << result.stats.scanElapsedMs << "ms";
        qDebug() << "  合并耗时:" << result.stats.mergeElapsedMs << "ms";
        qDebug() << "  总耗时:" << result.stats.totalElapsedMs << "ms";
        
        // 输出文件分类统计
        qDebug() << "  A 类文件 (单增 + 均匀):" << result.stats.sequentialUniformFiles;
        qDebug() << "  B 类文件 (单增 + 不均匀):" << result.stats.sequentialCorrectedFiles;
        qDebug() << "  C 类文件 (乱序/回退):" << result.stats.fallbackFiles;
        
        // 输出警告信息
        if (!result.warnings.isEmpty()) {
            qDebug() << "警告信息:";
            for (const QString& warning : result.warnings) {
                qWarning() << "  -" << warning;
            }
        }
    } else {
        // 使用旧版 DataFileMerger
        qDebug() << "使用 DataFileMerger（旧版引擎）";
        
        DataFileMerger merger(config);  // 创建数据文件合并器
        
        // 连接进度信号（每 10% 显示一次进度）
        QObject::connect(&merger, &DataFileMerger::progressChanged,
            [&merger](int percent) {
                if (percent % 10 == 0) {  // 只在 10%、20%、30%... 时显示
                    qDebug().noquote() << QString("进度：%1%").arg(percent);
                }
            });
        
        // 连接状态消息信号
        QObject::connect(&merger, &DataFileMerger::statusMessage,
            [&merger](const QString& message) {
                qDebug().noquote() << "[状态]" << message;
            });
        
        // 连接警告信号
        QObject::connect(&merger, &DataFileMerger::warningOccurred,
            [&merger](const QString& warning) {
                qWarning().noquote() << "[警告]" << warning;
            });
        
        // 执行合并操作
        // 这是核心步骤，会：
        // 1. 加载所有输入文件
        // 2. 以第一个文件的时间轴为基准
        // 3. 对其他文件进行插值对齐
        // 4. 写入输出文件
        MergeResult result = merger.merge(inputFiles, outputFile);
        mergeSuccess = result.success;
        
        // ==================== 输出结果 ====================
        qDebug();
        printResult(result, config.verbose);  // 打印合并结果
    }
    
    return mergeSuccess ? 0 : 1;
}
