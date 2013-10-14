/*****************************************************************************
 * dbf.h
 *****************************************************************************
 * Library to read information from dBASE files
 * Author: Bjoern Berg, clergyman@gmx.de
 * (C) Copyright 2004, Björn Berg
 *
 *****************************************************************************
 * Permission to use, copy, modify and distribute this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation. The
 * author makes no representations about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied warranty.
 *
 *****************************************************************************
 * $Id: dbf.h,v 1.5 2004/09/07 16:08:20 steinm Exp $
 ****************************************************************************/

#ifndef __DBF_CORE__
#define __DBF_CORE__

#include "config.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(a) dgettext(GETTEXT_PACKAGE, a)
#else
#define _(a) a
#endif

#ifdef __unix__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef __ANUBISNET_TYPES__
#define __ANUBISNET_TYPES__
	typedef u_int16_t uint16_t;
	typedef u_int32_t uint32_t;
#endif
#elif _WIN32
#include <io.h>
#include <sys\stat.h>
#include <windows.h>
#ifndef __ANUBISNET_TYPES__
#define __ANUBISNET_TYPES__
	typedef UINT32 u_int32_t;
	// Windows does not know UINT16 types
	typedef unsigned short u_int16_t;
#endif
#elif __MSDOS__
#include <io.h>
#include <sys\stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

/*
 * special anubisnet and dbf includes
 */
#include "endian.h"

//@{
/** These defines are used to distinguish between types in the dbf fields. */
#define IS_STRING 1
#define IS_NUMERIC 2
//@}

/*
 *	STRUCTS
 */

/*! \struct DB_HEADER
	\brief table file header
 	 Standard dBASE Header
	 Offsets of this header are the same in all versions of
	 dBASE except dBASE 7.0
	\warning It is recommend not to access DB_HEADER directly.
 */
typedef struct {
	/*! Byte: 0; dBase version */
	unsigned char version;
	/*! Byte: 1-3; date of last update */
	unsigned char last_update[3];
	/*! Byte: 4-7; number of records in table */
	u_int32_t records;
	/*! Byte: 8-9; number of bytes in the header */
	u_int16_t header_length;
	/*! Byte: 10-11; number of bytes in the record */
	u_int16_t record_length;
	/*! Byte: 12-13; reserved, see specification of dBase databases */
	unsigned char reserved01[2];
	/*! Byte: 14; Flag indicating incomplete transaction */
	unsigned char transaction;
	/*! Byte: 15; Encryption Flag */
	unsigned char encryption;
	/*! Byte: 16-27; reserved for dBASE in a multiuser environment*/
	unsigned char reserved02[12];
	/*! Byte: 28; Production MDX file flag */
	unsigned char mdx;
	/*! Byte: 29; Language driver ID, for Visual FoxPro */
	unsigned char language;
	/*! Byte: 30-31; reserved, filled with zero */
	unsigned char reserved03[2];
} DB_HEADER;

/*! \struct DB_FIELD
	\brief The field descriptor array
	Offsets of this header are the same in all versions of dBASE
	\warning It is recommend not to access DB_FIELD directly.
 */
struct _DB_FIELD {
	/*! Byte: 0-10; fieldname in ASCII */
	unsigned char field_name[11];
	/*! Byte: 11; field type in ASCII (C, D, L, M or N) */
	unsigned char field_type;
	/*! Byte: 12-15; field data adress */
	u_int32_t field_address;
	/*! Byte: 16; field length in binary */
	unsigned char field_length;
	/*! Byte: 17; field decimal count in binary */
	unsigned char field_decimals;
	/*! Byte: 18-30; reserved */
	unsigned char reserved1[2];
	u_int32_t field_offset;
	unsigned char reserved2[7];
	/*! Byte: 31; Production MDX field flag */
	unsigned char mdx;
};

/*! \struct P_DBF
	\brief P_DBF is a global file handler

	P_DBF store the file handlers for the dbf-file and if exists
	the appropriate memo file.
*/
struct _P_DBF {
	/*! filehandler of *.dbf */
	int dbf_fh;
	/*! filehandler of memo */
	int dbt_fh;
	/*! the pysical size of the file, as stated from filesystem */
	u_int32_t real_filesize;
	/*! the calculated filesize */
	u_int32_t calc_filesize;
	/*! header of .dbf file */
	DB_HEADER *header;
	/*! array of field specification */
	DB_FIELD *fields;
	/*! number of fields */
	u_int32_t columns;
	/*! integrity could be: valid, invalid */
	unsigned char integrity[7];
	/*! record counter */
	int cur_record;
	/*! errorhandler, maximum of 254 characters */
	char errmsg[254];
};



/* Memo File Structure (.FPT)
 * Memo files contain one header record and any number of block structures.
 * The header record contains a pointer to the next free block and the size
 * of the block in bytes. The size is determined by the SET BLOCKSIZE command
 * when the file is created. The header record starts at file position zero and
 * occupies 512 bytes. The SET BLOCKSIZE TO 0 command sets the block size width to 1.
 */
/* Following the header record are the blocks that contain a block header and
 * the text of the memo. The table file contains block numbers that are used to
 * reference the memo blocks. The position of the block in the memo file is
 * determined by multiplying the block number by the block size (found in the
 * memo file header record). All memo blocks start at even block boundary
 * addresses. A memo block can occupy more than one consecutive block.
 */


/* Memo Header Record */

struct DB_MEMO_HEADER {
	/* 00  03	Location of next free block [1] */
	unsigned int block_adress;
	/* 04  05	Unused */
	unsigned char reserved[2];
	/* 06  07	Block size (bytes per block) [1] */
	unsigned short block_size;
	/* 08  511	Unused */
	unsigned char reserved2[504];
 };

 /* [1] Integers stored with the most
 significant byte first. See: endian.h   */

/* Memo Block Header and Memo Text */
struct DB_MEMO_BLOCK_TOP {
	/* 00  03			Block signature [1]  */
	/*	(indicates the type of data in the block)
	 *	0  picture (picture field type)
	 *	1  text (memo field type) */
	unsigned int signature;
	/* 04  07	Length [1] of memo (in bytes) */
	unsigned int block_length;
	/* 08 -  n	Memo text (n = length) */
};

/* [1] Integers stored with the most
significant byte first.    */


#endif

