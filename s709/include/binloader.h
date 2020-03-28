/***********************************************************************
*
* binloader.h - IBM 7090 emulator binary loader header.
*
* Changes:
*   10/20/03   DGP   Original.
*   12/28/04   DGP   Changed for new object formats.
*	
***********************************************************************/

#define IBSYSSYM '$'

#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
#define TRA 0002000000000I64
#else
#define TRA 0002000000000ULL
#endif
#else
#define TRAh 0
#define TRAl 02000000000
#endif

/*
** Object tags
*/

#define IDT_TAG		'0'	/* 0SSSSSS0LLLLL */
#define ABSENTRY_TAG	'1'	/* 10000000AAAAA */
#define RELENTRY_TAG	'2'	/* 20000000RRRRR */
#define ABSEXTRN_TAG	'3'	/* 3SSSSSS0RRRRR */
#define RELEXTRN_TAG	'4'	/* 4SSSSSS0RRRRR */
#define ABSGLOBAL_TAG	'5'	/* 5SSSSSS0RRRRR */
#define RELGLOBAL_TAG	'6'	/* 6SSSSSS0RRRRR */
#define ABSORG_TAG	'7'	/* 70000000AAAAA */
#define RELORG_TAG	'8'	/* 80000000RRRRR */
#define ABSDATA_TAG	'9'	/* 9AAAAAAAAAAAA */
#define RELADDR_TAG 	'A'	/* AAAAAAAARRRRR */
#define RELDECR_TAG 	'B'	/* BARRRRRAAAAAA */
#define RELBOTH_TAG 	'C'	/* CARRRRRARRRRR */
#define BSS_TAG		'D'     /* D0000000PPPPP */
#define ABSXFER_TAG	'E'	/* E0000000RRRRR */
#define RELXFER_TAG	'F'	/* F0000000RRRRR */
#define EVEN_TAG	'G'	/* G0000000RRRRR */
#define FAPCOMMON_TAG	'H'	/* H0000000AAAAA */

/* Where:
 *    SSSSSS - Symbol
 *    LLLLLL - Length of module
 *    AAAAAA - Absolute field
 *    RRRRRR - Relocatable field
*/

#if defined(__s390__) || defined(USS) || defined(OS390) || defined(sparc)
#define MSL 0
#define LSL 1
#else
#define MSL 1
#define LSL 0
#endif

extern int binloader (char *, int);

