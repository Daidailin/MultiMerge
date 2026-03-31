#ifndef DELIMITERDETECTOR_H
#define DELIMITERDETECTOR_H

#include <string>

class DelimiterDetector {
public:
    static std::string detect(const std::string& filename);
    static std::string detectFromContent(const std::string& content);
};

#endif // DELIMITERDETECTOR_H