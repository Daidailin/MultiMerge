#include "DataFileMerger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <map>

DataFileMerger::DataFileMerger(Interpolator::Method method, long long tolerance) 
    : interpolationMethod(method), timeTolerance(tolerance) {}

bool DataFileMerger::addFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    FileData fileData;
    std::string line;

    // 读取表头
    if (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string header;
        while (ss >> header) {
            fileData.headers.push_back(header);
        }
    }

    // 读取数据
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string timeStr;
        if (!(ss >> timeStr)) {
            continue;
        }

        TimePoint time;
        if (!time.fromString(timeStr)) {
            continue;
        }

        fileData.times.push_back(time);

        std::vector<std::string> values;
        std::string value;
        while (ss >> value) {
            values.push_back(value);
        }
        fileData.values.push_back(values);
    }

    fileDataList.push_back(fileData);
    return true;
}

bool DataFileMerger::merge(const std::string& outputFilename, const std::string& delimiter) {
    if (fileDataList.empty()) {
        std::cerr << "No files added for merging" << std::endl;
        return false;
    }

    // 准备输出文件
    std::ofstream output(outputFilename);
    if (!output.is_open()) {
        std::cerr << "Error opening output file: " << outputFilename << std::endl;
        return false;
    }

    // 收集所有时间点
    std::vector<TimePoint> allTimes;
    for (const auto& fileData : fileDataList) {
        allTimes.insert(allTimes.end(), fileData.times.begin(), fileData.times.end());
    }

    // 去重并排序
    std::sort(allTimes.begin(), allTimes.end());
    auto last = std::unique(allTimes.begin(), allTimes.end());
    allTimes.erase(last, allTimes.end());

    // 写入表头
    for (size_t i = 0; i < fileDataList[0].headers.size(); ++i) {
        if (i > 0) {
            output << delimiter;
        }
        output << fileDataList[0].headers[i];
    }

    for (size_t i = 1; i < fileDataList.size(); ++i) {
        for (size_t j = 1; j < fileDataList[i].headers.size(); ++j) {
            output << delimiter << fileDataList[i].headers[j];
        }
    }
    output << std::endl;

    // 处理每个时间点
    for (const auto& time : allTimes) {
        output << time.toString();

        // 处理第一个文件（作为基准）
        const auto& baseFile = fileDataList[0];
        auto it = std::lower_bound(baseFile.times.begin(), baseFile.times.end(), time);
        if (it != baseFile.times.end() && std::abs(*it - time) <= timeTolerance) {
            size_t index = it - baseFile.times.begin();
            for (const auto& value : baseFile.values[index]) {
                output << delimiter << value;
            }
        } else {
            // 时间点不在容差范围内，输出空值
            for (size_t j = 0; j < baseFile.headers.size() - 1; ++j) {
                output << delimiter << "";
            }
        }

        // 处理其他文件
        for (size_t i = 1; i < fileDataList.size(); ++i) {
            const auto& fileData = fileDataList[i];
            auto it = std::lower_bound(fileData.times.begin(), fileData.times.end(), time);

            if (it != fileData.times.end() && std::abs(*it - time) <= timeTolerance) {
                size_t index = it - fileData.times.begin();
                for (const auto& value : fileData.values[index]) {
                    output << delimiter << value;
                }
            } else if (it != fileData.times.begin() && std::abs(*(it - 1) - time) <= timeTolerance) {
                size_t index = (it - 1) - fileData.times.begin();
                for (const auto& value : fileData.values[index]) {
                    output << delimiter << value;
                }
            } else {
                // 时间点不在容差范围内，输出空值
                for (size_t j = 0; j < fileData.headers.size() - 1; ++j) {
                    output << delimiter << "";
                }
            }
        }

        output << std::endl;
    }

    output.close();
    return true;
}

std::string DataFileMerger::detectDelimiter(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return " "; // 默认空格
    }

    std::string line;
    if (!std::getline(file, line)) {
        return " ";
    }

    // 统计可能的分隔符
    std::map<char, int> delimiterCount;
    delimiterCount[','] = 0;
    delimiterCount[';'] = 0;
    delimiterCount['\t'] = 0;
    delimiterCount[' '] = 0;

    for (char c : line) {
        if (delimiterCount.find(c) != delimiterCount.end()) {
            delimiterCount[c]++;
        }
    }

    // 选择出现次数最多的分隔符
    char bestDelimiter = ' ';
    int maxCount = 0;
    for (const auto& pair : delimiterCount) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            bestDelimiter = pair.first;
        }
    }

    // 转换为字符串
    switch (bestDelimiter) {
        case '\t': return "\t";
        case ',': return ",";
        case ';': return ";";
        default: return " ";
    }
}