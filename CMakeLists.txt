cmake_minimum_required(VERSION 3.12)

set(TARGET_NAME demo)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(${TARGET_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
	message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

include_directories("./examples")
include_directories("./examples/bitmap")

get_filename_component(PARENT_DIR ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)
# include_directories(${PARENT_DIR})
# add_subdirectory("../")

add_compile_options(-Wall
		-Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
		-Wno-unused-function # we have some for the docs that aren't called
		-Wno-maybe-uninitialized
		)
		
set(CMAKE_SYSTEM_NAME Generic)		

add_executable(${TARGET_NAME}
	examples/demo.cpp
	pico-oled.cpp
	)


set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# pull in common dependencies
target_link_libraries(${TARGET_NAME} 
	pico_stdlib 
	hardware_i2c 
	)


# enable usb output, disable uart output
#pico_enable_stdio_usb(${TARGET_NAME} 1)
#pico_enable_stdio_uart(${TARGET_NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${TARGET_NAME})

