/***********************************************************************
*
* objdef.h - Object defs.
*
* Changes:
*   01/20/05   DGP   Original.
*   12/15/05   RMS   MINGW changes.
*	
***********************************************************************/

/*
** Field masks
*/

#if defined(WIN32) && !defined(MINGW)
#define ADDRMASK 0000000077777I64
#define DECRMASK 0077777000000I64
#define WORDMASK 0777777777777I64
#else
#define ADDRMASK 0000000077777ULL
#define DECRMASK 0077777000000ULL
#define WORDMASK 0777777777777ULL
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
#define COMMON_TAG	'I'	/* ISSSSSS0AAAAA */
#define ABSSYM_TAG	'J'     /* JSSSSSS0AAAAA */
#define RELSYM_TAG	'K'     /* KSSSSSS0RRRRR */

/* Where:
 *    SSSSSS - Symbol
 *    LLLLLL - Length of module
 *    AAAAAA - Absolute field
 *    RRRRRR - Relocatable field
*/

#define FAPCOMMONSTART 077461	/* Start of COMMON in FMS (FAP) */
