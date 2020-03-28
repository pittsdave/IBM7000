/***********************************************************************
*
* disasm - Simple disassembler of binary IBM 7090 image files.
*
* Changes:
*   03/18/05   DGP   Original.
*   12/15/05   RMS   MINGW changes.
*   12/27/07   DGP   Add -m option and expand PSE/MSE support.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>

#include "sysdef.h"
#include "nativebcd.h"


#if defined(WIN32) && !defined(MINGW)
#define IOCP     0400000000000I64
#define IOCT     0500000000000I64
#define OP1MASK  0700000000000I64
#define OPAMASK  0300000000000I64
#define OPMASK   0777700000000I64
#define FLAGMASK 0000060000000I64
#define CCMASK   0000077000000I64
#define DECRMASK 0077777000000I64
#define TAGMASK  0000000700000I64
#define BOOLMASK 0000000777777I64
#define ADDRMASK 0000000077777I64
#else
#define IOCP     0400000000000ULL
#define IOCT     0500000000000ULL
#define OP1MASK  0700000000000ULL
#define OPAMASK  0300000000000ULL
#define OPMASK   0777700000000ULL
#define FLAGMASK 0000060000000ULL
#define CCMASK   0000077000000ULL
#define DECRMASK 0077777000000ULL
#define TAGMASK  0000000700000ULL
#define BOOLMASK 0000000777777ULL
#define ADDRMASK 0000000077777ULL
#endif

/* Opcode types */

enum optypes
{
   TYPE_A=1,	/* O DDDDD T AAAAA */
   TYPE_B,	/* OOOO FF T AAAAA */
   TYPE_C,	/* OOOO CC T AAAAA */
   TYPE_D,	/* OOOO FF  MMMMMM */
   TYPE_E	/* OOOO FF T OOOOO */
};

/* OpCode table definition */

typedef struct
{
   char *opcode;
   unsigned opvalue;
   unsigned opmod;
   int optype;
} OpCode;


/* MONSYM SysDefs table definition */

typedef struct
{
   char *name;
   unsigned val;
} SysDefs;

/*
** Standard opcodes
*/

#define NUMOPS 293

static OpCode optable[NUMOPS] =
{
   { "ACL",    00361,    0,     TYPE_B },
   { "ADD",    00400,    0,     TYPE_B },
   { "ADM",    00401,    0,     TYPE_B },
   { "ALS",    00767,    0,     TYPE_B },
   { "ANA",    04320,    0,     TYPE_B },
   { "ANS",    00320,    0,     TYPE_B },
   { "ARS",    00771,    0,     TYPE_B },
   { "AXC",    04774,    0,     TYPE_B },
   { "AXT",    00774,    0,     TYPE_B },
   { "BSF",    04764,    0,     TYPE_E },
   { "BSR",    00764,    0,     TYPE_E },
   { "BTT",    00760,    0,     TYPE_E },
   { "CAD",    04700,    0,     TYPE_B },
   { "CAL",    04500,    0,     TYPE_B },
   { "CAQ",    04114,    0,     TYPE_C },
   { "CAS",    00340,    0,     TYPE_B },
   { "CFF",    00760,    00030, TYPE_E },
   { "CHS",    00760,    00002, TYPE_E },
   { "CLA",    00500,    0,     TYPE_B },
   { "CLM",    00760,    00000, TYPE_E },
   { "CLS",    00502,    0,     TYPE_B },
   { "COM",    00760,    00006, TYPE_E },
   { "CPY",    00700,    0,     TYPE_B },
   { "CRQ",    04154,    0,     TYPE_C },
   { "CVR",    00114,    0,     TYPE_C },
   { "DCT",    00760,    00012, TYPE_E },
   { "DFAD",   00301,    0,     TYPE_B },
   { "DFAM",   00305,    0,     TYPE_B },
   { "DFDH",   04240,    0,     TYPE_B },
   { "DFDP",   04241,    0,     TYPE_B },
   { "DFMP",   00261,    0,     TYPE_B },
   { "DFSB",   00303,    0,     TYPE_B },
   { "DFSM",   00307,    0,     TYPE_B },
   { "DLD",    00443,    0,     TYPE_B },
   { "DST",    04603,    0,     TYPE_B },
   { "DUAM",   04305,    0,     TYPE_B },
   { "DUFA",   04301,    0,     TYPE_B },
   { "DUFM",   04261,    0,     TYPE_B },
   { "DUFS",   04303,    0,     TYPE_B },
   { "DUSM",   04307,    0,     TYPE_B },
   { "DVH",    00220,    0,     TYPE_B },
   { "DVP",    00221,    0,     TYPE_B },
   { "ECTM",   04760,    00006, TYPE_E },
   { "EFTM",   04760,    00002, TYPE_E },
   { "EMTM",   04760,    00016, TYPE_E },
   { "ENB",    00564,    0,     TYPE_B },
   { "ENK",    00760,    00004, TYPE_E },
   { "ERA",    00322,    0,     TYPE_B },
   { "ESNT",   04021,    0,     TYPE_B },
   { "ESTM",   04760,    00005, TYPE_E },
   { "ETM",    00760,    00007, TYPE_E },
   { "ETT",    04760,    0,     TYPE_E },
   { "FAD",    00300,    0,     TYPE_B },
   { "FAM",    00304,    0,     TYPE_B },
   { "FDH",    00240,    0,     TYPE_B },
   { "FDP",    00241,    0,     TYPE_B },
   { "FMP",    00260,    0,     TYPE_B },
   { "FRN",    00760,    00011, TYPE_E },
   { "FSB",    00302,    0,     TYPE_B },
   { "FSM",    00306,    0,     TYPE_B },
   { "HPR",    00420,    0,     TYPE_B },
   { "IIA",    00041,    0,     TYPE_B },
   { "IIL",    04051,    0,     TYPE_D },
   { "IIR",    00051,    0,     TYPE_D },
   { "IIS",    00440,    0,     TYPE_B },
   { "IOT",    00760,    00005, TYPE_E },
   { "LAC",    00535,    0,     TYPE_B },
   { "LAS",    04340,    0,     TYPE_B },
   { "LBT",    00760,    00001, TYPE_E },
   { "LCHA",   00544,    0,     TYPE_B },
   { "LCHB",   04544,    0,     TYPE_B },
   { "LCHC",   00545,    0,     TYPE_B },
   { "LCHD",   04545,    0,     TYPE_B },
   { "LCHE",   00546,    0,     TYPE_B },
   { "LCHF",   04546,    0,     TYPE_B },
   { "LCHG",   00547,    0,     TYPE_B },
   { "LCHH",   04547,    0,     TYPE_B },
   { "LDA",    00460,    0,     TYPE_B },
   { "LDC",    04535,    0,     TYPE_B },
   { "LDI",    00441,    0,     TYPE_B },
   { "LDQ",    00560,    0,     TYPE_B },
   { "LFT",    04054,    0,     TYPE_D },
   { "LFTM",   04760,    00004, TYPE_E },
   { "LGL",    04763,    0,     TYPE_B },
   { "LGR",    04765,    0,     TYPE_B },
   { "LLS",    00763,    0,     TYPE_B },
   { "LMTM",   00760,    00016, TYPE_E },
   { "LNT",    04056,    0,     TYPE_D },
   { "LRS",    00765,    0,     TYPE_B },
   { "LSNM",   04760,    00010, TYPE_E },
   { "LTM",    04760,    00007, TYPE_E },
   { "LXA",    00534,    0,     TYPE_B },
   { "LXD",    04534,    0,     TYPE_B },
   { "MON",    05000,    0,     TYPE_A },
   { "MPR",    04200,    0,     TYPE_B },
   { "MPY",    00200,    0,     TYPE_B },
   { "MSE",    04760,    00000, TYPE_E },
   { "NOP",    00761,    0,     TYPE_B },
   { "NZT",    04520,    0,     TYPE_B },
   { "OAI",    00043,    0,     TYPE_B },
   { "OFT",    00444,    0,     TYPE_B },
   { "ONT",    00446,    0,     TYPE_B },
   { "ORA",    04501,    0,     TYPE_B },
   { "ORS",    04602,    0,     TYPE_B },
   { "OSI",    00442,    0,     TYPE_B },
   { "PAC",    00737,    0,     TYPE_B },
   { "PAI",    00044,    0,     TYPE_B },
   { "PAX",    00734,    0,     TYPE_B },
   { "PBT",    04760,    00001, TYPE_E },
   { "PCA",    00756,    0,     TYPE_B }, 
   { "PCD",    04756,    0,     TYPE_B },
   { "PDC",    04737,    0,     TYPE_B },
   { "PDX",    04734,    0,     TYPE_B },
   { "PIA",    04046,    0,     TYPE_B },
   { "PSE",    00760,    00000, TYPE_E },
   { "PXA",    00754,    0,     TYPE_B },
   { "PXD",    04754,    0,     TYPE_B },
   { "PZE",    00000,    0,     TYPE_A },
   { "RCD",    00762,    00321, TYPE_E },
   { "RCHA",   00540,    0,     TYPE_B },
   { "RCHB",   04540,    0,     TYPE_B },
   { "RCHC",   00541,    0,     TYPE_B },
   { "RCHD",   04541,    0,     TYPE_B },
   { "RCHE",   00542,    0,     TYPE_B },
   { "RCHF",   04542,    0,     TYPE_B },
   { "RCHG",   00543,    0,     TYPE_B },
   { "RCHH",   04543,    0,     TYPE_B },
   { "RCT",    00760,    00014, TYPE_E },
   { "RDC",    00760,    00352, TYPE_E },
   { "RDR",    00762,    00300, TYPE_E },
   { "RDS",    00762,    0,     TYPE_E },
   { "REW",    00772,    0,     TYPE_B },
   { "RFT",    00054,    0,     TYPE_D },
   { "RIA",    04042,    0,     TYPE_D },
   { "RIC",    00760,    00350, TYPE_E },
   { "RIL",    04057,    0,     TYPE_D },
   { "RIR",    00057,    0,     TYPE_D },
   { "RIS",    00445,    0,     TYPE_B },
   { "RND",    00760,    00010, TYPE_E },
   { "RNT",    00056,    0,     TYPE_D },
   { "RPR",    00762,    00361, TYPE_E },
   { "RQL",    04773,    0,     TYPE_B },
   { "RSCA",   00540,    0,     TYPE_B },
   { "RSCB",   04540,    0,     TYPE_B },
   { "RSCC",   00541,    0,     TYPE_B },
   { "RSCD",   04541,    0,     TYPE_B },
   { "RSCE",   00542,    0,     TYPE_B },
   { "RSCF",   04542,    0,     TYPE_B },
   { "RSCG",   00543,    0,     TYPE_B },
   { "RSCH",   04543,    0,     TYPE_B },
   { "RTB",    00762,    00220, TYPE_E },
   { "RTD",    00762,    00200, TYPE_E },
   { "RUN",    04772,    00220, TYPE_E },
   { "SBM",    04400,    0,     TYPE_B },
   { "SCA",    00636,    0,     TYPE_B },
   { "SCD",    04636,    0,     TYPE_B },
   { "SCHA",   00640,    0,     TYPE_B },
   { "SCHB",   04640,    0,     TYPE_B },
   { "SCHC",   00641,    0,     TYPE_B },
   { "SCHD",   04641,    0,     TYPE_B },
   { "SCHE",   00642,    0,     TYPE_B },
   { "SCHF",   04642,    0,     TYPE_B },
   { "SCHG",   00643,    0,     TYPE_B },
   { "SCHH",   04643,    0,     TYPE_B },
   { "SDN",    00776,    0,     TYPE_B },
   { "SIL",    04055,    0,     TYPE_D },
   { "SIR",    00055,    0,     TYPE_D },
   { "SLF",    04760,    00140, TYPE_E },
   { "SLF",    00760,    00140, TYPE_E },
   { "SLN",    00760,    00141, TYPE_E },
   { "SLN",    00760,    00142, TYPE_E },
   { "SLN",    00760,    00143, TYPE_E },
   { "SLN",    00760,    00144, TYPE_E },
   { "SLQ",    04620,    0,     TYPE_B },
   { "SLT",    04760,    00141, TYPE_E },
   { "SLT",    04760,    00142, TYPE_E },
   { "SLT",    04760,    00143, TYPE_E },
   { "SLT",    04760,    00144, TYPE_E },
   { "SLW",    00602,    0,     TYPE_B },
   { "SPR",    00760,    00360, TYPE_E },
   { "SPT",    00760,    00360, TYPE_E },
   { "SPU",    00760,    00340, TYPE_E },
   { "SSM",    04760,    00003, TYPE_E },
   { "SSP",    00760,    00003, TYPE_E },
   { "STA",    00621,    0,     TYPE_B },
   { "STCA",   00544,    0,     TYPE_E },
   { "STCB",   04544,    0,     TYPE_E },
   { "STCC",   00545,    0,     TYPE_E },
   { "STCD",   04545,    0,     TYPE_E },
   { "STCE",   00546,    0,     TYPE_E },
   { "STCF",   04546,    0,     TYPE_E },
   { "STCG",   00547,    0,     TYPE_E },
   { "STCH",   04547,    0,     TYPE_E },
   { "STD",    00622,    0,     TYPE_B },
   { "STI",    00604,    0,     TYPE_B },
   { "STL",    04625,    0,     TYPE_B },
   { "STO",    00601,    0,     TYPE_B },
   { "STP",    00630,    0,     TYPE_B },
   { "STQ",    04600,    0,     TYPE_B },
   { "STT",    00625,    0,     TYPE_B },
   { "STZ",    00600,    0,     TYPE_B },
   { "SUB",    00402,    0,     TYPE_B },
   { "SWT",    00760,    00161, TYPE_E },
   { "SWT",    00760,    00162, TYPE_E },
   { "SWT",    00760,    00163, TYPE_E },
   { "SWT",    00760,    00164, TYPE_E },
   { "SWT",    00760,    00165, TYPE_E },
   { "SWT",    00760,    00166, TYPE_E },
   { "SWT",    04760,    00161, TYPE_E },
   { "SWT",    04760,    00162, TYPE_E },
   { "SWT",    04760,    00163, TYPE_E },
   { "SWT",    04760,    00164, TYPE_E },
   { "SWT",    04760,    00165, TYPE_E },
   { "SWT",    04760,    00166, TYPE_E },
   { "SXA",    00634,    0,     TYPE_B },
   { "SXD",    04634,    0,     TYPE_B },
   { "TCNA",   04060,    0,     TYPE_B },
   { "TCNB",   04061,    0,     TYPE_B },
   { "TCNC",   04062,    0,     TYPE_B },
   { "TCND",   04063,    0,     TYPE_B },
   { "TCNE",   04064,    0,     TYPE_B },
   { "TCNF",   04065,    0,     TYPE_B },
   { "TCNG",   04066,    0,     TYPE_B },
   { "TCNH",   04067,    0,     TYPE_B },
   { "TCOA",   00060,    0,     TYPE_B },
   { "TCOB",   00061,    0,     TYPE_B },
   { "TCOC",   00062,    0,     TYPE_B },
   { "TCOD",   00063,    0,     TYPE_B },
   { "TCOE",   00064,    0,     TYPE_B },
   { "TCOF",   00065,    0,     TYPE_B },
   { "TCOG",   00066,    0,     TYPE_B },
   { "TCOH",   00067,    0,     TYPE_B },
   { "TEFA",   00030,    0,     TYPE_B },
   { "TEFB",   04030,    0,     TYPE_B },
   { "TEFC",   00031,    0,     TYPE_B },
   { "TEFD",   04031,    0,     TYPE_B },
   { "TEFE",   00032,    0,     TYPE_B },
   { "TEFF",   04032,    0,     TYPE_B },
   { "TEFG",   00033,    0,     TYPE_B },
   { "TEFH",   04033,    0,     TYPE_B },
   { "TIF",    00046,    0,     TYPE_B },
   { "TIO",    00042,    0,     TYPE_B },
   { "TIX",    02000,    0,     TYPE_A },
   { "TLQ",    00040,    0,     TYPE_B },
   { "TMI",    04120,    0,     TYPE_B },
   { "TNO",    04140,    0,     TYPE_B },
   { "TNX",    06000,    0,     TYPE_A },
   { "TNZ",    04100,    0,     TYPE_B },
   { "TOV",    00140,    0,     TYPE_B },
   { "TPL",    00120,    0,     TYPE_B },
   { "TQO",    00161,    0,     TYPE_B },
   { "TQP",    00162,    0,     TYPE_B },
   { "TRA",    00020,    0,     TYPE_B },
   { "TRCA",   00022,    0,     TYPE_B },
   { "TRCB",   04022,    0,     TYPE_B },
   { "TRCC",   00024,    0,     TYPE_B },
   { "TRCD",   04024,    0,     TYPE_B },
   { "TRCE",   00026,    0,     TYPE_B },
   { "TRCF",   04026,    0,     TYPE_B },
   { "TRCG",   00027,    0,     TYPE_B },
   { "TRCH",   04027,    0,     TYPE_B },
   { "TSX",    00074,    0,     TYPE_B },
   { "TTR",    00021,    0,     TYPE_B },
   { "TXH",    03000,    0,     TYPE_A },
   { "TXI",    01000,    0,     TYPE_A },
   { "TXL",    07000,    0,     TYPE_A },
   { "TZE",    00100,    0,     TYPE_B },
   { "UAM",    04304,    0,     TYPE_B },
   { "UFA",    04300,    0,     TYPE_B },
   { "UFM",    04260,    0,     TYPE_B },
   { "UFS",    04302,    0,     TYPE_B },
   { "USM",    04306,    0,     TYPE_B },
   { "VDH",    00224,    0,     TYPE_C },
   { "VDP",    00225,    0,     TYPE_C },
   { "VLM",    00204,    0,     TYPE_C },
   { "WDR",    00766,    00300, TYPE_E },
   { "WEF",    00770,    00220, TYPE_E },
   { "WPB",    00766,    00362, TYPE_E },
   { "WPD",    00766,    00361, TYPE_E },
   { "WPR",    00766,    00361, TYPE_E },
   { "WPU",    00766,    00341, TYPE_E },
   { "WRS",    00766,    0,     TYPE_E },
   { "WTB",    00766,    00220, TYPE_E },
   { "WTD",    00766,    00200, TYPE_E },
   { "WTV",    00766,    00030, TYPE_E },
   { "XCA",    00131,    0,     TYPE_B },
   { "XCL",    04130,    0,     TYPE_B },
   { "XEC",    00522,    0,     TYPE_B },
   { "XIT",    00021,    0,     TYPE_B },
   { "ZAC",    04754,    0,     TYPE_B },
   { "ZET",    00520,    0,     TYPE_B },
   { "ZSA",    00634,    0,     TYPE_B },
   { "ZSD",    04634,    0,     TYPE_B },
};

/*
** IBSYS System nucleus defintions (MONSYM) and
** IBJOB defintions (JOBSYM).
*/

static SysDefs sysdefs[] =
{
   { "SYSTRA", 000100 },
   { "SYSDAT", 000101 },
   { "SYSCUR", 000102 },
   { "SYSRET", 000103 },
   { "SYSKEY", 000104 },
   { "SYSSWS", 000105 },
   { "SYSPOS", 000106 },
   { "SYSUNI", 000107 },
   { "SYSUBC", 000110 },
   { "SYSUAV", 000111 },
   { "SYSUCW", 000112 },
   { "SYSRPT", 000113 },
   { "SYSCEM", 000114 },
   { "SYSDMP", 000115 },
   { "SYSIOX", 000116 },
   { "SYSIDR", 000117 },
   { "SYSCOR", 000120 },
   { "SYSLDR", 000121 },
   { "SYSACC", 000122 },
   { "SYSPID", 000123 },
   { "SYSCYD", 000124 },
   { "SYSSLD", 000126 },
   { "SYSTCH", 000127 },
   { "SYSTWT", 000131 },
   { "SYSGET", 000132 },
   { "SYSJOB", 000133 },
   { ".CHEXI", 000134 },
   { ".MODSW", 000135 },
   { "SYSLB1", 000140 },
   { "SYSLB2", 000141 },
   { "SYSLB3", 000142 },
   { "SYSLB4", 000143 },
   { "SYSCRD", 000144 },
   { "SYSPRT", 000145 },
   { "SYSPCH", 000146 },
   { "SYSOU1", 000147 },
   { "SYSOU2", 000150 },
   { "SYSIN1", 000151 },
   { "SYSIN2", 000152 },
   { "SYSPP1", 000153 },
   { "SYSPP2", 000154 },
   { "SYSCK1", 000155 },
   { "SYSCK2", 000156 },
   { "SYSUT1", 000157 },
   { "SYSUT2", 000160 },
   { "SYSUT3", 000161 },
   { "SYSUT4", 000162 },
   { "SYSUT5", 000163 },
   { "SYSUT6", 000164 },
   { "SYSUT7", 000165 },
   { "SYSUT8", 000166 },
   { "SYSUT9", 000167 },
   { ".ACTV",  000702 },
   { ".NDSEL", 000704 },
   { ".MWR",   000706 },
   { ".PUNCH", 000707 },
   { ".ENBSW", 000710 },
   { ".PAWS",  000711 },
   { ".PAUSE", 000712 },
   { ".STOP",  000713 },
   { ".SYMUN", 000714 },
   { ".DECVD", 000715 },
   { ".DECVA", 000716 },
   { ".CKWAT", 000717 },
   { ".BCD5R", 000720 },
   { ".BCD5X", 000721 },
   { ".CVPRT", 000722 },
   { ".STOPD", 000723 },
   { ".CHXAC", 000724 },
   { ".URRX",  000725 },
   { ".RCTX",  000726 },
   { ".RCHX",  000727 },
   { ".TCOX",  000730 },
   { ".TRCX",  000731 },
   { ".ETTX",  000732 },
   { ".TEFX",  000733 },
   { ".TRAPX", 000734 },
   { ".TRAPS", 000735 },
   { ".COMM",  000736 },
   { ".LTPOS", 000737 },
   { ".IOXSI", 000740 },
   { ".CHPSW", 000741 },
   { ".TRPSW", 000742 },
   { ".FDAMT", 000743 },
   { ".SDCXI", 000744 },
   { ".STCXI", 000745 },
   { ".COMMD", 000746 },
   { ".IBCDZ", 000747 },
   { ".CHXSP", 000750 },
   { ".BLKSW", 000751 },
   { "SYSORG", 002652 },
   { "SYSLOC", 021234 },
   { "SYSFAZ", 021235 },
   { "IBJCOR", 021236 },
   { "IBJDAT", 021237 },
   { ".JLDAT", 021240 },
   { ".JTYPE", 021242 },
   { ".JLIN",  021243 },
   { ".JVER",  021244 },
   { ".JKAPU", 021245 },
   { "SYSDSB", 021246 },
   { ".FDPOS", 021250 },
   { "SSTRA",  021253 },
   { "ACTION", 021254 },
   { "JOBIN",  021255 },
   { "JOBOU",  021256 },
   { "JOBPP",  021257 },
   { "IOEDIT", 021260 },
   { "JREEL",  021261 },
   { "SUBSP",  021262 },
   { "PUNCH",  021263 },
   { "SYSSHD", 021264 },
   { "LILDMP", 021265 },
   { "IBSLB",  021266 },
   { "PRSW",   021267 },
   { "DEFINE", 021347 },
   { "JOIN",   021351 },
   { "ATTACH", 021353 },
   { "CLOSE",  021355 },
   { "OPEN",   021357 },
   { "READ",   021361 },
   { "WRITE",  021363 },
   { "STASH",  021365 },
   { "SUBSYS", 021412 },
   { "SYSEND", 077777 },
   { "",       -1     }
};

static int monsym;
static int recmode;
static int recnum;
static int filemode;
static int filenum;
static char sym[12];

/***********************************************************************
* readword - read a word.
***********************************************************************/

static t_uint64
readword (FILE *fd)
{
   t_uint64 word;
   int i;
   int ch;

   word = 0;

   if (filenum)
   {
      ch = fgetc (fd);
      while (TRUE)
      {
	 ch = fgetc (fd);
	 if (ch == EOF) return -1;
	 if (ch == 0217)
	 {
	    if (--filenum == 0)
	    {
	       break;
	    }
	 }
      }
   }

   if (recnum)
   {
      ch = fgetc (fd);
      while (TRUE)
      {
	 ch = fgetc (fd);
	 if (ch == EOF) return -1;
	 if (ch & 0200)
	 {
	    if (--recnum == 0)
	    {
	       ungetc (ch & 0177, fd);
	       break;
	    }
	 }
      }
   }

   for (i = 0; i < 6; i++)
   {
      ch = fgetc (fd);
      if (ch == EOF) return -1;
      if (recmode && (ch & 0200)) return -1;
      word = word << 6 | (t_uint64)(ch & 077);
   }
#ifdef DEBUG
   printf ("* readword: word = %12.12llo\n", word);
#endif

   return word;
}

/***********************************************************************
* oplookup - Load opcode.
***********************************************************************/

static OpCode *
oplookup (unsigned opc, unsigned opm)
{
   int i;

#ifdef DEBUG
   printf ("* oplookup: op = %4.4o, opm = %o\n", opc, opm);
#endif

   if ((opc & 03777) == 00760)
      opm &= 000777;

   for (i = 0; i < NUMOPS; i++)
   {
      if (optable[i].opvalue == opc && optable[i].opmod == opm)
	 return (&optable[i]);
   }
   return (NULL);
}

/***********************************************************************
* allbcd - Check word for valid BCD chacters.
***********************************************************************/

static int
isallbcd (t_uint64 word)
{
   int i;

   for (i = 0; i < 6; i++)
   {
      if (!isbcd[word & 077]) return FALSE;
      word >>= 6;
   }
   return TRUE;
}

/***********************************************************************
* outputbcd - Output word as a BCI pseudo op.
***********************************************************************/

static void
outputbcd (t_uint64 word)
{
   int i;

   printf ("       BCI     1,");
   for (i = 0; i < 6; i++)
   {
      fputc (tonative[(uint8)((word >> 30) & 077)], stdout);
      word <<= 6;
   }
}

/***********************************************************************
* checksym - Check if in sysdefs symbols.
***********************************************************************/

static int
checksym (unsigned addr)
{
   int i;

   if (monsym)
   {
      for (i = 0; sysdefs[i].val > 0; i++)
      {
	 if (sysdefs[i].val == addr)
	 {
	    strcpy (sym, sysdefs[i].name);
	    return TRUE;
	 }
      }
   }
   sprintf (sym, "%d", addr);
   return FALSE;
}

/***********************************************************************
* loadprog - Load Program and generate code.
***********************************************************************/

static void
loadprog (FILE *infd)
{
   OpCode *pop;   
   t_uint64 ctlword;
   t_uint64 word;
   unsigned op, flag, tag, addr, decr;
   unsigned bool, cc;
   unsigned ldaddr;
   unsigned wrdcnt;
   int done;
   int i;

   done = FALSE;
   while (!done)
   {
      ctlword = readword (infd);
      if (ctlword == -1)
	 done = TRUE;
      else if ((ctlword & OP1MASK) == IOCT)
         done = TRUE;
      else
      {
         ldaddr = (int)(ctlword & ADDRMASK);
         wrdcnt = (int)((ctlword >> DECRSHIFT) & ADDRMASK);
	 checksym (ldaddr);
	 printf ("       ORG     %s\n", sym);
#ifdef DEBUG
	 printf ("* loadmem: ldaddr = %05o, wrdcnt = %05o(%d)\n",
	       ldaddr, wrdcnt, (int)wrdcnt);
#endif
	 for (i = 0; i < (int)wrdcnt; i++)
	 {
	    char genline[82];
	    char genlabel[8];
	    char genarg[32];

	    word = readword (infd);
	    if (word == -1)
	    {
	       done = TRUE;
	       break;
	    }

	    genlabel[0] = '\0';
#if 0
	    if (checksym (ldaddr))
	       strcpy (genlabel, sym);
#endif

	    op   = (unsigned)((word & OPMASK) >> OPSHIFT);
	    tag  = (unsigned)((word & TAGMASK) >> TAGSHIFT);
	    addr = (unsigned)(word & ADDRMASK);
	    checksym (addr);

	    if (op == 0 || word & OPAMASK)
	    {
	       pop = oplookup (op & 07000, 0);
	       if (pop)
	       {
		  decr = (unsigned)((word & DECRMASK) >> DECRSHIFT);
		  if (tag)
		     sprintf (genline, "%-6.6s %-7.7s %s,%d,%d",
			     genlabel, pop->opcode, sym, tag, decr);
		  else
		     sprintf (genline, "%-6.6s %-7.7s %s,,%d",
			     genlabel, pop->opcode, sym, decr);
	       }
	    }
	    else
	    {
	       flag = (unsigned)((word & FLAGMASK) >> FLAGSHIFT);
	       cc = (unsigned)((word & CCMASK) >> DECRSHIFT);
	       if ((op & 03777) == 00760)
	          pop = oplookup (op, addr);
	       else
		  pop = oplookup (op, 0);

	       if (pop)
	       {
		  char opc[12];

		  sprintf (opc, "%s%c",
			   pop->opcode, (flag == 060) ? '*' : ' ');

		  switch (pop->optype)
		  {
		  case TYPE_B:
		     if (flag == 060)
			cc &= 017;
		     else
			cc &= 037;

		     sprintf (genline, "%-6.6s %-7.7s %s,",
			      genlabel, opc, sym);
		     if (tag)
		     {
			sprintf (genarg, "%d", tag);
			strcat (genline, genarg);
		     }
		     if (cc)
		     {
			sprintf (genarg, ",%d", cc);
			strcat (genline, genarg);
		     }
		     break;

	          case TYPE_C:
		     sprintf (genline, "%-6.6s %-7.7s %s,",
			      genlabel, opc, sym);
		     if (tag)
		     {
			sprintf (genarg, "%d", tag);
			strcat (genline, genarg);
		     }
		     sprintf (genarg, ",%d", cc);
		     strcat (genline, genarg);
		     break;

	          case TYPE_D:
		     bool = (unsigned)(word & BOOLMASK);
		     sprintf (genline, "%-6.6s %-7.7s %o,",
			      genlabel, opc, bool);
		     break;

		  case TYPE_E:
		     if ((op & 03777) == 0760)
		     {
			if (addr <= 0140)
			{
			   sprintf (genline, "%-6.6s %-7.7s",
				    genlabel, opc);
			}
		        if (addr < 0170)
			{
			   sprintf (genline, "%-6.6s %-7.7s %d",
				    genlabel, opc, addr & 07);
			}
			else
			{
			   char temp[10];
			   sprintf (temp, "%s%c", opc, 'A'+((addr >> 9) - 1));
			   sprintf (genline, "%-6.6s %-7.7s",
				    genlabel, temp);
			}
		     }
		     else
		     {
			sprintf (genline, "%-6.6s %-7.7s %s,",
				 genlabel, opc, sym);
		     }
		     if (tag)
		     {
			sprintf (genarg, "%d", tag);
			strcat (genline, genarg);
		     }
		  }
	       }
	    }
	    if (!pop)
	    {
#if defined(WIN32)
	       sprintf (genline, "%-6.6s OCT     %12.12I64o", genlabel, word);
#else
	       sprintf (genline, "%-6.6s OCT     %12.12llo", genlabel, word);
#endif
	    }
	    printf ("%-30.30s", genline);
	    if (isallbcd (word))
	    {
	       outputbcd (word);
	    }
	    printf ("\n");
	    ldaddr++;
	 }
      }
   }
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *infd;
   char *infile;
   char *bp;
   int status;
   int i;

   /*
   ** Process command line arguments
   */

   infile = NULL;
   status = 0;
   recnum = 0;
   monsym = FALSE;
   recmode = FALSE;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
	 bp++;
	 switch (*bp)
	 {
	 case 'f':
	    filemode = TRUE;
	    bp++;
	    filenum = atoi(bp);
	    break;

	 case 'm':
	    monsym = TRUE;
	    break;

	 case 'r':
	    recmode = TRUE;
	    bp++;
	    recnum = atoi(bp);
	    break;

	 default:
	    goto usage;
	 }
      }
      else if (infile == NULL)
         infile = argv[i];
      else
      {
      usage:
         fprintf (stderr, "usage: disasm [-m] infile \n");
	 exit (1);
      }
   }

   if (infile == NULL ) goto usage;
#ifdef DEBUG
   printf ("* disasm: infile = %s\n", infile);
#endif

   /*
   ** Open the files.
   */

   if ((infd = fopen (infile, "rb")) == NULL)
   {
      fprintf (stderr, "disasm: input open failed: %s\n",
	       strerror (errno));
      fprintf (stderr, "filename: %s\n", infile);
      exit (1);
   }

   printf ("* Dissassembly of %s", infile);
   if (filemode)
      printf (", file %d", filenum);
   if (recmode)
      printf (", record %d", recnum);
   putchar ('\n');

   loadprog (infd);

   printf ("       END \n");
   
   fclose (infd);

   return status;

}
