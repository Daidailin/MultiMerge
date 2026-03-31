#ifndef DATAFILEMERGER_H
#define DATAFILEMERGER_H

#include "../time/TimePoint.h"
#include "../interpolate/Interpolator.h"
#include <vector>
#include <string>

class DataFileMerger {
private:
    struct FileData {
        std::vector<TimePoint> times;
        std::vector<std::vector<std::string>> values;
        std::vector<std::string> headers;
    };

    std::vector<FileData> fileDataList;
    Interpolator::Method interpolationMethod;
    long long timeTolerance;

public:
    DataFileMerger(Interpolator::Method method, long long tolerance = 0);
    
    bool addFile(const std::string& filename);
    bool merge(const std::string& outputFilename, const std::string& delimiter = " ");
    
    // 静态方法：检测文件分隔符
    static std::string detectDelimiter(const std::string& filename);
};

#endif // DATAFILEMERGER_H