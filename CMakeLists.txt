
#Current cmake version in the raspbian repositories
cmake_minimum_required(VERSION 3.0.2)

#project infos
project(CRpi VERSION 0.1 LANGUAGES CXX)

#Set the output folders
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories(include ../../)
link_directories(../../CCRCodingUtils/arm)


#all the files of the library, so demos are excluded
set(c_libFiles	src/lib/memoryManagement/addressing.cpp
			src/lib/memoryManagement/cacheCoherentMemoryProvider.cpp
			src/lib/memoryManagement/physMemoryManagement.cpp
			src/lib/notMine/mailbox.cpp
			src/lib/peripherals/clockManager.cpp
			src/lib/peripherals/dma.cpp
			src/lib/peripherals/gpio.cpp
			src/lib/peripherals/pwm.cpp
			src/lib/utils/pwmByDma.cpp
			)

#Set the flags for compiling
set (CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-arith -std=c++11 ")

#set the library to be created
add_library(CRpi STATIC ${c_libFiles})
#Set the executable to generate
add_executable(demo  src/demo/demo.cpp)
#link the library to the executabe
target_link_libraries(demo CRpi libCCRCodingUtils.a)


