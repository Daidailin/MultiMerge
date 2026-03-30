# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\MultiMerge_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\MultiMerge_autogen.dir\\ParseCache.txt"
  "MultiMerge_autogen"
  )
endif()
