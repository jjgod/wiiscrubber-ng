# Make sure the compiler can find include files from our Hello library.
include_directories (
	${OPENSSL_INCLUDE_DIR}
)

add_executable (
	wiiscrubber

	main.cpp
#	AddPartition.cpp
#	AddPartition.h
#	BootMode.cpp
#	BootMode.h
#	ResizePartition.cpp
#	ResizePartition.h
	structs.h
	WIIDisc.cpp
	WIIDisc.h
	misc.c
	misc.h
	util.c
	util.h
)

find_library(CRYPTO_LIBRARY NAMES crypto)

# Link the executable to the Hello library.
target_link_libraries (
        wiiscrubber

	${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARY}
)

if (WIN32)
	# We only want a version number on Windows (Is it any use?)
	set_target_properties (friidump PROPERTIES VERSION 0.2)
endif (WIN32)

install (
	TARGETS wiiscrubber
	RUNTIME DESTINATION bin
)
