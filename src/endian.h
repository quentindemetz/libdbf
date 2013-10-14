/****************************************************************************
 * endian.h
 ****************************************************************************
 * Routines for Little Endian and Big Endian Systems
 * Library version
 *
 * Version 0.4, 2003-09-08
 * Author: Björn Berg, clergyman@gmx.de
 *
 ****************************************************************************
 * $Id: endian.h,v 1.2 2004/09/09 10:32:22 steinm Exp $
 ***************************************************************************/

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

/*
 * I N C L U D E S
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#ifdef __unix__   
   #include <sys/types.h>
   #ifndef __ANUBISNET_TYPES__
   #define __ANUBISNET_TYPES__
     typedef u_int16_t uint16_t;
     typedef u_int32_t uint32_t;
   #endif
/*
 * Windows does not know UINT16 types, therefore we have to make an improvement
 * for 32 Bit systems. unsigned short is only verified to work properly on 32 Bit
 * systems.
 */
#elif _WIN32
	#include <windows.h>
    #ifndef __ANUBISNET_TYPES__
    #define __ANUBISNET_TYPES__
      typedef UINT32 u_int32_t; 
      typedef unsigned short u_int16_t;
    #endif
#else
   #include <sys/types.h>
#endif

/* 
 * F U N C T I O N S 
 */
u_int16_t rotate2b ( u_int16_t var );
u_int32_t rotate4b ( u_int32_t var );

#endif
