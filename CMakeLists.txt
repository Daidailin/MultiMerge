cmake_minimum_required(VERSION 3.16)

project(MultiMerge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_compile_options(-fno-aligned-new)

find_package(Qt5 COMPONENTS Core REQUIRED)

set(SOURCES
    TimePoint.cpp
    TimeParser.cpp
    utils/DelimiterDetector.cpp
    io/FileReader.cpp
    core/Interpolator.cpp
    core/DataFileMerger.cpp
    core/StreamMergeEngine.cpp
    cli/main.cpp
)

set(HEADERS
    TimePoint.h
    TimeParser.h
    utils/DelimiterDetector.h
    io/FileReader.h
    core/Interpolator.h
    core/DataFileMerger.h
    core/StreamMergeEngine.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
)
