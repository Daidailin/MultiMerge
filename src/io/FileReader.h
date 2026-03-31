#ifndef FILEREADER_H
#define FILEREADER_H

#include <QString>
#include <QVector>
#include <QMap>

struct FileMetadata {
    QStringList headers;
    QVector<QStringList> data;
};

class FileReader {
public:
    // 读取文件并返回文件元数据
    static FileMetadata readFile(const QString& filePath, QChar delimiter);
    
    // 读取文件的表头
    static QStringList readHeaders(const QString& filePath, QChar delimiter);
    
    // 读取文件数据
    static QVector<QStringList> readData(const QString& filePath, QChar delimiter);
};

#endif // FILEREADER_H