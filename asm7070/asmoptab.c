/***********************************************************************
*
* asmoptab.c - Opcode table for the IBM 7070 assembler.
*
* Changes:
*   03/01/07   DGP   Original.
*   05/31/13   DGP   Instruction corrections.
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
   { "A1",        +14,     0,  0,  TYPE_A,  7070 },
   { "A2",        +24,     0,  0,  TYPE_A,  7070 },
   { "A3",        +34,     0,  0,  TYPE_A,  7070 },
#endif
   { "AA",        +17,     0,  0,  TYPE_A,  7070 },
   { "AAS1",      +19,     0,  0,  TYPE_A,  7070 },
   { "AAS2",      +29,     0,  0,  TYPE_A,  7070 },
   { "AAS3",      +39,     0,  0,  TYPE_A,  7070 },
#if !defined(USS) && !defined(OS390)
   { "AS1",       +18,     0,  0,  TYPE_A,  7070 },
   { "AS2",       +28,     0,  0,  TYPE_A,  7070 },
   { "AS3",       +38,     0,  0,  TYPE_A,  7070 },
#endif
   { "ASSF",      +04,    20,  0,  TYPE_B,  7074 },
   { "ASSN",      +04,    10,  0,  TYPE_B,  7074 },
#if defined(USS) || defined(OS390)
   { "AS1",       +18,     0,  0,  TYPE_A,  7070 },
   { "AS2",       +28,     0,  0,  TYPE_A,  7070 },
   { "AS3",       +38,     0,  0,  TYPE_A,  7070 },
   { "A1",        +14,     0,  0,  TYPE_A,  7070 },
   { "A2",        +24,     0,  0,  TYPE_A,  7070 },
   { "A3",        +34,     0,  0,  TYPE_A,  7070 },
#endif
   { "B",         +01,     0,  0,  TYPE_A,  7070 },
   { "BAL",       +60,     0,  0,  TYPE_L,  7070 },
   { "BAS",       +51,    00,  0,  TYPE_C,  7070 },
   { "BASS",      +04,    00,  0,  TYPE_B,  7074 },
   { "BCB",       +51,    01,  0,  TYPE_C,  7070 },
   { "BCX",       -43,     0,  0,  TYPE_I,  7070 },
   { "BDCA",      +60,    90,  4,  TYPE_L,  7070 },
   { "BDCB",      +51,    02,  0,  TYPE_C,  7070 },
   { "BDCL",      +60,    80,  4,  TYPE_L,  7070 },
   { "BDX",       -49,     0,  0,  TYPE_I,  7070 },
   { "BE",        -41,     0,  0,  TYPE_A,  7070 },
   { "BES",       +61,    00,  0,  TYPE_E,  7070 },
   { "BFV",       +41,    00,  0,  TYPE_B,  7070 },
   { "BH",        -40,     0,  0,  TYPE_A,  7070 },
   { "BIX",       +49,     0,  0,  TYPE_I,  7070 },
   { "BL",        +40,     0,  0,  TYPE_A,  7070 },
   { "BLX",       +02,     0,  0,  TYPE_I,  7070 },
   { "BM1",       -10,     0,  0,  TYPE_A,  7070 },
   { "BM2",       -20,     0,  0,  TYPE_A,  7070 },
   { "BM3",       -30,     0,  0,  TYPE_A,  7070 },
   { "BQL",       +60,     3,  2,  TYPE_L,  7070 },
   { "BSC",       -03,    04,  0,  TYPE_B,  7070 },
   { "BSF",       +61,    40,  0,  TYPE_E,  7070 },
   { "BSN",       +61,    30,  0,  TYPE_E,  7070 },
   { "BTL",       +60,     0,  3,  TYPE_L,  7070 },
   { "BUL",       +60,     1,  1,  TYPE_L,  7070 },
   { "BV1",       +11,     0,  0,  TYPE_A,  7070 },
   { "BV2",       +21,     0,  0,  TYPE_A,  7070 },
   { "BV3",       +31,     0,  0,  TYPE_A,  7070 },
   { "BXM",       -44,     0,  0,  TYPE_I,  7070 },
   { "BXN",       +44,     0,  0,  TYPE_I,  7070 },
   { "BZ1",       +10,     0,  0,  TYPE_A,  7070 },
   { "BZ2",       +20,     0,  0,  TYPE_A,  7070 },
   { "BZ3",       +30,     0,  0,  TYPE_A,  7070 },
#if !defined(USS) && !defined(OS390)
   { "C1",        +15,     0,  0,  TYPE_A,  7070 },
   { "C2",        +25,     0,  0,  TYPE_A,  7070 },
   { "C3",        +35,     0,  0,  TYPE_A,  7070 },
#endif
   { "CA",        -15,     0,  0,  TYPE_A,  7070 },
   { "CD",        +03,     0,  0,  TYPE_A,  7070 },
   { "CNTRL", CNTRL_T,     0,  0,  TYPE_P,  7070 },
   { "CODE",   CODE_T,     0,  0,  TYPE_P,  7070 },
   { "CSA",       -03,    30,  0,  TYPE_B,  7070 },
   { "CSM",       -03,    60,  0,  TYPE_B,  7070 },
   { "CSP",       -03,    90,  0,  TYPE_B,  7070 },
#if defined(USS) || defined(OS390)
   { "C1",        +15,     0,  0,  TYPE_A,  7070 },
   { "C2",        +25,     0,  0,  TYPE_A,  7070 },
   { "C3",        +35,     0,  0,  TYPE_A,  7070 },
#endif
   { "D",         -53,     0,  0,  TYPE_A,  7070 },
   { "DA",       DA_T,     0,  0,  TYPE_P,  7070 },
   { "DC",       DC_T,     0,  0,  TYPE_P,  7070 },
   { "DCAF",      -62,    90,  4,  TYPE_L,  7070 },
   { "DCAN",      -61,    90,  4,  TYPE_L,  7070 },
   { "DCLF",      -62,    80,  4,  TYPE_L,  7070 },
   { "DCLN",      -61,    80,  4,  TYPE_L,  7070 },
   { "DIOCS", DIOCS_T,     0,  0,  TYPE_P,  7070 },
   { "DLINE", DLINE_T,     0,  0,  TYPE_P,  7070 },
   { "DRDW",   DRDW_T,     0,  0,  TYPE_P,  7070 },
   { "DSW",     DSW_T,     0,  0,  TYPE_P,  7070 },
   { "DTF",     DTF_T,     0,  0,  TYPE_P,  7070 },
   { "DUF",     DUF_T,     0,  0,  TYPE_P,  7070 },
   { "EAN",       -57,     0,  0,  TYPE_I,  7070 },
   { "EJECT", EJECT_T,     0,  0,  TYPE_P,  7070 },
   { "ENA",       +56,     0,  0,  TYPE_I,  7070 },
   { "ENB",       +57,     0,  0,  TYPE_I,  7070 },
   { "ENS",       -56,     0,  0,  TYPE_I,  7070 },
   { "EQU",     EQU_T,     0,  0,  TYPE_P,  7070 },
   { "ESF",       +61,    20,  0,  TYPE_E,  7070 },
   { "ESN",       +61,    10,  0,  TYPE_E,  7070 },
   { "FA",        +74,     0,  0,  TYPE_A,  7070 },
   { "FAA",       +77,     0,  0,  TYPE_A,  7070 },
   { "FAD",       +76,     0,  0,  TYPE_A,  7070 },
   { "FADS",      -76,     0,  0,  TYPE_A,  7070 },
   { "FBU",       -70,     0,  0,  TYPE_A,  7070 },
   { "FBV",       +70,     0,  0,  TYPE_A,  7070 },
   { "FD",        -73,     0,  0,  TYPE_A,  7070 },
   { "FDD",       -75,     0,  0,  TYPE_A,  7070 },
   { "FM",        +73,     0,  0,  TYPE_A,  7070 },
   { "FR",        +71,     0,  0,  TYPE_A,  7070 },
   { "FS",        -74,     0,  0,  TYPE_A,  7070 },
   { "FSA",       -77,     0,  0,  TYPE_A,  7070 },
   { "FZA",       +75,     0,  0,  TYPE_A,  7070 },
   { "HB",         00,    +1,  0,  TYPE_A,  7070 },
   { "HMFV",      +41,    02,  0,  TYPE_B,  7070 },
   { "HMSC",      -03,    03,  0,  TYPE_B,  7070 },
   { "HP",         00,    -1,  1,  TYPE_A,  7070 },
   { "ITS",       +69,    91,  0,  TYPE_B,  7070 },
   { "ITZ",       +69,    90,  0,  TYPE_B,  7070 },
   { "LE",        +67,     0,  0,  TYPE_A,  7070 },
   { "LEH",       +68,     0,  0,  TYPE_A,  7070 },
   { "LL",        +66,     0,  0,  TYPE_A,  7070 },
   { "M",         +53,     0,  0,  TYPE_A,  7070 },
   { "MSA",       -03,    31,  0,  TYPE_B,  7070 },
   { "MSM",       -03,    61,  0,  TYPE_B,  7070 },
   { "MSP",       -03,    91,  0,  TYPE_B,  7070 },
   { "NOP",       -01,     0,  1,  TYPE_A,  7070 },
   { "PC",        +55,     0,  0,  TYPE_B,  7070 },
   { "PR",        +64,     0,  0,  TYPE_B,  7070 },
   { "PTM",       +80,     0,  1,  TYPE_T,  7070 },
   { "PTR",       +80,     1,  0,  TYPE_T,  7070 },
   { "PTRA",      +80,     9,  0,  TYPE_T,  7070 },
   { "PTRR",      +80,     2,  0,  TYPE_T,  7070 },
   { "PTSB",      +80,     8,  0,  TYPE_T,  7070 },
   { "PTSF",      +80,     7,  0,  TYPE_T,  7070 },
   { "PTSM",      +80,     0,  5,  TYPE_T,  7070 },
   { "PTW",       +80,     3,  0,  TYPE_T,  7070 },
   { "PTWC",      +80,     6,  0,  TYPE_T,  7070 },
   { "PTWR",      +80,     4,  0,  TYPE_T,  7070 },
   { "PTWZ",      +80,     5,  0,  TYPE_T,  7070 },
   { "QLF",       -62,     3,  2,  TYPE_L,  7070 },
   { "QLN",       -61,     3,  2,  TYPE_L,  7070 },
   { "QR",        +54,     0,  0,  TYPE_Q,  7070 },
   { "QW",        +54,     1,  0,  TYPE_Q,  7070 },
   { "RG",        -65,     0,  0,  TYPE_I,  7070 },
   { "RS",        +65,     0,  0,  TYPE_I,  7070 },
#if !defined(USS) && !defined(OS390)
   { "S1",        -14,     0,  0,  TYPE_A,  7070 },
   { "S2",        -24,     0,  0,  TYPE_A,  7070 },
   { "S3",        -34,     0,  0,  TYPE_A,  7070 },
#endif
   { "SA",        -17,     0,  0,  TYPE_A,  7070 },
   { "SL",        -50,    02,  0,  TYPE_D,  7070 },
#if !defined(USS) && !defined(OS390)
   { "SL1",       +50,    12,  0,  TYPE_S,  7070 },
   { "SL2",       +50,    22,  0,  TYPE_S,  7070 },
   { "SL3",       +50,    32,  0,  TYPE_S,  7070 },
#endif
   { "SLC",       -50,    03,  0,  TYPE_D,  7070 },
   { "SLC1",      +50,    13,  0,  TYPE_S,  7070 },
   { "SLC2",      +50,    23,  0,  TYPE_S,  7070 },
   { "SLC3",      +50,    33,  0,  TYPE_S,  7070 },
   { "SLS",       -50,    05,  0,  TYPE_D,  7070 },
#if defined(USS) || defined(OS390)
   { "SL1",       +50,    12,  0,  TYPE_S,  7070 },
   { "SL2",       +50,    22,  0,  TYPE_S,  7070 },
   { "SL3",       +50,    32,  0,  TYPE_S,  7070 },
#endif
   { "SMFV",      +41,    01,  0,  TYPE_B,  7070 },
   { "SMSC",      -03,    02,  0,  TYPE_B,  7070 },
   { "SR",        -50,    00,  0,  TYPE_D,  7070 },
#if !defined(USS) && !defined(OS390)
   { "SR1",       +50,    10,  0,  TYPE_S,  7070 },
   { "SR2",       +50,    20,  0,  TYPE_S,  7070 },
   { "SR3",       +50,    30,  0,  TYPE_S,  7070 },
#endif
   { "SRR",       -50,    01,  0,  TYPE_D,  7070 },
   { "SRR1",      +50,    11,  0,  TYPE_S,  7070 },
   { "SRR2",      +50,    21,  0,  TYPE_S,  7070 },
   { "SRR3",      +50,    31,  0,  TYPE_S,  7070 },
   { "SRS",       -50,    04,  0,  TYPE_D,  7070 },
#if defined(USS) || defined(OS390)
   { "SR1",       +50,    10,  0,  TYPE_S,  7070 },
   { "SR2",       +50,    20,  0,  TYPE_S,  7070 },
   { "SR3",       +50,    30,  0,  TYPE_S,  7070 },
#endif
   { "SS1",       -18,     0,  0,  TYPE_A,  7070 },
   { "SS2",       -28,     0,  0,  TYPE_A,  7070 },
   { "SS3",       -38,     0,  0,  TYPE_A,  7070 },
#if !defined(USS) && !defined(OS390)
   { "ST1",       +12,     0,  0,  TYPE_A,  7070 },
   { "ST2",       +22,     0,  0,  TYPE_A,  7070 },
   { "ST3",       +32,     0,  0,  TYPE_A,  7070 },
#endif
   { "STD1",      -12,     0,  0,  TYPE_A,  7070 },
   { "STD2",      -22,     0,  0,  TYPE_A,  7070 },
   { "STD3",      -32,     0,  0,  TYPE_A,  7070 },
#if defined(USS) || defined(OS390)
   { "ST1",       +12,     0,  0,  TYPE_A,  7070 },
   { "ST2",       +22,     0,  0,  TYPE_A,  7070 },
   { "ST3",       +32,     0,  0,  TYPE_A,  7070 },
   { "S1",        -14,     0,  0,  TYPE_A,  7070 },
   { "S2",        -24,     0,  0,  TYPE_A,  7070 },
   { "S3",        -34,     0,  0,  TYPE_A,  7070 },
#endif
   { "TEF",       -80,     0,  7,  TYPE_T,  7070 },
   { "TLF",       -62,     0,  3,  TYPE_L,  7070 },
   { "TLN",       -61,     0,  3,  TYPE_L,  7070 },
   { "TM",        -80,     0,  1,  TYPE_T,  7070 },
   { "TR",        -80,     1,  0,  TYPE_T,  7070 },
   { "TRA",       -80,     9,  0,  TYPE_T,  7070 },
   { "TRB",       -80,     0,  4,  TYPE_T,  7070 },
   { "TRR",       -80,     2,  0,  TYPE_T,  7070 },
   { "TRU",       -80,     0,  3,  TYPE_T,  7070 },
   { "TRW",       -80,     0,  2,  TYPE_T,  7070 },
   { "TSB",       -80,     8,  0,  TYPE_T,  7070 },
   { "TSEL",      -80,     0,  0,  TYPE_T,  7070 },
   { "TSF",       -80,     7,  0,  TYPE_T,  7070 },
   { "TSHD",      -80,     0,  9,  TYPE_T,  7070 },
   { "TSK",       -80,     0,  6,  TYPE_T,  7070 },
   { "TSLD",      -80,     0,  8,  TYPE_T,  7070 },
   { "TSM",       -80,     0,  5,  TYPE_T,  7070 },
   { "TW",        -80,     3,  0,  TYPE_T,  7070 },
   { "TWC",       -80,     6,  0,  TYPE_T,  7070 },
   { "TWR",       -80,     4,  0,  TYPE_T,  7070 },
   { "TWZ",       -80,     5,  0,  TYPE_T,  7070 },
   { "TYP",       +69,     4,  0,  TYPE_U,  7070 },
   { "ULF",       -62,     1,  1,  TYPE_L,  7070 },
   { "ULN",       -61,     1,  1,  TYPE_L,  7070 },
   { "UP",        +69,     2,  0,  TYPE_U,  7070 },
   { "UPIV",      +69,     3,  0,  TYPE_U,  7070 },
   { "UR",        +69,     1,  0,  TYPE_U,  7070 },
   { "US",        +69,     0,  0,  TYPE_U,  7070 },
   { "UW",        +69,     2,  0,  TYPE_U,  7070 },
   { "UWIV",      +69,     3,  0,  TYPE_U,  7070 },
   { "XA",        +47,     0,  0,  TYPE_I,  7070 },
   { "XL",        +45,     0,  0,  TYPE_I,  7070 },
   { "XLIN",      -48,     0,  0,  TYPE_I,  7070 },
   { "XS",        -47,     0,  0,  TYPE_I,  7070 },
   { "XSN",       +48,     0,  0,  TYPE_I,  7070 },
   { "XU",        -45,     0,  0,  TYPE_I,  7070 },
   { "XZA",       +46,     0,  0,  TYPE_I,  7070 },
   { "XZS",       -46,     0,  0,  TYPE_I,  7070 },
#if !defined(USS) && !defined(OS390)
   { "ZA1",       +13,     0,  0,  TYPE_A,  7070 },
   { "ZA2",       +23,     0,  0,  TYPE_A,  7070 },
   { "ZA3",       +33,     0,  0,  TYPE_A,  7070 },
#endif
   { "ZAA",       +16,     0,  0,  TYPE_A,  7070 },
#if defined(USS) || defined(OS390)
   { "ZA1",       +13,     0,  0,  TYPE_A,  7070 },
   { "ZA2",       +23,     0,  0,  TYPE_A,  7070 },
   { "ZA3",       +33,     0,  0,  TYPE_A,  7070 },
#endif
#if !defined(USS) && !defined(OS390)
   { "ZS1",       -13,     0,  0,  TYPE_A,  7070 },
   { "ZS2",       -23,     0,  0,  TYPE_A,  7070 },
   { "ZS3",       -33,     0,  0,  TYPE_A,  7070 },
#endif
   { "ZSA",       -16,     0,  0,  TYPE_A,  7070 },
   { "ZST1",      -11,     0,  0,  TYPE_A,  7070 },
   { "ZST2",      -21,     0,  0,  TYPE_A,  7070 },
   { "ZST3",      -31,     0,  0,  TYPE_A,  7070 },
#if defined(USS) || defined(OS390)
   { "ZS1",       -13,     0,  0,  TYPE_A,  7070 },
   { "ZS2",       -23,     0,  0,  TYPE_A,  7070 },
   { "ZS3",       -33,     0,  0,  TYPE_A,  7070 },
#endif
};

extern int cpumodel;		/* CPU model (7070, 7074) */


/***********************************************************************
* oplookup - Lookup opcode.
***********************************************************************/

OpCode *
oplookup (char *op)
{
   OpCode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGOP
   fprintf (stderr, "oplookup: Entered: op = %s\n", op);
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
	 if (optable[mid].model <= cpumodel)
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
