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
    core/StreamMergeEngine.cpp \
    cli/main.cpp

# 定义头文件
HEADERS += \
    TimePoint.h \
    TimeParser.h \
    utils/DelimiterDetector.h \
    io/FileReader.h \
    core/Interpolator.h \
    core/DataFileMerger.h \
    core/StreamMergeEngine.h

# 定义 UI 文件（如果有）
# FORMS += mainwindow.ui

# C++17 支持
CONFIG += c++17

# 输出目录
DESTDIR = $$PWD/../build-MultiMerge-Desktop_Qt_5_9_1_MSVC2015_64bit-Debug

