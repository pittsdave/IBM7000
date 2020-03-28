/***********************************************************************
*
* asmoptab.c - Opcode table for the IBM 7080 assembler.
*
* Changes:
*   02/08/07   DGP   Original.
*   05/24/13   DGP   Correct TMT type.
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
   { "AAM",       '@',     0,     TYPE_A,     705  },
   { "ACON4", ACON4_T,     0,     TYPE_P,     705  },
   { "ACON5", ACON5_T,     0,     TYPE_P,     705  },
   { "ACON6", ACON6_T,     0,     TYPE_P,     705  },
   { "ADCON", ADCON_T,     0,     TYPE_P,     705  },
   { "ADD",       'G',     0,     TYPE_A,     705  },
   { "ADM",       '6',     0,     TYPE_A,     705  },
   { "ALTSW", ALTSW_T,     0,     TYPE_P,     705  },
   { "BITCD", BITCD_T,     0,     TYPE_P,     705  },
   { "BLM",       '$',     0,     TYPE_A,     705  },
   { "BLMS",      '$',     1,     TYPE_B,     705  },
   { "BSF",       '3', 00401,     TYPE_C,     705  },
   { "BSP",       '3', 00400,     TYPE_C,     705  },
   { "CHR",       '3',    16,     TYPE_B,    7053  },
   { "CHRCD", CHRCD_T,     0,     TYPE_P,     705  },
   { "CMP",       '4',     0,     TYPE_A,     705  },
   { "CNO",       ',',    11,     TYPE_B,    7080  },
   { "CON",     CON_T,     0,     TYPE_P,     705  },
   { "CRD",       'Y',     4,     TYPE_B,    7053  },
   { "CWR",       'R',     4,     TYPE_B,    7053  },
   { "DIV",       'W',     0,     TYPE_A,     705  },
   { "DMP",       'R',     1,     TYPE_B,     705  },
   { "ECB",       '3', 01400,     TYPE_D,    7053  },
   { "EEM",       '3', 01600,     TYPE_D,    7080  },
   { "EIA",       ',',    10,     TYPE_B,    7080  },
   { "EIM",       ',',     6,     TYPE_B,    7080  },
   { "EJECT", EJECT_T,     0,     TYPE_P,     705  },
   { "END",     END_T,     0,     TYPE_P,     705  },
   { "FPN",     FPN_T,     0,     TYPE_P,     705  },
   { "FSP",       'Y',     1,     TYPE_B,     705  },
   { "HLT",       'J',     0,     TYPE_A,     705  },
   { "INCL",   INCL_T,     0,     TYPE_P,     705  },
   { "IOF",       '3',     0,     TYPE_C,     705  },
   { "ION",       '3', 00300,     TYPE_D,     705  },
   { "LASN",   LASN_T,     0,     TYPE_P,     705  },
   { "LDA",       '#',     0,     TYPE_A,     705  },
   { "LEM",       '3', 01700,     TYPE_D,    7080  },
   { "LFC",       ',',     2,     TYPE_B,    7080  },
   { "LIM",       ',',     7,     TYPE_B,    7080  },
   { "LIP",       ',',    15,     TYPE_B,    7080  },
   { "LITOR", LITOR_T,     0,     TYPE_P,     705  },
   { "LNG",       'D',     0,     TYPE_A,     705  },
   { "LOD",       '8',     0,     TYPE_A,     705  },
   { "LSB",       ',',     4,     TYPE_B,    7080  },
   { "MACRO", MACRO_T,     0,     TYPE_P,     705  },
   { "MEND",   MEND_T,     0,     TYPE_P,     705  },
   { "MODE",   MODE_T,     0,     TYPE_P,     705  },
   { "MPY",       'V',     0,     TYPE_A,     705  },
   { "NAME",   NAME_T,     0,     TYPE_P,     705  },
   { "NOP",       'A',     0,     TYPE_A,     705  },
   { "NTR",       'X',     0,     TYPE_A,     705  },
   { "RAD",       'H',     0,     TYPE_A,     705  },
   { "RCD",     RCD_T,     0,     TYPE_P,     705  },
   { "RCV",       'U',     0,     TYPE_A,     705  },
   { "RD",        'Y',     0,     TYPE_A,     705  },
   { "RMA",       'Y',     2,     TYPE_B,     705  },
   { "RMB",       'Y',     5,     TYPE_B,    7053  },
   { "RND",       'E',     0,     TYPE_A,     705  },
   { "RPT",     RPT_T,     0,     TYPE_P,     705  },
   { "RSU",       'Q',     0,     TYPE_A,     705  },
   { "RUN",       '3', 00201,     TYPE_D,    7053  },
   { "RWD",       '3', 00200,     TYPE_D,     705  },
   { "RWW",       'S',     0,     TYPE_A,     705  },
   { "SASN",   SASN_T,     0,     TYPE_P,     705  },
   { "SBA",       '%',     7,     TYPE_B,     705  },
   { "SBN",       '%',     0,     TYPE_A,     705  },
   { "SBR",       '%',     8,     TYPE_B,     705  },
   { "SBZ",       '%',     0,     TYPE_A,     705  },
   { "SCC",       'R',     3,     TYPE_B,    7053  },
   { "SDH",       '3', 04600,     TYPE_C,    7053  },
   { "SDL",       '3', 04500,     TYPE_C,    7053  },
   { "SEL",       '2',     0,     TYPE_A,     705  },
   { "SET",       'B',     0,     TYPE_A,     705  },
   { "SGN",       'T',     0,     TYPE_A,     705  },
   { "SHR",       'C',     0,     TYPE_A,     705  },
   { "SKP",       '3', 01100,     TYPE_C,     705  },
   { "SND",       '/',     0,     TYPE_A,     705  },
   { "SPC",       ',',     0,     TYPE_B,    7080  },
   { "SPR",       '5',     0,     TYPE_A,     705  },
   { "SRC",       'R',     2,     TYPE_B,     705  },
   { "SST",       'Y',     3,     TYPE_B,    7053  },
   { "ST",        'F',     0,     TYPE_A,     705  },
   { "SUB",       'P',     0,     TYPE_A,     705  },
   { "SUBOR", SUBOR_T,     0,     TYPE_P,     705  },
   { "SUP",       '3', 00500,     TYPE_C,     705  },
   { "SWN",     SWN_T,     0,     TYPE_P,     705  },
   { "SWT",     SWT_T,     0,     TYPE_P,     705  },
   { "TAA",       'I',     1,     TYPE_B,     705  },
   { "TAB",       'I',     2,     TYPE_B,     705  },
   { "TAC",       'I',     3,     TYPE_B,     705  },
   { "TAD",       'I',     4,     TYPE_B,     705  },
   { "TAE",       'I',     5,     TYPE_B,     705  },
   { "TAF",       'I',     6,     TYPE_B,     705  },
   { "TAR",       'O',     9,     TYPE_B,    7080  },
   { "TCD",     TCD_T,     0,     TYPE_P,     705  },
   { "TCT",       ',',     8,     TYPE_B,    7080  },
   { "TEC",       'O',    13,     TYPE_B,     705  },
   { "TIC",       'O',    10,     TYPE_B,     705  },
   { "TIP",       ',',    14,     TYPE_B,    7080  },
   { "TITLE", TITLE_T,     0,     TYPE_P,     705  },
   { "TMC",       'O',    11,     TYPE_B,     705  },
   { "TMT",       '9',     0,     TYPE_A,     705  },
   { "TMTS",      '9',     1,     TYPE_B,     705  },
   { "TNS",       'I',     7,     TYPE_B,    7053  },
   { "TOC",       'O',    14,     TYPE_B,     705  },
   { "TR",        '1',     0,     TYPE_B,     705  },
   { "TRA",       'I',     0,     TYPE_A,     705  },
   { "TRANS", TRANS_T,     0,     TYPE_P,     705  },
   { "TRC",       'O',    12,     TYPE_B,     705  },
   { "TRE",       'L',     0,     TYPE_A,     705  },
   { "TRH",       'K',     0,     TYPE_A,     705  },
   { "TRS",       'O',     0,     TYPE_A,     705  },
   { "TRP",       'M',     0,     TYPE_A,     705  },
   { "TRZ",       'N',     0,     TYPE_A,     705  },
   { "TSA",       'O',     3,     TYPE_B,     705  },
   { "TSC",       'O',    15,     TYPE_B,     705  },
   { "TSL",       '1',     1,     TYPE_B,     705  },
   { "TTC",       'O',     2,     TYPE_B,     705  },
   { "TTR",       'O',     1,     TYPE_B,     705  },
   { "TZB",       '.',     0,     TYPE_A,     705  },
   { "UFC",       ',',     3,     TYPE_B,    7080  },
   { "ULA",       '*',     0,     TYPE_A,     705  },
   { "UNL",       '7',     0,     TYPE_A,     705  },
   { "USB",       ',',     5,     TYPE_B,    7080  },
   { "WMC",       'R',     5,     TYPE_B,    7053  },
   { "WR",        'R',     0,     TYPE_A,     705  },
   { "WRE",       'Z',     0,     TYPE_A,     705  },
   { "WRZ",       'Z',     1,     TYPE_B,     705  },
   { "WTM",       '3', 00100,     TYPE_D,     705  },
};

extern int cpumodel;		/* CPU model (705, 7053, 7080) */


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
