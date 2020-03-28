/***********************************************************************
*
* posop.c - IBM 7090 emulator positive opcode routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   10/20/03   DGP   Added 7094 instructions.
*   01/28/05   DGP   Revamped channel and tape controls.
*   05/17/05   DGP   Corrected ENB mask.
*   02/11/06   DGP   Added CTSS cpu mode.
*   12/21/06   DGP   Streamline I/O.
*   05/29/07   DGP   Put CYCLE call back.
*   02/19/08   DGP   Put variable initalization for RDS/WRS before check_chan.
*   02/29/08   DGP   Changed to use bit mask flags.
*   03/22/11   DGP   Added CPU_PROTMODE and CPU_RELOMODE support.
*   11/14/11   DGP   Generate trap for illegal instruction in CTSS mode.
*   12/01/11   DGP   Fix Sign conversions (some gcc issue...)
*   12/14/11   DGP   Set run to CPU_STOP in HTR and HPR.
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>

#include "sysdef.h"
#include "regs.h"
#include "s709.h"
#include "arith.h"
#include "chan7607.h"
#include "chan7909.h"
#include "io.h"
#include "posop.h"

extern FILE *trace_fd;
extern int traceinsts;
extern int traceuser;
extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];

/***********************************************************************
* cvr - Do the work for the CVR instruction.
***********************************************************************/

static void
cvr ()
{
   uint32 i;

#ifdef USE64
   shcnt = (uint8)((sr & CVRMASK) >> DECRSHIFT);
   sr = iaddr;
   while (shcnt)
   {
      i = (sr + (ac & 077)) & ADDRMASK;
      ACCESS (i);
      ac = (ac & ACSIGN) | ((ac >> 6) & 0007777777777) | (sr & CHARHMSK);
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
      i = (srl + (acl & 077)) & ADDRMASK;
      ACCESS (i);
      for (i = 0; i < 6; i++)
      {
         acl >>= 1;
         if (ach & 1)
            acl |= BIT4;
         ach = (ach & SIGN) | ((ach >> 1) & (P|HMSK));
      }
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      ach |= srh;
      acl |= srl & 030000000000;
      shcnt--;
   }
   tag &= 1;
   if (tag)
      setxr (srl);
#endif
}

/***********************************************************************
* posop - Process the "positive" Op Codes.
***********************************************************************/

void
posop ()
{
   register int chan;
   register int dev;

   switch (op)
   {

   case 0000000:   /* HTR */
      if (chk_user ()) break;
      cpuflags |= CPU_PROGSTOP;
      cycle_count++;
      ic--;
      if (trap_enb == 0)
	 run = CPU_STOP;
      break;

   case 0000020:   /* TRA */
      if (cpuflags & CPU_TRAPMODE)
      {
         traptrace ();
         ic = 00001;
      }
      else
         ic = y;
      break;

   case 0000021:   /* TTR */
#if defined(DEBUGIO) || defined(DEBUGTRAP)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "TTR ic = %05o\n", y);
#endif
      ic = y;
      break;

   case 0000027:   /* TRCG */
      op += 1;
   case 0000026:   /* TRCE */
   case 0000024:   /* TRCC */
   case 0000022:   /* TRCA */
      if (!chk_user ())
      {
         chan = (op & 077) - 022;
	 check_cchk (chan);
      }
      break;

   case 0000030:   /* TEFA */
   case 0000031:   /* TEFC */
   case 0000032:   /* TEFE */
   case 0000033:   /* TEFG */
      if (chk_user ()) break;
      chan = (op & 03) << 1;
      check_eof (chan);
      break;

   case 0000040:   /* TLQ */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((ac & ACSIGN) == 0)
      {
         if ((mq & SIGN) == 0)
	 {
            if ((ac & (Q|DATAMASK)) > (mq & MAGMASK))
               goto tlqtr;
            if ((ac & (Q|DATAMASK)) < (mq & MAGMASK))
               break;
            break;
         }
	 else
            goto tlqtr;
      }
      else
      {
         if ((mq & SIGN) == 0)
            break;
	 else
	 {
            if ((ac & (Q|DATAMASK)) > (mq & MAGMASK))
               break;
            if ((ac & (Q|DATAMASK)) < (mq & MAGMASK))
               goto tlqtr;
            break;
         }
      }
#else
      if ((ach & SIGN) == 0)
      {
         if ((mqh & SIGN) == 0)
	 {
            if ( (ach & (Q|P|HMSK)) > (mqh & HMSK) )
               goto tlqtr;
            if ( (ach & (Q|P|HMSK)) < (mqh & HMSK) )
               break;
            if ( acl > mql )
               goto tlqtr;
            break;
         }
	 else
            goto tlqtr;
      }
      else
      {
         if ((mqh & SIGN) == 0)
            break;
	 else
	 {
            if ( (ach & (Q|P|HMSK)) > (mqh & HMSK) )
               break;
            if ( (ach & (Q|P|HMSK)) < (mqh & HMSK) )
               goto tlqtr;
            if ( acl > mql )
               break;
            if ( acl < mql )
               goto tlqtr;
            break;
         }
      }
#endif

tlqtr:
      if (cpuflags & CPU_TRAPMODE)
         ic = 00001;
      else
         ic = y;
      break;

   case 0000041:   /* IIA */
#ifdef USE64
      si ^= ac & DATAMASK;
#else
      sih ^= ach & (P|HMSK);
      sil ^= acl;
#endif
      break;

   case 0000042:   /* TIO */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((ac & si) == (ac & DATAMASK))
#else
      if ( ((ach & (P|HMSK)) & sih) == (ach & (P|HMSK)) && (acl & sil) == acl )
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      CYCLE ();
      break;

   case 0000043:   /* OAI */
#ifdef USE64
      si |= ac & DATAMASK;
#else
      sih |= ach & (P|HMSK);
      sil |= acl;
#endif
      break;

   case 0000044:   /* PAI */
#ifdef USE64
      si = ac & DATAMASK;
#else
      sih = ach & (P|HMSK);
      sil = acl;
#endif
      break;

   case 0000046:   /* TIF */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((ac & si) == 0)
#else
      if ( ((ach & (P|HMSK)) & ~sih) == (ach & (P|HMSK)) &&
           (acl & ~sil) == acl )
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      CYCLE ();
      break;

   case 0000051:   /* IIR */
#ifdef USE64
      si ^= sr & (TAGMASK|ADDRMASK);
#else
      sil ^= srl & (TAGMASK|ADDRMASK);
#endif
      break;

   case 0000054:   /* RFT */
#ifdef USE64
      if (((sr & (TAGMASK|ADDRMASK)) & si) == 0)
#else
      if (((srl & (TAGMASK|ADDRMASK)) & ~sil) == (srl & (TAGMASK|ADDRMASK)))
#endif
         ic++;
      DCYCLE ();
      DCYCLE ();
      break;

   case 0000055:   /* SIR */
#ifdef USE64
      si |= sr & (TAGMASK|ADDRMASK);
#else
      sil |= srl & (TAGMASK|ADDRMASK);
#endif
      break;

   case 0000056:   /* RNT */
#ifdef USE64
      if (((sr & (TAGMASK|ADDRMASK)) & si) == (sr & (TAGMASK|ADDRMASK)))
#else
      if (((srl & (TAGMASK|ADDRMASK)) & sil) == (srl & (TAGMASK|ADDRMASK)))
#endif
         ic++;
      DCYCLE ();
      DCYCLE ();
      break;

   case 0000057:   /* RIR */
#ifdef USE64
      si &= ~(sr & (TAGMASK|ADDRMASK));
#else
      sil &= ~(srl & (TAGMASK|ADDRMASK));
#endif
      break;

   case 0000060:   /* TCOA */
   case 0000061:   /* TCOB */
   case 0000062:   /* TCOC */
   case 0000063:   /* TCOD */
   case 0000064:   /* TCOE */
   case 0000065:   /* TCOF */
   case 0000066:   /* TCOG */
   case 0000067:   /* TCOH */
      if (chk_user ()) break;
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
      chan = op & 07;
#if defined(DEBUGIO1) || defined(DEBUG7607)
      if (chan == 1)
      {
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "TCO Channel %c, ic = %05o, csel = %d, cact = %d\n",
	    'A'+chan, ic-1, channel[chan].csel, channel[chan].cact);
      }
#endif
      if (channel[chan].csel)
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0000074:   /* TSX */
      if (cpuflags & CPU_PROGSTOP)
         setxr (-(ic));
      else
         setxr (-(ic-1));
      if (cpuflags & CPU_TRAPMODE)
      {
         traptrace ();
         ic = 00001;
      }
      else
         ic = iaddr;
      break;

   case 0000100:   /* TZE */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((ac & (Q|DATAMASK)) == 0)
#else
      if ((ach & (Q|P|HMSK)) == 0 && acl == 0)
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0000101:   /* CTSS TIA */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
#ifdef DEBUGCTSS
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "TIA: y = %o\n", y);
#endif
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
      bcoreinst = 0;
      ic = y;
      trap_inhibit = 2;
      break;

   case 0000114:   /* CVR */
   case 0000115:
   case 0000116:
   case 0000117:
      cvr ();
      break;

   case 0000120:   /* TPL */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((ac & ACSIGN) == 0)
#else
      if ((ach & SIGN) == 0)
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0000131:   /* XCA */
#ifdef USE64
      adder = ac;
      ac = (mq & MAGMASK) | ((mq & SIGN) ? ACSIGN : 0);
      mq = (adder & MAGMASK) | ((adder & ACSIGN) ? SIGN : 0);
#else
      srh = ach & (SIGN|HMSK);
      srl = acl;
      ach = mqh;
      acl = mql;
      mqh = srh;
      mql = srl;
#endif
      break;

   case 0000140:   /* TOV */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
      if (cpuflags & CPU_ACOFLOW)
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
	 cpuflags &= ~CPU_ACOFLOW;
      }
      break;

   case 0000161:   /* TQO */
      if (!(cpuflags & CPU_FPTRAP))
      {
	 if (cpuflags & CPU_TRAPMODE)
	    traptrace ();
	 if (cpuflags & CPU_MQOFLOW)
	 {
	    if (cpuflags & CPU_TRAPMODE)
	       ic = 00001;
	    else
	       ic = y;
	    cpuflags &= ~CPU_MQOFLOW;
	 }
      }
      break;

   case 0000162:   /* TQP */
      if (cpuflags & CPU_TRAPMODE)
         traptrace ();
#ifdef USE64
      if ((mq & SIGN) == 0)
#else
      if ((mqh & SIGN) == 0)
#endif
      {
	 if (cpuflags & CPU_TRAPMODE)
            ic = 00001;
         else
            ic = y;
      }
      break;

   case 0000200:   /* MPY */
      ACCESS (y);
      shcnt = 043;
      mpy ();
      break;

   case 0000204:   /* VLM */
   case 0000205:   /* VLM (for 9M51B) */
#ifdef USE64
      shcnt =  ((sr & VOPMASK) >> DECRSHIFT);
#else
      shcnt = ((srl & VOPMASK) >> DECRSHIFT);
#endif
      ACCESS (y);
      if (shcnt)
         mpy ();
      break;

   case 0000220:   /* DVH */
      ACCESS (y);
      shcnt = 043;
      intdiv ();
      if ((cpuflags & CPU_DIVCHK) && !chk_user ())
         run = CPU_STOP;
      break;

   case 0000221:   /* DVP */
      ACCESS (y);
      shcnt = 043;
      intdiv ();
      break;

   case 0000224:   /* VDH */
#ifdef USE64
      shcnt =  ((sr & VOPMASK) >> DECRSHIFT);
#else
      shcnt = ((srl & VOPMASK) >> DECRSHIFT);
#endif
      ACCESS (y);
      if (shcnt)
      {
         intdiv ();
	 if ((cpuflags & CPU_DIVCHK) && !chk_user ())
            run = CPU_STOP;
      }
      break;

   case 0000225:   /* VDP */
   case 0000227:   /* VDP */
#ifdef USE64
      shcnt =  ((sr & VOPMASK) >> DECRSHIFT);
#else
      shcnt = ((srl & VOPMASK) >> DECRSHIFT);
#endif
      ACCESS (y);
      if (shcnt)
         intdiv ();
      break;

   case 0000240:   /* FDH */
      ACCESS (y);
      fdiv ();
      if ((cpuflags & CPU_DIVCHK) && !chk_user ())
         run = CPU_STOP;
      break;

   case 0000241:   /* FDP */
      ACCESS (y);
      fdiv ();
      break;

   case 0000260:   /* FMP */
      ACCESS (y);
      fmpy (1);
      break;

   case 0000261:   /* DFMP */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
	 dfmpy (1);
      }
      break;

   case 0000300:   /* FAD */
      ACCESS (y);
      fadd (1);
      break;

   case 0000301:   /* DFAD */
      if (cpumode < 7094) goto ILLINST;
      if (y & 1 && (cpuflags & CPU_FPTRAP))
         spill = SPILL_ODD;
      else
      {
	 ACCESS (y);
	 ACCESS2 (y|1);
	 dfadd (1);
      }
      break;

   case 0000302:   /* FSB */
      ACCESS (y);
#ifdef USE64
      sr ^= SIGN;
#else
      srh ^= SIGN;
#endif
      fadd (1);
      break;

   case 0000303:   /* DFSB */
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
	 dfadd (1);
      }
      break;

   case 0000304:   /* FAM */
      ACCESS (y);
#ifdef USE64
      sr &= ~SIGN;
#else
      srh &= ~SIGN;
#endif
      fadd (1);
      break;

   case 0000305:   /* DFAM */
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
	 dfadd (1);
      }
      break;

   case 0000306:   /* FSM */
      ACCESS (y);
#ifdef USE64
      sr |= SIGN;
#else
      srh |= SIGN;
#endif
      fadd (1);
      break;

   case 0000307:   /* DFSM */
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
	 dfadd (1);
      }
      break;

   case 0000320:   /* ANS */
      ACCESS (y);
#ifdef USE64
      sr = ac & sr;
      STORE (y, sr);
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      srh = ach & srh;
      srl = acl & srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
      STORE (y, srh, srl);
#endif
      CYCLE ();
      break;

   case 0000322:   /* ERA */
      ACCESS (y);
      CYCLE ();
#ifdef USE64
      ac = (ac ^ sr) & DATAMASK;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      ach = (ach ^ srh) & (P|HMSK);
      acl = acl ^ srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      DCYCLE ();
      break;

   case 0000340:   /* CAS */
      ACCESS (y);
#ifdef USE64
      if ((ac & ACSIGN) == 0)
      {
         if ((sr & SIGN) == 0)
	 {
            if ((ac & (Q|DATAMASK)) > (sr & MAGMASK))
               goto skip0;
            if ((ac & (Q|DATAMASK)) < (sr & MAGMASK))
               goto skip2;
            goto skip1;
         }
	 else
            goto skip0;
      }
      else
      {
         if ((sr & SIGN) == 0)
            goto skip2;
	 else
	 {
            if ((ac & (Q|DATAMASK)) > (sr & MAGMASK))
               goto skip2;
            if ((ac & (Q|DATAMASK)) < (sr & MAGMASK))
               goto skip0;
            goto skip1;
         }
      }
#else
      if ((ach & SIGN) == 0)
      {
         if ((srh & SIGN) == 0)
	 {
            if ( (ach & (Q|P|HMSK)) > (srh & HMSK) )
               goto skip0;
            if ( (ach & (Q|P|HMSK)) < (srh & HMSK) )
               goto skip2;
            if ( acl > srl )
               goto skip0;
            if ( acl < srl )
               goto skip2;
            goto skip1;
         }
	 else
            goto skip0;
      }
      else
      {
         if ((srh & SIGN) == 0)
            goto skip2;
	 else
	 {
            if ( (ach & (Q|P|HMSK)) > (srh & HMSK) )
               goto skip2;
            if ( (ach & (Q|P|HMSK)) < (srh & HMSK) )
               goto skip0;
            if ( acl > srl )
               goto skip2;
            if ( acl < srl )
               goto skip0;
            goto skip1;
         }
      }
#endif
skip2:ic++;
skip1:ic++;
skip0:break;

   case 0000361:   /* ACL */
      ACCESS (y);
#ifdef USE64
      adder = (sr + ac) & DATAMASK;
      if (adder < sr) adder = (adder + 1) & DATAMASK;
      ac = (ac & (ACSIGN|Q)) | adder;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      adderh = (srh & (P|HMSK)) + (ach & (P|HMSK));
      /*adderh = srh + (ach & (P|HMSK));*/
      adderl = srl + acl;
      if (adderl < srl)
         adderh++;
      if (adderh & Q)
      {
         adderl++;
         if (adderl == 0)
            adderh++;
      }
      if (srh & P)
	 srh = SIGN | (srh & ~P);
      ach = (adderh & (P|HMSK)) | (ach & (SIGN|Q));
      acl = adderl;
#endif
      CYCLE ();
      break;

   case 0000400:   /* ADD */
      ACCESS (y);
      add ();
      CYCLE ();
      break;

   case 0000401:   /* ADM */
      ACCESS (y);
#ifdef USE64
      sr &= ~SIGN;
#else
      srh &= ~SIGN;
#endif
      add ();
      CYCLE ();
      break;

   case 0000402:   /* SUB */
      ACCESS (y);
#ifdef USE64
      sr ^= SIGN;
#else
      srh ^= SIGN;
#endif
      add ();
      CYCLE ();
      break;

   case 0000420:   /* HPR */
      if (chk_user ()) break;
      cpuflags |= CPU_PROGSTOP;
      cycle_count++;
      iaddr = ic;
      ic--;
      run = CPU_STOP;
      break;

   case 0000440:   /* IIS */
      ACCESS (y);
#ifdef USE64
      si ^= sr;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      sih ^= srh;
      sil ^= srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      break;

   case 0000441:   /* LDI */
      ACCESS (y);
#ifdef USE64
      si = sr;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      sih = srh;
      sil = srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      break;

   case 0000442:   /* OSI */
      ACCESS (y);
#ifdef USE64
      si |= sr;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      sih |= srh;
      sil |= srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      break;

   case 0000443:   /* DLD */
      if (cpumode < 7094) goto ILLINST;
#ifdef USE64
      ACCESS (y);
      ac = (sr & MAGMASK) | ((sr & SIGN) ? ACSIGN : 0);
      ACCESS (y|1);
      mq = sr;
#else
      ACCESS (y);
      ach = srh;
      acl = srl;
      ACCESS (y|1);
      mqh = srh;
      mql = srl;
#endif
      if (y & 1) spill = SPILL_ODD;
      CYCLE ();
      CYCLE ();
      break;

   case 0000444:   /* OFT */
      ACCESS (y);
#ifdef USE64
      if ((sr & si) == 0)
         ic++;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      if ( (srh & ~sih) == srh &&
           (srl & ~sil) == srl )
         ic++;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      CYCLE ();
      DCYCLE ();
      break;

   case 0000445:   /* RIS */
      ACCESS (y);
#ifdef USE64
      si &= ~sr;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      sih &= ~srh;
      sil &= ~srl;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      break;

   case 0000446:   /* ONT */
      ACCESS (y);
#ifdef USE64
      if ((sr & si) == sr)
         ic++;
#else
      if (srh & SIGN)
	 srh = P | (srh & ~SIGN);
      if ( (srh & sih) == srh  &&
           (srl & sil) == srl )
         ic++;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
#endif
      CYCLE ();
      DCYCLE ();
      break;

   case 0000500:   /* CLA */
      ACCESS (y);
#ifdef USE64
      ac = (sr & MAGMASK) | ((sr & SIGN) ? ACSIGN : 0);
#else
      ach = srh;
      acl = srl;
#endif
      break;

   case 0000502:   /* CLS */
      ACCESS (y);
#ifdef USE64
      sr = (sr & MAGMASK) | ((sr & SIGN) ? 0 : ACSIGN);
      ac = sr;
#else
      srh ^= SIGN;
      ach = srh;
      acl = srl;
#endif
      break;

   case 0000520:   /* ZET */
      ACCESS (y);
#ifdef USE64
      if ((sr & MAGMASK) == 0)
#else
      if ((srh & HMSK) == 0 && srl == 0)
#endif
         ic++;
      break;

   case 0000534:   /* LXA */
      ACCESS (iaddr);
#ifdef USE64
      setxr ((int32)(sr & ADDRMASK));
#else
      setxr (srl);
#endif
      break;

   case 0000535:   /* LAC */
      ACCESS (iaddr);
#ifdef USE64
      setxr ((int32)(0100000 - (sr & ADDRMASK)));
#else
      setxr (0100000 - (srl & ADDRMASK));
#endif
      break;

   case 0000540:   /* RCHA/RSCA */
   case 0000541:   /* RCHC/RSCC */
   case 0000542:   /* RCHE/RSCE */
   case 0000543:   /* RCHG/RSCG */
      if (chk_user ()) break;
      chan = (op & 03) << 1;
      CYCLE ();
      check_reset (chan);
      break;

   case 0000544:   /* LCHA/STCA */
   case 0000545:   /* LCHC/STCC */
   case 0000546:   /* LCHE/STCE */
   case 0000547:   /* LCHG/STCG */
      if (chk_user ()) break;
      chan = (op & 03) << 1;
      CYCLE ();
      check_load (chan);
      break;

   case 0000560:   /* LDQ */
      ACCESS (y);
#ifdef USE64
      mq = sr;
#else
      mqh = srh;
      mql = srl;
#endif
      break;

   case 0000562:   /* CTSS LRI */
      if (cpumode < 7096) goto ILLINST;
      if (chk_user ()) break;
      ACCESS(y);
#ifdef USE64
      prog_reloc = sr & 077400;
      cpuflags |= sr & SIGN ? 0 : CPU_RELOMODE;
#else
      prog_reloc = srl & 077400;
      cpuflags |= srh & SIGN ? 0 : CPU_RELOMODE;
#endif
      if (traceinsts && !traceuser)
      {
         fprintf (trace_fd, "   relomode = %d, prog_reloc = %05o\n",
		  cpuflags & CPU_RELOMODE ? 1 : 0, prog_reloc);
      }
#ifdef DEBUGCTSS
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "LRI: relomode = %d, prog_reloc = %o\n",
	       cpuflags & CPU_RELOMODE ? 1 : 0, prog_reloc);
#endif
      trap_inhibit = 2;
      break;

   case 0000564:   /* ENB */
      if (chk_user ()) break;
      ACCESS (y);
#ifdef USE64
      trap_enb = sr & 00377000377;
      if (cpumode == 7096)
	 trap_enb |= sr & 00000770000;
#else
      trap_enb = srl & 00377000377;
      if (cpumode == 7096)
	 trap_enb |= srl & 00000770000;
#endif
      if (trap_enb)
	 cpuflags |= CPU_CHTRAP;
      else
	 cpuflags &= ~CPU_CHTRAP;
#if defined(DEBUGIO) || defined(DEBUGTRAP) || defined(DEBUG76071)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "ENB ic = %05o, trap_enb = %09o\n",
	       ic-1, trap_enb);
#endif
      trap_inhibit = 2;
      break;

   case 0000600:   /* STZ */
#ifdef USE64
      STORE (y, 0);
#else
      STORE (y, 0, 0);
#endif
      CYCLE ();
      break;

   case 0000601:   /* STO */
#ifdef USE64
      STORE (y, (ac & MAGMASK) | ((ac & ACSIGN) ? SIGN : 0));
#else
      STORE (y, ach & (SIGN|HMSK), acl);
#endif
      CYCLE ();
      break;

   case 0000602:   /* SLW */
#ifdef USE64
      STORE (y, ac & DATAMASK);
#else
      STORE (y, ((ach << 4) & SIGN) | (ach & HMSK), acl);
#endif
      CYCLE ();
      break;

   case 0000604:   /* STI */
#ifdef USE64
      STORE (y, si);
#else
      srh = sih;
      srl = sil;
      if (srh & P)
	 srh = SIGN | (srh & ~P);
      STORE (y, srh, srl);
#endif
      break;

   case 0000621:   /* STA */
      ACCESS (y);
#ifdef USE64
      STORE (y, (sr & ~ADDRMASK) | (ac & ADDRMASK));
#else
      STORE (y, srh, (srl & ~ADDRMASK) | (acl & ADDRMASK));
#endif
      CYCLE ();
      break;

   case 0000622:   /* STD */
      ACCESS (y);
#ifdef USE64
      STORE (y, (sr & ~DECRMASK) | (ac & DECRMASK));
#else
      srh = (srh & 0376) | (ach & 001);
      srl = (srl & ~DECRMASK) | (acl & DECRMASK);
      STORE (y, srh, srl);
#endif
      CYCLE ();
      break;

   case 0000625:   /* STT */
      ACCESS (y);
#ifdef USE64
      STORE (y, (sr & ~TAGMASK) | (ac & TAGMASK));
#else
      STORE (y, srh, (srl & ~TAGMASK) | (acl & TAGMASK));
#endif
      CYCLE ();
      break;

   case 0000630:   /* STP */
      ACCESS (y);
#ifdef USE64
      STORE (y, (sr & ~PREMASK) | (ac & PREMASK));
#else
      srh = (srh & 001) | ((ach << 4) & SIGN) | (ach & 006);
      STORE (y, srh, srl);
#endif
      CYCLE ();
      break;

   case 0000634:   /* SXA */
      ACCESS (iaddr);
#ifdef USE64
      STORE (iaddr, (sr & ~ADDRMASK) | getxr (TRUE));
#else
      STORE (iaddr, srh, (srl & ~ADDRMASK) | getxr (TRUE));
#endif
      CYCLE ();
      break;

   case 0000636:   /* SCA */
      if (cpumode < 7094) goto ILLINST;
      {
	 uint16 x = (0100000 - getxr (TRUE)) & ADDRMASK;
	 if (tag == 0) x = 0;
	 ACCESS (iaddr);
#ifdef USE64
	 STORE (iaddr, (sr & ~ADDRMASK) | x);
#else
	 STORE (iaddr, srh, (srl & ~ADDRMASK) | x);
#endif
      }
      break;

   case 0000640:   /* SCHA */
   case 0000641:   /* SCHC */
   case 0000642:   /* SCHE */
   case 0000643:   /* SCHG */
      if (chk_user ()) break;
      chan = (op & 03) << 1;
      CYCLE ();
      store_chan (chan);
      break;

   case 0000644:   /* SCDA */
   case 0000645:   /* SCDC */
   case 0000646:   /* SCDE */
   case 0000647:   /* SCDG */
      chan = (op & 03) << 1;
      CYCLE ();
      if (channel[chan].ctype) /* 7909/7750 */
         diag_7909 (chan, y);
      break;

   case 0000734:   /* PAX */
#ifdef USE64
      setxr ((int32)(ac & ADDRMASK));
#else
      setxr (acl);
#endif
      break;

   case 0000737:   /* PAC */
#ifdef USE64
      setxr (0100000 - (int32)(ac & ADDRMASK));
#else
      setxr (0100000 - (acl & ADDRMASK));
#endif
      break;

   case 0000754:   /* PXA */
#ifdef USE64
      ac = getxr (TRUE);
#else
      ach = 0;
      acl = getxr (TRUE);
#endif
      break;

   case 0000756:   /* PCA */
      if (cpumode < 7094) goto ILLINST;
#ifdef USE64
      if (tag != 0)
         ac = (0100000 - getxr (TRUE)) & ADDRMASK;
      else
         ac = 0;
#else
      ach = 0;
      if (tag != 0)
         acl = (0100000 - getxr (TRUE)) & ADDRMASK;
      else
         acl = 0;
#endif
      break;

   case 0000761:   /* NOP */
      cycle_count++;
      break;

   case 0000762:   /* RDS */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	 "RDS Channel %c, ic = %05o, iaddr = %05o, trap_enb = %o, eof = %s",
	       'A'+chan, ic-1, iaddr & ADDRMASK, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, ", flags = %08o\n", channel[chan].cflags);
#endif
      channel[chan].cflags &= ~(CHAN_TRAPPEND | CHAN_PRINTCLK);
      dev = check_chan (chan, 1, TRUE, READ_SEL);
      CYCLE ();
      break;

   case 0000763:   /* LLS */
#ifdef USE64
      ac &= Q|DATAMASK;
      for (y &= 0377; y; y--)
      {
	 ac = ((ac << 1) & (Q|DATAMASK)) | ((mq >> 34) & 1);
         mq = (mq & SIGN) | ((mq << 1) & MAGMASK);
         if (ac & P)
	    cpuflags |= CPU_ACOFLOW;
      }
      ac |= (mq & SIGN) ? ACSIGN : 0;
#else
      for (y &= 0377; y; y--)
      {
         ach = (ach & SIGN) | ((ach << 1) & (Q|P|HMSK));
         if (acl & BIT4)
            ach++;
         acl <<= 1;
         if (mqh & BIT1)
            acl++;
         mqh = (mqh & SIGN) | ((mqh << 1) & HMSK);
         if (mql & BIT4)
            mqh++;
         mql <<= 1;
         if (ach & P)
	    cpuflags |= CPU_ACOFLOW;
      }
      ach = (mqh & SIGN) | (ach & (Q|P|HMSK));
#endif
      break;

   case 0000764:   /* BSR */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "BSR Channel %c, ic = %05o, trap_enb = %o, eof = %s",
	       'A'+chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, ", flags = %08o\n", channel[chan].cflags);
#endif
      dev = check_chan (chan, -1, FALSE, BSR_SEL);
      CYCLE ();
      break;

   case 0000765:   /* LRS */
#ifdef USE64
      mq &= MAGMASK;
      shcnt = y & 0377;
      if (shcnt)
      {
	 adder = ac & (Q|DATAMASK);
	 ac &= ACSIGN;
	 if (shcnt < 35)
	 {
	    mq = ((mq >> shcnt) | (adder << (35 - shcnt))) & MAGMASK;
	    ac |= adder >> shcnt;
	 }
	 else if (shcnt < 37)
	 {
	    mq = (adder >> (shcnt - 35)) & MAGMASK;
	    ac |= adder >> shcnt;
	 }
	 else if (shcnt < 72)
	    mq = (adder >> (shcnt - 35)) & MAGMASK;
	 else
	    mq = 0;
      }
      mq |= (ac & ACSIGN) ? SIGN : 0;
#else
      for (y &= 0377; y; y--)
      {
         mql >>= 1;
         if (mqh & 1)
            mql |= BIT4;
         mqh = (mqh & SIGN) | ((mqh >> 1) & HMSK);
         if (acl & 1)
            mqh |= BIT1;
         acl >>= 1;
         if (ach & 1)
            acl |= BIT4;
         ach = (ach & SIGN) | ((ach >> 1) & (Q|P|HMSK));
      }
      mqh = (ach & SIGN) | (mqh & HMSK);
#endif
      break;

   case 0000766:   /* WRS */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	 "WRS Channel %c, ic = %05o, iaddr = %05o, trap_enb = %o, eof = %s",
	       'A'+chan, ic-1, iaddr & ADDRMASK, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, ", flags = %08o\n", channel[chan].cflags);
#endif
      channel[chan].cflags &= ~(CHAN_TRAPPEND | CHAN_PRINTCLK);
      dev = check_chan (chan, 0, TRUE, WRITE_SEL);
      CYCLE ();
      break;

   case 0000767:   /* ALS */
#ifdef USE64
      shcnt = y & 0377;
      if ((shcnt >= 35) ? ((ac & MAGMASK) != 0) :
			  (((ac & MAGMASK) >> (35 - shcnt)) != 0))
	 cpuflags |= CPU_ACOFLOW;
      if (shcnt >= 37) ac = ac & ACSIGN;
      else ac = (ac & ACSIGN) | ((ac << shcnt) & (Q|DATAMASK));
#else
      for (y &= 0377; y; y--)
      {
         ach = (ach & SIGN) | ((ach << 1) & (Q|P|HMSK));
         if (acl & BIT4)
            ach++;
         acl <<= 1;
         if (ach & P)
	    cpuflags |= CPU_ACOFLOW;
      }
#endif
      break;

   case 0000770:   /* WEF */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "WEF Channel %c, ic = %05o, trap_enb = %o, eof = %s",
	       'A'+chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, ", flags = %08o\n", channel[chan].cflags);
#endif
      dev = check_chan (chan, 0, FALSE, WEF_SEL);
      CYCLE ();
      break;

   case 0000771:   /* ARS */
#ifdef USE64
      shcnt = y & 0377;
      if (shcnt >= 37)
         ac = ac & ACSIGN;
      else
         ac = (ac & ACSIGN) | ((ac & (Q|DATAMASK)) >> shcnt);
#else
      for (y &= 0377; y; y--)
      {
         acl >>= 1;
         if (ach & 1)
            acl |= BIT4;
         ach = (ach & SIGN) | ((ach >> 1) & (Q|P|HMSK));
      }
#endif
      break;

   case 0000772:   /* REW */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "REW Channel %c, ic = %05o, trap_enb = %o, eof = %s",
	       'A'+chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, ", flags = %08o\n", channel[chan].cflags);
#endif
      dev = check_chan (chan, -1, FALSE, REW_SEL);
      CYCLE ();
      break;

   case 0000774:   /* AXT */
#ifdef USE64
      setxr ((int32)(sr & ADDRMASK));
#else
      setxr (srl);
#endif
      break;

   case 0000776:   /* SDN */
      if (chk_user ()) break;
      chan = ((iaddr & 017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "SDN Channel %c, ic = %05o, trap_enb = %o, eof = %s",
	       'A'+chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, ", flags = %08o\n", channel[chan].cflags);
#endif
      dev = check_chan (chan, -1, FALSE, SDN_SEL);
      CYCLE ();
      break;

   default:
ILLINST:
      cpuflags |= CPU_MACHCHK;
      if (!chk_user ())
      {
	 sprintf (errview[0], "Illegal instruction: op = %o, ic = %o\n",
		  op, ic);
	 run = CPU_STOP;
      }
      break;
   }
}
