/*************************************************************************
*
*  config.h - Compile-time configuration settings.
*
*************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

/*
 * Version info.
 */
#ifndef	CONFIG_VERSION_MAJOR 
#define CONFIG_VERSION_MAJOR	0
#endif
#ifndef	CONFIG_VERSION_MINOR
#define CONFIG_VERSION_MINOR	1
#endif

/*
 * Define compiler if not defined already.
 */
#if !defined(CONFIG_COMPILER_GCC)
#if !defined(CONFIG_COMPILER_BCC)
#if !defined(CONFIG_COMPILER_TCC)
#if !defined(CONFIG_COMPILER_MSC)

#if defined(__BORLANDC__)
#define CONFIG_COMPILER_BCC	__BORLANDC__
#elif defined(__TURBOC__)
#define CONFIG_COMPILER_TCC	__TURBOC__
#elif defined(_MSC_VER)
#define CONFIG_COMPILER_MSC	_MSC_VER
#else
#define CONFIG_COMPILER_CC
#endif

#endif
#endif
#endif
#endif

/*
 * Define platform if not defined already.
 */
#if !defined(CONFIG_PLATFORM_LINUX)
#if !defined(CONFIG_PLATFORM_UNIX)
#if !defined(CONFIG_PLATFORM_XLIB)
#if !defined(CONFIG_PLATFORM_MACOS)
#if !defined(CONFIG_PLATFORM_MSDOS)
#if !defined(CONFIG_PLATFORM_WIN32)
#if !defined(CONFIG_PLATFORM_WIN16)

#if defined(_WIN32) || defined(__WIN32__)
#define CONFIG_PLATFORM_WIN32
#elif defined(_Windows)
#define CONFIG_PLATFORM_WIN16
#define CONFIG_PLATFORM_MSDOS
#elif defined(__MSDOS__)
#define CONFIG_PLATFORM_MSDOS
#else
#define CONFIG_PLATFORM_UNIX
#endif

#endif
#endif
#endif
#endif
#endif
#endif
#endif

/*
 * Define access mode strings for fopen().
 */
#if !defined(CONFIG_PLATFORM_UNIX)

#if defined(CONFIG_COMPILER_BCC) || defined(CONFIG_COMPILER_TCC)

#define READ			"r"
#define WRITE			"w"
#define APPEND			"a"
#define READRW			"r+"
#define WRITERW			"w+"
#define APPENDRW		"a+"
#define READBIN			"rb"
#define WRITEBIN		"wb"
#define APPENDBIN		"ab"
#define READRWBIN		"r+b"
#define WRITERWBIN		"w+b"
#define APPENDRWBIN		"a+b"

#elif defined(CONFIG_COMPILER_MSC)

#define READ			"rt"
#define WRITE			"wt"
#define APPEND			"at"
#define READRW			"r+t"
#define WRITERW			"w+t"
#define APPENDRW		"a+t"
#define READBIN			"rb"
#define WRITEBIN		"wb"
#define APPENDBIN		"ab"
#define READRWBIN		"r+b"
#define WRITERWBIN		"w+b"
#define APPENDRWBIN		"a+b"

#else

#define READ			"r"
#define WRITE			"w"
#define APPEND			"a"
#define READRW			"r+"
#define WRITERW			"w+"
#define APPENDRW		"a+"
#define READBIN			"rb"
#define WRITEBIN		"wb"
#define APPENDBIN		"ab"
#define READRWBIN		"r+b"
#define WRITERWBIN		"w+b"
#define APPENDRWBIN		"a+b"

#endif

#else  /* UNIX */

#define READ			"r"
#define WRITE			"w"
#define APPEND			"a"
#define READRW			"r+"
#define WRITERW			"w+"
#define APPENDRW		"a+"
#define READBIN			"r"
#define WRITEBIN		"w"
#define APPENDBIN		"a"
#define READRWBIN		"r+"
#define WRITERWBIN		"w+"
#define APPENDRWBIN		"a+"

#endif

#if defined(CONFIG_PLATFORM_XLIB)
#define CONFIG_PLATFORM_NAME	"XLIB"
#elif defined(CONFIG_PLATFORM_UNIX)
#define CONFIG_PLATFORM_NAME	"UNIX"
#elif defined(CONFIG_PLATFORM_LINUX)
#define CONFIG_PLATFORM_NAME	"LINUX"
#elif defined(CONFIG_PLATFORM_WIN32)
#define CONFIG_PLATFORM_NAME	"WIN32"
#elif defined(CONFIG_PLATFORM_WIN16)
#define CONFIG_PLATFORM_NAME	"WIN16"
#elif defined(CONFIG_PLATFORM_MSDOS)
#define CONFIG_PLATFORM_NAME	"MSDOS"
#elif defined(CONFIG_PLATFORM_MACOS)
#define CONFIG_PLATFORM_NAME	"MACOS"
#else
#error A platform CONFIG_PLATFORM_XXX did not get defined
#endif

#if defined(CONFIG_COMPILER_GCC)
#define CONFIG_COMPILER_NAME	"GCC"
#elif defined(CONFIG_COMPILER_BCC)
#define CONFIG_COMPILER_NAME	"BCC"
#elif defined(CONFIG_COMPILER_TCC)
#define CONFIG_COMPILER_NAME	"TCC"
#elif	defined(CONFIG_COMPILER_MSC)
#define CONFIG_COMPILER_NAME	"MSC"
#elif	defined(CONFIG_COMPILER_CC)
#define CONFIG_COMPILER_NAME	"CC"
#else
#error A compiler CONFIG_COMPILER_XXX did not get defined
#endif

#ifdef NDEBUG
#define CONFIG_BUILDINFO	CONFIG_PLATFORM_NAME ", " CONFIG_COMPILER_NAME ", " __DATE__ ", " __TIME__
#else
#define CONFIG_BUILDINFO	CONFIG_PLATFORM_NAME ", " CONFIG_COMPILER_NAME ", " __DATE__ ", " __TIME__ ", Debug"
#endif

#endif  /* CONFIG_H */
