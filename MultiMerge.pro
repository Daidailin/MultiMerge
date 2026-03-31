QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += 
        src/main/cli/main.cpp
        src/core/time/TimePoint.cpp
        src/core/time/TimeParser.cpp
        src/core/interpolate/Interpolator.cpp
        src/core/merge/DataFileMerger.cpp
        src/core/merge/StreamMergeEngine.cpp
        src/io/FileReader.cpp
        src/utils/DelimiterDetector.cpp

HEADERS += 
        src/core/time/TimePoint.h
        src/core/time/TimeParser.h
        src/core/interpolate/Interpolator.h
        src/core/merge/DataFileMerger.h
        src/core/merge/StreamMergeEngine.h
        src/io/FileReader.h
        src/utils/DelimiterDetector.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
élse: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 包含头文件目录
INCLUDEPATH += 
    src/core/time
    src/core/interpolate
    src/core/merge
    src/io
    src/utils
