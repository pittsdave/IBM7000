/***********************************************************************
*
* sense.c - IBM 7090 emulator sense opcode routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   05/19/05   DGP   Added RIC[A-H] command.
*   06/21/06   DGP   Added RDC[A-H] command.
*   12/15/06   DGP   Separate MSE and PSE insts.
*   02/28/08   DGP   Added SPRA and printer clock, ala Lisp.
*   02/29/08   DGP   Changed to use bit mask flags.
*   08/17/11   DGP   Handle SPRA printer codes.
*   11/14/11   DGP   Generate trap for illegal instruction in CTSS mode.
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>
#include <time.h>

#include "sysdef.h"
#include "s709.h"
#include "regs.h"
#include "arith.h"
#include "io.h"
#include "chan7607.h"
#include "chan7909.h"

extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
extern int tagmode;
uint32 datecols[10];

#if defined(DEBUG7909)
#define DEBUGCHAN 2
#endif

/***********************************************************************
* sense_instr - Process the sensing istructions.
***********************************************************************/

void
sense_instr ()
{
   uint32 select;
   uint8 spracode;

   if ((op & 0100000) == 0) /* PSE */
   {
      switch (y)
      {

      case 00000:   /* CLM */
#ifdef USE64
	 ac = ac & ACSIGN;
#else
	 ach = ach & SIGN;
	 acl = 0;
#endif
	 break;

      case 00001:   /* LBT */
#ifdef USE64
	 if (ac & 00000000001)
#else
	 if (acl & 00000000001)
#endif
	    ic++;
	 break;

      case 00002:   /* CHS */
#ifdef USE64
	 ac = ac ^ ACSIGN;
#else
	 ach = (~ach & SIGN) | (ach & (Q|P|HMSK));
#endif
	 break;

      case 00003:   /* SSP */
#ifdef USE64
	 ac &= (Q|DATAMASK);
#else
	 ach &= (Q|P|HMSK);
#endif
	 break;

      case 00004:   /* ENK */
#ifdef USE64
	 mq = ky;
#else
	 mqh = kyh;
	 mql = kyl;
#endif
	 break;

      case 00005:   /* IOT */
	 if (chk_user ()) break;
	 if (cpuflags & CPU_IOCHK)
	    cpuflags &= ~CPU_IOCHK;
	 else
	    ic++;
	 break;

      case 00006:   /* COM */
#ifdef USE64
	 ac ^= (Q|DATAMASK);
#else
	 ach = (ach & SIGN) | (~ach & (Q|P|HMSK));
	 acl = ~acl;
#endif
	 break;

      case 00007:   /* ETM */
	 if (chk_user ()) break;
	 cpuflags |= CPU_TRAPMODE;
	 break;

      case 00010:   /* RND */
	 rnd (1);
	 break;

      case 00011:   /* FRN */
	 frnd ();
	 break;

      case 00012:   /* DCT */
	 if (cpuflags & CPU_DIVCHK)
	    cpuflags &= ~CPU_DIVCHK;
	 else
	    ic++;
	 break;

      case 00014:   /* RCT */
#ifdef DEBUGTRAP
	 fprintf (stderr, "%d: ", inst_count);
	 fprintf (stderr,
	    "RCT ic = %05o, trap_enb = %o\n",
		  ic, trap_enb);
#endif
	 cpuflags |= CPU_CHTRAP;
	 trap_inhibit = 1;
	 break;

      case 00016:   /* LMTM */
	 if (cpumode < 7094) goto ILLINST;
	 cpuflags &= ~CPU_MULTITAG;
	 tagmode = 0;
	 break;

      case 00140:   /* SLF */
	 sl = 0;
	 break;

      case 00141:   /* SLN */
      case 00142:
      case 00143:
      case 00144:
	 select = 1 << (0144 - y);
	 sl |= select;
	 break;

      case 00161:   /* SWT */
      case 00162:
      case 00163:
      case 00164:
      case 00165:
      case 00166:
	 if (ssw & (1 << (0166 - y)))
	    ic++;
	 break;

      case 001000:  /* BTT */
      case 002000:
      case 003000:
      case 004000:
      case 005000:
      case 006000:
      case 007000:
      case 010000:
	 if (chk_user ()) break;
	 select = ((y & 00017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr,
		  "BTT Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
		  'A' + select, ic-1, trap_enb,
		  channel[select].cflags & CHAN_EOF ? "Y" : "N");
#endif
	 if (select >= numchan)
	 {
	    cpuflags |= CPU_MACHCHK;
	    run = CPU_STOP;
	    sprintf (errview[0],
		     "Sense MACHINE check: select = %o\n",
		     select);
	    break;
	 }
	 if (channel[select].cflags & CHAN_BOT)
	    channel[select].cflags &= ~CHAN_BOT;
	 else
	    ic++;
	 break;

      case 001350:  /* RICA */
      case 002350:  /* RICB */
      case 003350:  /* RICC */
      case 004350:  /* RICD */
      case 005350:  /* RICE */
      case 006350:  /* RICF */
      case 007350:  /* RICG */
      case 010350:  /* RICH */
	 if (chk_user ()) break;
	 select = ((y & 00017000) >> 9) - 1;
#if defined(DEBUG7909)
	 if (select == DEBUGCHAN)
	 {
	    fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	    fprintf (stderr,
		     "RIC Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
		     'A' + select, ic-1, trap_enb,
		     channel[select].cflags & CHAN_EOF ? "Y" : "N");
	 }
#endif
	 if (channel[select].ctype) /* 7909/7750 */
	    reset_7909 (select);
	 break;

      case 001352:  /* RDCA */
      case 002352:  /* RDCB */
      case 003352:  /* RDCC */
      case 004352:  /* RDCD */
      case 005352:  /* RDCE */
      case 006352:  /* RDCF */
      case 007352:  /* RDCG */
      case 010352:  /* RDCH */
	 if (chk_user ()) break;
	 select = ((y & 00017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr,
		  "RDC Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
		  'A' + select, ic-1, trap_enb,
		  channel[select].cflags & CHAN_EOF ? "Y" : "N");
#endif
	 if (!channel[select].ctype) /* 7607 */
	    reset_chan (select);
	 break;

      case 001361:  /* SPRA 1 */
      case 001362:  /* SPRA 2 */
      case 001363:  /* SPRA 3 */
      case 001364:  /* SPRA 4 */
      case 001365:  /* SPRA 5 */
      case 001366:  /* SPRA 6 */
      case 001367:  /* SPRA 7 */
      case 001370:  /* SPRA 8 */
      case 001371:  /* SPRA 9 */
	 if (chk_user ()) break;
	 select = ((y & 00017000) >> 9) - 1;
	 spracode = y & 017;
#if defined(DEBUGIO) || defined(DEBUG7607)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr,
		  "\n\rSPRA Channel %c, ic = %05o, spracode = %d\n",
		  'A' + select, ic-1, spracode);
#endif
	 if ((channel[select].csel == READ_SEL) &&
	     (sysio[2].ioflags & IO_PRINTCLK))
	 {
	    if (spracode == 7) /* Sense printer clock */
	    {
	       int i;
	       int secs;
	       int mins;
	       int digit;
	       time_t curtim;
	       struct tm *timeblk;

	       /*
	       * The printer date clock
	       * The data is returned on the "ECHO" right reads.
	       * Format is:
	       *
	       *  000 000 MM0 DD0 mmm m0t 000 000 000 000 000 000
	       *
	       * The datecols data is left shifted 6 bits when read.
	       */

	       curtim = time(NULL);
	       timeblk = localtime (&curtim);
	       timeblk->tm_mon++;
	       mins = timeblk->tm_hour * 60 + timeblk->tm_min;
	       secs = timeblk->tm_sec / 10;
#ifdef DEBUGPCLK
	       fprintf (stderr, "   date = %02d/%02d, mins = %04d.%d\n",
			timeblk->tm_mon, timeblk->tm_mday, mins, secs);
#endif
	       for (i = 0; i < 10; i++) datecols[i] = 0;

	       digit = timeblk->tm_mon % 10;
	       timeblk->tm_mon /= 10;
	       datecols[digit] |= 1 << 28;
	       digit = timeblk->tm_mon % 10;
	       datecols[digit] |= 1 << 29;

	       digit = timeblk->tm_mday % 10;
	       timeblk->tm_mday /= 10;
	       datecols[digit] |= 1 << 25;
	       digit = timeblk->tm_mday % 10;
	       datecols[digit] |= 1 << 26;

	       digit = mins % 10;
	       mins /= 10;
	       datecols[digit] |= 1 << 20;
	       digit = mins % 10;
	       mins /= 10;
	       datecols[digit] |= 1 << 21;
	       digit = mins % 10;
	       mins /= 10;
	       datecols[digit] |= 1 << 22;
	       digit = mins % 10;
	       datecols[digit] |= 1 << 23;

	       digit = secs % 10;
	       datecols[digit] |= 1 << 18;

#ifdef DEBUGPCLK
	       for (i = 0; i < 10; i++)
	       {
		  fprintf (stderr, "   datecols[%d] = %012o\n", i, datecols[i]);
	       }
#endif
	    }
	    else if (spracode == 9) /* Echo right */
	    {
	       channel[select].cflags |= CHAN_PRINTCLK;
	    }
	 }
	 channel[select].spracode = spracode;
	 break;

      default:
	 break;            /* All others NOP */
      }
   }
   else /* MSE */
   {
      switch (y)
      {

      case 00000:   /* CLM */
#ifdef USE64
	 ac = ac & ACSIGN;
#else
	 ach = ach & SIGN;
	 acl = 0;
#endif
	 break;

      case 00001:   /* PBT */
#ifdef USE64
	 if (ac & P)
#else
	 if (ach & P)
#endif
	    ic++;
	 break;

      case 00002:   /* EFTM */
	 if (chk_user ()) break;
	 cpuflags |= CPU_FPTRAP;
	 cpuflags &= ~CPU_MQOFLOW;
	 break;

      case 00003:   /* SSM */
#ifdef USE64
	 ac |= ACSIGN;
#else
	 ach |= SIGN;
#endif
	 break;

      case 00004:   /* LFTM */
	 if (chk_user ()) break;
	 cpuflags &= ~CPU_FPTRAP;
	 break;

      case 00005:   /* ESTM */
	 break;

      case 00006:   /* ECTM */
	 break;

      case 00007:   /* LTM */
	 if (chk_user ()) break;
	 cpuflags &= ~CPU_TRAPMODE;
	 break;

      case 00010:   /* LSNM */
	 break;

      case 00016:   /* EMTM */
	 if (cpumode < 7094) goto ILLINST;
	 cpuflags |= CPU_MULTITAG;
	 tagmode = CPU_MULTITAG;
	 break;

      case 00140:   /* SLF */
	 sl = 0;
	 break;

      case 00141:   /* SLT */
      case 00142:
      case 00143:
      case 00144:
	 select = 1 << (0144 - y);
	 if (sl & select)
	 {
	    sl -= select;
	    ic++;
	 }
	 break;

      case 00161:   /* SWT */
      case 00162:
      case 00163:
      case 00164:
      case 00165:
      case 00166:
	 if (ssw & (1 << (0166 - y)))
	    ic++;
	 break;

      case 001000:  /* ETT */
      case 002000:
      case 003000:
      case 004000:
      case 005000:
      case 006000:
      case 007000:
      case 010000:
	 if (chk_user ()) break;
	 select = ((y & 00017000) >> 9) - 1;
#if defined(DEBUGIO) || defined(DEBUG7607)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr,
		  "[BE]TT Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
		  'A' + select, ic-1, trap_enb,
		  channel[select].cflags & CHAN_EOF ? "Y" : "N");
#endif
	 if (select >= numchan)
	 {
	    cpuflags |= CPU_MACHCHK;
	    run = CPU_STOP;
	    sprintf (errview[0],
		     "Sense MACHINE check: select = %o\n",
		     select);
	    break;
	 }
	 if (channel[select].cflags & CHAN_EOT)
	    channel[select].cflags &= ~CHAN_EOT;
	 else
	    ic++;
	 break;

      default:
	 break;            /* All others NOP */
      }
   }

   return;

ILLINST:
   cpuflags |= CPU_MACHCHK;
   if (!chk_user ())
   {
      sprintf (errview[0], "Illegal instruction: op = %o, ic = %o\n", op, ic);
      run = CPU_STOP;
   }
}
