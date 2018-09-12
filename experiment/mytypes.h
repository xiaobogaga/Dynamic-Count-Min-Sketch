#ifndef TYPES_H
#define TYPES_H

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

/*
* We have to include stdlib.h here because it defines many of these macros
* on some platforms, and we only want our definitions used if stdlib.h doesn't
* have its own.  The same goes for stddef and stdarg if present.
*/

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdint.h>
#include <sys/types.h>

#include <errno.h>

/* Must be before gettext() games below */
#include <locale.h>

#define _(x) gettext(x)

#ifdef ENABLE_NLS
#include <libintl.h>
#else
#define gettext(x) (x)
#define dgettext(d,x) (x)
#define ngettext(s,p,n) ((n) == 1 ? (s) : (p))
#define dngettext(d,s,p,n) ((n) == 1 ? (s) : (p))
#endif


/* ----------------------------------------------------------------
*				Section 1: hacks to cope with non-ANSI C compilers
*
* type prefixes (const, signed, volatile, inline) are handled in pg_config.h.
* ----------------------------------------------------------------
*/

/*
* CppAsString
*		Convert the argument to a string, using the C preprocessor.
* CppConcat
*		Concatenate two arguments together, using the C preprocessor.
*
* Note: There used to be support here for pre-ANSI C compilers that didn't
* support # and ##.  Nowadays, these macros are just for clarity and/or
* backward compatibility with existing PostgreSQL code.
*/
#define CppAsString(identifier) #identifier
#define CppConcat(x, y)			x##y

/*
* dummyret is used to set return values in macros that use ?: to make
* assignments.  gcc wants these to be void, other compilers like char
*/
#ifdef __GNUC__					/* GNU cc */
#define dummyret	void
#else
#define dummyret	char
#endif

/* ----------------------------------------------------------------
*				Section 2:	bool, true, false, TRUE, FALSE, NULL
* ----------------------------------------------------------------
*/

/*
* bool
*		Boolean value, either true or false.
*
* XXX for C++ compilers, we assume the compiler has a compatible
* built-in definition of bool.
*/

#ifndef __cplusplus

#ifndef bool
typedef char bool;
#endif

#ifndef true
#define true	((bool) 1)
#endif

#ifndef false
#define false	((bool) 0)
#endif
#endif   /* not C++ */

typedef bool *BoolPtr;

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

/*
* NULL
*		Null pointer.
*/
#ifndef NULL
#define NULL	((void *) 0)
#endif


/* ----------------------------------------------------------------
*				Section 3:	standard system types
* ----------------------------------------------------------------
*/

/*
* Pointer
*		Variable holding address of any memory resident object.
*
*		XXX Pointer arithmetic is done with this, so it can't be void *
*		under "true" ANSI compilers.
*/
typedef char *Pointer;

/*
* intN
*		Signed integer, EXACTLY N BITS IN SIZE,
*		used for numerical computations and the
*		frontend/backend protocol.
*/
#ifndef HAVE_INT8
typedef signed char int8;		/* == 8 bits */
typedef signed short int16;		/* == 16 bits */
typedef signed int int32;		/* == 32 bits */
#endif   /* not HAVE_INT8 */

								/*
								* uintN
								*		Unsigned integer, EXACTLY N BITS IN SIZE,
								*		used for numerical computations and the
								*		frontend/backend protocol.
								*/
#ifndef HAVE_UINT8
typedef unsigned char uint8;	/* == 8 bits */
typedef unsigned short uint16;	/* == 16 bits */
typedef unsigned int uint32;	/* == 32 bits */
#endif   /* not HAVE_UINT8 */

								/*
								* bitsN
								*		Unit of bitwise operation, AT LEAST N BITS IN SIZE.
								*/
typedef uint8 bits8;			/* >= 8 bits */
typedef uint16 bits16;			/* >= 16 bits */
typedef uint32 bits32;			/* >= 32 bits */

#define HAVE_LONG_INT_64 1

								/*
								* 64-bit integers
								*/
#ifdef HAVE_LONG_INT_64
#ifndef HAVE_INT64
typedef long int int64;
#endif
#ifndef HAVE_UINT64
typedef unsigned long int uint64;
#endif
#elif defined(HAVE_LONG_LONG_INT_64)
#ifndef HAVE_INT64
typedef long long int int64;
#endif
#ifndef HAVE_UINT64
typedef unsigned long long int uint64;
#endif
#else
#error must have a working 64-bit integer datatype
#endif

								/* Decide if we need to decorate 64-bit constants */
#ifdef HAVE_LL_CONSTANTS
#define INT64CONST(x)  ((int64) x##LL)
#define UINT64CONST(x) ((uint64) x##ULL)
#else
#define INT64CONST(x)  ((int64) x)
#define UINT64CONST(x) ((uint64) x)
#endif

								/* snprintf format strings to use for 64-bit integers */
#define INT64_FORMAT "%" INT64_MODIFIER "d"
#define UINT64_FORMAT "%" INT64_MODIFIER "u"

								/* sig_atomic_t is required by ANSI C, but may be missing on old platforms */
#ifndef HAVE_SIG_ATOMIC_T
typedef int sig_atomic_t;
#endif

/*
* Size
*		Size of any memory resident object, as returned by sizeof.
*/
typedef size_t Size;

/*
* Index
*		Index into any memory resident array.
*
* Note:
*		Indices are non negative.
*/
typedef unsigned int Index;

/*
* Offset
*		Offset into any memory resident array.
*
* Note:
*		This differs from an Index in that an Index is always
*		non negative, whereas Offset may be negative.
*/
typedef signed int Offset;

/*
* Common Postgres datatype names (as used in the catalogs)
*/
typedef float float4;
typedef double float8;
typedef unsigned long long ulong;

#endif
#pragma once
