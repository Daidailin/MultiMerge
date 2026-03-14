#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTextCodec>
#include <QDir>
#include "core/DataFileMerger.h"
#pragma execution_character_set("utf-8")

void printResult(const MergeResult& result, bool verbose) {
    qDebug() << "========================================";
    if (result.success) {
        qDebug() << "合并成功!";
        qDebug() << "输出文件:" << result.outputFile;
        qDebug() << "合并行数:" << result.mergedRows;
    } else {
        qDebug() << "合并失败!";
    }
    
    if (verbose || !result.success) {
        if (!result.warnings.isEmpty()) {
            qDebug() << "警告信息:";
            for (const QString& warning : result.warnings) {
                qDebug() << "  -" << warning;
            }
        }
    }
    qDebug() << "========================================";
}

int main(int argc, char *argv[]) {
    qDebug() << "=== 程序启动 ===";
    qDebug() << "argc:" << argc;
    qDebug() << "工作目录:" << QDir::currentPath();
    
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("MultiMerge");
    QCoreApplication::setApplicationVersion("1.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("数据文件合并工具 - 基于时间戳对齐合并多个数据文件");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "输入文件列表（至少 1 个）", "<file1> [file2] [file3] ...");
    
    parser.addOption({{"o", "output"}, "输出文件名", "filename", "merged_result.txt"});
    parser.addOption({{"i", "interpolation"}, "插值方法：nearest, linear, none", "method", "nearest"});
    parser.addOption({{"d", "delimiter"}, "输出分隔符：space, comma, tab", "delimiter", "space"});
    parser.addOption({{"e", "encoding"}, "输出编码：UTF-8, GBK", "encoding", "UTF-8"});
    parser.addOption({{"t", "tolerance"}, "时间容差（毫秒）", "milliseconds", "0"});
    parser.addOption({{"v", "verbose"}, "详细输出"});
    
    QStringList args = QCoreApplication::arguments();
    
    if (args.contains("-h") || args.contains("--help") || args.contains("-?")) {
        parser.showHelp(0);
    }
    if (args.contains("--version")) {
        qDebug() << "MultiMerge 1.0";
        return 0;
    }
    
    QString outputFile = "merged_result.txt";
    QString interpolationMethod = "nearest";
    QString delimiterMethod = "space";
    QString encoding = "UTF-8";
    int tolerance = 0;
    bool verbose = false;
    
    QVector<QString> inputFiles;
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];
        if (arg == "-o" && i + 1 < args.size()) {
            outputFile = args[++i];
        } else if (arg == "-i" && i + 1 < args.size()) {
            interpolationMethod = args[++i];
        } else if (arg == "-d" && i + 1 < args.size()) {
            delimiterMethod = args[++i];
        } else if (arg == "-e" && i + 1 < args.size()) {
            encoding = args[++i];
        } else if (arg == "-t" && i + 1 < args.size()) {
            tolerance = args[++i].toInt();
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (!arg.startsWith("-")) {
            inputFiles << arg;
        }
    }
    
    if (inputFiles.isEmpty()) {
        qWarning() << "错误：请至少指定一个输入文件";
        qWarning() << "使用 --help 查看使用说明";
        return 1;
    }
    
    for (const QString& file : inputFiles) {
        QFile f(file);
        if (!f.exists()) {
            qWarning() << "错误：文件不存在:" << file;
            return 1;
        }
    }
    
    MergeConfig config;
    interpolationMethod = interpolationMethod.toLower();
    if (interpolationMethod == "nearest") {
        config.interpolation = InterpolationMethod::Nearest;
    } else if (interpolationMethod == "linear") {
        config.interpolation = InterpolationMethod::Linear;
    } else if (interpolationMethod == "none") {
        config.interpolation = InterpolationMethod::None;
    }
    
    delimiterMethod = delimiterMethod.toLower();
    if (delimiterMethod == "space") config.outputDelimiter = " ";
    else if (delimiterMethod == "comma") config.outputDelimiter = ",";
    else if (delimiterMethod == "tab") config.outputDelimiter = "\t";
    else config.outputDelimiter = delimiterMethod;
    
    config.outputEncoding = encoding.toUpper();
    if (tolerance > 0) config.timeToleranceUs = tolerance * 1000;
    config.verbose = verbose;
    
    qDebug() << "========================================";
    qDebug() << "MultiMerge 数据文件合并工具 v1.0";
    qDebug() << "========================================";
    qDebug() << "输入文件:" << inputFiles.size();
    for (const QString& file : inputFiles) qDebug() << "  -" << file;
    qDebug() << "输出文件:" << outputFile;
    qDebug() << "插值方法:" << interpolationMethod;
    qDebug() << "输出分隔符:" << delimiterMethod;
    qDebug() << "输出编码:" << config.outputEncoding;
    if (config.timeToleranceUs > 0) qDebug() << "时间容差:" << (config.timeToleranceUs / 1000.0) << "ms";
    qDebug() << "========================================";
    
    DataFileMerger merger(config);
    
    QObject::connect(&merger, &DataFileMerger::progressChanged,
        [&merger](int percent) {
            if (percent % 10 == 0) qDebug().noquote() << QString("进度：%1%").arg(percent);
        });
    
    QObject::connect(&merger, &DataFileMerger::statusMessage,
        [&merger](const QString& message) {
            qDebug().noquote() << "[状态]" << message;
        });
    
    QObject::connect(&merger, &DataFileMerger::warningOccurred,
        [&merger](const QString& warning) {
            qWarning().noquote() << "[警告]" << warning;
        });
    
    MergeResult result = merger.merge(inputFiles, outputFile);
    
    qDebug();
    printResult(result, config.verbose);
    
    return result.success ? 0 : 1;
}
