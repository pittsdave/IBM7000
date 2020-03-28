/***********************************************************************
*
* s709.c - IBM 7090 emulator main routines.
*
* Changes:
*   ??/??/??   PRP   Original. Version 1.0.0
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   02/07/05   DGP   Added command file.
*   05/10/05   DGP   Added signal handler invocation.
*   01/03/06   DGP   Added alternate BCD support.
*   02/11/06   DGP   Added CTSS cpu mode.
*   05/30/07   DGP   Changed channel operation, decouple from cycle_count.
*   06/06/07   DGP   Changed panel default to off.
*   02/20/08   DGP   Fix regression.
*   02/29/08   DGP   Changed to use bit mask flags.
*   02/12/10   DGP   Put unistd header include at top.
*   04/26/10   DGP   Fixed trap code to clear srh.
*   05/26/10   DGP   Fixed up interval timer.
*   06/12/10   DGP   Fixed STR for B-Core.
*   03/21/11   DGP   Added IFT/EFT to trace.
*   03/22/11   DGP   Added CPU_PROTMODE and CPU_RELOMODE support.
*   11/17/11   DGP   Tighten chk_user for CTSS cpu mode.
*   
***********************************************************************/

#define EXTERN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#if defined (WIN32)
#define __TTYROUTINES 0
#include <conio.h>
#include <windows.h>
#include <signal.h>
#include <process.h>
#endif
#if defined(UNIX)
#if defined(LINUX) || defined(SOLARIS) || defined(DARWIN)
#include <unistd.h>
#include <errno.h>
#endif
#include <pthread.h>
#endif
#if defined(USS)
#include <sys/time.h>
#endif

#include "sysdef.h"
#include "regs.h"
#include "trace.h"
#include "s709.h"
#include "io.h"
#include "chan7607.h"
#include "chan7909.h"
#include "negop.h"
#include "posop.h"
#include "sense.h"
#include "console.h"
#include "lights.h"
#include "screen.h"

FILE *trace_fd;
int prtviewlen;
int panelmode;
int outputline;
int traceinsts;
int traceuser;
int tagmode;

char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
char prtview[PRTVIEWLINENUM][PRTVIEWLINELEN+2];

static FILE *cmdfd;
static int usecmdfile;
static char cmdfile[256];

extern void sigintr (int);
extern char tonative[64];

static uint16 last_op = 0;
static uint16 last_iaddr = 0;
static uint16 last_idecr = 0;
static uint16 last_y = 0;
static uint8  last_tag = 0;
static uint8  last_flag = 0;
static int last_count = 0;
static int interval_clk = 0;

static char *index_inst[] = {
   "",     "TXI",  "TIX",  "TXH",  "",     "STR",  "TNX",  "TXL"
};
static char *neg_inst[] = {
/* 0100000 */   "HTR",  "",     "",     "",     "",     "",     "",     "", 
/* 0100010 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100020 */   "TRA",  "ESNT", "TRCB", "",     "TRCD", "",     "TRCF", "TRCH",
/* 0100030 */   "TEFB", "TEFD", "TEFF", "TEFH", "",     "",     "",     "",
/* 0100040 */   "",     "",     "RIA",  "",     "",     "",     "PIA",  "",
/* 0100050 */   "",     "IIL",  "",     "",     "LFT",  "SIL",  "LNT",  "RIL",
/* 0100060 */   "TCNA", "TCNB", "TCNC", "TCND", "TCNE", "TCNF", "TCNG", "TCNH",
/* 0100070 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100100 */   "TNZ",  "TIB",  "",     "",     "",     "",     "",     "",
/* 0100110 */   "",     "",     "",     "",     "CAQ",  "CAQ",  "CAQ",  "CAQ",
/* 0100120 */   "TMI",  "",     "",     "",     "",     "",     "",     "", 
/* 0100130 */   "XCL",  "",     "",     "",     "",     "",     "",     "", 
/* 0100140 */   "TNO",  "",     "",     "",     "",     "",     "",     "", 
/* 0100150 */   "",     "",     "",     "",     "CRQ",  "CRQ",  "CRQ",  "CRQ", 
/* 0100160 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100170 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100200 */   "MPR",  "",     "",     "",     "",     "",     "",     "", 
/* 0100210 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100220 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100230 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100240 */   "DFDH", "DFDP", "",     "",     "",     "",     "",     "",
/* 0100250 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100260 */   "UFM",  "DUFM", "",     "",     "",     "",     "",     "",
/* 0100270 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100300 */   "UFA",  "DUFA", "UFS",  "DUFS", "UAM",  "DUAM", "USM",  "DUSM",
/* 0100310 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100320 */   "ANA",  "",     "",     "",     "",     "",     "",     "",
/* 0100330 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100340 */   "LAS",  "",     "",     "",     "",     "",     "",     "",
/* 0100350 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100360 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100370 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100400 */   "SBM",  "",     "",     "",     "",     "",     "",     "",
/* 0100410 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100420 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100430 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100440 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100450 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100460 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100470 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100500 */   "CAL",  "ORA",  "",     "",     "",     "",     "",     "",
/* 0100510 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100520 */   "NZT",  "",     "",     "",     "",     "",     "",     "",
/* 0100530 */   "",     "",     "",     "",     "LXD",  "LDC",  "",     "",
/* 0100540 */   "RCHB", "RCHD", "RCHF", "RCHH", "LCHB", "LCHD", "LCHF", "LCHH",
/* 0100550 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100560 */   "",     "",     "",     "",     "LPI",  "",     "",     "",
/* 0100570 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100600 */   "STQ",  "",     "ORS",  "DST",  "",     "",     "",     "",
/* 0100610 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100620 */   "SLQ",  "",     "",     "",     "",     "STL",  "",     "",
/* 0100630 */   "",     "",     "",     "",     "SXD",  "",     "SCD",  "",
/* 0100640 */   "SCHB", "SCHD", "SCHF", "SCHH", "SCDB", "SCDD", "SCDF", "SCDH",
/* 0100650 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100660 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100670 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100700 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100710 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100720 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100730 */   "",     "",     "",     "",     "PDX",  "",     "",     "PDC",
/* 0100740 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0100750 */   "",     "",     "",     "",     "PXD",  "",     "PCD",  "",
/* 0100760 */   "",     "SEA",  "SEB",  "LGL",  "BSF",  "LGR",  "IFT",  "EFT",
/* 0100770 */   "",     "",     "RUN",  "RQL",  "AXC",  "",     "",     ""
};
static char *pos_inst[] = {
/* 0000000 */   "HTR",  "",     "",     "",     "",     "",     "",     "", 
/* 0000010 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000020 */   "TRA",  "TTR",  "TRCA", "",     "TRCC", "",     "TRCE", "TRCG",
/* 0000030 */   "TEFA", "TEFC", "TEFD", "TEFG", "",     "",     "",     "",
/* 0000040 */   "TLQ",  "IIA",  "TIO",  "OAI",  "PAI",  "",     "TIF",  "",
/* 0000050 */   "",     "IIR",  "",     "",     "RFT",  "SIR",  "RNT",  "RIR",
/* 0000060 */   "TCOA", "TCOB", "TCOC", "TCOD", "TCOE", "TCOF", "TCOG", "TCOH",
/* 0000070 */   "",     "",     "",     "",     "TSX",  "",     "",     "",
/* 0000100 */   "TZE",  "TIA",  "",     "",     "",     "",     "",     "",
/* 0000110 */   "",     "",     "",     "",     "CVR",  "CVR",  "CVR",  "CVR",
/* 0000120 */   "TPL",  "",     "",     "",     "",     "",     "",     "", 
/* 0000130 */   "",     "XCA",  "",     "",     "",     "",     "",     "", 
/* 0000140 */   "TOV",  "",     "",     "",     "",     "",     "",     "", 
/* 0000150 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000160 */   "",     "TQO",  "TQP",  "",     "",     "",     "",     "",
/* 0000170 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000200 */   "MPY",  "",     "",     "",     "VLM",  "VLM",  "",     "", 
/* 0000210 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000220 */   "DVH",  "DVP",  "",     "",     "VDH",  "VDP",  "",     "VDP",
/* 0000230 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000240 */   "FDH", "FDP",   "",     "",     "",     "",     "",     "",
/* 0000250 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000260 */   "FMP",  "DFMP", "",     "",     "",     "",     "",     "",
/* 0000270 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000300 */   "FAD",  "DFAD", "FSB",  "DFSB", "FAM",  "DFAM", "FSM",  "DFSM",
/* 0000310 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000320 */   "ANS",  "",     "ERA",  "",     "",     "",     "",     "",
/* 0000330 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000340 */   "CAS",  "",     "",     "",     "",     "",     "",     "",
/* 0000350 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000360 */   "",     "ACL",  "",     "",     "",     "",     "",     "",
/* 0000370 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000400 */   "ADD",  "ADM",  "SUB",  "",     "",     "",     "",     "",
/* 0000410 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000420 */   "HPR",  "",     "",     "",     "",     "",     "",     "",
/* 0000430 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000440 */   "IIS",  "LDI",  "OSI",  "DLD",  "OFT",  "RIS",  "ONT",  "",
/* 0000450 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000460 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000470 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000500 */   "CLA",  "",     "CLS",  "",     "",     "",     "",     "",
/* 0000510 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000520 */   "ZET",  "",     "XEC",  "",     "",     "",     "",     "",
/* 0000530 */   "",     "",     "",     "",     "LXA",  "LAC",  "",     "",
/* 0000540 */   "RCHA", "RCHC", "RCHE", "RCHG", "LCHA", "LCHC", "LCHE", "LCHG",
/* 0000550 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000560 */   "LDQ",  "",     "LRI",  "",     "ENB",  "",     "",     "",
/* 0000570 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000600 */   "STZ",  "STO",  "SLW",  "",     "STI",  "",     "",     "",
/* 0000610 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000620 */   "",     "STA",  "STD",  "",     "",     "STT",  "",     "",
/* 0000630 */   "STP",  "",     "",     "",     "SXA",  "",     "SCA",  "",
/* 0000640 */   "SCHA", "SCHC", "SCHE", "SCHG", "SCDA", "SCDC", "SCDE", "SCDG",
/* 0000650 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000660 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000670 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000700 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000710 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000720 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000730 */   "",     "",     "",     "",     "PAX",  "",     "",     "PAC",
/* 0000740 */   "",     "",     "",     "",     "",     "",     "",     "",
/* 0000750 */   "",     "",     "",     "",     "PXA",  "",     "PCA",  "",
/* 0000760 */   "",     "NOP",  "RDS",  "LLS",  "BSR",  "LRS",  "WRS",  "ALS",
/* 0000770 */   "WEF",  "ARS",  "REW",  "",     "AXT",  "",     "SDN",  ""
};
static char *nsense_inst[] = {
/* 0000000 */   "CLM",  "PBT",  "ETFM", "SSM",  "LFTM", "ESTM", "ECTM", "LTM", 
/* 0000010 */   "LSNM", "",     "",     "",     "",     "",     "EMTM", "", 
/* 0000020 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000030 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000040 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000050 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000060 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000070 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000100 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000110 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000120 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000130 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000140 */   "SLF",  "SLT1", "SLT2", "SLT3", "SLT4", "",     "",     "", 
/* 0000150 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000160 */   "",     "SWT1", "SWT2", "SWT3", "SWT4", "SWT5", "SWT6", "", 
/* 0000170 */   "",     "",     "",     "",     "",     "",     "",     ""
};
static char *psense_inst[] = {
/* 0000000 */   "CLM",  "LBT",  "CHS",  "SSP",  "ENK",  "IOT",  "COM",  "ETM", 
/* 0000010 */   "RND",  "FRN",  "DCT",  "",     "RCT",  "",     "LMTM", "", 
/* 0000020 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000030 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000040 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000050 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000060 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000070 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000100 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000110 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000120 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000130 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000140 */   "SLF",  "SLN1", "SLN2", "SLN3", "SLN4", "",     "",     "", 
/* 0000150 */   "",     "",     "",     "",     "",     "",     "",     "", 
/* 0000160 */   "",     "SWT1", "SWT2", "SWT3", "SWT4", "SWT5", "SWT6", "", 
/* 0000170 */   "",     "",     "",     "",     "",     "",     "",     ""
};

#ifdef TRACEEOF
static uint32 eaoldop = 0777777;
static uint32 ea_count = 0;
#endif

#ifdef DEBUG7909
#define DEBUGCHAN 4
#endif

/***********************************************************************
* getxr - Get index register contents.
***********************************************************************/

uint16
getxr (int xrx)
{
   if (tag == 0) return (0);
   if (cpuflags & CPU_MULTITAG)
   {
      uint16 r;

      r = 0;
      if (tag & 1)
         r |= xr[1];
      if (tag & 2)
         r |= xr[2];
      if (tag & 4)
         r |= xr[4];
      if (xrx) setxr (r);
      return r & 077777;
   }
   return xr[tag] & 077777;
}

/***********************************************************************
* setxr - Set index register contents to new value.
***********************************************************************/

void
setxr (uint32 r)
{
   if (tag != 0)
   {
      r &= 077777;
      if (cpuflags & CPU_MULTITAG)
      {
	 if (tag & 1)
	    xr[1] = r;
	 if (tag & 2)
	    xr[2] = r;
	 if (tag & 4)
	    xr[4] = r;
      }
      else
	 xr[tag] = r;
   }
}

/***********************************************************************
* settrap - Set trap values.
***********************************************************************/

void
settrap (uint16 ta, uint16 pa, uint16 decr)
{
   uint32 ldecr = decr;

   if (bcoredata)
      ldecr |= BDATA_TRAP;
   if (bcoreinst)
      ldecr |= BINST_TRAP;

   LOADA (ta);
#ifdef USE64
   ilr = pa | (ilr & (PREMASK|TAGMASK)) | (t_uint64)ldecr << DECRSHIFT;
   STOREA (ta, ilr);
#else
   ilrh = (ilrh & (SIGN | 06)) | ldecr >> 14;
   ilrl = pa | (ilrl & TAGMASK) | ldecr << DECRSHIFT;
   STOREA (ta, ilrh, ilrl);
#endif

   ic = ta + 1;
   bcoredata = 0;
   bcoreinst = 0;
   trap_enb = 0;
   trap_inhibit = 2;
   if (ta == PROT_TRAP)
      cpuflags |= CPU_PROTTRAP;
   cpuflags &= ~(CPU_PROGSTOP|CPU_USERMODE|CPU_RELOMODE|CPU_PROTMODE);
}

/***********************************************************************
* access1 - Access memory for current instruction.
***********************************************************************/

void
access1 (uint16 a)
{
   if (cpuflags & CPU_USERMODE)
   {
      if (cpuflags & CPU_RELOMODE)
	 a = (a + prog_reloc) & ADDRMASK;
      if ((cpuflags & CPU_PROTMODE) && ((a < prog_base) || (a > prog_limit)))
      {
	 settrap (PROT_TRAP, ic, 0);
         return;
      }
   }
#ifdef USE64
   sr = mem[a|bcoredata];
#else
   srh = memh[a|bcoredata];
   srl = meml[a|bcoredata];
#endif
   CYCLE ();
}

/***********************************************************************
* access2 - Access second word for double precision instructions.
***********************************************************************/

void
access2 (uint16 a)
{
   if (cpuflags & CPU_USERMODE)
   {
      if (cpuflags & CPU_RELOMODE)
	 a = (a + prog_reloc) & ADDRMASK;
      if ((cpuflags & CPU_PROTMODE) && ((a < prog_base) || (a > prog_limit)))
      {
	 settrap (PROT_TRAP, ic, 0);
         return;
      }
   }
#ifdef USE64
   sr2 = mem[a|bcoredata];
#else
   sr2h = memh[a|bcoredata];
   sr2l = meml[a|bcoredata];
#endif
   CYCLE ();
}

/***********************************************************************
* load - Load instruction/data from memory.
***********************************************************************/

void
load (uint16 a, uint8 reladdr)
{
   if (!reladdr)
   {
#ifdef USE64
      ilr = mem[a];
#else
      ilrh = memh[a];
      ilrl = meml[a];
#endif
      CYCLE ();
      return;
   }
   if (cpuflags & CPU_USERMODE)
   {
      if (cpuflags & CPU_RELOMODE)
	 a = (a + prog_reloc) & ADDRMASK;
      if ((cpuflags & CPU_PROTMODE) && ((a < prog_base) || (a > prog_limit)))
      {
	 settrap (PROT_TRAP, ic, 0);
         return;
      }
   }

#ifdef USE64
   ilr = mem[a|bcoreinst];
#else
   ilrh = memh[a|bcoreinst];
   ilrl = meml[a|bcoreinst];
#endif
   CYCLE ();
}

/***********************************************************************
* store - Store results into memory.
***********************************************************************/

#ifdef USE64
void
store (uint16 a, t_uint64 v, uint8 reladdr)
{
   if (!reladdr)
   {
      mem[a] = v;
      if (watchstop && watchloc == a)
      {
         run = CPU_STOP;
	 atbreak = TRUE;
      }
      return;
   }
   if (cpuflags & CPU_USERMODE)
   {
      if (cpuflags & CPU_RELOMODE)
	 a = (a + prog_reloc) & ADDRMASK;
      if ((cpuflags & CPU_PROTMODE) && ((a < prog_base) || (a > prog_limit)))
      {
	 settrap (PROT_TRAP, ic, 0);
         return;
      }
   }
   mem[a|bcoredata] = v;
   if (watchstop && watchloc == (a|bcoredata))
   {
      run = CPU_STOP;
      atbreak = TRUE;
   }
}
#else
void
store (uint16 a, uint8 h, uint32 l, uint8 reladdr)
{
   if (!reladdr)
   {
      memh[a] = h;
      meml[a] = l;
      if (watchstop && watchloc == a)
      {
         run = CPU_STOP;
	 atbreak = TRUE;
      }
      return;
   }
   if (cpuflags & CPU_USERMODE)
   {
      if (cpuflags & CPU_RELOMODE)
	 a = (a + prog_reloc) & ADDRMASK;
      if ((cpuflags & CPU_PROTMODE) && ((a < prog_base) || (a > prog_limit)))
      {
	 settrap (PROT_TRAP, ic, 0);
         return;
      }
   }
   memh[a|bcoredata] = h;
   meml[a|bcoredata] = l;
   if (watchstop && watchloc == (a|bcoredata))
   {
      run = CPU_STOP;
      atbreak = TRUE;
   }
}
#endif

/***********************************************************************
* traptrace - Provide trap tracing.
***********************************************************************/

void
traptrace ()
{
   LOADA (0);
#ifdef USE64
   STOREA (0, (ilr & ~ADDRMASK) | (ic - 1));
#else
   STOREA (0, ilrh, (ilrl & ~ADDRMASK) | (ic - 1));
#endif
}

/***********************************************************************
* steal_cycle - Check if time to steal a cycle.
***********************************************************************/

void
steal_cycle ()
{
   if (single_cycle)
      console ();
   if (cycle_count >= next_lights)
   {
      lights ();
      next_lights = cycle_count + NEXTLIGHTS;
      check_intr ();
   }
   next_steal = next_lights;
}

/***********************************************************************
* chk_user - Check if in user mode for privileged instructions.
***********************************************************************/

int
chk_user ()
{
   if ((cpumode == 7096) && (cpuflags & CPU_USERMODE))
   {
      settrap (PROT_TRAP, ic, 0);
      return TRUE;
   }
   return FALSE;
}

/***************************************************************************
* smallsleep - sleep in U seconds
* NOTE: Depending on the system the sleep time can vary widely.
***************************************************************************/

static void
smallsleep (long Usec)
{
#if defined(UNIX)
#if defined(LINUX) || defined(SOLARIS) || defined(DARWIN)
#include <unistd.h>
#include <errno.h>
   struct timespec req, rem;
#endif
#if defined(LINUX)
#ifdef DEBUGCLOCK
   static clock_t starttime;
   static clock_t curtime;
   static int ticks;
   static int first = TRUE;

   if (first)
   {
      first = FALSE;
      ticks = 0;
      starttime = time (NULL) + 1;
   }
#endif

   req.tv_sec = 0;
   req.tv_nsec = Usec * 1000;
   nanosleep (&req, &rem);
#else /* ! LINUX */
   struct timeval tv;

   tv.tv_sec = 0;
   tv.tv_usec = Usec;
   if (select (1, NULL, NULL, NULL, &tv) < 0)
   {
#if defined(EINTR) || defined(EAGAIN)
      if (errno != EINTR && errno != EAGAIN)
#endif
	 perror ("smallsleep: select failed");
   }
#endif
#endif /* UNIX */

#if defined(WIN32)
   Sleep ((unsigned long) (Usec) / 1000L);
#endif /* WIN32 */

#if defined(OS2)
   DosSleep ((unsigned long) (Usec) / 1000L);
#endif /* OS2 */

}

/***********************************************************************
* clock_thread - run the CTSS interval timer.
***********************************************************************/

static void
clock_thread (void)
{
#ifdef USE64
   t_uint64 clock;
#else
   uint8 clockh;
   uint32 clockl;
#endif

   while (1)
   {
      smallsleep (16666);
      if (atbreak) continue;

#ifdef USE64
      clock = mem[CLOCK];
      clock = (clock + 1) & MAGMASK;
      mem[CLOCK] = clock;
      if (clock == 0) interval_clk = 1;
#else
      clockl = meml[CLOCK];
      clockh = memh[CLOCK];
      clockl++;
      if (clockl == 0) clockh++;
      clockh &= HMSK;
      memh[CLOCK] = clockh;
      meml[CLOCK] = clockl;
      if (clockh == 0 && clockl == 0) interval_clk = 1;
#endif
   }
}

/***********************************************************************
* startclk - Start clock.
***********************************************************************/

static void
startclk (void)
{
#if defined(UNIX)
   pthread_t thread_handle;
#if 0
   struct sched_param thread_param;
   int thread_policy;
#endif
#endif

   interval_clk = 0;
#if defined(UNIX)
   if (pthread_create (&thread_handle,
		       NULL,
		       (void *(*)(void *)) clock_thread,
		       NULL) < 0)
#endif
#if defined(WIN32)
   if (_beginthread (clock_thread,
		     NULL,
		     NULL) < 0)
#endif
      perror ("startclk: Can't start clock thread");
#if 0
#if defined(UNIX)
   if (pthread_getschedparam (thread_handle,
			      &thread_policy,
			      &thread_param) != 0)
      perror ("startclk: Can't get sched policy on clock thread");
#ifdef DEBUGCLOCK
   fprintf (stderr, "startclk: thread_policy = %d\n", thread_policy);
   fprintf (stderr, "startclk: sched_priority = %d\n",
	    thread_param.__sched_priority);
#endif
      
   thread_param.__sched_priority = 20;
   if (pthread_setschedparam (thread_handle, SCHED_FIFO, &thread_param) != 0)
      perror ("startclk: Can't alter sched policy on clock thread");
#endif
#endif

}

/***********************************************************************
* trace_open - Open the trace file.
***********************************************************************/

int
trace_open (char *file)
{
   char *ep;

   while (*file && isspace (*file)) file++;
   ep = file;
   while (*ep && !isspace (*ep)) ep++;
   *ep = '\0';
   if ((trace_fd = fopen (file, "w")) == NULL)
   {
      sprintf (errview[0], "Can't open: %s: %s", file, strerror(errno));
      return (-1);
   }
   return (0);
}

/***********************************************************************
* trace_close - Close the trace file.
***********************************************************************/

void
trace_close (void)
{
   fclose (trace_fd);
}

/***********************************************************************
* trace_word - print the word as characters.
***********************************************************************/

#ifdef USE64
static void
trace_word (t_uint64 val)
{
   fprintf (trace_fd, "%c%c%c%c%c%c",
           tonative[(val >> 30) & 077],
           tonative[(val >> 24) & 077],
           tonative[(val >> 18) & 077],
           tonative[(val >> 12) & 077],
           tonative[(val >>  6) & 077],
           tonative[ val        & 077]);
}
#else
static void
trace_word (uint8 valh, uint32 vall)
{
   uint32 c1;

   c1 = ((valh & SIGN) >> 2) | ((valh & HMSK) << 2) | (vall >> 30);
   fprintf (trace_fd, "%c%c%c%c%c%c",
           tonative[ c1          & 077],
           tonative[(vall >> 24) & 077],
           tonative[(vall >> 18) & 077],
           tonative[(vall >> 12) & 077],
           tonative[(vall >>  6) & 077],
           tonative[ vall        & 077]);
}
#endif

/***********************************************************************
* trace_inst - Trace the instructions.
***********************************************************************/

static void
trace_inst (void)
{
   int printed = FALSE;
   char printline[100];

   if (!traceinsts) return;
   if (traceuser && !bcoreinst) return;

   if (op & 03000) /* INDEX instructions */
   {
      if (!(op == last_op && iaddr == last_iaddr && tag == last_tag &&
	  idecr == last_idecr))
      {
	 printed = TRUE;
	 sprintf (printline, "%06o\t%s\t%05o,%o,%05o\n",
		  (ic-1) | bcoreinst,
		  index_inst[((op & 0100000) >> 13) | ((op & 0003000) >> 9)],
		  iaddr, tag, idecr);
      }
   }
   else if ((op & 0007777) == 0000760) /* PSE, MSE */
   {
      if (!(op == last_op && iaddr == last_iaddr && tag == last_tag &&
	  flag == last_flag && y == last_y))
      {
	 printed = TRUE;
	 if ((op &  0100000) == 0)
	 {
	    if (y < 0170)
	       sprintf (printline, "%06o\t%s%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			psense_inst[y & 0177],
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	    else if ((y & 000770) == 000000)
	       sprintf (printline, "%06o\t%s%c%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			"BTT", 'A'+((y >> 9) - 1),
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	    else if ((y & 000777) == 000350)
	       sprintf (printline, "%06o\t%s%c%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			"RIC", 'A'+((y >> 9) - 1),
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	    else if ((y & 000777) == 000352)
	       sprintf (printline, "%06o\t%s%c%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			"RCD", 'A'+((y >> 9) - 1),
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	    else if ((y & 000760) == 000360)
	       sprintf (printline, "%06o\t%s%c%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			"SPR", 'A'+((y >> 9) - 1),
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	 }
	 else
	 {
	    if (y < 0170)
	       sprintf (printline, "%06o\t%s%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			nsense_inst[y & 0177],
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	    else if ((y & 000770) == 000000)
	       sprintf (printline, "%06o\t%s%c%c\t%05o,%o   y = %06o\n",
			(ic-1) | bcoreinst,
			"ETT", 'A'+((y >> 9) - 1),
			flag ? '*' : ' ',
			iaddr, tag, y | bcoredata);
	 }
      }
   }
   else if ((op & 0100000) == 0) /* Positive opcodes */
   {
      if (!(op == last_op && iaddr == last_iaddr && tag == last_tag &&
	  flag == last_flag && y == last_y))
      {
	 int pop = op & 0777;

	 printed = TRUE;
	 if (pop > 050 && pop < 060) /* Type D */
	    sprintf (printline, "%06o\t%s%c\t%o%05o\n",
		     (ic-1) | bcoreinst,
		     pos_inst[pop],
		     flag ? '*' : ' ',
		     tag, iaddr);
	 else
	    sprintf (printline, "%06o\t%s%c\t%05o,%o   y = %06o\n",
		     (ic-1) | bcoreinst,
		     pos_inst[pop],
		     flag ? '*' : ' ',
		     iaddr, tag, y | bcoredata);
      }
   }
   else /* Negative opcodes */
   {
      if (!(op == last_op && iaddr == last_iaddr && tag == last_tag &&
	  flag == last_flag && y == last_y))
      {
	 int nop = op & 0777;

	 if (nop == 0761 && iaddr == 042) /* SEB */
	    nop++;
	 else if (nop == 0761 && iaddr == 043) /* IFT */
	    nop += 5;
	 else if (nop == 0761 && iaddr == 044) /* EFT */
	    nop += 6;
	 printed = TRUE;
	 if (nop > 050 && nop < 060) /* Type D */
	    sprintf (printline, "%06o\t%s%c\t%o%05o\n",
		     (ic-1) | bcoreinst,
		     neg_inst[nop],
		     flag ? '*' : ' ',
		     tag, iaddr);
	 else
	    sprintf (printline, "%06o\t%s%c\t%05o,%o   y = %06o\n",
		     (ic-1) | bcoreinst,
		     neg_inst[nop],
		     flag ? '*' : ' ',
		     iaddr, tag, y | bcoredata);
      }
   }

   if (printed)
   {
      if (last_count)
	 fprintf (trace_fd, "   last instruction repeats %d times\n",
		  last_count);
      /*fprintf (trace_fd, "%d: ", inst_count);*/
      fputs (printline, trace_fd);
      fprintf (trace_fd,
	 "   SSW %o%o%o%o%o%o  SL %o%o%o%o  ENB 00%010lo  cpuflags %06o\n",
	       (ssw >> 5) & 1,
	       (ssw >> 4) & 1,
	       (ssw >> 3) & 1,
	       (ssw >> 2) & 1,
	       (ssw >> 1) & 1,
	       ssw & 1,
	       (sl >> 3) & 1,
	       (sl >> 2) & 1,
	       (sl >> 1) & 1,
	       sl & 1,
	       (unsigned long)trap_enb, cpuflags);
      if (cpumode >= 7094)
	 fprintf (trace_fd,
       "   X1 %05o  X2 %05o  X3 %05o  X4 %05o  X5 %05o  X6 %05o  X7 %05o\n",
		  xr[1], xr[2], xr[3], xr[4], xr[5], xr[6], xr[7]);
      else
	 fprintf (trace_fd, "   X1 %05o  X2 %05o  X4 %05o\n",
		  xr[1], xr[2], xr[4]);
#ifdef USE64
      fprintf (trace_fd, "   AC %c%013llo  MQ %c %012llo  SI %012llo\n",
	       (ac & ACSIGN)? '-' : '+',
	       ac & (Q|DATAMASK),
	       (mq & SIGN)? '-' : '+',
	       mq & MAGMASK,
	       si);
      fputs ("         '", trace_fd);
      trace_word (ac);
      fputs ("'            '", trace_fd);
      trace_word (mq);
      fputs ("'         '", trace_fd);
      trace_word (si);
      fputs ("'\n", trace_fd);
#else
      fprintf (trace_fd,
	       "   AC %c%03o%010lo  MQ %c %02o%010lo  SI %02o%010lo\n",
	       (ach & SIGN)? '-' : '+',
	       ((ach & 037) << 2) | (short)(acl >> 30),
	       (unsigned long)acl & 07777777777,
	       (mqh & SIGN)? '-' : '+',
	       ((mqh & HMSK) << 2) | (short)(mql >> 30),
	       (unsigned long)mql & 07777777777,
	       ((sih & 017) << 2) | (short)(sil >> 30),
	       (unsigned long)sil & 07777777777);
      fputs ("         '", trace_fd);
      trace_word (ach, acl);
      fputs ("'            '", trace_fd);
      trace_word (mqh, mql);
      fputs ("'         '", trace_fd);
      trace_word (sih, sil);
      fputs ("'\n", trace_fd);
#endif

      last_count = 0;
      last_op = op;
      last_iaddr = iaddr;
      last_tag = tag;
      last_y = y;
      last_idecr = idecr;
   }
   else
   {
      last_count++;
   }
}

/***********************************************************************
* do_trap - Perform the trap.
***********************************************************************/

static void
do_trap (int ch, uint16 y)
{
   uint32 ldecr = 0;

   if (bcoredata)
      ldecr |= BDATA_TRAP;
   if (bcoreinst)
      ldecr |= BINST_TRAP;

   LOADA (y);
#ifdef USE64
   sr |= ic | (ilr & (PREMASK|TAGMASK)) | ((t_uint64)ldecr << DECRSHIFT);
   STOREA (y, sr);
#else
   srh |= (ilrh & (SIGN | 06)) | ldecr >> 14;
   srl |= ic | (ilrl & TAGMASK) | (ldecr << DECRSHIFT);
   STOREA (y, srh, srl);
#endif

#if defined(DEBUGTRAP) || defined(DEBUG7607)
   if (!channel[ch].ctype)
   {
      fprintf (stderr, "%d: %06o: ", inst_count, ic ? ic-1 : 0);
      fprintf (stderr,
	    "TRAP: Channel %c trap_enb = %09o, trap = %s",
	    'A' + ch, trap_enb,
	    cpuflags & CPU_TRAPMODE ? "Y" : "N");
#ifdef USE64
      fprintf (stderr, ", sr = %c%012llo, addr = %o\n",
	       (sr & SIGN)? '-' : '+', sr & MAGMASK, y);
#else
      fprintf (stderr, ", sr = %c%02o%010lo, addr = %o\n",
	       (srh & SIGN)? '-' : '+',
	       ((srh & HMSK) << 2) | (short)(srl >> 30),
	       (unsigned long)srl & 07777777777,
	       y);
#endif
   }
#endif
#if defined(DEBUGTRAP) || defined(DEBUG7909)
   if (ch == DEBUGCHAN)
   {
   if (channel[ch].ctype)
   {
      fprintf (stderr, "%d: %06o: ", inst_count, ic ? ic-1 : 0);
      fprintf (stderr,
	    "TRAP: Channel %c trap_enb = %09o, trap = %s",
	    'A' + ch, trap_enb,
	    cpuflags & CPU_TRAPMODE ? "Y" : "N");
#ifdef USE64
      fprintf (stderr, ", sr = %c%012llo, addr = %o\n",
	       (sr & SIGN)? '-' : '+', sr & MAGMASK, y);
#else
      fprintf (stderr, ", sr = %c%02o%010lo, addr = %o\n",
	       (srh & SIGN)? '-' : '+',
	       ((srh & HMSK) << 2) | (short)(srl >> 30),
	       (unsigned long)srl & 07777777777,
	       y);
#endif
   }
   }
#endif

   ic = y + 1;
   LOADA (ic);
#ifdef USE64
   sr = ilr;
#else
   srh = ilrh;
   srl = ilrl;
#endif

   bcoredata = 0;
   bcoreinst = 0;
   trap_enb = 0;
   trap_inhibit = 2;
   cpuflags &= ~(CPU_PROGSTOP|CPU_USERMODE|CPU_RELOMODE|CPU_PROTMODE);
}

/***********************************************************************
* s709 - The main simulator loop.
***********************************************************************/

static void
s709 ()
{
   int    htrzero;
   int32  i;

   clear ();
   run = CPU_STOP;
   trap_inhibit = 0;

   if (cpumode == 7096) startclk ();

   for (;;)
   {
      if (((cpuflags & CPU_PROGSTOP) && (chan_in_op == 0)) ||
           (addrstop && (ic|bcoreinst) == stopic))
      {
         run = CPU_STOP;
	 atbreak = TRUE;
      }
      if ((uint32)tagmode != (cpuflags & CPU_MULTITAG))
      {
         run = CPU_STOP;
	 addrstop = TRUE;
	 stopic = ic|bcoreinst;
	 atbreak = TRUE;
      }

      if ((run != CPU_RUN) || !(cpuflags & CPU_AUTO))
      {
         run = CPU_STOP;
	 if (usecmdfile)
	 {
	    if (commands (cmdfd) < 0)
	       usecmdfile = FALSE;
	 }
	 else
	 {
	    console ();
	 }

         if (!run)
            continue;
         if (run == CPU_EXEC)
	 {
            run = CPU_STOP;
            goto exec;
         }
         if (run == CPU_CYCLE)
	 {
            run = CPU_RUN;
            goto exec;
         }
	 if (cpuflags & CPU_PROGSTOP)
            ic = iaddr;
	 cpuflags &= ~CPU_PROGSTOP;
      }
      if (cpuflags & CPU_PROGSTOP)
      {
         cycle_count++;
         goto chans;
      }

      /*
       * Fetch
       */

      LOAD (ic);
      htrzero = FALSE;
      if (cpuflags & CPU_PROTTRAP)
         goto TAKE_PROTTRAP;
#ifdef USE64
      sr = ilr;
      if (sr == 0002000000000) htrzero = TRUE;
      itrc[itrc_idx] = sr;
#else
      srh = ilrh;
      srl = ilrl;
      if (srh == 0 && srl == 02000000000) htrzero = TRUE;
      itrc_h[itrc_idx] = srh;
      itrc_l[itrc_idx] = srl;
#endif
      itrc_buf[itrc_idx++] = ic | bcoreinst;
      if (itrc_idx == TRACE_SIZE)
         itrc_idx = 0;

      /*
      ** In CTSS mode: If we're doing a TRA 0 at 0, then we're idle.
      ** So, yield the processor for a bit.
      */

      if (ic == 0 && cpumode == 7096 && trap_enb && htrzero)
      {
         smallsleep (1);
      }
      ic++;

   exec:
      inst_count++;

#ifdef USE64
      op = ((sr & SIGN) >> 20) | ((sr & OPMASK) >> OPSHIFT);
      flag = (sr & FLAGMASK) == FLAGMASK;
      tag =  (sr & TAGMASK) >> TAGSHIFT;
      iaddr = (sr & ADDRMASK);
#else
      op = (srh << 8) | ((srl & OPMASK) >> OPSHIFT);
      flag =  (srl & FLAGMASK) == FLAGMASK;
      tag =  ((srl & TAGMASK) >> TAGSHIFT);
      iaddr =  (srl & ADDRMASK);
#endif

      CYCLE ();
      if (op & 03000)
      {
#ifdef USE64
         idecr = (sr & DECRMASK) >> DECRSHIFT;
#else
         idecr = ((srh & 1) << 14) | ((srl & DECRMASK) >> DECRSHIFT);
#endif
	 trace_inst ();
         switch (op & 0103000)
	 {

         case 0101000:		/* STR */
            LOAD (0);
#ifdef USE64
            STORE (0, (ilr & ~ADDRMASK) | ic);
#else
            STORE (0, ilrh, (ilrl & ~ADDRMASK) | ic);
#endif
            ic = 00002;
	    cpuflags &= ~(CPU_RELOMODE|CPU_PROTMODE);
            CYCLE ();
            break;

         case 0001000:		/* TXI */
	    if (cpuflags & CPU_TRAPMODE)
               traptrace ();
            setxr (getxr (TRUE) + idecr);
	    if (cpuflags & CPU_TRAPMODE)
               ic = 00001;
            else
               ic = iaddr;
            CYCLE ();
            break;

         case 0002000:		/* TIX */
	    if (cpuflags & CPU_TRAPMODE)
               traptrace ();
            if (getxr (TRUE) > idecr)
	    {
               setxr (getxr (TRUE) - idecr);
	       if (cpuflags & CPU_TRAPMODE)
                  ic = 00001;
               else
                  ic = iaddr;
            }
            CYCLE ();
            break;

         case 0102000:		/* TNX */
	    if (cpuflags & CPU_TRAPMODE)
               traptrace ();
            if (getxr (TRUE) <= idecr)
	    {
	       if (cpuflags & CPU_TRAPMODE)
                  ic = 00001;
               else
                  ic = iaddr;
            }
	    else
               setxr (getxr (TRUE) - idecr);
            CYCLE ();
            break;

         case 0003000:		/* TXH */
	    if (cpuflags & CPU_TRAPMODE)
               traptrace ();
            if (getxr (TRUE) > idecr)
	    {
	       if (cpuflags & CPU_TRAPMODE)
                  ic = 00001;
               else
                  ic = iaddr;
            }
            CYCLE ();
            break;

         case 0103000:		/* TXL */
	    if (cpuflags & CPU_TRAPMODE)
               traptrace ();
            if (getxr (TRUE) <= idecr)
	    {
	       if (cpuflags & CPU_TRAPMODE)
                  ic = 00001;
               else
                  ic = iaddr;
            }
            CYCLE ();
            break;

         }
      }

      else
      {
         if (flag)
	 {
            switch (op)
	    {
            case 0000767:	/* ALS */
            case 0000771:	/* ARS */
            case 0000763:	/* LLS */
            case 0000765:	/* LRS */
            case 0100763:	/* LGL */
            case 0100765:	/* LGR */
            case 0100773:	/* RQL */
               flag = 0;
	    default: ;
            }
         }
         if (flag)
	 {
	    uint16 laddr;
	    uint8  t;

            LOAD ((iaddr - getxr (FALSE)) & MEMLIMIT);
	    if (cpuflags & CPU_PROTTRAP)
	       goto TAKE_PROTTRAP;
            t = tag;
#ifdef USE64
            tag = (ilr & TAGMASK) >> TAGSHIFT;
	    laddr = ilr & ADDRMASK;
#else
            tag = (ilrl & TAGMASK) >> TAGSHIFT;
	    laddr = ilrl & ADDRMASK;
#endif
            y = (laddr - getxr (FALSE)) & MEMLIMIT;
            tag = t;
         }
	 else
            y = (iaddr - getxr (FALSE)) & MEMLIMIT;

#ifdef TRACEEOF
	if (y == 0450)
	{
	   t_uint64 val;
	   if (op != eaoldop)
	   {
	      val = mem[y];
	      fprintf (stderr,\
		    "%05o: op = %04o, ea = %05o, (ea) = %012llo, repeat = %d\n",
		       ic-1, op, y, val, ea_count);
	      fprintf (stderr, "   X1 %05o X2 %05o X4 %05o\n",
		       xr[1], xr[2], xr[4]);
	      fprintf (stderr, "   AC %012llo MQ %012llo SI %012llo\n",
		       ac, mq, si);
	      ea_count = 1;
	      /*eaoldop = op;*/
	   }
	   else
	   {
	      ea_count++;
	   }
	}
#endif
         if (op == 0000522)   /* XEC */
	 {
	    trace_inst();
            LOAD (y);
	    if (cpuflags & CPU_PROTTRAP)
	       goto TAKE_PROTTRAP;
#ifdef USE64
            sr = ilr;
#else
            srh = ilrh;
            srl = ilrl;
#endif
	    trap_inhibit = 1;
            goto exec;
         }
	 else if ((op & 0007777) == 0000760) /* PSE, MSE */
	 {
	    trace_inst();
	    sense_instr ();
	    CYCLE ();
         }
	 else if ( (op & 0100000) == 0 )
	 {
	    trace_inst();
            posop ();
	 }
	 else
	 {
	    trace_inst();
            negop ();
	 }
      }
      single_cycle = FALSE;
      if (spill)
      {
	 if (cpuflags & CPU_FPTRAP)
	 {
	    settrap (STD_TRAP, ic, spill);
	    ic = FP_TRAP;
	 }
         else
	 {
	    if (spill & SPILL_MQ) cpuflags |= CPU_MQOFLOW;
	    if (spill & SPILL_AC) cpuflags |= CPU_ACOFLOW;
	 }
	 spill = 0;
      }

      if (cpuflags & CPU_PROTTRAP)
      {
TAKE_PROTTRAP:
	 cpuflags &= ~CPU_PROTTRAP;
	 LOADA (ic);
#ifdef USE64
	 sr = ilr;
#else
	 srh = ilrh;
	 srl = ilrl;
#endif
         goto exec;
      }

   chans:

      /*
      ** Check for interval clock interrupt
      */

      if (!trap_inhibit && (trap_enb & 0400000) && interval_clk)
      {
         interval_clk = 0;
#ifdef USE64
	 sr = 0;
#else
	 srh = 0;
         srl = 0;
#endif
	 do_trap (0, CLOCK_TRAP);
	 goto exec;
      }

      /*
      ** Check for channels needing attention
      */

      for (i = 0; i < numchan; i++)
      {
         if (channel[i].csel && channel[i].ccyc)
	 {
            if (channel[i].ccyc > 1)
	    {
	       channel[i].ccyc--;
	    }
            else
            {
               if (channel[i].cact)
               {
                  if (channel[i].ctype && !(channel[i].cflags & CHAN_CTSSDRUM))
		  {
                     active_7909 (i);
		  }
                  else
		  {
                     active_chan (i);
		  }
               }
               else
               {
                  endrecord (i, 0);
                  channel[i].csel = NOT_SEL;
               }
            }
         }

         if (!trap_inhibit && (cpuflags & CPU_CHTRAP))
	 {
            if (trap_enb & (1 << i))
	    {
               if (channel[i].cflags & CHAN_TRAPPEND)
	       {
#if defined(DEBUGIO) || defined(DEBUG7607)
		  fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
                  fprintf (stderr, "TRAP: Channel %c run = %d\n", 'A'+i, run);
#endif
		  channel[i].cflags &= ~CHAN_TRAPPEND;
		  chan_in_op &= ~(1 << i);
#ifdef USE64
                  sr = 0000001000000;
#else
		  srh = 0;
                  srl = 0000001000000;
#endif
                  goto chan_trap;
               }
               if (channel[i].cflags & CHAN_EOF)
	       {
#if defined(DEBUGIO) || defined(DEBUG7607)
		  fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
                  fprintf (stderr,
			   "TRAP: Channel %c Clear EOF \n", i+'A');
#endif
                  channel[i].cflags &= ~CHAN_EOF;
#ifdef USE64
                  sr = 0000004000000;
#else
		  srh = 0;
                  srl = 0000004000000;
#endif
                  goto chan_trap;
               }
            }
	    if ((trap_enb & (01000000 << i)) &&
		 (channel[i].cflags & CHAN_CHECK))
	    {
#if defined(DEBUGIO) || defined(DEBUG7607)
	       fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	       fprintf (stderr,
			"TRAP: Channel %c CHECK \n", i+'A');
#endif
	       channel[i].cflags &= ~CHAN_CHECK;
#ifdef USE64
	       sr = 0000002000000;
#else
	       srh = 0;
	       srl = 0000002000000;
#endif
	    chan_trap:

               channel[i].ccyc = 0;
               y = (i << 1) + CHAN_TRAP;
	       do_trap (i, y);
               goto exec;
            }
         }

	 /*
	 ** Check for 7909/7750 interupts.
	 */
	 if (channel[i].ctype)
	 {
	    check_7909 (i);
	 }
      }

      if (trap_inhibit)
	 trap_inhibit --;
   }
}

/***********************************************************************
* Main procedure.
***********************************************************************/

int
main (int argc, char **argv)
{
   int   i;

   cmdfile[0] = '\0';
   cpumode = 7090;
   strcpy (txtcpumode, "7090");
   windowlen = 25;
   numchan = NUMCHAN;
   panelmode = FALSE;
   usecmdfile = FALSE;
   traceinsts = FALSE;
   traceuser = FALSE;
   altbcd = FALSE;
   tagmode = CPU_MULTITAG;
   ioinit ();

   for (i = 1; i < argc; i++)
   {
      if (*argv[i] == '-')
      {
	 char *bp = &argv[i][1];
	 switch (*bp)
	 {
	 case 'a': /* Alternate BCD */
	    altbcd = TRUE;
	    break;

	 case 'C': /* cHannels on system */
	    bp++;
	    numchan = atoi (bp);
	    if (numchan < 1 || numchan > MAXCHAN)
	    {
	       fprintf (stderr, "s709: Invalid number of Channels: %s\n", bp);
	       goto usage;
	    }
	    break;

	 case 'c': /* Command file spec */
	    bp++;
	    strcpy (cmdfile,bp);
	    usecmdfile = TRUE;
	    break;

	 case 'm': /* cpu Model */
	    bp++;
	    if (!strcmp (bp, "ctss") || !strcmp (bp, "CTSS"))
	    {
	       cpumode = 7096;
	       strcpy (txtcpumode, "7094-CTSS");
	    }
	    else
	    {
	       cpumode = atoi (bp);
	       if (cpumode != 709 && cpumode != 7090 && cpumode != 7094)
	       {
		  fprintf (stderr, "s709: Invalid CPU model: %s\n", bp);
		  goto usage;
	       }
	       strcpy (txtcpumode, bp);
	    }
	    break;

	 case 'p': /* turn off Panel mode */
	    panelmode = FALSE;
	    break;

	 case 'w': /* set Window length */
	    panelmode = TRUE;
	    bp++;
	    windowlen = atoi (bp);
	    if (windowlen < 25) windowlen = 25;
	    break;

	 case 'h': /* help */
    usage:
	    fprintf (stderr,
	      "usage: s709 [-options] [r=cr] [p=print] [u=punch] [c#=tape]\n");
	    fprintf (stderr,
	      " options:\n");
	    fprintf (stderr,
	      "    -a           - Use alternate BCD encoding\n");
	    fprintf (stderr,
	      "    -CN          - Number of Channels on system, default = 4\n");
	    fprintf (stderr,
	      "    -cCMDFILE    - Command file\n"); 
	    fprintf (stderr,
	      "    -mCPU        - CPU model\n");
	    fprintf (stderr,
	      "            CPU = 709, 7090, 7094 or CTSS, default = 7090\n");
	    fprintf (stderr,
	      "    -p           - Turn off panel mode\n");
	    fprintf (stderr,
	      "    -wNN         - Window size for panel\n");
	    exit (1);

	 default:
	    goto usage;
	 }
      }
      else
      {
	 if (mount (argv[i]) < 0)
	 {
	    fprintf (stderr, "s709: ");
	    for (i = 0; i < ERRVIEWLINENUM; i++)
	    {
	       if (!errview[i][0]) break;
	       fprintf (stderr, "%s", errview[i]);
	    }
	    exit (1);
	 }
      }
   }

   if (numchan > 4)
      outputline = OUTPUTLINE + 7;
   else
      outputline = OUTPUTLINE;

   prtviewlen = windowlen - outputline - 1;
   if (prtviewlen <= 0) prtviewlen = 1;

   /*
   ** If specified, Open command file.
   */

   if (usecmdfile)
   {
      if ((cmdfd = fopen (cmdfile, "r")) == NULL)
      {
         char temp[256];
	 sprintf (temp, "s709: Unable to open command file: %s", cmdfile);
	 perror (temp);
	 exit (1);
      }
   }

   if (panelmode)
   {
      clearscreen ();
      screenposition (TOPLINE,1);
   }
   else
   {
      printf ("IBM %s Simulator %s\n", txtcpumode, VERSION); 
   }

   signal (SIGINT, sigintr);
   inst_count = 0;

#ifdef USE64
   ky = 0;
#else
   kyh = 0;
   kyl = 0;
#endif
   cpuflags |= CPU_AUTO;
   s709 ();
   iofin ();
   if (panelmode)
      screenposition (windowlen, 1);
   return 0;
}
