/***********************************************************************
*
* asmdef.h - Assembler header for the IBM 7070 computer.
*
* Changes:
*   03/01/07   DGP   Original.
*   05/28/13   DGP   Much added.
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

#define VERSION "0.2.0"

#define COMMENTSYM  '*'		/* Begins a comment */
#define STRINGSYM  '@'		/* Begins a string */

#define MAXMACROLINES 1000	/* Max MACRO lines */
#define MAXETCLINES 10		/* Max ETC lines */

#define MAXPCCOUNTERS 10	/* Max PC counters */
#define DEFAULTPCLOC 325	/* Default PC location */

#define MAXINDEX 99		/* Max index value */
#define MAXSYNCRO 3		/* Max syncronizers on system */
#define MAXCHANNELS 4		/* Max channels on system */
#define MAXTAPES 49		/* Max tapes on a system */
#define MAXUNITS 9		/* Max tapes on a channel */
#define MAXSWITCHES 30		/* Max switches on system */

#define FLOATPOINTLENGTH 10	/* Length of a floating point number */
#define MAXDATALEN 99		/* Maximum length of a data field */

#define MAXSRCLINE 90		/* Max length of a single source line */
#define MAXLINE MAXSRCLINE*MAXETCLINES /* Max length of source line, w/ ETCs  */
#define MAXFILENAMELEN 1024     /* Max length of a file name */

#define MAXDECKNAMELEN 6	/* Max length of deck name */
#define MAXMACNAMELEN 6		/* Max length of MACRO name */
#define MAXSYMLEN 10		/* Max length of symbol */
#define MAXSYMENTRYLEN 80	/* Maximum length of a symbol table entry */
#define MAXFIELDSIZE 1024	/* Max length of a MACRO field */

#define MAXERRORS 3000		/* Max errors in assembly */

#define MAXMACROS 100		/* Max MACROs in an assembly */
#define MAXMACARGS 63		/* Max MACRO args */

#define MAXSYMBOLS 4000		/* Max symbols in an assembly */

#define MAXDAS 20		/* Max DA definitions. */
#define MAXRDWS 20		/* Max RDW definitions. */

#define TEMPSPEC "asXXXXXX"

/*
** Source card columns
*/

#define SYMSTART 5		/* Start of name field */
#define OPCSTART 15		/* Start of opcode field */
#define VARSTART 22		/* Start of variable field */
#define NOOPERAND 26		/* If we get this far, no variable field */
#define RIGHTMARGIN 74		/* End of variable field */

/*
** Object output formats and definitions.
*/

#define OBJSEQUENCEFORMAT "%04d"

#define OBJINSTFORMAT "%c%c%02d%02d%02d%04d"
#define OBJDATAFORMAT "%c%c%010lld"
#define OBJCHARFORMAT "%c%c%010lld"

#define OBJRECORDLENGTH	80	/* Length of the object record */
#define OBJLENGTH 75		/* Length of object on the record */

/*
** Listing formats and definitions.
*/

#define NARROWPRINTMARGIN 56	/* Narrow mode print margin */
#define NARROWTITLELEN 23	/* Narrow mode title length */
#define WIDETITLELEN 57		/* Wide mode title length */
#define LINESPAGE 55		/* Max lines on a listing page */
#define TTLSIZE 132		/* Max length of TTL */

#define H1FORMAT     "ASM7070 %-8.8s  %-24.24s  %s Page %04d\n" 
#define H1WIDEFORMAT "ASM7070 %-8.8s  %-60.60s  %s   Page %04d\n" 
#define H2FORMAT "%s\n\n"

#define L1FORMAT "%s %s %4d%c %s\n"
#define L2FORMAT "%s %s \n"
#define SL1FORMAT "%02d %s %c %s %s %s %s %s %s %s\n"
#define SLCFORMAT "%02d %s %c %-70.70s  %s %s %s %s\n"
#define SL2FORMAT "%02d %-79.79s %s %s %s %s\n"

#define PCBLANKS "    "
#define PCFORMAT "%04d"
#define FDBLANKS "  "
#define FDFORMAT "%02d"

#define LITFORMAT "%s %s       %s\n"
#define SLITFORMAT "           %-72.72s         %s %s\n"

#define OPBLANKS "             "
#define OPFORMAT "%c%02d%02d%02d%04d  "
#define DATAFORMAT "%c%010lld  "
#define CHARFORMAT "%c%010lld  "
#define ADDRFORMAT "       %04d  "

#define SYMFORMAT " %-10.10s %04d %-2.2s %-2.2s %-4.4s "

/*
** Modes for processing
*/

#define GENINST 0x00000001
#define DUPINST 0x00000002
#define INSERT  0x00000004
#define SKPINST 0x00000010
#define CONTINU 0x00000020
#define MACDEF  0x00001000
#define MACEXP  0x00002000
#define MACCALL 0x00004000
#define MACALT  0x00008000

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

#define NUMOPS 205
#define MAX_INST_TYPES 11

/* Opcode types */

enum optypes
{
   TYPE_A=0,	/* SOO II PP AAAA */
   TYPE_B,	/* SOO II PP AAAA */
   TYPE_C,	/* SOO II PP AAAA */
   TYPE_D,	/* SOO II PP AAAA */
   TYPE_E,	/* SOO II PP AAAA */
   TYPE_I,	/* SOO II ii AAAA */
   TYPE_L,	/* SOO II PP AAAA */
   TYPE_Q,	/* SOO II PP AAAA */
   TYPE_S,	/* SOO II PP AAAA */
   TYPE_T,	/* S8C II UO AAAA */
   TYPE_U,	/* SOO II PP AAAA */
   TYPE_P  	/* Pseudo op */
};

/* Pseudo ops */

enum psops
{
   CODE_T=1, CNTRL_T,  DA_T,     DC_T,     DIOCS_T,  DLINE_T,  DRDW_T,
   DSW_T,    DTF_T,    DUF_T,    EJECT_T,  EQU_T
};

/* OpCode table definition */

typedef struct
{
   char *opcode;
   int opvalue;
   int opmod;
   int opmod2;
   int optype;
   int model;
} OpCode;

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
   char symbol[MAXSYMENTRYLEN+2];
   uint32 flags;
   int value;
   int fdpos;
   int length;
   int line;
   int index;
} SymNode;

/* Symbol table flags */

#define RELOCATABLE	0x000000001
#define EXTERNAL	0x000000002
#define GLOBAL		0x000000004
#define NOEXPORT	0x000000008
#define SGLOBAL		0x000000010
#define P0SYM		0x000000020
#define AREADEF		0x000000040
#define RDWVAR		0x000000080
#define CONVAR		0x000000100
#define EQUVAR		0x000000200
#define AREAVAR		0x000000400
#define DSWVAR		0x000000800
#define TCUVAR		0x000001000
#define TCVAR		0x000002000
#define TUVAR		0x000004000
#define AFVAR		0x000008000
#define IWVAR		0x000010000
#define SWVAR		0x000020000
#define SNVAR		0x000040000
#define ULVAR		0x000080000
#define URVAR		0x000100000
#define UPVAR		0x000200000
#define UWVAR		0x000400000
#define IQVAR		0x000800000
#define TYVAR		0x001000000
#define DSWSVAR		0x002000000
#define FLOATLITERAL	0x010000000
#define MIXEDLITERAL	0x020000000
#define SIGNEDLITERAL	0x040000000
#define ADCONLITERAL	0x080000000
#define LITERALMASK	0x0F0000000

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
** Object format
*/

#define ADDR_TAG	'A'
#define BRANCH_TAG	'B'
#define INST_TAG	'I'
#define DATA_TAG	'D'

#define INSTFIELD	0	/* Where to start in the object record */
#define INSTLENGTH	11	/* Length of the instruction field */

#define CHARSPERREC	74	/* Chars per object record */
#define SEQUENCENUM	76	/* Where to put the sequence */

/*
** DA stuct.
*/

typedef struct
{
   int origin;
   int count;
   int length;
   int genrdw;
   int address;
   int index;
   char sign;
   char symbol[MAXSYMLEN+2];
} DA_t;

/*
** RDW stuct.
*/

typedef struct
{
   int origin;
   int length;
   char sign;
} RDW_t;

/*
** Pass 0/1 error table - pass 2 does the printing.
*/

typedef struct
{
   char *errortext;
   int errorline;
} ErrTable;

/*
** Parser definitions
*/

#include "asm7070.tok"

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
   EOL
};

#define PCSYMBOL ASTER
 
/*
** External definitions
*/

extern int  detab (FILE *, FILE *, char *);

extern int asmpass0 (FILE *, FILE *);

extern int asmpass1 (FILE *, int);
extern int chkerror (int);

extern int asmpass2 (FILE *, FILE *, int, FILE *);

extern OpCode *oplookup (char *);

extern void logerror (char *);
extern void Parse_Error (int, int, int);
extern char *tokscan (char *, char **, int *, int *, char *);
extern void freexref (XrefNode *);
extern void freesym (SymNode *);
extern SymNode *symlookup (char *, int, int);
extern void symdelete (SymNode *);
extern char *exprscan (char *, int *, int *, int *, char *, int);

extern tokval Parser (char *, int *, int *, int *, int, int);
extern toktyp Scanner (char *, int *, tokval *, int *, int *,
		     char *, char *, int);

