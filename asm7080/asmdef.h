/***********************************************************************
*
* asmdef.h - Assembler header for the IBM 7080 computer.
*
* Changes:
*   02/08/07   DGP   Original.
*   02/20/07   DGP   Changed object for 60 characters.
*   05/22/13   DGP   Set default starting PC to 160.
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

#define VERSION "0.3.0"

#define COMMENTSYM  '*'		/* Begins a comment */
#define LITERALSYM  '#'		/* Begins a literal */

#define MACPARMSYM '['		/* Defines a macro parameter in template */
#define MACLABELSYM '('		/* Defines a macro label in template */

#define MAXMACROLINES 100	/* Max MACRO lines */
#define MAXETCLINES 10		/* Max ETC lines */

#define MAXPCCOUNTERS 10	/* Max PC counters */
#define DEFAULTPCLOC 160	/* Default PC location */

#define FLOATPOINTLENGTH 10	/* Length of a floating point number */
#define MAXDATALEN 99		/* Maximum length of a data field */
#define MAXREGISTERS 15		/* Maximum number of registers */
#define MAXZONES 15		/* Maximum nuber of memory zones */

#define MAXSRCLINE 90		/* Max length of a single source line */
#define MAXLINE MAXSRCLINE*MAXETCLINES /* Max length of source line, w/ ETCs  */
#define MAXFILENAMELEN 1024     /* Max length of a file name */

#define MAXDECKNAMELEN 6	/* Max length of deck name */
#define MAXMACNAMELEN 6		/* Max length of MACRO name */
#define MAXSYMLEN 10		/* Max length of symbol */
#define MAXSYMENTRYLEN 80	/* Maximum length of a symbol table entry */
#define MAXFIELDSIZE 100 	/* Max length of a MACRO field */

#define MAXERRORS 3000		/* Max errors in assembly */

#define MAXMACROS 100		/* Max MACROs in an assembly */
#define MAXMACARGS 20		/* Max MACRO args */

#define MAXSYMBOLS 4000		/* Max symbols in an assembly */

#define TEMPSPEC "asXXXXXX"

#define STARTPC 160		/* Default starting PC */

/*
** Source card columns
*/

#define SYMSTART 5		/* Start of name field */
#define OPCSTART 15		/* Start of opcode field */
#define NUMSTART 20		/* Start of numeric field */
#define VARSTART 22		/* Start of variable field */
#define NOOPERAND 26		/* If we get this far, no variable field */
#define RIGHTMARGIN 71		/* End of variable field */

/*
** Object output formats and definitions.
*/

#define OBJIDENTFORMAT "%-6.6s"
#define OBJSERIALFORMAT "%03d"
#define OBJADDRESSFORMAT "%c%c%c%c"
#define OBJCOLUMNFORMAT "%02d"

#define OBJINSTFORMAT "%c%c%c%c%c"
#define OBJACON4FORMAT "%c%c%c%c"
#define OBJACON5FORMAT "%c%c%c%c%c"
#define OBJACON6FORMAT "%c%c%c%c%c%c"

#define OBJRECORDLENGTH	80	/* Length of the object record */
#define OBJLENGTH 75		/* Length of object on the record */

/*
** Listing formats and definitions.
*/

#define NARROWPRINTMARGIN 49	/* Narrow mode print margin */
#define NARROWTITLELEN 23	/* Narrow mode title length */
#define WIDETITLELEN 57		/* Wide mode title length */
#define LINESPAGE 55		/* Max lines on a listing page */
#define TTLSIZE 132		/* Max length of TTL */

#define H1FORMAT     "ASM7080 %-8.8s  %-24.24s  %s Page %04d\n" 
#define H1WIDEFORMAT "ASM7080 %-8.8s  %-60.60s  %s   Page %04d\n" 
#define H2FORMAT "%s\n\n"

#define L1FORMAT "%s %s %4d%c %s\n"
#define SL1FORMAT "%s %s %s %s %s%s %s %s\n"
#define SLCFORMAT "%s %-66.66s  %s %s\n"

#define PCBLANKS "      "
#define PCFORMAT "%06d"

#define LITFORMAT "%s  %s\n"
#define SLITFORMAT "      %-72.72s%s\n"

#define OPBLANKS "                  "
#define OPFORMAT "%c %02d %06d %c%c%c%c  "
#define ADDRFORMAT "     %6.6d       "
#define ADC4FORMAT "  %02d %6.6d %c%c%c%c  "
#define ADC5FORMAT "     %6.6d %c%c%c%c%c "
#define ADC6FORMAT "     %6.6d %c%c%c%c%c%c"

#define SYMFORMAT " %-10.10s %6.6d %c %-4.4s "

/*
** Modes for processing
*/

#define GENINST 0x00000001
#define DUPINST 0x00000002
#define INSERT  0x00000004
#define SKPINST 0x00000010
#define CONTINU 0x00000020
#define RMTSEQ  0x00000040
#define MACDEF  0x00001000
#define MACEXP  0x00002000
#define MACCALL 0x00004000
#define MACALT  0x00008000
#define CALL    0x00010000
#define RETURN  0x00020000
#define SAVE    0x00040000
#define CTLCARD 0x00100000

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

#define NUMOPS 129

/* Opcode types */

enum optypes
{
   TYPE_A=1,	/* O DD AAAAAA */
   TYPE_B,
   TYPE_C,
   TYPE_D,
   TYPE_P  	/* Pseudo op */
};

/* Pseudo ops */

enum psops
{
   ACON4_T=1, ACON5_T, ACON6_T, ADCON_T, ALTSW_T, BITCD_T, CHRCD_T,
   CON_T,     EJECT_T, END_T,   FPN_T,   INCL_T,  LASN_T,  LITOR_T,
   MACRO_T,   MEND_T,  MODE_T,  NAME_T,  RCD_T,   RPT_T,   SASN_T,
   SUBOR_T,   SWN_T,   SWT_T,   TCD_T,   TITLE_T, TRANS_T
};

/* OpCode table definition */

typedef struct
{
   char *opcode;
   int opvalue;
   int opmod;
   int optype;
   int model;
} OpCode;

/* Macro expansion lines */

typedef struct Mac_Lines
{
   struct Mac_Lines *next;
   char label[MAXFIELDSIZE];
   char opcode[MAXFIELDSIZE];
   char numfield[MAXFIELDSIZE];
   char operand[MAXLINE];
} MacLines;

/* Macro template definition */

typedef struct
{
   char macname[MAXMACNAMELEN+2];
   int macstartline;
   int macusenum;
   int macnested;
   int macargcount;
   int maclinecount;
   int macnumfield;
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
   int swvalue;
   int length;
   int line;
} SymNode;

/* Symbol table flags */

#define RELOCATABLE	0x000000001
#define EXTERNAL	0x000000002
#define GLOBAL		0x000000004
#define NOEXPORT	0x000000008
#define SGLOBAL		0x000000010
#define P0SYM		0x000000020
#define CONVAR		0x000001000
#define FPNVAR		0x000002000
#define RPTVAR		0x000004000
#define RCDVAR		0x000008000
#define CHRCDVAR	0x000010000
#define BITCDVAR	0x000020000
#define NUMSACCESS	0x000040000
#define FLOATLITERAL	0x001000000
#define MIXEDLITERAL	0x002000000
#define SIGNEDLITERAL	0x004000000
#define ADCONLITERAL	0x008000000

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
** 705 System object format
*/

#define IDENTFIELD	0	/* Location of the Identification field */
#define SERIALFIELD	6	/* Location of the serial number field */
#define ADDRESSFIELD	9	/* Location of the load address field */
#define COLUMNFIELD	13	/* Location of the number of columns field */
#define INSTFIELD	15	/* Location of the instruction/data field */

#define IDENTLENGTH	6	/* Length of the Identification field */
#define SERIALLENGTH	3	/* Length of the serial number field */
#define ADDRESSLENGTH	4	/* Length of the load address field */
#define COLUMNLENGTH	2	/* Length of the number of columns field */
#define INSTLENGTH	5	/* Length of the instruction field */
#define ACON4LENGTH	4	/* Length of the ACON4 field */
#define ACON5LENGTH	5	/* Length of the ACON5 field */
#define ACON6LENGTH	6	/* Length of the ACON6 field */

#define CHARSPERREC	74	/* Chars per object record */
#define SEQUENCENUM	76	/* Where to put the sequence */

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

#include "asm7080.tok"

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
extern char *exprscan (char *, int *, char *, int);

extern tokval Parser (char *, int *, int, int);
extern toktyp Scanner (char *, int *, tokval *, char *, char *, int);

