cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
	execute_process(
			COMMAND bash -c "echo /home/nyan/PlaydateSDK"
			OUTPUT_VARIABLE SDK
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game Name Customization
set(PLAYDATE_GAME_NAME deflect)
set(PLAYDATE_GAME_DEVICE deflect_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)

set(PROJECT_SRC
		src/main.c
		src/game.c
		src/game.h
)

if (TOOLCHAIN STREQUAL armgcc)
	set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
	add_executable(${PLAYDATE_GAME_DEVICE} ${PROJECT_SRC})
	add_link_options(-specs=nano.specs -specs=nosys.specs)
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED ${PROJECT_SRC})
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)