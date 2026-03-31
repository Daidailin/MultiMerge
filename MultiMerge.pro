QT += core

TARGET = MultiMerge
TEMPLATE = app

# 定义源文件
SOURCES += \
    src/core/time/TimePoint.cpp \
    src/core/time/TimeParser.cpp \
    src/utils/DelimiterDetector.cpp \
    src/io/FileReader.cpp \
    src/core/interpolate/Interpolator.cpp \
    src/core/merge/DataFileMerger.cpp \
    src/core/merge/StreamMergeEngine.cpp \
    src/main/cli/main.cpp

# 定义头文件
HEADERS += \
    src/core/time/TimePoint.h \
    src/core/time/TimeParser.h \
    src/utils/DelimiterDetector.h \
    src/io/FileReader.h \
    src/core/interpolate/Interpolator.h \
    src/core/merge/DataFileMerger.h \
    src/core/merge/StreamMergeEngine.h

# C++17 支持
CONFIG += c++17

# 输出目录
DESTDIR = $$PWD/build

# 包含路径
INCLUDEPATH += \
    $$PWD \
    $$PWD/src