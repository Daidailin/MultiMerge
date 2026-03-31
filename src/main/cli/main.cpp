#include <iostream>
#include <vector>
#include <string>
#include <getopt.h>
#include "../../core/merge/DataFileMerger.h"
#include "../../core/merge/StreamMergeEngine.h"
#include "../../core/interpolate/Interpolator.h"
#include "../../utils/DelimiterDetector.h"

void printHelp() {
    std::cout << "MultiMerge - 数据文件合并工具" << std::endl;
    std::cout << "用法: MultiMerge [选项] <输入文件 1> <输入文件 2> ..." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -o, --output <文件名>    输出文件名 (默认: merged_result.txt)" << std::endl;
    std::cout << "  -i, --interpolation <方法>  插值方法: nearest, linear, none (默认: nearest)" << std::endl;
    std::cout << "  -d, --delimiter <分隔符>  输出分隔符: space, comma, tab (默认: space)" << std::endl;
    std::cout << "  -e, --encoding <编码>    输出编码: UTF-8, GBK (默认: UTF-8)" << std::endl;
    std::cout << "  -t, --tolerance <毫秒>   时间容差 (默认: 0)" << std::endl;
    std::cout << "  -v, --verbose            详细输出模式" << std::endl;
    std::cout << "  -h, --help               显示此帮助信息" << std::endl;
    std::cout << "  --version                显示版本信息" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  MultiMerge sensor1.txt sensor2.txt -o result.txt -v" << std::endl;
    std::cout << "  MultiMerge data1.txt data2.txt data3.txt -i linear -d comma" << std::endl;
}

void printVersion() {
    std::cout << "MultiMerge v1.0" << std::endl;
    std::cout << "数据文件合并工具 - 基于时间戳对齐合并多个数据文件" << std::endl;
}

int main(int argc, char* argv[]) {
    // 默认参数
    std::string outputFile = "merged_result.txt";
    Interpolator::Method interpolationMethod = Interpolator::Method::NEAREST;
    std::string delimiter = " ";
    std::string encoding = "UTF-8";
    long long tolerance = 0;
    bool verbose = false;
    bool useStreamMerge = false;

    // 解析命令行参数
    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"interpolation", required_argument, 0, 'i'},
        {"delimiter", required_argument, 0, 'd'},
        {"encoding", required_argument, 0, 'e'},
        {"tolerance", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {"stream", no_argument, 0, 's'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "o:i:d:e:t:vhsV", long_options, &option_index)) != -1) {
        switch (c) {
            case 'o':
                outputFile = optarg;
                break;
            case 'i': {
                std::string method = optarg;
                if (method == "nearest") {
                    interpolationMethod = Interpolator::Method::NEAREST;
                } else if (method == "linear") {
                    interpolationMethod = Interpolator::Method::LINEAR;
                } else if (method == "none") {
                    interpolationMethod = Interpolator::Method::NONE;
                } else {
                    std::cerr << "错误: 无效的插值方法: " << method << std::endl;
                    return 1;
                }
                break;
            }
            case 'd': {
                std::string delimiterArg = optarg;
                if (delimiterArg == "space") {
                    delimiter = " ";
                } else if (delimiterArg == "comma") {
                    delimiter = ",";
                } else if (delimiterArg == "tab") {
                    delimiter = "\t";
                } else {
                    delimiter = delimiterArg;
                }
                break;
            }
            case 'e':
                encoding = optarg;
                break;
            case 't':
                tolerance = std::stoll(optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case 'h':
                printHelp();
                return 0;
            case 'V':
                printVersion();
                return 0;
            case 's':
                useStreamMerge = true;
                break;
            default:
                printHelp();
                return 1;
        }
    }

    // 收集输入文件
    std::vector<std::string> inputFiles;
    for (int i = optind; i < argc; ++i) {
        inputFiles.push_back(argv[i]);
    }

    if (inputFiles.empty()) {
        std::cerr << "错误: 至少需要一个输入文件" << std::endl;
        printHelp();
        return 1;
    }

    if (verbose) {
        std::cout << "MultiMerge v1.0" << std::endl;
        std::cout << "输入文件: " << std::endl;
        for (const auto& file : inputFiles) {
            std::cout << "  - " << file << std::endl;
        }
        std::cout << "输出文件: " << outputFile << std::endl;
        std::cout << "插值方法: " << (interpolationMethod == Interpolator::Method::NEAREST ? "nearest" : 
                                     interpolationMethod == Interpolator::Method::LINEAR ? "linear" : "none") << std::endl;
        std::cout << "输出分隔符: " << (delimiter == " " ? "space" : delimiter == "," ? "comma" : "tab") << std::endl;
        std::cout << "时间容差: " << tolerance << " 毫秒" << std::endl;
        std::cout << "使用流式合并: " << (useStreamMerge ? "是" : "否") << std::endl;
        std::cout << "" << std::endl;
    }

    bool success = false;

    if (useStreamMerge) {
        // 使用流式合并引擎
        StreamMergeEngine engine(inputFiles, interpolationMethod, tolerance);
        if (engine.initialize()) {
            success = engine.merge(outputFile, delimiter);
        }
    } else {
        // 使用传统合并引擎
        DataFileMerger merger(interpolationMethod, tolerance);
        for (const auto& file : inputFiles) {
            if (!merger.addFile(file)) {
                std::cerr << "错误: 无法添加文件: " << file << std::endl;
                return 1;
            }
        }
        success = merger.merge(outputFile, delimiter);
    }

    if (success) {
        if (verbose) {
            std::cout << "合并成功! 输出文件: " << outputFile << std::endl;
        }
        return 0;
    } else {
        std::cerr << "合并失败!" << std::endl;
        return 1;
    }
}