cmake_minimum_required (VERSION 3.13)
project (sandbox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")

include(${CMAKE_ROOT}/Modules/FetchContent.cmake)

set(DEPENDENCY_DIR "${PROJECT_SOURCE_DIR}/dependencies")

FetchContent_Populate(spdlog SOURCE_DIR ${DEPENDENCY_DIR}/spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG 74dbf4cf702b49c98642c9afe74d114a238a6a07
)

add_subdirectory (launcher)
add_subdirectory (internal)
