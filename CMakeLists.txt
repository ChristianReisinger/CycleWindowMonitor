cmake_minimum_required(VERSION 3.14)
project(CycleWindowMonitor)

add_compile_options(-Wall -Wextra -pedantic -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
set(CMAKE_CXX_STANDARD 20)

set(BUILD_SFX "${PLATFORM}/${CMAKE_BUILD_TYPE}")

include_directories(
	include
#	"${Dependency_ROOT}/include"
)

#find_library(Dependency_LIB lib_name "${Dependency_ROOT}/lib/${BUILD_SFX}")

add_executable(CycleWindowMonitor
	src/main.cc
)

target_precompile_headers(CycleWindowMonitor PRIVATE
	<iostream>
	<stdexcept>
	<vector>
	<windows.h>
)	

#target_link_libraries(CycleWindowMonitor
#	"${Dependency_LIB}"
#)

install(TARGETS
	CycleWindowMonitor
DESTINATION "${PROJECT_SOURCE_DIR}/bin/${BUILD_SFX}")
