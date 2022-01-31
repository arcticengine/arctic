#pragma once

namespace piLibs {

#ifdef WINDOWS
 typedef __int64     	    sint64;
 typedef unsigned __int64   uint64;
 #ifdef A64BITS
 typedef unsigned __int64   pint;       // integer with the size of a pointer
 #else
 typedef unsigned int       pint;       // integer with the size of a pointer
 #endif
 #define IQAPICALL __stdcall
#endif


#ifdef MACOS
#endif

#ifdef LINUX
 #ifdef A64BITS
  typedef   signed long  sint64;
  typedef unsigned long  uint64;
  typedef unsigned long  pint;       // integer with the size of a pointer
 #else
  typedef unsigned long long int uint64;
  typedef long long int sint64;
  typedef unsigned int  pint;       // integer with the size of a pointer
 #endif
 #define IQAPICALL
#endif


typedef unsigned int    uint32;
typedef   signed int    sint32;
typedef unsigned short  uint16;
typedef   signed short  sint16;
typedef unsigned char   uint8;
typedef   signed char   sint8;

typedef uint16          half;


half  float2half(float x);
float half2float(half x);

} // namespace piLibs