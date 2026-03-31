#include "DelimiterDetector.h"
#include <fstream>
#include <sstream>
#include <map>

std::string DelimiterDetector::detect(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return " "; // 默认空格
    }

    std::string line;
    if (!std::getline(file, line)) {
        return " ";
    }

    file.close();
    return detectFromContent(line);
}

std::string DelimiterDetector::detectFromContent(const std::string& content) {
    // 统计可能的分隔符
    std::map<char, int> delimiterCount;
    delimiterCount[','] = 0;
    delimiterCount[';'] = 0;
    delimiterCount['\t'] = 0;
    delimiterCount[' '] = 0;

    for (char c : content) {
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