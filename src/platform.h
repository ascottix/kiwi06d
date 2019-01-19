/*
    Kiwi
    Platform dependent definitions

    Copyright (c) 1999-2004,2005 Alessandro Scotti
    http://www.ascotti.org/

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef PLATFORM_H_
#define PLATFORM_H_

#if defined(_MSC_VER)
#define CDECL __cdecl
#define CACHE_ALIGN __declspec(align(64))
#else
#define CDECL
#define CACHE_ALIGN
#endif

#if defined(LINUX_I386) || defined(WIN_I386) || defined(MAC_G4)
// 32-bit platform
#if defined(__GNUC__)

typedef unsigned long long  Uint64;
typedef unsigned            Uint32;

#ifndef PRIx64
#define PRIx64  "llx"
#endif

#define MK_U64( n ) n##ull

#else // Visual C++

typedef unsigned __int64    Uint64;
typedef unsigned            Uint32;

#ifndef PRIx64
#define PRIx64  "I64x"
#endif

#define MK_U64( n ) n

#endif
#endif // 32 or 64 bit platform

#endif // PLATFORM_H_
