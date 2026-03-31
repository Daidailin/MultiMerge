cmake_minimum_required(VERSION 3.10)

project(MultiMerge VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core REQUIRED)

set(SOURCES
    src/core/time/TimePoint.cpp
    src/core/time/TimeParser.cpp
    src/utils/DelimiterDetector.cpp
    src/io/FileReader.cpp
    src/core/interpolate/Interpolator.cpp
    src/core/merge/DataFileMerger.cpp
    src/core/merge/StreamMergeEngine.cpp
    src/main/cli/main.cpp
)

set(HEADERS
    src/core/time/TimePoint.h
    src/core/time/TimeParser.h
    src/utils/DelimiterDetector.h
    src/io/FileReader.h
    src/core/interpolate/Interpolator.h
    src/core/merge/DataFileMerger.h
    src/core/merge/StreamMergeEngine.h
)

add_executable(MultiMerge ${SOURCES} ${HEADERS})

target_link_libraries(MultiMerge Qt5::Core)

target_include_directories(MultiMerge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 安装目标
install(TARGETS MultiMerge
    DESTINATION bin
)

# 安装头文件
install(FILES ${HEADERS}
    DESTINATION include/MultiMerge
)