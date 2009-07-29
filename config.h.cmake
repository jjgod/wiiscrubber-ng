#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/* Program name and version */
#define PROGRAM_NAME "${PROJECT_NAME}"
#define PROGRAM_VERSION "${WIISCRUBBER_VERSION_MAJOR}.${WIISCRUBBER_VERSION_MINOR}.${WIISCRUBBER_VERSION_PATCH}"

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#cmakedefine WORDS_BIGENDIAN ${CMAKE_WORDS_BIGENDIAN}

/* Define to 1 if stdbool.h conforms to C99. */
#cmakedefine HAVE_STDBOOL_H 1

/* Define to 1 if under FreeBSD and the CAM architecture is supported */
#cmakedefine HAVE_FREEBSD_CAM

/* Define if libcdio stuff is needed and/or found */
#cmakedefine ENABLE_LIBCDIO
#cmakedefine HAVE_LIBCDIO_HEADERS
#cmakedefine HAVE_LIBCDIO_LIBS

/* Debug support */
#cmakedefine DEBUG 1

#cmakedefine HAVE_STRNDUP

#cmakedefine HAVE_FSEEKO
#cmakedefine HAVE_FTELLO
#cmakedefine HAVE_FSEEK64
#cmakedefine HAVE_FTELL64

#cmakedefine HAVE_OFF_T
#ifdef HAVE_OFF_T
#cmakedefine OFF_T ${OFF_T}
#define SIZEOF_OFF_T OFF_T
#endif

#cmakedefine HAVE_FPOS_T
#ifdef HAVE_FPOS_T
#cmakedefine FPOS_T ${FPOS_T}
#define SIZEOF_FPOS_T FPOS_T
#endif


/* Large file support - Can't this be done better with CMake!? What about on Windows? */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGE_FILES
#define HAVE_LARGEFILE_SUPPORT

#if defined(_FILE_OFFSET_BITS) 
# if (_FILE_OFFSET_BITS<64)
#  undef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 64
# endif
#else
# define _FILE_OFFSET_BITS 64
#endif

#endif	// CONFIG_H_INCLUDED
