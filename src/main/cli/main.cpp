#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStringList>
#include <QDebug>
#include "../../core/merge/DataFileMerger.h"
#include "../../core/merge/StreamMergeEngine.h"
#include "../../core/interpolate/Interpolator.h"

struct MergeConfig {
    QStringList inputFiles;
    QString outputFile;
    Interpolator::InterpolationType interpolationType;
    bool useStreamEngine;
};

MergeConfig parseCommandLine(QCommandLineParser& parser) {
    MergeConfig config;
    
    // 获取输入文件
    config.inputFiles = parser.positionalArguments();
    
    // 获取输出文件
    if (parser.isSet("output")) {
        config.outputFile = parser.value("output");
    } else {
        config.outputFile = "output.txt";
    }
    
    // 获取插值类型
    if (parser.isSet("interpolation")) {
        QString type = parser.value("interpolation");
        if (type == "linear") {
            config.interpolationType = Interpolator::LINEAR;
        } else {
            config.interpolationType = Interpolator::NEAREST_NEIGHBOR;
        }
    } else {
        config.interpolationType = Interpolator::NEAREST_NEIGHBOR;
    }
    
    // 获取合并引擎类型
    config.useStreamEngine = parser.isSet("stream");
    
    return config;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("MultiMerge");
    QCoreApplication::setApplicationVersion("1.0");
    
    QCommandLineParser parser;
    parser.setApplicationDescription("MultiMerge - 多文件数据合并工具");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // 添加命令行选项
    QCommandLineOption outputOption({"o", "output"}, "输出文件路径", "output");
    parser.addOption(outputOption);
    
    QCommandLineOption interpolationOption({"i", "interpolation"}, "插值类型 (nearest/linear)", "interpolation");
    parser.addOption(interpolationOption);
    
    QCommandLineOption streamOption({"s", "stream"}, "使用流式合并引擎");
    parser.addOption(streamOption);
    
    // 添加位置参数
    parser.addPositionalArgument("files", "要合并的输入文件路径", "file1 file2 ...");
    
    // 解析命令行
    parser.process(app);
    
    // 解析配置
    MergeConfig config = parseCommandLine(parser);
    
    // 检查输入文件
    if (config.inputFiles.isEmpty()) {
        qDebug() << "错误: 没有指定输入文件";
        parser.showHelp(1);
        return 1;
    }
    
    qDebug() << "输入文件:" << config.inputFiles;
    qDebug() << "输出文件:" << config.outputFile;
    qDebug() << "插值类型:" << (config.interpolationType == Interpolator::LINEAR ? "linear" : "nearest neighbor");
    qDebug() << "使用流式引擎:" << config.useStreamEngine;
    
    // 执行合并
    bool success;
    if (config.useStreamEngine) {
        success = StreamMergeEngine::mergeFiles(config.inputFiles, config.outputFile, config.interpolationType);
    } else {
        success = DataFileMerger::mergeFiles(config.inputFiles, config.outputFile, config.interpolationType);
    }
    
    if (success) {
        qDebug() << "合并成功! 输出文件:" << config.outputFile;
    } else {
        qDebug() << "合并失败!";
        return 1;
    }
    
    return 0;
}