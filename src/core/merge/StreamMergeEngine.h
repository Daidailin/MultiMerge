#ifndef STREAMMERGEENGINE_H
#define STREAMMERGEENGINE_H

#include "../time/TimePoint.h"
#include "../interpolate/Interpolator.h"
#include <vector>
#include <string>
#include <fstream>

class StreamMergeEngine {
private:
    struct FileStream {
        std::ifstream file;
        std::string currentLine;
        TimePoint currentTime;
        std::vector<std::string> currentValues;
        bool hasData;

        FileStream(const std::string& filename);
        bool readNextLine();
    };

    std::vector<FileStream> streams;
    std::vector<std::string> headers;
    Interpolator::Method interpolationMethod;
    long long timeTolerance;

public:
    StreamMergeEngine(const std::vector<std::string>& filenames, Interpolator::Method method, long long tolerance = 0);
    ~StreamMergeEngine();

    bool initialize();
    bool merge(const std::string& outputFilename, const std::string& delimiter = " ");
};

#endif // STREAMMERGEENGINE_H