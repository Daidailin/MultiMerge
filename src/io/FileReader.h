#ifndef FILEREADER_H
#define FILEREADER_H

#include <vector>
#include <string>

class FileReader {
private:
    std::string filename;
    std::string delimiter;

public:
    FileReader(const std::string& filename, const std::string& delimiter = " ");
    
    bool readFile(std::vector<std::vector<std::string>>& data, std::vector<std::string>& headers);
    bool readFileWithTime(std::vector<long long>& times, std::vector<std::vector<std::string>>& data, std::vector<std::string>& headers);
    
    void setDelimiter(const std::string& delimiter);
    std::string getDelimiter() const;
};

#endif // FILEREADER_H