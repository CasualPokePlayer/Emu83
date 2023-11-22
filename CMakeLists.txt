cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(POLICY CMP0092)
	cmake_policy(SET CMP0092 NEW)
endif()

project(emu83 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET internal)

if(MSVC)
	# silence dumb warnings over CRT functions being "unsafe"
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

	# max warnings, treat as errors
	add_compile_options(/W4)
	add_compile_options(/WX)

	# ignore some warnings
	# this is not needed for clang-cl
	if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
		add_compile_options(
			/wd4146 # unary minus operator applied to unsigned type, result still unsigned
			/wd4244 # 'conversion' conversion from 'type1' to 'type2', possible loss of data
		)
	endif()

	# all files are utf8
	add_compile_options(/utf-8)

	# max conformance mode
	add_compile_options(/permissive-)
	add_compile_options(/volatile:iso)
	add_compile_options(/fp:precise)

	# cmake will not insert a lib prefix for libraries on MSVC targets
	set(EMU83_TARGET libemu83)
else()
	# max warnings, treat as errors
	add_compile_options(-Wall -Wextra -Wpedantic)
	add_compile_options(-Werror)

	# use internal visibility by default (exports will explicitly mark themselves for default visibility)
	add_compile_options(-Wall -Wextra)

	# strip in release, optimize for gdb usage in debug
	add_link_options($<$<CONFIG:RELEASE>:-s>)
	add_compile_options($<$<CONFIG:DEBUG>:-ggdb>)

	# use lld for clang (needed if doing lto)
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
		add_link_options(-fuse-ld=lld)
	endif()

	# cmake will insert a lib prefix for libraries on non-MSVC targets 
	set(EMU83_TARGET emu83)
endif()

add_library(
	${EMU83_TARGET}
	SHARED
	crc32.c
	crc32.h
	events.c
	events.h
	link.c
	link.h
	memory.c
	memory.h
	queue.c
	queue.h
	savestate.c
	savestate.h
	stream.c
	stream.h
	ti83.c
	ti83.h
	z80.c
	z80.h
)

option(BUILD_FOR_BIZHAWK "Copy output to BizHawk folders" OFF)

if(BUILD_FOR_BIZHAWK)
	add_custom_command(
		TARGET ${EMU83_TARGET}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy $<TARGET_FILE:${EMU83_TARGET}> ${CMAKE_SOURCE_DIR}/../../Assets/dll
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy $<TARGET_FILE:${EMU83_TARGET}> ${CMAKE_SOURCE_DIR}/../../output/dll
	)
endif()
