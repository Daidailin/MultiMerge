#include "FileReader.h"
#include "../core/time/TimePoint.h"
#include <fstream>
#include <sstream>
#include <iostream>

FileReader::FileReader(const std::string& filename, const std::string& delimiter) 
    : filename(filename), delimiter(delimiter) {}

bool FileReader::readFile(std::vector<std::vector<std::string>>& data, std::vector<std::string>& headers) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::vector<std::string> row;
        std::string token;

        while (std::getline(ss, token, delimiter[0])) {
            // 移除前后空格
            size_t start = token.find_first_not_of(" ");
            size_t end = token.find_last_not_of(" ");
            if (start != std::string::npos && end != std::string::npos) {
                token = token.substr(start, end - start + 1);
            }
            row.push_back(token);
        }

        if (firstLine) {
            headers = row;
            firstLine = false;
        } else {
            data.push_back(row);
        }
    }

    file.close();
    return true;
}

bool FileReader::readFileWithTime(std::vector<long long>& times, std::vector<std::vector<std::string>>& data, std::vector<std::string>& headers) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::vector<std::string> row;
        std::string token;

        while (std::getline(ss, token, delimiter[0])) {
            // 移除前后空格
            size_t start = token.find_first_not_of(" ");
            size_t end = token.find_last_not_of(" ");
            if (start != std::string::npos && end != std::string::npos) {
                token = token.substr(start, end - start + 1);
            }
            row.push_back(token);
        }

        if (firstLine) {
            headers = row;
            firstLine = false;
        } else if (!row.empty()) {
            // 解析时间列
            TimePoint time;
            if (time.fromString(row[0])) {
                times.push_back(time.getTotalMilliseconds());
                // 移除时间列，只保留数据
                if (row.size() > 1) {
                    row.erase(row.begin());
                    data.push_back(row);
                } else {
                    data.push_back({});
                }
            }
        }
    }

    file.close();
    return true;
}

void FileReader::setDelimiter(const std::string& delimiter) {
    this->delimiter = delimiter;
}

std::string FileReader::getDelimiter() const {
    return delimiter;
}