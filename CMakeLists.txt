cmake_minimum_required(VERSION 3.21)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()

if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include(VcpkgIntegration)
project(metamod-gui)
include(CompilerRuntime)

list(APPEND SOURCES_LIST
	"src/h_export.cpp"
	"src/meta_api.cpp"
	"src/dllapi.cpp"
	"src/engine_api.cpp"
	"src/fltk_gui.cpp"
	"src/player_table.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SOURCES_LIST})

find_path(HLSDK_DIRECTORY "cl_dll/GameStudioModelRenderer.h" PATH_SUFFIXES "hlsdk")
find_path(METAMOD_DIRECTORY "common/BaseSystemModule.h" PATH_SUFFIXES "metamod")

target_include_directories(${PROJECT_NAME} PRIVATE
	"${HLSDK_DIRECTORY}/common"
	"${HLSDK_DIRECTORY}/dlls"
	"${HLSDK_DIRECTORY}/engine"
	"${HLSDK_DIRECTORY}/game_shared"
	"${HLSDK_DIRECTORY}/pm_shared"
	"${HLSDK_DIRECTORY}/public"
	"${METAMOD_DIRECTORY}"
	"src"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	OUTPUT_NAME "metamod-gui"
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED YES
	CXX_EXTENSIONS NO
)

# compiler options
if(WIN32)
	target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:CONSOLE)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		_CRT_SECURE_NO_WARNINGS=1
	)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wfatal-errors)
	target_compile_options(${PROJECT_NAME} PRIVATE -fpermissive)

	# Architecture-specific flags and FLTK path
	if(VCPKG_TARGET_TRIPLET MATCHES "^x64")
		# x64 build
		set(LIB_ARCH "x86_64-linux-gnu")
		set(FLTK_PREFIX "/opt/fltk-x64")
	else()
		# x86 build
		set(LIB_ARCH "i386-linux-gnu")
		set(FLTK_PREFIX "/opt/fltk-x86")
		target_compile_options(${PROJECT_NAME} PRIVATE -m32)
		target_link_options(${PROJECT_NAME} PRIVATE -m32)
	endif()

	# Find FLTK 1.4 static libraries from custom build
	unset(FLTK_LIBRARY CACHE)
	unset(FLTK_IMAGES_LIBRARY CACHE)
	find_library(FLTK_LIBRARY
		NAMES libfltk.a fltk
		PATHS "${FLTK_PREFIX}/lib"
		NO_DEFAULT_PATH
		REQUIRED
	)
	find_library(FLTK_IMAGES_LIBRARY
		NAMES libfltk_images.a fltk_images
		PATHS "${FLTK_PREFIX}/lib"
		NO_DEFAULT_PATH
		REQUIRED
	)
	set(FLTK_INCLUDE_DIR "${FLTK_PREFIX}/include")

	# Find Cairo (required by FLTK when built with Cairo support)
	find_path(CAIRO_INCLUDE_DIR cairo.h PATH_SUFFIXES cairo)

	# X11 and related libraries
	# Use static libXft to avoid runtime dependency issues
	find_library(XFT_LIBRARY NAMES libXft.a PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	# These remain dynamic as they're standard X11 libs available on any Linux desktop
	find_library(X11_LIBRARY NAMES X11 PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(XRENDER_LIBRARY NAMES Xrender PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(FONTCONFIG_LIBRARY NAMES fontconfig PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(FREETYPE_LIBRARY NAMES freetype PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(XEXT_LIBRARY NAMES Xext PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(XINERAMA_LIBRARY NAMES Xinerama PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(XCURSOR_LIBRARY NAMES Xcursor PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)
	find_library(XFIXES_LIBRARY NAMES Xfixes PATHS "/usr/lib/${LIB_ARCH}" NO_DEFAULT_PATH)

	if(FLTK_LIBRARY)
		target_include_directories(${PROJECT_NAME} PRIVATE ${FLTK_INCLUDE_DIR} ${CAIRO_INCLUDE_DIR})
		# Static FLTK + static Xft + dynamic X11 backend libs
		target_link_libraries(${PROJECT_NAME} PRIVATE
			${FLTK_LIBRARY}
			${FLTK_IMAGES_LIBRARY}
			${XFT_LIBRARY}
			${FONTCONFIG_LIBRARY}
			${FREETYPE_LIBRARY}
			${X11_LIBRARY}
			${XRENDER_LIBRARY}
			${XEXT_LIBRARY}
			${XINERAMA_LIBRARY}
			${XCURSOR_LIBRARY}
			${XFIXES_LIBRARY}
		)
	else()
		message(FATAL_ERROR "FLTK library not found at ${FLTK_PREFIX}")
	endif()

	# Use static C++ runtime to avoid conflicts with old libs in HLDS directories
	target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static-libgcc)
endif()

# link platform-specific libraries
if(NOT WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE dl pthread)
endif()

# set build output directory
set(DIR_COMMON_OUTPUT
	${CMAKE_BINARY_DIR}/$<CONFIG>/bin
)

set_target_properties(${PROJECT_NAME} PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY ${DIR_COMMON_OUTPUT}
	LIBRARY_OUTPUT_DIRECTORY ${DIR_COMMON_OUTPUT}
	RUNTIME_OUTPUT_DIRECTORY ${DIR_COMMON_OUTPUT}
)

install(TARGETS ${PROJECT_NAME}
	DESTINATION "${CMAKE_INSTALL_PREFIX}"
	PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
	    WORLD_READ WORLD_EXECUTE
)
