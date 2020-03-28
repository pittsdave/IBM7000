/***********************************************************************
*
* lnkdef.h - Linker header for the IBM 7090 linker.
*
* Changes:
*   05/21/03   DGP   Original.
*   12/28/04   DGP   Added new tag and MAP suppport.
*   12/15/05   RMS   MINGW changes.
*   03/17/10   DGP   Bump MAXFILES to 300.
*   03/23/10   DGP   Added transfer vector tags.
*   06/10/10   DGP   Added "MOVIE)" definitions.
*	
***********************************************************************/

#include <stdio.h>

/*
** Definitions
*/

#define NORMAL 0
#define ABORT  12

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define VERSION "2.2.2"

#define IBSYSSYM   '$'		/* Begins an IBSYS control card */

#define MAXLINE 256
#define LINESPAGE 55
#define MAXFILES 300

#define TEMPSPEC "lnkXXXXXX"

#define MOVIE "MOVIE)"

#if defined(WIN32) && !defined(MINGW)
#define ADDRMASK 0777777700000I64
#define DECRMASK 0700000777777I64
#define BOTHMASK 0700000700000I64
#define WORDMASK 0777777777777I64
#define TRAINST  0002000000000I64
#else
#define ADDRMASK 0777777700000ULL
#define DECRMASK 0700000777777ULL
#define BOTHMASK 0700000700000ULL
#define WORDMASK 0777777777777ULL
#define TRAINST  0002000000000ULL
#endif

/*
** Object output formats
*/

#if defined(WIN32)
#define OBJFORMAT "%c%12.12I64o"
#else
#define OBJFORMAT "%c%12.12llo"
#endif
#define OBJSYMFORMAT "%c%-6.6s %5.5o"
#define OBJSEQFORMAT "%4.4d\n"

/*
** Listing formats
*/

#define H1FORMAT "LNK7090 %-8.8s  %-24.24s  %s Page %04d\n\n"
#define H2FORMAT "LNK7090 %-8.8s  %-64.64s  %s Page %04d\n\n"

#define MODFORMAT "%-6.6s %3d   %05o   %05o  %s  %s  %s  %s\n"
#define SYMFORMAT "%-6.6s %05o%c%c %3d  "

/*
** Data type definitions
*/

#define int8            char
#define int16           short
#define int32           int
typedef int             t_stat;                         /* status */
typedef int             t_bool;                         /* boolean */
typedef unsigned int8   uint8;
typedef unsigned int16  uint16;
typedef unsigned int32  uint32, t_addr;                 /* address */

#if defined(WIN32)                                      /* Windows */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (VMS)                /* Alpha VMS */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (__unix__)           /* Alpha UNIX */
#define t_int64 long
#else                                                   /* default GCC */
#define t_int64 long long
#endif                                                  /* end OS's */
typedef unsigned t_int64        t_uint64, t_value;      /* value */
typedef t_int64                 t_svalue;               /* signed value */

#if defined(__s390__) || defined(USS) || defined(sparc) || defined(OS390)
#define MSL 0
#define LSL 1
#else
#define MSL 1
#define LSL 0
#endif

/*
** Memory
*/

#define MEMSIZE 32768

typedef struct
{
   t_uint64 word;
   char tag;
   int	reladdr;
   int	reldecr;
   int	relboth;
   int  external;
   int  global;
   int  modnum;
} Memory;

/*
** Symbol table
*/

#define MAXSYMLEN 6
#define MAXSYMBOLS 2000

/* Xref entry */

typedef struct Xref_Node
{
   struct Xref_Node *next;
   int value;
   int modnum;
} XrefNode;

typedef struct
{
   char symbol[MAXSYMLEN+2];
   char module[MAXSYMLEN+2];
   XrefNode *xref_head;
   XrefNode *xref_tail;
   int value;
   int relocatable;
   int external;
   int global;
   int muldef;
   int undef;
   int modnum;
} SymNode;

/*
** Module table
*/

#define MAXMODULES 100

typedef struct 
{
   char name[MAXSYMLEN+2];
   char date[12];
   char time[12];
   char creator[12];
   char objfile[256];
   int origin;
   int length;
} Module;

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
#define ABSXFERVEC_TAG  'I'	/* ISSSSSS0AAAAA */
#define RELXFERVEC_TAG  'J'	/* JSSSSSS0RRRRR */

/* Where:
 *    SSSSSS - Symbol
 *    LLLLLL - Length of module
 *    AAAAAA - Absolute field
 *    RRRRRR - Relocatable field
*/

#define FAPCOMMONSTART 077461	/* Start of COMMON in FMS (FAP) */

#define TIMEOFFSET 17
#define DATEOFFSET 27
#define CREATOROFFSET 38

#define CHARSPERREC	66	/* Chars per object record */
#define WORDTAGLEN	13	/* Word + Object Tag length */
#define EXTRNLEN	13	/* Tag + SYMBOL + addr */
#define GLOBALLEN	13	/* Tag + SYMBOL + addr */
#define LBLSTART        72	/* Where to put the LBL */
#define SEQUENCENUM	76	/* Where to put the sequence */

extern int lnkloader (FILE *, int, int, char *);
extern void chkxref (SymNode *s, int refaddr);
extern int lnkpunch (FILE *);
extern void printheader (FILE *);
extern SymNode *symlookup (char *, char *, int);
