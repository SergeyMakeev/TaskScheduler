#pragma once
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef signed char int8;
typedef unsigned char uint8;
typedef unsigned char byte;
typedef short int16;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;
typedef unsigned int uint;


#if defined(_MSC_VER)

typedef __int64 int64;
typedef unsigned __int64 uint64;

#else

typedef long long int64;
typedef unsigned long long uint64;

#endif
