/***********************************************************************
*
* asmdef.h - Assembler header for the IBM 7090 computer.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/20/03   DGP   Changed to call the SLR(1) parser.
*   12/08/04   DGP   Started adding MAP pseudo ops.
*   06/07/05   DGP   Added RMT_T and increased NUMOPS for CTSS insts.
*   06/16/05   DGP   Changed SymNode to use bit array for flags.
*   12/15/05   RMS   MINGW changes.
*   02/17/06   DGP   Correct L1/L2 print format.
*   05/10/10   DGP   Added opcore ACORE/BCORE support.
*   12/15/10   DGP   Tighten up opcode mode (added optypes XXX_OP)
*   12/16/10   DGP   Added a unified source read function readsource().
*   12/17/10   DGP   Added boolean/relocatable error.
*   12/02/11   DGP   Added LINK opcode.
*   06/20/12   DGP   Changed MAXMACROS for z/OS (USS).
*   06/21/12   DGP   More realistic sizes.
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

#define VERSION "2.2.29"

#define IBSYSSYM    '$'		/* Begins an IBSYS control card */
#define COMMENTSYM  '*'		/* Begins a comment */
#define INDIRECTSYM '*'		/* Indirect addressing symbol */
#define LITERALSYM  '='		/* Begins a literal */

#define SYMSTART 0		/* Start of name field */
#define OPCSTART 7		/* Start of opcode field */
#define VARSTART 15		/* Start of variable field */
#define NOOPERAND 16		/* If we get this far, no variable field */
#define RIGHTMARGIN 71		/* End of variable field */

#define NARROWPRINTMARGIN 49	/* Narrow mode print margin */
#define NARROWTITLELEN 23	/* Narrow mode title length */
#define WIDETITLELEN 57		/* Wide mode title length */

#define MAXDUPLINES 200		/* Max DUP lines */
#define MAXMACROLINES 200	/* Max MACRO lines */
#define MAXETCLINES 10		/* Max ETC lines */

#define MAXSRCLINE 90		/* Max length of a single source line */
#define MAXLINE MAXSRCLINE*MAXETCLINES /* Max length of source line, w/ ETCs  */
#define MAXASMFILES 60		/* Max depth of INSERT files */
#define MAXFILENAMELEN 1024     /* Max length of a file name */
#define MAXINCLUDES 10		/* Max number of include directories */

#define TTLSIZE 60		/* Max length of TTL */
#define MAXLBLLEN 8		/* Max length of LBL */
#define MAXMACNAMELEN 6		/* Max length of MACRO name */
#define MAXSYMLEN 6		/* Max length of symbol */
#define MAXFIELDSIZE 80		/* Max length of a MACRO field */

#define LINESPAGE 55		/* Max lines on a listing page */

#define MAXERRORS 3000		/* Max errors in assembly */

#if defined(USS) || defined(OS390)
#define MAXMACROS 200		/* Max MACROs in an assembly */
#else
#define MAXMACROS 1000		/* Max MACROs in an assembly */
#endif
#define MAXMACARGS 20		/* Max MACRO args */

#define MAXQUALS 32		/* Max QUALs in an assembly */
#define MAXBEGINS 32		/* Max BEGINs in an assembly */
#define MAXUSES   32		/* Max USEs in an assembly */
#define MAXHEADS  10		/* Max HEADs in an assembly */

#define MAXSYMBOLS 4000		/* Max symbols in an assembly */
#define MAXDEFOPS  2000		/* Max defined opcodes in an assembly */

#define FAPCOMMONSTART 077461	/* Start of COMMON in FMS (FAP) */

#define TEMPSPEC "asXXXXXX"

/*
** Object output formats
*/

#define OBJFORMAT "%c%4.4o%2.2o%1o%5.5o"
#define OBJAFORMAT "%c%1o%5.5o%1o%5.5o"
#define OBJDFORMAT "%c%4.4o%2.2o%6.6o"
#define OBJFFORMAT "%c%4.4o%8.8o"
#define OBJCHANFORMAT "%c%2.2o%4.4o%1.1o%5.5o"
#define OBJDISKFORMAT "%c%4.4o%4.4o%4.4o"
#define OBJSYMFORMAT "%c%-6.6s %5.5o"
#ifdef WIN32
#define OCTLFORMAT "%c%12.12I64o"
#else
#define OCTLFORMAT "%c%12.12llo"
#endif
#define OCT1FORMAT "%c%2.2o%10.10o"
#define OCT2FORMAT "%c%6.6o%6.6o"
#define WORDFORMAT "%c%12.12o"
#define SEQFORMAT "%4.4d\n"

/*
** Listing formats
*/

#define H1FORMAT     "ASM7090 %-8.8s  %-24.24s  %s Page %04d\n" 
#define H1WIDEFORMAT "ASM7090 %-8.8s  %-58.58s  %s Page %04d\n" 
#define H2FORMAT "%s\n\n"

#define L1FORMAT "%s  %s  %5d%c %s"
#define L2FORMAT "%s  %s\n"

#define PCBLANKS "     "
#define PCFORMAT "%05o"

#define OPBLANKS "                "
#define OPFORMAT "%c%4.4o %2.2o %1o %5.5o"
#define OPAFORMAT "%c%1o %5.5o %1o %5.5o"
#define OPDFORMAT "%c%4.4o %2.2o  %6.6o"
#define OPFFORMAT "%c%4.4o%8.8o   "
#define OPCHANFORMAT "%c%2.2o %4.4o %1o %5.5o"
#define OPDISKFORMAT " %4.4o %4.4o %4.4o "
#define ADDRFORMAT "           %5.5o"
#define BOOLFORMAT "          %6.6o"
#define CHAR1FORMAT "%2.2o"
#define BIT12FORMAT "%4.4o"
#ifdef WIN32
#define OPLFORMAT "%c%12.12I64o   "
#else
#define OPLFORMAT "%c%12.12llo   "
#endif
#define OP1FORMAT "%c%2.2o%10.10o   "
#define OP2FORMAT "%c%6.6o%6.6o   "
#define LITFORMAT "%s  %s  %s\n"

#define SYMFORMAT " %-6.6s   %5.5o%c  "
#define BSYMFORMAT " %-6.6s  %6.6o%c  "

/*
** Modes for processing
*/

#define GENINST		0x00000001
#define DUPINST		0x00000002
#define INSERT		0x00000004
#define SKPINST		0x00000010
#define CONTINU		0x00000020
#define RMTSEQ 		0x00000040
#define MACDEF 		0x00001000
#define MACEXP 		0x00002000
#define MACCALL		0x00004000
#define MACALT 		0x00008000
#define CALL   		0x00010000
#define RETURN 		0x00020000
#define SAVE   		0x00040000
#define CTLCARD		0x00100000
#define NESTMASK	0x3F000000
#define NESTSHFT	24

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

/*
** OpCode table definitions
*/

#define NUMOPS 493

/* Opcode types */

enum optypes
{
   TYPE_A=0,	/* O DDDDD T AAAAA */
   TYPE_B,	/* OOOO FF T AAAAA */
   TYPE_C,	/* OOOO CC T AAAAA */
   TYPE_D,	/* OOOO FF  MMMMMM */
   TYPE_E,	/* OOOO FF T OOOOO */
   TYPE_CHAN,   /* OO DDDD T AAAAA */
   TYPE_DISK,   /* OOOO MMMM TTTT  */
                /* TTTT RRRR XXXX  */
   TYPE_P  	/* Pseudo op */
};
#define MAX_INST_TYPES 6

/* Pseudo ops */

enum psops
{
 /* Standard */
   ABS_T=1,  BCD_T,    BCI_T,    BEGIN_T,  BES_T,    BFT_T,    BNT_T,
   BOOL_T,   BSS_T,    CALL_T,   COMMON_T, CONTRL_T, COUNT_T,  DEC_T,
   DETAIL_T, DUP_T,    END_T,    ENDM_T,   ENDQ_T,   ENT_T,    EJE_T,
   EQU_T,    ETC_T,    EVEN_T,   EXT_T,    FILE_T,   FUL_T,    GOTO_T,
   HEAD_T,   HED_T,    IIB_T,    IFF_T,    IFT_T,    INDEX_T,  IRP_T,
   KEEP_T,   LABEL_T,  LBL_T,    LBOOL_T,  LDIR_T,   LIST_T,   LIT_T,
   LITORG_T, LOC_T,    LORG_T,   MACRO_T,  MAX_T,    MIN_T,    NOCRS_T,
   NULL_T,   OCT_T,    OPD_T,    OPSYN_T,  OPVFD_T,  ORG_T,    ORGCRS_T,
   PCC_T,    PCG_T,    PMC_T,    PUNCH_T,  PURGE_T,  QUAL_T,   RIB_T,
   RBOOL_T,  REF_T,    REM_T,    RETURN_T, RMT_T,    SAVE_T,   SAVEN_T,
   SET_T,    SIB_T,    SPC_T,    SST_T,    TAPENO_T, TCD_T,    TITLE_T,
   TTL_T,    UNLIST_T, UNPNCH_T, USE_T,    VFD_T, 
 /* CTSS */
   ACORE_T,  BCORE_T,  BIT12_T,  COMBES_T, COMBSS_T, INSERT_T, LSTNG_T,
   MACRON_T, NOLNK_T,  LINK_T
};

/* Op flags */

#define FAP_OP       00001
#define MAP_OP       00002
#define BCORE_INST   00010
#define BCORE_BITS   00020
#define BCORE_ILL    00040
#define ALL_OP       (FAP_OP | MAP_OP)
#define BCORE_CHAN   (BCORE_BITS | ALL_OP)
#define BCORE_BAD    (BCORE_ILL | ALL_OP)

/* OpCode table definition */

typedef struct
{
   char *opcode;
   unsigned opvalue;
   unsigned opmod;
   unsigned opflags;
   int optype;
   int model;
} OpCode;

/* User defined OpCode table definition */

typedef struct Op_DefCode
{
   struct Op_DefCode *next;
   char opcode[MAXSYMLEN+2];
   unsigned opvalue;
   unsigned opmod;
   unsigned opflags;
   int optype;
   int model;
} OpDefCode;

/* Macro expansion lines */

typedef struct Mac_Lines
{
   struct Mac_Lines *next;
   char label[MAXFIELDSIZE];
   char opcode[MAXFIELDSIZE];
   char operand[MAXLINE];
} MacLines;

/* Macro template definition */

typedef struct
{
   char macname[MAXMACNAMELEN+2];
   int macstartline;
   int macaltdef;
   int macnested;
   int macargcount;
   int maclinecount;
   int macindirect;
   unsigned macopcode;
   char macargs[MAXMACARGS][MAXFIELDSIZE];
   MacLines *maclines[MAXMACROLINES];
} MacDef;

/*
** Symbol table definitions
*/

/* Xref entry */

typedef struct Xref_Node
{
   struct Xref_Node *next;
   int line;
} XrefNode;

/* Symbol table entry */

typedef struct Sym_Node
{
   struct Sym_Node *next;
   XrefNode *xref_head;
   XrefNode *xref_tail;
   struct Sym_Node *vsym;
   char symbol[MAXFIELDSIZE];
   char qualifier[MAXFIELDSIZE];
   uint32 flags;
   int value;
   int locctr;
   int line;
} SymNode;

/* Symbol table flags */

#define RELOCATABLE 0x000000001
#define EXTERNAL    0x000000002
#define GLOBAL      0x000000004
#define NOEXPORT    0x000000008
#define SGLOBAL     0x000000010
#define TAPENO      0x000000020
#define COMMON      0x000000040
#define SETVAR      0x000000080
#define BOOL        0x000000100
#define LRBOOL      0x000000200
#define RBOOL       0x000000400
#define P0SYM       0x000001000
#define EQUVAR      0x000002000
#define BOOLBITS    (BOOL | LRBOOL | RBOOL)

/* MONSYM SysDefs table definition */

typedef struct
{
   char *name;
   char *qual;
   int val;
} SysDefs;

/*
** IF[FT] relations
*/

enum relation
{
   EQ = 1,
   GT,
   LT
};

/*
** IF[FT] 
*/

enum conditional
{
   IFOR = 1,
   IFAND
};

/*
** Expression types for parser/scanner
*/

enum exprtypes
{
   ADDREXPR = 1,
   BOOLEXPR,
   DATAEXPR
};

#define EXPRTYPEMASK 007
#define ADDRVALUE    000
#define BOOLVALUE    010
#define DATAVALUE    020

/*
** BEGIN/USE table
*/

typedef struct
{
   char symbol[MAXSYMLEN+2];
   int  chain;
   int  value;
   int  bvalue;
} BeginTable;

/*
** Object tags
*/

#define IDT_TAG		'0'	/* 0SSSSSS0LLLLL */
#define ABSENTRY_TAG	'1'	/* 10000000AAAAA */
#define RELENTRY_TAG	'2'	/* 20000000RRRRR */
#define ABSEXTRN_TAG	'3'	/* 3SSSSSS0AAAAA */
#define RELEXTRN_TAG	'4'	/* 4SSSSSS0RRRRR */
#define ABSGLOBAL_TAG	'5'	/* 5SSSSSS0AAAAA */
#define RELGLOBAL_TAG	'6'	/* 6SSSSSS0RRRRR */
#define ABSORG_TAG	'7'	/* 70000000AAAAA */
#define RELORG_TAG	'8'	/* 80000000RRRRR */
#define ABSDATA_TAG	'9'	/* 9AAAAAAAAAAAA */
#define RELADDR_TAG 	'A'	/* AAAAAAAARRRRR */
#define RELDECR_TAG 	'B'	/* BARRRRRAAAAAA */
#define RELBOTH_TAG 	'C'	/* CARRRRRARRRRR */
#define BSS_TAG		'D'     /* D0000000PPPPP */
#define ABSXFER_TAG	'E'	/* E0000000AAAAA */
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

#define CHARSPERREC	66	/* Chars per object record */
#define WORDTAGLEN	13	/* Word + Object Tag length */
#define EXTRNLEN	13	/* Tag + SYMBOL + addr */
#define GLOBALLEN	13	/* Tag + SYMBOL + addr */
#define LBLSTART        72	/* Where to put the LBL */
#define SEQUENCENUM	76	/* Where to put the sequence */

/*
** ReMoTe sequences
*/

typedef struct RMT_Seq
{
   struct RMT_Seq *next;
   int  linemode;
   char line[MAXLINE];
} RMTSeq;

/*
** INSERT files data
*/

typedef struct
{
   FILE *fd;
   int linenum;
   int lastseq;
   char tmpname[MAXFILENAMELEN+2];
   char etcline[MAXLINE];
} INSERTfile;

/*
** Pass 0/1 error table - pass 2 does the printing.
*/

typedef struct
{
   char *errortext;
   int errorline;
   int errorfile;
} ErrTable;

/*
** Parser definitions
*/

#include "asm7090.tok"

typedef uint8  tchar;  /* 0..255 */
typedef uint8  pstate; /* 0..255 */
typedef uint16 toktyp; /* 0..0x7F */
typedef int    tokval;

#define TOKENLEN  80 /* symbol and string length */
#define VALUEZERO 0

/*
** Token types from tokscan
*/

enum toktypes
{
   PC = 30,
   LITERAL,
   SNGLFNUM,
   DBLFNUM,
   SNGLBNUM,
   DBLBNUM,
   EOL
};

#define PCSYMBOL ASTER
 
/*
** Bit masks
*/

#if defined(WIN32) && !defined(MINGW)
#define SIGNMASK 0400000000000I64
#else
#define SIGNMASK 0400000000000ULL
#endif

/*
** External definitions
*/

extern void definemonsyms (int);
extern void definejobsyms (void);
extern int detab (FILE *, FILE *, char *);

extern int asmpass0 (FILE *, FILE *);

extern int asmpass1 (FILE *, int);

extern int asmpass2 (FILE *, FILE *, int, FILE *);

extern OpCode *oplookup (char *);
extern void freeopd (OpDefCode *);
extern void opadd (char *, unsigned, unsigned, int);
extern void opdel (char *);

extern int readsource (FILE *, int *, int *, int *, char *);
extern void writesource (FILE *, int, int, int, char *, char *);
extern FILE *opencopy (char *, FILE *, char *);
extern FILE *closecopy (FILE *, char *);
extern void rmtseqadd (int, char *);
extern char *rmtseqget (int *, char *);
extern void logerror (char *);
extern void logwarning (char *);
extern void Parse_Error (int, int, int);
extern char *tokscan (char *, char **, int *, int *, char *, int);
extern void freexref (XrefNode *);
extern void freesym (SymNode *);
extern SymNode *symlookup (char *, char *, int, int);
extern SymNode *entsymlookup (char *, char *, int, int);
extern SymNode *symlocate (char *, int *);
extern tokval symparse (char *, char *, int *, int *, int, int);
extern void symdelete (SymNode *);
extern char *exprscan (char *, int *, char *, int *, int, int, int);
extern char *condexpr (char *, int *, char *);

extern tokval Parser (char *, int *, int *, int, int, int);
extern toktyp Scanner (char *, int *, tokval *, char *, char *, char *, int,
                       int);

