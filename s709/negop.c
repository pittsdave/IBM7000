/***********************************************************************
*
* negop.c - IBM 7090 emulator negative opcode routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   10/20/03   DGP   Added 7094 instructions.
*   01/28/05   DGP   Revamped channel and tape controls.
*   02/11/06   DGP   Added CTSS cpu mode.
*   12/21/06   DGP   Streamline I/O.
*   05/29/07   DGP   Put CYCLE call back.
*   02/29/08   DGP   Changed to use bit mask flags.
*   03/21/11   DGP   Added IFT, EFT, SRI and SPI insts.
*   03/22/11   DGP   Added CPU_PROTMODE and CPU_RELOMODE support.
*   11/14/11   DGP   Generate trap for illegal instruction in CTSS mode.
*   12/01/11   DGP   Fix Sign conversions (some gcc issue...)
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>

#include "sysdef.h"
#include "regs.h"
#include "s709.h"
#include "arith.h"
#include "io.h"
#include "chan7607.h"
#include "chan7909.h"
#include "negop.h"

extern FILE *trace_fd;
extern int traceinsts;
extern int traceuser;
extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];

/***********************************************************************
* crq - Do the work for the CRQ instruction.
***********************************************************************/

static void
crq ()
{
   uint32 i;
#ifdef USE64
   shcnt = (uint8)((sr & CVRMASK) >> DECRSHIFT);
   sr = iaddr;
   while (shcnt)
   {
      i = (sr + (mq >> 30)) & ADDRMASK;
      ACCESS (i);
      mq = ((mq << 6) & DATAMASK) | (sr >> 30);
      shcnt--;
   }
   tag &= 1;
   if (tag)
      setxr ((int32)(sr & ADDRMASK));
#else
   srl = (srl & 037777700000) | iaddr;
   shcnt = ((srl & CVRMASK) >> DECRSHIFT);
   while (shcnt)
   {
      i = (mqh & SIGN) >> 2 | (mqh & HMSK) << 2 | mql >> 30;
      i = (srl + i) & ADDRMASK;
      ACCESS (i);
      for (i = 0; i < 6; i++)
      {
         mqh = (mqh << 1) & (P|HMSK);
         if (mqh & P)
            mqh = SIGN | (mqh & ~P);
         if (mql & BIT4)
            mqh++;
         mql <<= 1;
      }
      mql |= (srh & SIGN) >> 2 | (srh & HMSK) << 2 | srl >> 30;
      shcnt--;
   }
   tag &= 1;
   if (tag)
      setxr (srl);
#endif
}

/***********************************************************************
* caq - Do the work for the CAQ instruction.
***********************************************************************/

static void
caq ()
{
   uint32 i;
#ifdef USE64

   shcnt = (uint8)((sr & CVRMASK) >> DECRSHIFT);
   sr = iaddr;
   while (shcnt)
   {
      i = (sr + (mq >> 30)) & ADDRMASK;
      ACCESS (i);
      mq = ((mq << 6) & DATAMASK) | (mq >> 30);
      ac = ((ac + sr) & (Q|DATAMASK)) | (ac & ACSIGN);
      shcnt--;
   }
   tag &= 1;
   if (tag)
      setxr ((int32)(sr & ADDRMASK));
#else
   int32 t;

   srl = (srl & 037777700000) | iaddr;
   shcnt = ((srl & CVRMASK) >> DECRSHIFT);
   while (shcnt)
   {
      i = (mqh & SIGN) >> 2 | (mqh & HMSK) << 2 | mql >> 30;
      i = (srl + i) & ADDRMASK;
      ACCESS (i);
      for (i = 0; i < 6; i++)
      {
         t = mqh & SIGN;
         mqh = (mqh << 1) & (P|HMSK);
         if (mqh & P)
            mqh = SIGN | (mqh & ~P);
         if (mql & BIT4)
            mqh++;
         mql <<= 1;
         mql |= t >> 7;
      }
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      adderl = srl + acl;
      adderh = (srh & (P|HMSK)) + (ach & (Q|P|HMSK));
      if (adderl < srl)
         adderh++;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
      acl = adderl;
      ach = (adderh & (Q|P|HMSK)) | (ach & SIGN);
      shcnt--;
   }
   tag &= 1;
   if (tag)
      setxr (srl);
#endif
}

/***********************************************************************
* negop - Process the "negative" Op Codes.
***********************************************************************/

void
negop ()
{
   int16 chan;
   int16 dev;
   uint16 xdata;

   switch (op)
   {

   case 0100000:   /* HTR */
      if (chk_user ()) break;
      cpuflags |= CPU_PROGSTOP;
      cycle_count++;
      ic--;
      if (trap_enb == 0)
	 run = CPU_STOP;
      break;

   case 0100020:   /* TRA */
      if (cpuflags & CPU_TRAPMODE)
      {
         traptrace ();
         ic = 00001;
      }
      else
         ic = y;
      break;

   case 0100021:   /* ESNT */
      if (cpumode < 7094) goto ILLINST;
      break;

   case 0100027:   /* TRCH */
      op += 1;
   case 0100026:   /* TRCF */
   case 0100024:   /* TRCD */
   case 0100022:   /* TRCB */
      if (!chk_user ())
      {
	 chan = (op & 077) - 021;
	 check_cchk (chan);
      }
      break;

   case 0100030:   /* TEFB */
   case 0100031:   /* TEFD */
   case 0100032:   /* TEFF */
   case 0100033:   /* TEFH */
      if (chk_user ()) break;
      chan = ((op & 03) << 1) + 1;
      check_eof (chan);
      break;

   case 0100042:   /* RIA */
#ifdef USE64
      si &= ~ac;
#else
      sih &= ~ach;
      sil &= ~acl;
#endif
      break;

   case 0100046:   /* PIA */
#ifdef USE64
      ac = si;
#else
      ach = sih & (P|HMSK);
      acl = sil;
#endif
      break;

   case 0100051:   /* IIL */
#ifdef USE64
      si ^= (sr & (TAGMASK|ADDRMASK)) << DECRSHIFT;
#else
      sih ^= (srl & 0740000) >> 14;
      sil ^= (srl & 0037777) << DECRSHIFT;
#endif
      break;

   case 0100054:   /* LFT */
#ifdef USE64
      adder = (sr & (TAGMASK|ADDRMASK)) << DECRSHIFT;
      if ((adder & si) == 0)
         ic++;
#else
      adderh = (srl & 0740000) >> 14;
      adderl = (srl & 0037777) << DECRSHIFT;
      if (((adderh & ~sih) == adderh) &&
          ((adderl & ~sil) == adderl))
         ic++;
#endif
      DCYCLE ();
      DCYCLE ();
      break;

   case 0100055:   /* SIL */
#ifdef USE64
      si |= (sr & (TAGMASK|ADDRMASK)) << DECRSHIFT;
#else
      sih |= (srl & 0740000) >> 14;
      sil |= (srl & 0037777) << DECRSHIFT;
#endif
      break;

   case 0100056:   /* LNT */
#ifdef USE64
      adder = (sr & (TAGMASK|ADDRMASK)) << DECRSHIFT;
      if ((adder & si) == adder)
         ic++;
#else
      adderh = (srl & 0740000) >> 14;
      adderl = (srl & 0037777) << DECRSHIFT;
      if ( (adderh & sih) == adderh &&
           (adderl & sil) == adderl )
         ic++;
#endif
      DCYCLE ();
      DCYCLE ();
      break;

   case 0100057:   /* RIL */
#ifdef USE64
      si &= ~((sr & (TAGMASK|ADDRMASK)) << DECRSHIFT);
#else
      sih &= ~((srl & 0740000) >> 14);
      sil &= ~((srl & 0037777) << DECRSHIFT);
#endif
      break;

   case 0100060:   /* TCNA */
   case 0100061:   /* TCNB */
   case 0100062:   /* TCNC */
   case 0100063:   /* TCND */
   case 0100064:   /* TCNE */
   case 0100065:   /* TCNF */
   case 0100066:   /* TCNG */
   case 0100067:   /* TCNH */
      if (chk_user ()) break;
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
      chan = op & 07;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "TCN Channel %c, ic = %05o\n", 'A'+chan, ic-1);
#endif
      if (!channel[chan].csel)
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0100100:   /* TNZ */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();

#ifdef USE64
      if ((ac & (Q|DATAMASK)) != 0 )
#else
      if ((ach & (Q|P|HMSK)) != 0 || acl != 0)
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0100101:   /* CTSS TIB */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
#ifdef DEBUGCTSS
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "TIB: y = %o\n", y);
#endif
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
      bcoreinst = BCORE;
      if (cpuflags & (CPU_PROTMODE | CPU_RELOMODE))
	 cpuflags |= CPU_USERMODE;
      ic = y;
      trap_inhibit = 2;
      break;

   case 0100114:   /* CAQ */
   case 0100115:
   case 0100116:
   case 0100117:
      caq ();
      break;

   case 0100120:   /* TMI */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((ac & ACSIGN) != 0)
#else
      if ((ach & SIGN) != 0)
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0100130:   /* XCL */
#ifdef USE64
      adder = ac & DATAMASK;
      ac = mq;
      mq = adder;
#else
      srh = ((ach << 4) & SIGN) | (ach & HMSK);
      srl = acl;
      ach = ((mqh & SIGN) >> 4) | (mqh & HMSK);
      acl = mql;
      mqh = srh;
      mql = srl;
#endif
      break;

   case 0100140:   /* TNO */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
      if (!(cpuflags & CPU_ACOFLOW))
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      else
	 cpuflags &= ~CPU_ACOFLOW;
      break;

   case 0100154:   /* CRQ */
   case 0100155:
   case 0100156:
   case 0100157:
      crq ();
      break;

   case 0100200:   /* MPR */
      ACCESS (y);
      shcnt = 043;
      mpy ();
      rnd (0);
      break;

   case 0100240:   /* DFDH */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
	 dfdiv ();
	 if ((cpuflags & CPU_DIVCHK) && !chk_user ())
	    run = CPU_STOP;
      }
      break;

   case 0100241:   /* DFDP */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
	 dfdiv ();
      }
      break;

   case 0100260:   /* UFM */
      ACCESS (y);
      fmpy (0);
      break;

   case 0100261:   /* DUFM */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
	 dfmpy (0);
      }
      break;

   case 0100300:   /* UFA */
      ACCESS (y);
      fadd (0);
      break;

   case 0100301:   /* DUFA */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
	 dfadd (0);
      }
      break;

   case 0100302:   /* UFS */
      ACCESS (y);
#ifdef USE64
      sr ^= SIGN;
#else
      srh ^= SIGN;
#endif
      fadd (0);
      break;

   case 0100303:   /* DUFS */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
#ifdef USE64
	 sr ^= SIGN;
#else
	 srh ^= SIGN;
#endif
	 dfadd (0);
      }
      break;

   case 0100304:   /* UAM */
      ACCESS (y);
#ifdef USE64
      sr &= ~SIGN;
#else
      srh &= ~SIGN;
#endif
      fadd (0);
      break;

   case 0100305:   /* DUAM */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
#ifdef USE64
	 sr &= ~SIGN;
#else
	 srh &= ~SIGN;
#endif
	 dfadd (0);
      }
      break;

   case 0100306:   /* USM */
      ACCESS (y);
#ifdef USE64
      sr |= SIGN;
#else
      srh |= SIGN;
#endif
      fadd (0);
      break;

   case 0100307:   /* DUSM */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
#ifdef USE64
	 sr |= SIGN;
#else
	 srh |= SIGN;
#endif
	 dfadd (0);
      }
      break;

   case 0100320:   /* ANA */
      ACCESS (y);
#ifdef USE64
      ac = ac & sr;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      ach = ach & srh;
      acl = acl & srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      CYCLE ();
      break;

   case 0100340:   /* LAS */
      ACCESS (y);
#ifdef USE64
      adder = ac & (Q|DATAMASK);
      if (adder < sr) ic += 2;
      else if (adder == sr) ic++;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      if ( (ach & (Q|P|HMSK)) > (srh & 017) )
         goto skip0;
      if ( (ach & (Q|P|HMSK)) < (srh & 017) )
         goto skip2;
      if ( acl > srl )
         goto skip0;
      if ( acl < srl )
         goto skip2;
      goto skip1;
skip2: ic++;
skip1: ic++;
skip0: if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      break;

   case 0100400:   /* SBM */
      ACCESS (y);
#ifdef USE64
      sr |= SIGN;
#else
      srh |= SIGN;
#endif
      add ();
      CYCLE ();
      break;

   case 0100500:   /* CAL */
      ACCESS (y);
#ifdef USE64
      ac = sr;
#else
      ach = ((srh & SIGN) >> 4) | (srh & HMSK);
      acl = srl;
#endif
      break;

   case 0100501:   /* ORA */
      ACCESS (y);
#ifdef USE64
      ac = ac | sr;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      ach = ach | srh;
      acl = acl | srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      CYCLE ();
      break;

   case 0100520:   /* NZT */
      ACCESS (y);
#ifdef USE64
      if ((sr & MAGMASK) != 0 )
         ic++;
#else
      if ((srh & HMSK) != 0 || srl != 0)
         ic++;
#endif
      break;

   case 0100534:   /* LXD */
      ACCESS (iaddr);
#ifdef USE64
      setxr ((int32)((sr & DECRMASK) >> DECRSHIFT));
#else
      setxr (((srh & 1) << 14) |
             ((srl & DECRMASK) >> DECRSHIFT));
#endif
      break;

   case 0100535:   /* LDC */
      ACCESS (iaddr);
#ifdef USE64
      setxr (0100000 - (int32)((sr & DECRMASK) >> DECRSHIFT));
#else
      setxr (0100000 - (((srh & 1) << 14) |
                        ((srl & DECRMASK) >> DECRSHIFT)));
#endif
      break;

   case 0100540:   /* RCHB/RSCB */
   case 0100541:   /* RCHD/RSCD */
   case 0100542:   /* RCHF/RSCF */
   case 0100543:   /* RCHH/RSCH */
      if (chk_user ()) break;
      chan = ((op & 03) << 1) + 1;
      check_reset (chan);
      CYCLE ();
      break;

   case 0100544:   /* LCHB/STCB */
   case 0100545:   /* LCHD/STCD */
   case 0100546:   /* LCHF/STCF */
   case 0100547:   /* LCHH/STCH */
      if (chk_user ()) break;
      chan = ((op & 03) << 1) + 1;
      check_load (chan);
      CYCLE ();
      break;

   case 0100564:   /* CTSS LPI */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
      ACCESS(y);
#ifdef USE64
      prog_base = sr & 077400;
      prog_limit = ((sr >> DECRSHIFT) & 077400) | 0377;
      cpuflags |= sr & SIGN ? 0 : CPU_PROTMODE;
#else
      prog_base = srl & 077400;
      prog_limit = (((srh & 1) << 14) | ((srl >> DECRSHIFT) & 037400)) | 0377;
      cpuflags |= srh & SIGN ? 0 : CPU_PROTMODE;
#endif
      if (traceinsts && !traceuser)
      {
         fprintf (trace_fd,
	 	  "   protmode = %d, prog_base = %05o, prog_limit = %05o\n",
	 	  cpuflags & CPU_PROTMODE ? 1 : 0, prog_base, prog_limit);
      }
#ifdef DEBUGCTSS
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "LPI: protmode = %d, base = %o, limit = %o\n",
	       cpuflags & CPU_PROTMODE ? 1 : 0, prog_base, prog_limit);
#endif
      trap_inhibit = 2;
      break;

   case 0100600:   /* STQ */
#ifdef USE64
      STORE (y, mq);
#else
      STORE (y, mqh, mql);
#endif
      CYCLE ();
      break;

   case 0100601:   /* CTSS SRI */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
      if (traceinsts && !traceuser)
      {
         fprintf (trace_fd, "   relomode = %d, prog_reloc = %05o\n",
	 	  cpuflags & CPU_RELOMODE ? 1 : 0, prog_reloc);
      }
#ifdef DEBUGCTSS
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "SRI: relomode = %d, prog_reloc = %o\n", 
	       cpuflags & CPU_RELOMODE ? 1 : 0, rog_reloc);
#endif
#ifdef USE64
      sr = prog_reloc | ((cpuflags & CPU_RELOMODE) ? BIT1 : 0);
      STORE (y, sr);
#else
      srh = (cpuflags & CPU_RELOMODE) ? BIT1 : 0;
      srl = prog_reloc;
      STORE (y, srh, srl);
#endif
      break;

   case 0100602:   /* ORS */
      ACCESS (y);
#ifdef USE64
      sr = (ac & (DATAMASK)) | sr;
      STORE (y, sr);
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      srh = (ach & (P|HMSK)) | srh;
      srl = acl | srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
      STORE (y, srh, srl);
#endif
      CYCLE ();
      break;

   case 0100603:   /* DST */
      if (cpumode < 7094) goto ILLINST;
#ifdef USE64
      STORE (y, (ac & MAGMASK) | ((ac & ACSIGN) ? SIGN : 0));
      STORE (y+1, mq);
#else
      STORE (y, ach & (SIGN|HMSK), acl);
      STORE (y+1, mqh, mql);
#endif
      CYCLE ();
      CYCLE ();
      break;

   case 0100604:   /* CTSS SPI */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
      if (traceinsts && !traceuser)
      {
         fprintf (trace_fd,
	 	  "   protmode = %d, prog_base = %05o, prog_limit = %05o\n",
	 	  cpuflags & CPU_PROTMODE ? 1 : 0, prog_base, prog_limit);
      }
#ifdef DEBUGCTSS
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "SPI: protmode = %d, base = %o, limit = %o\n",
	       cpuflags & CPU_PROTMODE ? 1 : 0, prog_base, prog_limit);
#endif
#ifdef USE64
      sr = ((prog_limit & 077400) << DECRSHIFT) | prog_base;
      sr |= ((cpuflags & CPU_PROTMODE) ? BIT2 : 0);
      STORE (y, sr);
#else
      srh = (prog_limit >> 14) & 1;
      srh |= (cpuflags & CPU_PROTMODE) ? BIT2 : 0;
      srl = ((prog_limit & 037400) << DECRSHIFT) | prog_base;
      STORE (y, srh, srl);
#endif
      break;

   case 0100620:   /* SLQ */
      ACCESS (y);
#ifdef USE64
      STORE (y, (sr & (TAGMASK|ADDRMASK)) | (mq & ~(TAGMASK|ADDRMASK)));
#else
      STORE (y, mqh, (srl & (TAGMASK|ADDRMASK)) | (mql & ~(TAGMASK|ADDRMASK)));
#endif
      CYCLE ();
      break;

   case 0100625:   /* STL */
      ACCESS (y);
#ifdef USE64
      STORE (y, (sr & ~ADDRMASK) | ic);
#else
      STORE (y, srh, (srl & ~ADDRMASK) | ic);
#endif
      CYCLE ();
      break;

   case 0100634:   /* SXD */
      ACCESS (iaddr);
      xdata = getxr (TRUE);
#ifdef USE64
      STORE (iaddr, (sr & ~DECRMASK) |
	    (((t_uint64)xdata << DECRSHIFT) & DECRMASK));
#else
      STORE (iaddr, (srh & 0376) | (xdata >> 14), (srl & ~DECRMASK) |
	     (((uint32)xdata << DECRSHIFT) & DECRMASK));
#endif
      CYCLE ();
      break;

   case 0100636:   /* SCD */
      if (cpumode < 7094) goto ILLINST;
      {
	 uint16 x = (0100000 - getxr (TRUE)) & ADDRMASK;
	 if (tag == 0) x = 0;
	 ACCESS (iaddr);
#ifdef USE64
	 STORE (iaddr, (sr & ~DECRMASK) |
	        (((t_uint64)x << DECRSHIFT) & DECRMASK));
#else
	 STORE (iaddr, (srh & 0376) | (x >> 14), (srl & ~DECRMASK) |
	        (((uint32 )x << DECRSHIFT) & DECRMASK));
#endif
      }
      CYCLE ();
      break;

   case 0100640:   /* SCHB */
   case 0100641:   /* SCHD */
   case 0100642:   /* SCHF */
   case 0100643:   /* SCHH */
      if (chk_user ()) break;
      chan = ((op & 03) << 1) + 1;
      CYCLE ();
      store_chan (chan);
      break;

   case 0100644:   /* SCDB */
   case 0100645:   /* SCDD */
   case 0100646:   /* SCDF */
   case 0100647:   /* SCDH */
      chan = ((op & 03) << 1) + 1;
      CYCLE ();
      if (channel[chan].ctype) /* 7909/7750 */
         diag_7909 (chan, y);
      break;

   case 0100734:   /* PDX */
#ifdef USE64
      setxr ((int32)((ac & DECRMASK) >> DECRSHIFT));
#else
      setxr (((ach & 1) << 14) | ((acl & 037777000000) >> DECRSHIFT));
#endif
      break;

   case 0100737:   /* PDC */
#ifdef USE64
      setxr (0100000 - (int32)((ac & DECRMASK) >> DECRSHIFT));
#else
      setxr (0100000 - (((ach & 1) << 14) |
                        ((acl & 037777000000) >> DECRSHIFT)));
#endif
      break;

   case 0100754:   /* PXD */
#ifdef USE64
      ac = 0;
#else
      ach = 0;
      acl = 0;
#endif
      if (tag != 0)
      {
         xdata = getxr (TRUE);
#ifdef USE64
         ac = ((t_uint64)xdata << DECRSHIFT) & DECRMASK;
#else
         ach = (xdata >> 14) & 1;
         acl = ((uint32)xdata << DECRSHIFT) & 037777000000;
#endif
      }
      break;

   case 0100756:   /* PCD */
      if (cpumode < 7094) goto ILLINST;
      if (tag == 0)
      {
#ifdef USE64
         ac = 0;
#else
         ach = 0;
         acl = 0;
#endif
      }
      else
      {
         uint16 x = (0100000 - getxr (TRUE)) & ADDRMASK;
#ifdef USE64
         ac = ((t_uint64)x << DECRSHIFT) & DECRMASK;
#else
         ach = x >> 14;
         acl = (x << DECRSHIFT) & 037777000000;
#endif
      }
      break;

   case 0100761:   /* CTSS mode instructions */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
      switch (y)
      {
         case 00041:   /* CTSS SEA */
#ifdef DEBUGCTSS
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr, "SEA: \n");
#endif
	    bcoredata = 0;
	    break;
	 case 00042:   /* CTSS SEB */
#ifdef DEBUGCTSS
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr, "SEB: \n");
#endif
	    bcoredata = BCORE;
	    break;
	 case 00043:   /* CTSS IFT */
#ifdef DEBUGCTSS
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr, "IFT: \n");
#endif
	    if (!bcoreinst)
	       ic++;
	    break;
	 case 00044:   /* CTSS EFT */
#ifdef DEBUGCTSS
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr, "EFT: \n");
#endif
	    if (!bcoredata)
	       ic++;
	    break;
	 default: 
	    goto ILLINST;
      }
      trap_inhibit = 2;
      break;

   case 0100763:   /* LGL */
      for (y &= 0377; y; y--)
      {
#ifdef USE64
         ac = (ac & ACSIGN) | ((ac << 1) & (Q|DATAMASK)) | ((mq >> 35) & 1);
	 mq = (mq << 1) & DATAMASK;
         if (ac & P)
	    cpuflags |= CPU_ACOFLOW;
#else
         ach = (ach & SIGN) | ((ach << 1) & (Q|P|HMSK));
         if (ach & P)
	    cpuflags |= CPU_ACOFLOW;
         if (acl & BIT4)
            ach++;
         acl <<= 1;
         if (mqh & SIGN)
            acl++;
         mqh = mqh << 1;
         if (mqh & P)
            mqh = SIGN | (mqh & ~P);
         if (mql & BIT4)
            mqh++;
         mql <<= 1;
#endif
      }
      break;

   case 0100764:   /* BSF */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	    "BSF Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	       'A'+chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
#endif
      dev = check_chan (chan, -1, FALSE, BSF_SEL);
      CYCLE ();
      break;

   case 0100765:   /* LGR */
      shcnt = y & 0377;
#ifdef USE64
      if (shcnt)
      {
	 adder = ac & (Q|DATAMASK);
	 ac = ac & ACSIGN;
         if (shcnt < 36)
	 {
	    mq = ((mq >> shcnt) | (adder << (36 - shcnt))) & DATAMASK;
	    ac |= (adder >> shcnt);
	 }
         else if (shcnt == 36)
	 {
	    mq = adder & DATAMASK;
	    ac |= adder >> 36;
	 }
	 else if (shcnt < 73)
	    mq = (adder >> (shcnt - 36)) & DATAMASK;
	 else
	    mq = 0;
      }
#else
      for (; shcnt; shcnt--)
      {
         mql >>= 1;
         if (mqh & 1)
            mql |= BIT4;
         if (mqh & SIGN)
	    mqh = P | (mqh & ~SIGN);
         mqh >>= 1;
         if (acl & 1)
            mqh |= SIGN;
         acl >>= 1;
         if (ach & 1)
            acl |= BIT4;
         ach = (ach & SIGN) | ((ach >> 1) & (P|HMSK));
      }
#endif
      break;

   case 0100772:   /* RUN */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	    "RUN Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	       'A'+chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
#endif
      dev = check_chan (chan, -1, FALSE, RUN_SEL);
      CYCLE ();
      break;

   case 0100773:   /* RQL */
#ifdef USE64
      shcnt = (y & 0377) % 36;
      if (shcnt)
         mq = ((mq << shcnt) | (mq >> (36 - shcnt))) & DATAMASK;
#else
      for (y &= 0377; y; y--)
      {
         adderh = mqh;
         mqh <<= 1;
         if (mqh & P)
            mqh = SIGN | (mqh & ~P);
         if (mql & BIT4)
            mqh++;
         mql <<= 1;
         if (adderh & SIGN)
            mql++;
      }
#endif
      break;

   case 0100774:   /* AXC */
#ifdef USE64
      setxr (0100000 - (int32)(sr & ADDRMASK));
#else
      setxr (0100000 - (srl & ADDRMASK));
#endif
      break;

   default:
ILLINST:
      cpuflags |= CPU_MACHCHK;
      if (!chk_user())
      {
	 sprintf (errview[0], "Illegal instruction: op = %o, ic = %o\n",
		  op, ic);
	 run = CPU_STOP;
      }
      break;
   }
}
