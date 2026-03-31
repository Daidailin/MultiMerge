#include "FileReader.h"
#include <QFile>
#include <QTextStream>

FileMetadata FileReader::readFile(const QString& filePath, QChar delimiter) {
    FileMetadata metadata;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return metadata;
    }

    QTextStream in(&file);
    
    // 读取表头
    QString headerLine = in.readLine();
    if (!headerLine.isEmpty()) {
        metadata.headers = headerLine.split(delimiter);
    }
    
    // 读取数据
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.isEmpty()) {
            QStringList fields = line.split(delimiter);
            metadata.data.append(fields);
        }
    }
    
    file.close();
    return metadata;
}

QStringList FileReader::readHeaders(const QString& filePath, QChar delimiter) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringList();
    }

    QTextStream in(&file);
    QString headerLine = in.readLine();
    file.close();
    
    if (!headerLine.isEmpty()) {
        return headerLine.split(delimiter);
    }
    return QStringList();
}

QVector<QStringList> FileReader::readData(const QString& filePath, QChar delimiter) {
    QVector<QStringList> data;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return data;
    }

    QTextStream in(&file);
    
    // 跳过表头
    in.readLine();
    
    // 读取数据
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.isEmpty()) {
            QStringList fields = line.split(delimiter);
            data.append(fields);
        }
    }
    
    file.close();
    return data;
}