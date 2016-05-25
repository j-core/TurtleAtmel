/*---------------------------------------------------------------------------/
/  Petit gFatFs - FAT file system module include file  R0.02a   (C)ChaN, 2010
/----------------------------------------------------------------------------/
/ Petit gFatFs module is an open source software to implement FAT file system to
/ small embedded systems. This is a free software and is opened for education,
/ research and commercial developments under license policy of following trems.
/
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * The Petit gFatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial use UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/----------------------------------------------------------------------------*/

#include "Turtle.h"

/*---------------------------------------------------------------------------/
/ Petit gFatFs Configuration Options
/
/ CAUTION! Do not forget to make clean the project after any changes to
/ the configuration options.
/
/----------------------------------------------------------------------------*/
#ifndef _FATFS
#define _FATFS

#define	_USE_READ	1	/* 1:Enable pf_read() */

#define	_USE_DIR	1	/* 1:Enable pf_opendir() and pf_readdir() */

//#define	_USE_LSEEK	1	/* 1:Enable pf_lseek() */

//#define	_USE_WRITE	1	/* 1:Enable pf_write() */

//#define _FS_FAT12	1	/* 1:Enable FAT12 support */
#define _FS_FAT32	1	/* 1:Enable FAT32 support */


#define	_CODE_PAGE	1
/* Defines which code page is used for path name. Supported code pages are:
/  932, 936, 949, 950, 437, 720, 737, 775, 850, 852, 855, 857, 858, 862, 866,
/  874, 1250, 1251, 1252, 1253, 1254, 1255, 1257, 1258 and 1 (ASCII only).
/  SBCS code pages except for 1 requiers a case conversion table. This
/  might occupy 128 bytes on the RAM on some platforms, e.g. avr-gcc. */


#define _WORD_ACCESS	0
/* The _WORD_ACCESS option defines which access method is used to the word
/  data in the FAT structure.
/
/   0: Byte-by-byte access. Always compatible with all platforms.
/   1: Word access. Do not choose this unless following condition is met.
/
/  When the byte order on the memory is big-endian or address miss-aligned
/  word access results incorrect behavior, the _WORD_ACCESS must be set to 0.
/  If it is not the case, the value can also be set to 1 to improve the
/  performance and code efficiency. */


/* End of configuration options. Do not change followings without care.     */
/*--------------------------------------------------------------------------*/



#if _FS_FAT32
#define	CLUST	uint32
#else
#define	CLUST	uint16
#endif


/* File system object structure */

typedef struct {
	uint8	fs_type;	/* FAT sub type */
	uint8	flag;		/* File status flags */
	uint8	csize;		/* Number of sectors per cluster */
	uint8	init;
	uint16	n_rootdir;	/* Number of root directory entries (0 on FAT32) */
	CLUST	n_fatent;	/* Number of FAT entries (= number of clusters + 2) */
	uint32	fatbase;	/* FAT start sector */
	uint32	dirbase;	/* Root directory start sector (Cluster# on FAT32) */
	uint32	database;	/* Data start sector */
	uint32	fptr;		/* File R/W pointer */
	uint32	fsize;		/* File size */
	CLUST	org_clust;	/* File start cluster */
	CLUST	curr_clust;	/* File current cluster */
	uint32	dsect;		/* File current data sector */
} FATFS;



/* Directory object structure */

typedef struct {
	uint16	index;		/* Current read/write index number */
	uint8*	fn;			/* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
	CLUST	sclust;		/* Table start cluster (0:Static table) */
	CLUST	clust;		/* Current cluster */
	uint32	sect;		/* Current sector */
} DIR;



/* File status structure */

typedef struct {
	uint32	fsize;		/* File size */
	uint16	fdate;		/* Last modified date */
	uint16	ftime;		/* Last modified time */
	uint8	fattrib;	/* Attribute */
	char	fname[13];	/* File name */
} FILINFO;



/* File function return code (FRESULT) */

typedef enum {
	FR_OK = 0,			/* 0 */
	FR_DISK_ERR,		/* 1 */
	FR_NOT_READY,		/* 2 */
	FR_NO_FILE,			/* 3 */
	FR_NO_PATH,			/* 4 */
	FR_NOT_OPENED,		/* 5 */
	FR_NOT_ENABLED,		/* 6 */
	FR_NO_FILESYSTEM,	/* 7 */
	FR_EOF
} FRESULT;

extern FATFS gFatFs;	/* Pointer to the file system object (logical drive) */


/*--------------------------------------------------------------*/
/* Petit gFatFs module application interface                     */

FRESULT pf_mount (uint8);						/* Mount/Unmount a logical drive */
FRESULT pf_open (const char*);					/* Open a file */
FRESULT pf_read (void*, uint16, uint16*);			/* Read data from the open file */
FRESULT pf_write (const void*, uint16, uint16*);	/* Write data to the open file */
FRESULT pf_lseek (uint32);						/* Move file pointer of the open file */
FRESULT pf_opendir (DIR*, const char*);			/* Open a directory */
FRESULT pf_readdir (DIR*, FILINFO*);			/* Read a directory item from the open directory */



/*--------------------------------------------------------------*/
/* Flags and offset address                                     */

/* File status flag (FATFS.flag) */

#define	FA_OPENED	0x01
#define	FA_WPRT		0x02
#define	FA__WIP		0x40


/* FAT sub type (FATFS.fs_type) */

#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3


/* File attribute bits for directory entry */

#define	AM_RDO	0x01	/* Read only */
#define	AM_HID	0x02	/* Hidden */
#define	AM_SYS	0x04	/* System */
#define	AM_VOL	0x08	/* Volume label */
#define AM_LFN	0x0F	/* LFN entry */
#define AM_DIR	0x10	/* Directory */
#define AM_ARC	0x20	/* Archive */
#define AM_MASK	0x3F	/* Mask of defined bits */


/*--------------------------------*/
/* Multi-byte word access macros  */

#if _WORD_ACCESS == 1	/* Enable word access to the FAT structure */
#define	LD_WORD(ptr)		(uint16)(*(uint16*)(uint8*)(ptr))
#define	LD_DWORD(ptr)		(uint32)(*(uint32*)(uint8*)(ptr))
#define	ST_WORD(ptr,val)	*(uint16*)(uint8*)(ptr)=(uint16)(val)
#define	ST_DWORD(ptr,val)	*(uint32*)(uint8*)(ptr)=(uint32)(val)
#else					/* Use byte-by-byte access to the FAT structure */
#define	LD_WORD(ptr)		(uint16)(((uint16)*((uint8*)(ptr)+1)<<8)|(uint16)*(uint8*)(ptr))
#define	LD_DWORD(ptr)		(uint32)(((uint32)*((uint8*)(ptr)+3)<<24)|((uint32)*((uint8*)(ptr)+2)<<16)|((uint16)*((uint8*)(ptr)+1)<<8)|*(uint8*)(ptr))
#define	ST_WORD(ptr,val)	*(uint8*)(ptr)=(uint8)(val); *((uint8*)(ptr)+1)=(uint8)((uint16)(val)>>8)
#define	ST_DWORD(ptr,val)	*(uint8*)(ptr)=(uint8)(val); *((uint8*)(ptr)+1)=(uint8)((uint16)(val)>>8); *((uint8*)(ptr)+2)=(uint8)((uint32)(val)>>16); *((uint8*)(ptr)+3)=(uint8)((uint32)(val)>>24)
#endif


#endif /* _FATFS */
