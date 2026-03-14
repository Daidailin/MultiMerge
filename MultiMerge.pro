QT += core

TARGET = MultiMerge
TEMPLATE = app

# 定义源文件
SOURCES += \
    TimePoint.cpp \
    TimeParser.cpp \
    utils/DelimiterDetector.cpp \
    io/FileReader.cpp \
    core/Interpolator.cpp \
    core/DataFileMerger.cpp \
    cli/main.cpp

# 定义头文件
HEADERS += \
    TimePoint.h \
    TimeParser.h \
    utils/DelimiterDetector.h \
    io/FileReader.h \
    core/Interpolator.h \
    core/DataFileMerger.h

# C++17 支持
CONFIG += c++17

QMAKE_CXXFLAGS += /utf-8
