cmake_minimum_required(VERSION 3.10)

project(MultiMerge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt5
find_package(Qt5 COMPONENTS Core REQUIRED)

# 包含源文件
set(SOURCES
    src/main/cli/main.cpp
    src/core/time/TimePoint.cpp
    src/core/time/TimeParser.cpp
    src/core/interpolate/Interpolator.cpp
    src/core/merge/DataFileMerger.cpp
    src/core/merge/StreamMergeEngine.cpp
    src/io/FileReader.cpp
    src/utils/DelimiterDetector.cpp
)

# 包含头文件目录
include_directories(
    src/core/time
    src/core/interpolate
    src/core/merge
    src/io
    src/utils
)

# 创建可执行文件
add_executable(MultiMerge ${SOURCES})

# 链接Qt5库
target_link_libraries(MultiMerge Qt5::Core)

# 设置输出目录
set_target_properties(MultiMerge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# 安装规则
install(TARGETS MultiMerge
    RUNTIME DESTINATION bin
)
