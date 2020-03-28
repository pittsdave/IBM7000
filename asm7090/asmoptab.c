/***********************************************************************
*
* asmoptab.c - Opcode table for the IBM 7090 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   12/08/04   DGP   Started adding MAP pseudo ops.
*   02/02/05   DGP   Added some missing opcodes, THREE, SEVEN & ZERO.
*   03/01/05   DGP   Change IIA inst type.
*   03/25/05   DGP   Added 704 model and instructions.
*   06/07/05   DGP   Added CTSS (7094) opcodes, RMT pseudo op and TAPENO.
*   03/26/06   DGP   Correct REW[A-H] opcode.
*   04/27/10   DGP   Change PIA inst type.
*   05/05/10   DGP   Correct RUN/WEF/REW/BSR modifiers.
*   05/10/10   DGP   Added opcore ACORE/BCORE support.
*   12/15/10   DGP   Tighten up opcode mode (added optypes XXX_OP)
*   03/30/11   DGP   Fixed TAPENO 'U' processing.
*   05/13/11   DGP   Allow OPx, where x is a BOOL.
*   12/02/11   DGP   Added LINK opcode.
*   05/07/13   DGP   Added SCDA..SCDH opcodes.
*	
***********************************************************************/

#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include "asmdef.h"
#include "asmdmem.h"

/*
** Standard opcodes
*/

OpCode optable[NUMOPS] =
{
#if !defined(USS) && !defined(OS390)
   { "12BIT",  BIT12_T,  0,     FAP_OP,     TYPE_P,     7096 },
#endif
   { "ABS",    ABS_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "ACL",    00361,    0,     ALL_OP,     TYPE_B,     704  },
   { "ACORE",  ACORE_T,  0,     FAP_OP,     TYPE_P,     7096 },
   { "ADD",    00400,    0,     ALL_OP,     TYPE_B,     704  },
   { "ADM",    00401,    0,     ALL_OP,     TYPE_B,     704  },
   { "ALS",    00767,    0,     ALL_OP,     TYPE_B,     704  },
   { "ANA",    04320,    0,     ALL_OP,     TYPE_B,     704  },
   { "ANS",    00320,    0,     ALL_OP,     TYPE_B,     704  },
   { "ARS",    00771,    0,     ALL_OP,     TYPE_B,     704  },
   { "AXC",    04774,    0,     ALL_OP,     TYPE_B,     709  },
   { "AXT",    00774,    0,     ALL_OP,     TYPE_B,     709  },
   { "BCD",    BCD_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "BCI",    BCI_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "BCORE",  BCORE_T,  0,     FAP_OP,     TYPE_P,     7096 },
   { "BEGIN",  BEGIN_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "BES",    BES_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "BFT",    BFT_T,    0,     MAP_OP,     TYPE_P,     709  },
   { "BLK",    02000,    0,     FAP_OP,     TYPE_A,     7096 },
   { "BNT",    BNT_T,    0,     MAP_OP,     TYPE_P,     709  },
   { "BOOL",   BOOL_T,   0,     ALL_OP,     TYPE_P,     704  },
   { "BRA",    07000,    0,     ALL_OP,     TYPE_A,     704  },
   { "BRN",    03000,    0,     ALL_OP,     TYPE_A,     704  },
   { "BSF",    04764,    0,     ALL_OP,     TYPE_E,     709  },
   { "BSR",    00764,    00200, ALL_OP,     TYPE_E,     709  },
   { "BSS",    BSS_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "BST",    00764,    00200, ALL_OP,     TYPE_E,     704  },
   { "BTT",    00760,    0,     ALL_OP,     TYPE_E,     709  },
   { "CAD",    04700,    0,     ALL_OP,     TYPE_B,     704  },
   { "CAL",    04500,    0,     ALL_OP,     TYPE_B,     704  },
   { "CALL",   CALL_T,   0,     ALL_OP,     TYPE_P,     704  },
   { "CAQ",    04114,    0,     ALL_OP,     TYPE_C,     709  },
   { "CAS",    00340,    0,     ALL_OP,     TYPE_B,     704  },
   { "CFF",    00760,    00030, ALL_OP,     TYPE_E,     704  },
   { "CHS",    00760,    00002, ALL_OP,     TYPE_E,     704  },
   { "CLA",    00500,    0,     ALL_OP,     TYPE_B,     704  },
   { "CLM",    00760,    00000, ALL_OP,     TYPE_E,     704  },
   { "CLS",    00502,    0,     ALL_OP,     TYPE_B,     704  },
   { "COM",    00760,    00006, ALL_OP,     TYPE_E,     704  },
   { "COMBES", COMBES_T, 0,     FAP_OP,     TYPE_P,     7096 },
   { "COMBSS", COMBSS_T, 0,     FAP_OP,     TYPE_P,     7096 },
   { "COMMON", COMMON_T, 0,     ALL_OP,     TYPE_P,     704  },
   { "CONTRL", CONTRL_T, 0,     ALL_OP,     TYPE_P,     704  },
   { "COUNT",  COUNT_T,  0,     FAP_OP,     TYPE_P,     704  },
   { "CPY",    00700,    0,     ALL_OP,     TYPE_B,     704  },
   { "CPYD",   05000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "CPYP",   04000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "CRQ",    04154,    0,     ALL_OP,     TYPE_C,     709  },
   { "CTL",    02000,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "CTLN",   02200,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "CTLR",   02000,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "CTLRN",  02200,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "CTLW",   02400,    00,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "CTLWN",  02600,    00,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "CVR",    00114,    0,     ALL_OP,     TYPE_C,     709  },
   { "DCT",    00760,    00012, ALL_OP,     TYPE_E,     704  },
   { "DEBM",   01210,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DEC",    DEC_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "DETAIL", DETAIL_T, 0,     ALL_OP,     TYPE_P,     704  },
   { "DFAD",   00301,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DFAM",   00305,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DFDH",   04240,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DFDP",   04241,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DFMP",   00261,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DFSB",   00303,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DFSM",   00307,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DLD",    00443,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DNOP",   01212,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DREL",   01204,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DSAI",   01007,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DSBM",   01211,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DSEK",   01012,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DST",    04603,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DUAM",   04305,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DUFA",   04301,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DUFM",   04261,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DUFS",   04303,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DUP",    DUP_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "DUSM",   04307,    0,     ALL_OP,     TYPE_B,     7094 },
   { "DVCY",   01005,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DVH",    00220,    0,     ALL_OP,     TYPE_B,     704  },
   { "DVHA",   01011,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DVP",    00221,    0,     ALL_OP,     TYPE_B,     704  },
   { "DVSR",   01002,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DVTA",   01010,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DVTN",   01004,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DWRC",   01006,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "DWRF",   01003,    01212, ALL_OP,     TYPE_DISK,  709  },
   { "EAD",    00671,    0,     ALL_OP,     TYPE_B,     704  },
   { "EAXM",   00760,    00016, ALL_OP,     TYPE_E,     709  },
   { "ECA",    00561,    0,     ALL_OP,     TYPE_B,     704  },
   { "ECQ",    04561,    0,     ALL_OP,     TYPE_B,     704  },
   { "ECTM",   04760,    00006, BCORE_BAD,  TYPE_E,     709  },
   { "EDP",    00672,    0,     ALL_OP,     TYPE_B,     704  },
   { "EFA",    00761,    0,     FAP_OP,     TYPE_B,     7096 },
   { "EFT",    04761,    00044, FAP_OP,     TYPE_E,     7096 },
   { "EFTM",   04760,    00002, ALL_OP,     TYPE_E,     709  },
   { "EJECT",  EJE_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "ELD",    00670,    0,     ALL_OP,     TYPE_B,     704  },
   { "EMP",    00673,    0,     ALL_OP,     TYPE_B,     704  },
   { "EMTM",   04760,    00016, ALL_OP,     TYPE_E,     7094 },
   { "ENB",    00564,    0,     ALL_OP,     TYPE_B,     709  },
   { "END",    END_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "ENDM",   ENDM_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "ENDQ",   ENDQ_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "ENK",    00760,    00004, ALL_OP,     TYPE_E,     709  },
   { "ENTRY",  ENT_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "EQU",    EQU_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "ERA",    00322,    0,     ALL_OP,     TYPE_B,     709  },
   { "ESB",    04671,    0,     ALL_OP,     TYPE_B,     704  },
   { "ESM",    04761,    00040, ALL_OP,     TYPE_E,     709  },
   { "ESNT",   04021,    0,     ALL_OP,     TYPE_B,     7094 },
   { "EST",    04673,    0,     ALL_OP,     TYPE_B,     704  },
   { "ESTM",   04760,    00005, BCORE_BAD,  TYPE_E,     709  },
   { "ETC",    ETC_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "ETM",    00760,    00007, BCORE_BAD,  TYPE_E,     704  },
   { "ETT",    04760,    0,     ALL_OP,     TYPE_E,     704  },
   { "EVEN",   EVEN_T,   0,     ALL_OP,     TYPE_P,     704  },
   { "EXTERN", EXT_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "FAD",    00300,    0,     ALL_OP,     TYPE_B,     704  },
   { "FAM",    00304,    0,     ALL_OP,     TYPE_B,     709  },
   { "FDH",    00240,    0,     ALL_OP,     TYPE_B,     704  },
   { "FDP",    00241,    0,     ALL_OP,     TYPE_B,     704  },
   { "FILE",   FILE_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "FIVE",   05000,    0,     ALL_OP,     TYPE_A,     704  },
   { "FMP",    00260,    0,     ALL_OP,     TYPE_B,     704  },
   { "FOR",    04000,    0,     ALL_OP,     TYPE_A,     704  },
   { "FOUR",   04000,    0,     ALL_OP,     TYPE_A,     704  },
   { "FRN",    00760,    00011, ALL_OP,     TYPE_E,     709  },
   { "FSB",    00302,    0,     ALL_OP,     TYPE_B,     704  },
   { "FSM",    00306,    0,     ALL_OP,     TYPE_B,     709  },
   { "FUL",    FUL_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "FVE",    05000,    0,     ALL_OP,     TYPE_A,     704  },
   { "GOTO",   GOTO_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "HEAD",   HEAD_T,   0,     FAP_OP,     TYPE_P,     704  },
   { "HED",    HED_T,    0,     FAP_OP,     TYPE_P,     704  },
   { "HPR",    00420,    0,     ALL_OP,     TYPE_B,     704  },
   { "HTR",    00000,    0,     ALL_OP,     TYPE_B,     704  },
   { "ICC",    07000,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "IFF",    IFF_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "IFT",    IFT_T,    0,     MAP_OP,     TYPE_P,     704  },
   { "IIA",    00041,    0,     ALL_OP,     TYPE_B,     709  },
   { "IIB",    IIB_T,    0,     MAP_OP,     TYPE_P,     709  },
   { "IIL",    04051,    0,     ALL_OP,     TYPE_D,     709  },
   { "IIR",    00051,    0,     ALL_OP,     TYPE_D,     709  },
   { "IIS",    00440,    0,     ALL_OP,     TYPE_B,     709  },
   { "INDEX",  INDEX_T,  0,     ALL_OP,     TYPE_P,     704  },
   { "INSERT", INSERT_T, 0,     FAP_OP,     TYPE_P,     7096 },
   { "IOCD",   00000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IOCDN",  00000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IOCP",   04000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IOCPN",  04000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IOCT",   05000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IOCTN",  05000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IOD",    00766,    00333, ALL_OP,     TYPE_E,     704  },
   { "IORP",   02000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IORPN",  02000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IORT",   03000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IORTN",  03000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IOSP",   06000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IOSPN",  06000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IOST",   07000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "IOSTN",  07000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "IOT",    00760,    00005, ALL_OP,     TYPE_E,     709  },
   { "IRP",    IRP_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "KEEP",   KEEP_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "LABEL",  LABEL_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "LAC",    00535,    0,     ALL_OP,     TYPE_B,     709  },
   { "LAR",    03000,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "LAS",    04340,    0,     ALL_OP,     TYPE_B,     709  },
   { "LAXM",   04760,    00016, ALL_OP,     TYPE_E,     709  },
   { "LBL",    LBL_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "LBOOL",  LBOOL_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "LBT",    00760,    00001, ALL_OP,     TYPE_E,     704  },
   { "LCC",    06400,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "LCHA",   00544,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHB",   04544,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHC",   00545,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHD",   04545,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHE",   00546,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHF",   04546,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHG",   00547,    0,     ALL_OP,     TYPE_B,     709  },
   { "LCHH",   04547,    0,     ALL_OP,     TYPE_B,     709  },
   { "LDA",    00460,    0,     ALL_OP,     TYPE_B,     704  },
   { "LDC",    04535,    0,     ALL_OP,     TYPE_B,     709  },
   { "LDI",    00441,    0,     ALL_OP,     TYPE_B,     709  },
   { "LDIR",   LDIR_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "LDQ",    00560,    0,     ALL_OP,     TYPE_B,     704  },
   { "LFT",    04054,    0,     ALL_OP,     TYPE_D,     709  },
   { "LFTM",   04760,    00004, ALL_OP,     TYPE_E,     709  },
   { "LGL",    04763,    0,     ALL_OP,     TYPE_B,     704  },
   { "LGR",    04765,    0,     ALL_OP,     TYPE_B,     709  },
   { "LINK",   LINK_T,   0,     FAP_OP,     TYPE_P,     7096 },
   { "LIP",    06000,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "LIPT",   01000,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "LIST",   LIST_T,   0,     ALL_OP,     TYPE_P,     704  },
   { "LIT",    LIT_T,    0,     MAP_OP,     TYPE_P,     704  },
   { "LITORG", LITORG_T, 0,     MAP_OP,     TYPE_P,     704  },
   { "LLS",    00763,    0,     ALL_OP,     TYPE_B,     704  },
   { "LMTM",   00760,    00016, ALL_OP,     TYPE_E,     7094 },
   { "LNT",    04056,    0,     ALL_OP,     TYPE_D,     709  },
   { "LOC",    LOC_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "LORG",   LORG_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "LPI",    04564,    0,     FAP_OP,     TYPE_B,     7096 },
   { "LRI",    00562,    0,     FAP_OP,     TYPE_B,     7096 },
   { "LRS",    00765,    0,     ALL_OP,     TYPE_B,     704  },
   { "LSNM",   04760,    00010, ALL_OP,     TYPE_E,     709  },
   { "LSTNG",  LSTNG_T,  0,     FAP_OP,     TYPE_P,     7096 },
   { "LTM",    04760,    00007, ALL_OP,     TYPE_E,     704  },
   { "LXA",    00534,    0,     ALL_OP,     TYPE_B,     704  },
   { "LXD",    04534,    0,     ALL_OP,     TYPE_B,     704  },
   { "MACRO",  MACRO_T,  0,     ALL_OP,     TYPE_P,     704  },
   { "MACRON", MACRON_T, 0,     FAP_OP,     TYPE_P,     7096 },
   { "MAX",    MAX_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "MIN",    MIN_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "MON",    05000,    0,     ALL_OP,     TYPE_A,     704  },
   { "MPR",    04200,    0,     ALL_OP,     TYPE_B,     704  },
   { "MPY",    00200,    0,     ALL_OP,     TYPE_B,     704  },
   { "MSE",    04760,    00000, ALL_OP,     TYPE_E,     704  },
   { "MTH",    07000,    0,     ALL_OP,     TYPE_A,     704  },
   { "MTW",    06000,    0,     ALL_OP,     TYPE_A,     704  },
   { "MZE",    04000,    0,     ALL_OP,     TYPE_A,     704  },
   { "NOCRS",  NOCRS_T,  0,     ALL_OP,     TYPE_P,     704  },
   { "NOLNK",  NOLNK_T,  0,     FAP_OP,     TYPE_P,     7096 },
   { "NOP",    00761,    0,     ALL_OP,     TYPE_B,     704  },
   { "NTR",    01000,    0,     ALL_OP,     TYPE_A,     704  },
   { "NULL",   NULL_T,   0,     ALL_OP,     TYPE_P,     704  },
   { "NZT",    04520,    0,     ALL_OP,     TYPE_B,     709  },
   { "OAI",    00043,    0,     ALL_OP,     TYPE_B,     709  },
   { "OCT",    OCT_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "OFT",    00444,    0,     ALL_OP,     TYPE_B,     709  },
   { "ONE",    01000,    0,     ALL_OP,     TYPE_A,     704  },
   { "ONT",    00446,    0,     ALL_OP,     TYPE_B,     709  },
   { "OPD",    OPD_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "OPSYN",  OPSYN_T,  0,     ALL_OP,     TYPE_P,     704  },
   { "OPVFD",  OPVFD_T,  0,     ALL_OP,     TYPE_P,     704  },
   { "ORA",    04501,    0,     ALL_OP,     TYPE_B,     704  },
   { "ORG",    ORG_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "ORGCRS", ORGCRS_T, 0,     ALL_OP,     TYPE_P,     704  },
   { "ORS",    04602,    0,     ALL_OP,     TYPE_B,     704  },
   { "OSI",    00442,    0,     ALL_OP,     TYPE_B,     709  },
   { "PAC",    00737,    0,     ALL_OP,     TYPE_B,     709  },
   { "PAI",    00044,    0,     ALL_OP,     TYPE_B,     709  },
   { "PAR",    03000,    0,     FAP_OP,     TYPE_A,     7096 },
   { "PAX",    00734,    0,     ALL_OP,     TYPE_B,     704  },
   { "PBT",    04760,    00001, ALL_OP,     TYPE_E,     704  },
   { "PCA",    00756,    0,     ALL_OP,     TYPE_B,     7094 }, 
   { "PCC",    PCC_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "PCD",    04756,    0,     ALL_OP,     TYPE_B,     7094 },
   { "PCG",    PCG_T,    0,     MAP_OP,     TYPE_P,     704  },
   { "PDC",    04737,    0,     ALL_OP,     TYPE_B,     709  },
   { "PDX",    04734,    0,     ALL_OP,     TYPE_B,     704  },
   { "PIA",    04046,    0,     ALL_OP,     TYPE_B,     709  },
   { "PMC",    PMC_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "PON",    01000,    0,     ALL_OP,     TYPE_A,     704  },
   { "PSE",    00760,    00000, ALL_OP,     TYPE_E,     704  },
   { "PSLA",   00664,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLB",   04664,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLC",   00665,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLD",   04665,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLE",   00666,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLF",   04666,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLG",   00667,    0,     ALL_OP,     TYPE_B,     709  },
   { "PSLH",   04667,    0,     ALL_OP,     TYPE_B,     709  },
   { "PTH",    03000,    0,     ALL_OP,     TYPE_A,     704  },
   { "PTW",    02000,    0,     ALL_OP,     TYPE_A,     704  },
   { "PUNCH",  PUNCH_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "PURGE",  PURGE_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "PXA",    00754,    0,     ALL_OP,     TYPE_B,     709  },
   { "PXD",    04754,    0,     ALL_OP,     TYPE_B,     704  },
   { "PZE",    00000,    0,     ALL_OP,     TYPE_A,     704  },
   { "QUAL",   QUAL_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "RBOOL",  RBOOL_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "RCD",    00762,    00321, ALL_OP,     TYPE_E,     704  },
   { "RCHA",   00540,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHB",   04540,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHC",   00541,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHD",   04541,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHE",   00542,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHF",   04542,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHG",   00543,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCHH",   04543,    0,     ALL_OP,     TYPE_B,     709  },
   { "RCT",    00760,    00014, ALL_OP,     TYPE_E,     709  },
   { "RDC",    00760,    00352, ALL_OP,     TYPE_E,     709  },
   { "RDDD",   00762,    04240, FAP_OP,     TYPE_E,     7096 },
   { "RDR",    00762,    00300, ALL_OP,     TYPE_E,     704  },
   { "RDS",    00762,    0,     ALL_OP,     TYPE_E,     704  },
   { "REF",    REF_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "REM",    REM_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "RETURN", RETURN_T, 0,     MAP_OP,     TYPE_P,     704  },
   { "REW",    00772,    00200, ALL_OP,     TYPE_E,     704  },
   { "RFT",    00054,    0,     ALL_OP,     TYPE_D,     709  },
   { "RIA",    04042,    0,     ALL_OP,     TYPE_B,     709  },
   { "RIB",    RIB_T,    0,     MAP_OP,     TYPE_P,     709  },
   { "RIC",    00760,    00350, ALL_OP,     TYPE_E,     709  },
   { "RIL",    04057,    0,     ALL_OP,     TYPE_D,     709  },
   { "RIR",    00057,    0,     ALL_OP,     TYPE_D,     709  },
   { "RIS",    00445,    0,     ALL_OP,     TYPE_B,     709  },
   { "RMT",    RMT_T,    0,     FAP_OP,     TYPE_P,     704  },
   { "RND",    00760,    00010, ALL_OP,     TYPE_E,     704  },
   { "RNT",    00056,    0,     ALL_OP,     TYPE_D,     709  },
   { "RPR",    00762,    00361, ALL_OP,     TYPE_E,     704  },
   { "RQL",    04773,    0,     ALL_OP,     TYPE_B,     704  },
   { "RSCA",   00540,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCB",   04540,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCC",   00541,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCD",   04541,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCE",   00542,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCF",   04542,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCG",   00543,    0,     ALL_OP,     TYPE_B,     709  },
   { "RSCH",   04543,    0,     ALL_OP,     TYPE_B,     709  },
   { "RTB",    00762,    00220, ALL_OP,     TYPE_E,     704  },
   { "RTD",    00762,    00200, ALL_OP,     TYPE_E,     704  },
   { "RTT",    04760,    00012, ALL_OP,     TYPE_E,     704  },
   { "RUN",    04772,    00200, ALL_OP,     TYPE_E,     709  },
   { "SAR",    03000,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "SAVE",   SAVE_T,   0,     MAP_OP,     TYPE_P,     704  },
   { "SAVEN",  SAVEN_T,  0,     MAP_OP,     TYPE_P,     704  },
   { "SBM",    04400,    0,     ALL_OP,     TYPE_B,     704  },
   { "SCA",    00636,    0,     ALL_OP,     TYPE_B,     7094 },
   { "SCD",    04636,    0,     ALL_OP,     TYPE_B,     7094 },
   { "SCDA",   00644,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDB",   04644,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDC",   00645,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDD",   04645,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDE",   00646,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDF",   04646,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDG",   00647,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCDH",   04647,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHA",   00640,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHB",   04640,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHC",   00641,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHD",   04641,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHE",   00642,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHF",   04642,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHG",   00643,    0,     ALL_OP,     TYPE_B,     709  },
   { "SCHH",   04643,    0,     ALL_OP,     TYPE_B,     709  },
   { "SDH",    00776,    00220, ALL_OP,     TYPE_E,     709  },
   { "SDL",    00776,    00200, ALL_OP,     TYPE_E,     709  },
   { "SDN",    00776,    0,     ALL_OP,     TYPE_B,     709  },
   { "SEA",    04761,    00041, BCORE_BAD,  TYPE_E,     7096 },
   { "SEB",    04761,    00042, BCORE_BAD,  TYPE_E,     7096 },
   { "SET",    SET_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "SEVEN",  07000,    0,     ALL_OP,     TYPE_A,     704  },
   { "SIB",    SIB_T,    0,     MAP_OP,     TYPE_P,     709  },
   { "SIL",    04055,    0,     ALL_OP,     TYPE_D,     709  },
   { "SIR",    00055,    0,     ALL_OP,     TYPE_D,     709  },
   { "SIX",    06000,    0,     ALL_OP,     TYPE_A,     704  },
   { "SLF",    00760,    00140, ALL_OP,     TYPE_E,     704  },
   { "SLN",    00760,    00140, ALL_OP,     TYPE_E,     704  },
   { "SLQ",    04620,    0,     ALL_OP,     TYPE_B,     704  },
   { "SLT",    04760,    00140, ALL_OP,     TYPE_E,     704  },
   { "SLW",    00602,    0,     ALL_OP,     TYPE_B,     704  },
   { "SMS",    07000,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "SNS",    02400,    02,    ALL_OP,     TYPE_CHAN,  709  },
   { "SPACE",  SPC_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "SPI",    04604,    0,     FAP_OP,     TYPE_B,     7096 },
   { "SPR",    00760,    00360, ALL_OP,     TYPE_E,     704  },
   { "SPT",    00760,    00360, ALL_OP,     TYPE_E,     704  },
   { "SPU",    00760,    00340, ALL_OP,     TYPE_E,     704  },
   { "SRI",    04601,    0,     FAP_OP,     TYPE_B,     7096 },
   { "SSLA",   00660,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLB",   04660,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLC",   00661,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLD",   04661,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLE",   00662,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLF",   04662,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLG",   00663,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSLH",   04663,    0,     ALL_OP,     TYPE_B,     709  },
   { "SSM",    04760,    00003, ALL_OP,     TYPE_E,     704  },
   { "SSP",    00760,    00003, ALL_OP,     TYPE_E,     704  },
   { "SST",    SST_T,    0,     FAP_OP,     TYPE_P,     704  },
   { "STA",    00621,    0,     ALL_OP,     TYPE_B,     704  },
   { "STCA",   00544,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCB",   04544,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCC",   00545,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCD",   04545,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCE",   00546,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCF",   04546,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCG",   00547,    0,     ALL_OP,     TYPE_E,     709  },
   { "STCH",   04547,    0,     ALL_OP,     TYPE_E,     709  },
   { "STD",    00622,    0,     ALL_OP,     TYPE_B,     704  },
   { "STI",    00604,    0,     ALL_OP,     TYPE_B,     709  },
   { "STL",    04625,    0,     ALL_OP,     TYPE_B,     709  },
   { "STO",    00601,    0,     ALL_OP,     TYPE_B,     704  },
   { "STP",    00630,    0,     ALL_OP,     TYPE_B,     704  },
   { "STQ",    04600,    0,     ALL_OP,     TYPE_B,     704  },
   { "STR",    05000,    0,     ALL_OP,     TYPE_A,     709  },
   { "STT",    00625,    0,     ALL_OP,     TYPE_B,     709  },
   { "STZ",    00600,    0,     ALL_OP,     TYPE_B,     704  },
   { "SUB",    00402,    0,     ALL_OP,     TYPE_B,     704  },
   { "SVN",    07000,    0,     ALL_OP,     TYPE_A,     704  },
   { "SWT",    00760,    00160, ALL_OP,     TYPE_E,     704  },
   { "SXA",    00634,    0,     ALL_OP,     TYPE_B,     709  },
   { "SXD",    04634,    0,     ALL_OP,     TYPE_B,     704  },
   { "SYN",    EQU_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "TAPENO", TAPENO_T, 0,     FAP_OP,     TYPE_P,     704  },
   { "TCD",    TCD_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "TCH",    01000,    0,     BCORE_CHAN, TYPE_A,     709  },
   { "TCM",    05000,    02,    BCORE_CHAN, TYPE_CHAN,  709  },
   { "TCNA",   04060,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCNB",   04061,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCNC",   04062,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCND",   04063,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCNE",   04064,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCNF",   04065,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCNG",   04066,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCNH",   04067,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOA",   00060,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOB",   00061,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOC",   00062,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOD",   00063,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOE",   00064,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOF",   00065,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOG",   00066,    0,     ALL_OP,     TYPE_B,     709  },
   { "TCOH",   00067,    0,     ALL_OP,     TYPE_B,     709  },
   { "TDC",    06400,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "TEFA",   00030,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFB",   04030,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFC",   00031,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFD",   04031,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFE",   00032,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFF",   04032,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFG",   00033,    0,     ALL_OP,     TYPE_B,     709  },
   { "TEFH",   04033,    0,     ALL_OP,     TYPE_B,     709  },
   { "THREE",  03000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TIA",    00101,    0,     FAP_OP,     TYPE_B,     7096 },
   { "TIB",    04101,    0,     FAP_OP,     TYPE_B,     7096 },
   { "TIF",    00046,    0,     ALL_OP,     TYPE_B,     709  },
   { "TIO",    00042,    0,     ALL_OP,     TYPE_B,     709  },
   { "TITLE",  TITLE_T,  0,     ALL_OP,     TYPE_P,     704  },
   { "TIX",    02000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TLQ",    00040,    0,     ALL_OP,     TYPE_B,     704  },
   { "TMI",    04120,    0,     ALL_OP,     TYPE_B,     704  },
   { "TNO",    04140,    0,     ALL_OP,     TYPE_B,     704  },
   { "TNX",    06000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TNZ",    04100,    0,     ALL_OP,     TYPE_B,     704  },
   { "TOV",    00140,    0,     ALL_OP,     TYPE_B,     704  },
   { "TPL",    00120,    0,     ALL_OP,     TYPE_B,     704  },
   { "TQO",    00161,    0,     ALL_OP,     TYPE_B,     704  },
   { "TQP",    00162,    0,     ALL_OP,     TYPE_B,     704  },
   { "TRA",    00020,    0,     ALL_OP,     TYPE_B,     704  },
   { "TRCA",   00022,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCB",   04022,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCC",   00024,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCD",   04024,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCE",   00026,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCF",   04026,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCG",   00027,    0,     ALL_OP,     TYPE_B,     709  },
   { "TRCH",   04027,    0,     ALL_OP,     TYPE_B,     709  },
   { "TSX",    00074,    0,     ALL_OP,     TYPE_B,     704  },
   { "TTL",    TTL_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "TTR",    00021,    0,     ALL_OP,     TYPE_B,     704  },
   { "TWO",    02000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TWT",    03400,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "TXH",    03000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TXI",    01000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TXL",    07000,    0,     ALL_OP,     TYPE_A,     704  },
   { "TZE",    00100,    0,     ALL_OP,     TYPE_B,     704  },
   { "UAM",    04304,    0,     ALL_OP,     TYPE_B,     709  },
   { "UFA",    04300,    0,     ALL_OP,     TYPE_B,     704  },
   { "UFM",    04260,    0,     ALL_OP,     TYPE_B,     704  },
   { "UFS",    04302,    0,     ALL_OP,     TYPE_B,     704  },
   { "UNLIST", UNLIST_T, 0,     ALL_OP,     TYPE_P,     704  },
   { "UNPNCH", UNPNCH_T, 0,     MAP_OP,     TYPE_P,     704  },
   { "USE",    USE_T,    0,     MAP_OP,     TYPE_P,     704  },
   { "USM",    04306,    0,     ALL_OP,     TYPE_B,     709  },
   { "VDH",    00224,    0,     ALL_OP,     TYPE_C,     709  },
   { "VDP",    00225,    0,     ALL_OP,     TYPE_C,     709  },
   { "VFD",    VFD_T,    0,     ALL_OP,     TYPE_P,     704  },
   { "VLM",    00204,    0,     ALL_OP,     TYPE_C,     709  },
   { "WDDD",   00766,    04240, FAP_OP,     TYPE_E,     7096 },
   { "WDR",    00766,    00300, ALL_OP,     TYPE_E,     704  },
   { "WEF",    00770,    00200, ALL_OP,     TYPE_E,     704  },
   { "WPB",    00766,    00362, ALL_OP,     TYPE_E,     704  },
   { "WPD",    00766,    00361, ALL_OP,     TYPE_E,     704  },
   { "WPR",    00766,    00361, ALL_OP,     TYPE_E,     704  },
   { "WPU",    00766,    00341, ALL_OP,     TYPE_E,     704  },
   { "WRS",    00766,    0,     ALL_OP,     TYPE_E,     704  },
   { "WTB",    00766,    00220, ALL_OP,     TYPE_E,     704  },
   { "WTD",    00766,    00200, ALL_OP,     TYPE_E,     704  },
   { "WTR",    00000,    0,     BCORE_CHAN, TYPE_CHAN,  709  },
   { "WTV",    00766,    00030, ALL_OP,     TYPE_E,     704  },
   { "XCA",    00131,    0,     ALL_OP,     TYPE_B,     709  },
   { "XCL",    04130,    0,     ALL_OP,     TYPE_B,     709  },
   { "XEC",    00522,    0,     ALL_OP,     TYPE_B,     709  },
   { "XIT",    00021,    0,     ALL_OP,     TYPE_B,     704  },
   { "XMT",    00000,    02,    BCORE_CHAN, TYPE_A,     709  },
   { "ZAC",    04754,    0,     ALL_OP,     TYPE_B,     704  },
   { "ZERO",   00000,    0,     ALL_OP,     TYPE_A,     704  },
   { "ZET",    00520,    0,     ALL_OP,     TYPE_B,     709  },
   { "ZSA",    00634,    0,     ALL_OP,     TYPE_B,     709  },
   { "ZSD",    04634,    0,     ALL_OP,     TYPE_B,     704  },
#if defined(USS) || defined(OS390)
   { "12BIT",  BIT12_T,  0,     FAP_OP,     TYPE_P,     7096 },
#endif
};

extern int cpumodel;		/* CPU model (704, 709, 7090, 7094, 7096) */
extern int fapmode;		/* FAP assembly mode */
extern int inpass;		/* Which pass are we in */
extern int bcore;		/* BCORE assembly */

/*
** Added opcodes
*/

extern int opdefcount;		/* Number of user defined opcode in opdef */
extern OpDefCode *freeops;	/* Reusable opdef codes */
extern OpDefCode *opdefcode[MAXDEFOPS];/* The user defined opcode table */

static char retopcode[MAXSYMLEN+2]; /* Return opcode symbol buffer */
static OpCode retop;		/* Return opcode buffer */

/***********************************************************************
* freeopd - Link a opcode entry on free chain.
***********************************************************************/

void
freeopd (OpDefCode *or)
{
   or->next = freeops;
   freeops = or;
}

/***********************************************************************
* stdlookup - Lookup standard opcode. Uses binary search.
***********************************************************************/

static OpCode *
stdlookup (char *op)
{
   OpCode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGOP
   fprintf (stderr, "stdlookup: Entered: op = %s\n", op);
#endif

   lo = 0;
   up = NUMOPS;

   done = FALSE;
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGOP
      fprintf (stderr, " mid = %d, last = %d\n", mid, last);
#endif
      if (last == mid) break;
      r = strcmp (optable[mid].opcode, op);
      if (r == 0)
      {
	 int bits;

	 bits = (fapmode ? FAP_OP : MAP_OP);
	 if ((optable[mid].opflags & bits) && optable[mid].model <= cpumodel)
	    ret = &optable[mid];
	 done = TRUE;
      }
      else if (r < 0)
      {
	 lo = mid;
      }
      else 
      {
	 up = mid;
      }
      last = mid;
   }

#ifdef DEBUGOP
   fprintf (stderr, " ret = %8x\n", ret);
#endif
   return (ret);
}

/***********************************************************************
* addlookup - Lookup added opcode. Uses binary search.
***********************************************************************/

static OpCode *
addlookup (char *op)
{
   OpCode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGOP
   fprintf (stderr, "addlookup: Entered: op = %s\n", op);
#endif

   lo = 0;
   up = opdefcount;
   
   while (opdefcount && !done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGOP
      fprintf (stderr, " mid = %d, last = %d\n", mid, last);
#endif
      if (opdefcount == 1) done = TRUE;
      else if (last == mid) break;
         
      r = strcmp (opdefcode[mid]->opcode, op);
      if (r == 0)
      {
	 strcpy (retopcode, op);
	 retop.opcode = retopcode;
	 retop.opvalue = opdefcode[mid]->opvalue;
	 retop.opmod = opdefcode[mid]->opmod;
	 retop.optype = opdefcode[mid]->optype;
	 retop.model = cpumodel;
	 retop.opflags = ALL_OP;
	 ret = &retop;
	 done = TRUE;
      }
      else if (r < 0)
      {
	 lo = mid;
      }
      else 
      {
	 up = mid;
      }
      last = mid;
   }

#ifdef DEBUGOP
   fprintf (stderr, " ret = %8x\n", ret);
#endif
   return (ret);
}

/***********************************************************************
* oplookup - Lookup opcode.
***********************************************************************/

OpCode *
oplookup (char *op)
{
   OpCode *ret = NULL;
   int i;
   int opch, opcho;
   char opbf[MAXSYMLEN+2];

#ifdef DEBUGOP
   fprintf (stderr, "oplookup: Entered: op = %s\n", op);
#endif

   /*
   ** Check added opcodes first, incase of override
   */

   if (!(ret = addlookup (op)))
   {
      /*
      ** Check standard opcode table
      */

      if (!(ret = stdlookup (op)))
      {

	 /*
	 ** Check if it's an I/O opcode. Like WPR[A-H]
	 */

	 strcpy (opbf, op);
	 i = strlen(opbf) - 1;
	 opcho = opch = opbf[i];
	 opbf[i] = '\0';
	 /*
	 ** Handle cases like RCHU for CTSS.
	 */
	 if (i == 3 && opch == 'U')
	 {
	    SymNode *sym;
	    char tmp[2];

	    tmp[0] = opch;
	    tmp[1] = 0;
	    sym = symlookup (tmp, "", FALSE, inpass == 020 ? TRUE : FALSE);
	    if ((sym == NULL) || !(sym->flags & TAPENO))
	    {
	       if (sym && (sym->flags & BOOL))
		  opch = 'A' + ((sym->value >> 9) - 1);
	       else
		  opch = 'G';
	       opbf[i] = opch;
	       if ((ret = stdlookup (opbf)) != 0) return (ret);
	       opbf[i] = '\0';
	    }
	 }

	 if (i == 3 && opch >= 'A' && opch <= 'H')
	 {
	    if ((ret = stdlookup (opbf)) != NULL)
	    {
	       strcpy (retopcode, op);
	       retop.opcode = retopcode;
	       retop.opvalue = ret->opvalue;
	       retop.opmod = ret->opmod | (opch - 'A' + 1) << 9;
	       retop.optype = ret->optype;
	       retop.opflags = ret->opflags;
	       retop.model = ret->model;
	       ret = &retop;
	    }
         }

	 /*
	 ** Maybe a TAPENO'd opcode.
	 */

	 else
	 {
	    SymNode *sym;

	    opbf[i] = opcho;
#ifdef DEBUGTAPENOOP
	    fprintf (stderr, "oplookup: Entered: opbf = %s\n", opbf);
#endif
	    if ((sym = symlookup (&opbf[3], "",
			      FALSE, inpass == 020 ? TRUE : FALSE)) != NULL)
	    {
#ifdef DEBUGTAPENOOP
	       fprintf (stderr, "   symbol = %s\n", &opbf[3]);
#endif
	       if (sym->flags & TAPENO)
	       {
		  opbf[i] = '\0';
		  if ((ret = stdlookup (opbf)) == NULL)
		  {
		     opbf[i] = ((sym->value >> 9) & 017) - 1 + 'A';
		     opbf[i+1] = '\0';
#ifdef DEBUGTAPENOOP
		     fprintf (stderr, "   op = %s\n", opbf);
#endif
		     ret = stdlookup (opbf);
		  }

		  if (ret)
		  {
		     strcpy (retopcode, op);
		     retop.opcode = retopcode;
		     retop.opvalue = ret->opvalue;
		     retop.opmod = ret->opmod | sym->value;
		     retop.optype = ret->optype;
		     retop.opflags = ret->opflags;
		     retop.model = ret->model;
		     ret = &retop;
		  }
	       }
	    }
	 }
      }
   }

#ifdef DEBUGOP
   fprintf (stderr, " ret = %8x\n", ret);
#endif
   return (ret);
}

/***********************************************************************
* opadd - Add opcode.
***********************************************************************/

void
opadd (char *op, unsigned c0, unsigned c1, int type)
{
   OpDefCode *new;
   int lo, up;

#ifdef DEBUGOP
   fprintf (stderr,
	    "opadd: Entered: op = %s, c0 = %4.4o, c1 = %8.8o, type = %d\n",
	    op, c0, c1, type);
#endif

   /*
   ** Allocate storage for opcode and fill it in
   */

   if (opdefcount+1 > MAXDEFOPS)
   {
      fprintf (stderr, "asm7090: Op Code table exceeded\n");
      exit (ABORT);
   }

   if (freeops)
   {
      new = freeops;
      freeops = new->next;
   }
   else if ((new = (OpDefCode *)DBG_MEMGET (sizeof (OpDefCode))) == NULL)
   {
      fprintf (stderr, "asm7090: Unable to allocate memory\n");
      exit (ABORT);
   }

   memset (new, '\0', sizeof (OpDefCode));
   strcpy (new->opcode, op);
   new->opvalue = c0;
   new->opmod = c1;
   new->optype = type;
   new->opflags = ALL_OP;
   new->model = cpumodel;

   if (opdefcount == 0)
   {
      opdefcode[0] = new;
      opdefcount++;
      return;
   }

   /*
   ** Insert pointer in sort order.
   */

   for (lo = 0; lo < opdefcount; lo++)
   {
      if (strcmp (opdefcode[lo]->opcode, op) > 0)
      {
	 for (up = opdefcount + 1; up > lo; up--)
	 {
	    opdefcode[up] = opdefcode[up-1];
	 }
	 opdefcode[lo] = new;
	 opdefcount++;
	 return;
      }
   }
   opdefcode[opdefcount] = new;
   opdefcount++;
}

/***********************************************************************
* opdel - Delete an op code from the table.
***********************************************************************/

void
opdel (char *op)
{
   int i;

#ifdef DEBUGOP
   fprintf (stderr, "opdel: Entered: op = %s\n", op);
#endif
   for (i = 0; i < opdefcount; i++)
   {
      if (!strcmp (opdefcode[i]->opcode, op))
      {
         DBG_MEMREL (opdefcode[i]);
	 for (; i < opdefcount; i++)
	 {
	    opdefcode[i] = opdefcode[i+1];
	 }
	 opdefcount --;
         return;
      }
   }
}
