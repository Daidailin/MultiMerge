#include "StreamMergeEngine.h"
#include <iostream>
#include <sstream>
#include <algorithm>

StreamMergeEngine::FileStream::FileStream(const std::string& filename) : hasData(false) {
    file.open(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    hasData = readNextLine();
}

bool StreamMergeEngine::FileStream::readNextLine() {
    if (!std::getline(file, currentLine)) {
        return false;
    }

    std::istringstream ss(currentLine);
    std::string timeStr;
    if (!(ss >> timeStr)) {
        return false;
    }

    if (!currentTime.fromString(timeStr)) {
        return false;
    }

    currentValues.clear();
    std::string value;
    while (ss >> value) {
        currentValues.push_back(value);
    }

    return true;
}

StreamMergeEngine::StreamMergeEngine(const std::vector<std::string>& filenames, Interpolator::Method method, long long tolerance) 
    : interpolationMethod(method), timeTolerance(tolerance) {
    for (const auto& filename : filenames) {
        streams.emplace_back(filename);
    }
}

StreamMergeEngine::~StreamMergeEngine() {
    for (auto& stream : streams) {
        if (stream.file.is_open()) {
            stream.file.close();
        }
    }
}

bool StreamMergeEngine::initialize() {
    // 读取所有文件的第一行作为表头
    for (size_t i = 0; i < streams.size(); ++i) {
        if (!streams[i].hasData) {
            return false;
        }

        std::istringstream ss(streams[i].currentLine);
        std::string header;
        if (!(ss >> header)) {
            return false;
        }

        // 对于第一个文件，保留时间列
        if (i == 0) {
            headers.push_back(header);
        }

        // 添加数据列
        std::string column;
        while (ss >> column) {
            headers.push_back(column);
        }

        // 读取下一行数据
        if (!streams[i].readNextLine()) {
            streams[i].hasData = false;
        }
    }

    return true;
}

bool StreamMergeEngine::merge(const std::string& outputFilename, const std::string& delimiter) {
    std::ofstream output(outputFilename);
    if (!output.is_open()) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return false;
    }

    // 写入表头
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i > 0) {
            output << delimiter;
        }
        output << headers[i];
    }
    output << std::endl;

    // 主合并循环
    while (true) {
        // 找到最小的时间点
        TimePoint minTime;
        size_t minIndex = -1;
        bool hasData = false;

        for (size_t i = 0; i < streams.size(); ++i) {
            if (streams[i].hasData) {
                if (!hasData || streams[i].currentTime < minTime) {
                    minTime = streams[i].currentTime;
                    minIndex = i;
                    hasData = true;
                }
            }
        }

        if (!hasData) {
            break; // 所有文件都已处理完毕
        }

        // 输出时间列
        output << minTime.toString();

        // 处理每个文件的数据
        for (size_t i = 0; i < streams.size(); ++i) {
            if (streams[i].hasData) {
                // 检查时间是否在容差范围内
                if (std::abs(streams[i].currentTime - minTime) <= timeTolerance) {
                    // 输出当前数据
                    for (const auto& value : streams[i].currentValues) {
                        output << delimiter << value;
                    }
                    // 读取下一行
                    if (!streams[i].readNextLine()) {
                        streams[i].hasData = false;
                    }
                } else if (streams[i].currentTime < minTime) {
                    // 数据过期，跳过
                    if (!streams[i].readNextLine()) {
                        streams[i].hasData = false;
                    }
                    // 递归调用以处理新数据
                    continue;
                } else {
                    // 数据尚未到达，输出空值
                    for (size_t j = 0; j < streams[i].currentValues.size(); ++j) {
                        output << delimiter << "";
                    }
                }
            } else {
                // 文件已结束，输出空值
                size_t valueCount = (i == 0) ? streams[0].currentValues.size() : streams[i].currentValues.size();
                for (size_t j = 0; j < valueCount; ++j) {
                    output << delimiter << "";
                }
            }
        }

        output << std::endl;
    }

    output.close();
    return true;
}