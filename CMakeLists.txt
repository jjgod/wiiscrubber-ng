cmake_minimum_required (VERSION 2.4)

# The name of our project is "WiiScrubber".  CMakeLists files in this project can
# refer to the root source directory of the project as ${WiiScrubber_SOURCE_DIR} and
# to the root binary directory of the project as ${WiiScrubber_BINARY_DIR}.
project (WiiScrubber)

set (WIISCRUBBER_VERSION_MAJOR "131")
set (WIISCRUBBER_VERSION_MINOR "0")
set (WIISCRUBBER_VERSION_PATCH "0")

include (CheckFunctionExists)
include (CheckTypeSize)
include (CheckLibraryExists)
include (TestBigEndian)
include (CheckIncludeFiles)

if (MSVC)
	# Avoid tons of harmless MSVC deprecated warnings
	add_definitions (-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
	# Exclude rarely-used stuff from Windows headers
	add_definitions (VC_EXTRALEAN)
elseif (APPLE)
	add_definitions (-DAPPLE)
	# MacOS X?
	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		add_definitions (-DMACOSX)
	endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
elseif (UNIX)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		add_definitions (-DLINUX)
		set (LINUX 1)
	elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		add_definitions (-DFREEBSD)
		set (FREEBSD 1)
	endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_definitions (-fPIC)
endif (MSVC)

test_big_endian (CMAKE_WORDS_BIGENDIAN)

check_include_files (stdbool.h HAVE_STDBOOL_H)

#check_function_exists (strndup HAVE_STRNDUP)

check_function_exists (fseeko HAVE_FSEEKO)
check_function_exists (ftello HAVE_FTELLO)
check_function_exists (fseek64 HAVE_FSEEK64)
check_function_exists (ftell64 HAVE_FTELL64)

# We need OpenSSL
include (FindOpenSSL)
if (!OPENSSL_FOUND)
	message (SEND_ERROR "This program requires OpenSSL, please install it")
endif (!OPENSSL_FOUND)

set (CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64)

set (CMAKE_EXTRA_INCLUDE_FILES sys/types.h)
check_type_size ("off_t" OFF_T)
set (CMAKE_EXTRA_INCLUDE_FILES)

set (CMAKE_EXTRA_INCLUDE_FILES stdio.h)
check_type_size ("fpos_t" FPOS_T)
set (CMAKE_EXTRA_INCLUDE_FILES)

set (CMAKE_REQUIRED_DEFINITIONS)


option (
	DEBUG
	"Enable debugging messages"
	OFF
)


set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")

# set (CMAKE_BUILD_TYPE release)

# config.h stuff
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions (-DHAVE_CONFIG_H)
include_directories (
	${WiiScrubber_BINARY_DIR}
)


# Recurse into subdirectories
add_subdirectory (src)

# Packaging instructions
include (CPackLists.txt)
