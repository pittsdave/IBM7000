/***********************************************************************
*
* lights.c - IBM 7090 emulator front panel routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   02/14/05   DGP   Clean up inst. trace display.
*   05/23/05   DGP   Added 7909 support.
*   12/18/05   DGP   Added fflush for MINGW.
*   02/20/08   DGP   Added command enhancements to help().
*   02/29/08   DGP   Changed to use bit mask flags.
*   04/04/11   DGP   Clear pview line when displayed.
*   08/17/11   DGP   Handle SPRA printer codes.
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>
#include <string.h>

#include "sysdef.h"
#include "regs.h"
#include "trace.h"
#include "io.h"
#include "screen.h"

extern int prtviewlen;
extern int panelmode;
extern int outputline;
extern long total_cycles;

extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
extern char prtview[PRTVIEWLINENUM][PRTVIEWLINELEN+2];
extern char tonative[64];

static char chsel[] = " CRWBFMSCNCCC";


/***********************************************************************
* lights - Display the console.
***********************************************************************/

void
lights ()
{
   char *status;
   int32 c1, c2;


   if (!panelmode) return;

   screenposition (TOPLINE,1);
   clearline ();
#ifdef DISPLAYCYCLEST
   printf ("IBM %s Simulator %s  cycles: %10ld total: %10ld insts: %10ld\n",
	   txtcpumode, VERSION, cycle_count, total_cycles + cycle_count,
	   inst_count);
#else
   printf ("IBM %s Simulator %s\n", txtcpumode, VERSION); 
#endif

  /*
   *   TRAP  ACCUMULATOR QUOTIENT READ/WRITE DIVIDE  IO   MACHINE
   *          OVERFLOW   OVERFLOW   SELECT   CHECK  CHECK  CHECK
   * PROGRAM STOP  SSW bbbbbb  SL bbbb  CYCLE  RUN d+ooo  KEYS ooooooooooo
   * IC oooooo INSTR +oooo FLAG TAG o ADDR ooooo  EA ooooo SCNT   ooo
   * X1 ooooo  X2 ooooo  X3 ooooo  X4 ooooo  X5 ooooo  X6 ooooo  X7 ooooo
   * SR + oooooooooooo     ENB oooooooooooo     IND     o
   * AC +ooooooooooooo      SI oooooooooooo    DS oooooo +000000000000 'cccccc'
   * MQ + oooooooooooo    ADDRESS STOP   ooooo  WATCH STOP   ooooo
   *
   * A7 ..........  B7 ..........  C9 ..........  D7 ..........
   * OP oN UN oooR  OP oN UN oooW  OP oo CC oo    OP oN UN ooo
   * AR     oooooT  AR     ooooo   AR     ooooo   AR     ooooo
   * WR     oooooE  WR     ooooo   WR     ooooo   WR     ooooo
   * LR     oooooC  LR     ooooo   LR     ooooo   LR     ooooo
   * oooooooooooo   oooooooooooo   oooooooooooo   oooooooooooo
   */

   clearline ();
   printf ("  %s  %s %s %s %s %s %s\n",
	   cpuflags & CPU_TRAPMODE  ? "TRAP"          : "    ",
	   cpuflags & CPU_ACOFLOW   ? "ACCUMULATOR"   : "           ",
	   cpuflags & CPU_MQOFLOW   ? "QUOTIENT"      : "        ",
	   cpuflags & CPU_RWSEL     ? "READ/WRITE"    : "          ",
	   cpuflags & CPU_DIVCHK    ? "DIVIDE"        : "      ",
	   cpuflags & CPU_IOCHK     ? " IO  "         : "     ",
	   cpuflags & CPU_MACHCHK   ? "MACHINE"       : "       ");

   clearline ();
   printf ("        %s %s %s %s %s %s\n",
	   cpuflags & CPU_ACOFLOW  ? " OVERFLOW  "   : "           ",
	   cpuflags & CPU_MQOFLOW  ? "OVERFLOW"      : "        ",
	   cpuflags & CPU_RWSEL    ? "  SELECT  "    : "          ",
	   cpuflags & CPU_DIVCHK   ? "CHECK "        : "      ",
	   cpuflags & CPU_IOCHK    ? "CHECK"         : "     ",
	   cpuflags & CPU_MACHCHK  ? " CHECK "       : "       ");

   if (!(cpuflags & CPU_AUTO))
      status = "MANUAL      ";
   else if (cpuflags & CPU_PROGSTOP)
      status = "PROGRAM STOP";
   else if (run)
      status = "AUTOMATIC   ";
   else
      status = "READY       ";
   clearline ();
   printf (
#ifdef USE64
     "%s  SSW %o%o%o%o%o%o  SL %o%o%o%o  %s  RUN %d+%03o  KEYS %c%012llo \n",
#else
     "%s  SSW %o%o%o%o%o%o  SL %o%o%o%o  %s  RUN %d+%03o  KEYS %c%02o%010lo \n",
#endif
      status,
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
      single_cycle? "CYCLE" : "     ",
      run, chan_in_op,
#ifdef USE64
      (ky & SIGN)? '-' : '+',
      ky & MAGMASK
#else
      (kyh & SIGN)? '-' : '+',
      ((kyh & HMSK) << 2) | (short)(kyl >> 30),
      (unsigned long)kyl & 07777777777
#endif
      );

   clearline ();
   printf ("IC %06o INSTR %c%04o %s TAG %o ADDR %05o  EA %05o SCNT  %03o\n",
           ic | bcoreinst, (op & 0100000)? '-' : '+', op & 03777,
           flag? "FLAG" : "    ", tag, iaddr, y, shcnt);

   clearline ();
   if (cpumode >= 7094)
      printf ("X1 %05o  X2 %05o  X3 %05o  X4 %05o  X5 %05o  X6 %05o  X7 %05o\n",
	      xr[1], xr[2], xr[3], xr[4], xr[5], xr[6], xr[7]);
   else
      printf ("X1 %05o  X2 %05o  X4 %05o\n",
	      xr[1], xr[2], xr[4]);

   clearline ();
#ifdef USE64
   printf ("SR %c %012llo     ENB 00%010lo     IND     %01o\n",
	   (sr & SIGN)? '-' : '+',
	   sr & MAGMASK,
	   (unsigned long)trap_enb, cpuflags & CPU_CHTRAP ? 1 : 0);
#else
   printf ("SR %c %02o%010lo     ENB 00%010lo     IND     %01o\n",
	   (srh & SIGN)? '-' : '+',
	   ((srh & HMSK) << 2) | (short)(srl >> 30),
	   (unsigned long)srl & 07777777777,
	   (unsigned long)trap_enb, cpuflags & CPU_CHTRAP ? 1 : 0);
#endif

   clearline ();
#ifdef USE64
   dsr = mem[dsra];
   printf ("AC %c%013llo      SI %012llo    DS %06o %c%012llo",
	   (ac & ACSIGN)? '-' : '+',
	   ac & (Q|DATAMASK),
	   si,
	   dsra,
	   (dsr & SIGN)? '-' : '+',
	   dsr & MAGMASK);
   printf (" '%c%c%c%c%c%c'\n",
           tonative[(dsr >> 30) & 077],
           tonative[(dsr >> 24) & 077],
           tonative[(dsr >> 18) & 077],
           tonative[(dsr >> 12) & 077],
           tonative[(dsr >>  6) & 077],
           tonative[ dsr        & 077]);
#else
   dsrh = memh[dsra]; dsrl = meml[dsra];
   c1 = ((dsrh & SIGN) >> 2) | ((dsrh & HMSK) << 2) | (dsrl >> 30);
   printf ("AC %c%03o%010lo      SI %02o%010lo    DS %06o %c%02o%010lo",
	   (ach & SIGN)? '-' : '+',
	   ((ach & 037) << 2) | (short)(acl >> 30),
	   (unsigned long)acl & 07777777777,
	   ((sih & 017) << 2) | (short)(sil >> 30),
	   (unsigned long)sil & 07777777777,
	   dsra,
	   (dsrh & SIGN)? '-' : '+',
	   c1 & 037,
	   (unsigned long)dsrl & 07777777777);
   printf (" '%c%c%c%c%c%c'\n",
           tonative[ c1          & 077],
           tonative[(dsrl >> 24) & 077],
           tonative[(dsrl >> 18) & 077],
           tonative[(dsrl >> 12) & 077],
           tonative[(dsrl >>  6) & 077],
           tonative[ dsrl        & 077]);
#endif

   clearline ();
#ifdef USE64
   printf ("MQ %c %012llo",
	   (mq & SIGN)? '-' : '+',
	   mq & MAGMASK);
#else
   printf ("MQ %c %02o%010lo",
	   (mqh & SIGN)? '-' : '+',
	   ((mqh & HMSK) << 2) | (short)(mql >> 30),
	   (unsigned long)mql & 07777777777);
#endif

   if (addrstop)
      printf ("  ADDRESS STOP %06o", stopic);
   if (watchstop)
      printf ("  WATCH STOP %06o", watchloc);
   putchar ('\n');

   screenposition (CHANNELLINE,1);
   clearline ();
   for (c1 = 0; c1 < numchan; c1 += 4)
   {
      printf ("\n");
      clearline ();
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
      {
	 int dev;
	 char c;

	 printf ("%c%c ", c2 + 'A', channel[c2].ctype == CHAN_7607 ? '7' : '9');

	 if (channel[c2].ctype == CHAN_7607) /* 7607 channel */
	 {
	    for (dev = 10*c2 + TAPEOFFSET; dev <= 10*c2 + 12; dev++)
	    {
	       if ((channel[c2].cunit & 0100) == 0 &&
		   dev == (channel[c2].cunit & 017) + 2 + 10*c2)
		  c = 'S';
	       else if (sysio[dev].iofd == NULL)
		  c = '.';
	       else if (sysio[dev].ioflags & IO_REALDEV)
		  c = '*';
	       else if (sysio[dev].iopos == 0)
		  c = 'b';
	       else if (sysio[dev].iorw < IO_WRITE)
		  c = 'p';
	       else
		  c = 'o';
	       printf ("%c", c);
	    }
	    printf ("  ");
	 }
	 else if (channel[c2].ctype == CHAN_7909) /* 7909 channel */
	 {
	    for (dev = 10*c2 + DASDOFFSET; dev <= 10*c2 + 112; dev++)
	    {
	       if (sysio[dev].iofd == NULL)
		  c = '.';
	       else if (sysio[dev].iopos == 0)
		  c = 'b';
	       else if (sysio[dev].iorw < IO_WRITE)
		  c = 'p';
	       else
		  c = 'o';
	       printf ("%c", c);
	    }
	    printf ("  ");
	 }
	 else /* 7750 channel */
	 {
	    if (channel[c2].cflags & CHAN_COMMUP)
	       printf ("%2d ONLINE   ", channel[c2].cmodstart & 077);
	    else
	       fputs (" DISABLED   ", stdout);
	 }
      }
	 
      printf ("\n");
      clearline ();
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
      {
	 if (channel[c2].ctype == CHAN_7607) /* 7607 channel */
	    printf ("OP %o%c UN %03o%c  ",
		    channel[c2].cop & 007,
		    (channel[c2].cop & 010)? 'N' : ' ',
		    channel[c2].cunit & 0777,
		    chsel[channel[c2].csel]);
	 else  /* 7909 channel */
	    printf ("OP %02o UN %03o%c  ",
		    channel[c2].cop,
		    ((channel[c2].caccess << 6) | channel[c2].cunit) & 0777,
		    chsel[channel[c2].csel]);
      }
      printf ("\n");
      clearline ();
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
	 printf ("AR    %c%05o%c  ",
	         channel[c2].ccore ? '1' : ' ',
	         channel[c2].car,
		 channel[c2].cflags & CHAN_TRAPPEND ? 'T' : ' ');

      printf ("\n");
      clearline ();
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
	 printf ("WR     %05o%c  ",
	         channel[c2].cwr,
		 channel[c2].cflags & CHAN_EOF ? 'E' : ' ');

      printf ("\n");
      clearline ();
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
	 printf ("LR     %05o%c  ",
		 channel[c2].clr,
		 channel[c2].cflags & CHAN_CHECK ? 'C' : ' ');

      printf ("\n");
      clearline ();
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
      {
#ifdef USE64
	 printf ("%012llo   ", channel[c2].cdr & DATAMASK);
#else
	 printf ("%02o%010lo   ",
	         ((channel[c2].cdrh & SIGN) >> 2) |
	         ((channel[c2].cdrh & HMSK) << 2) |
	         (uint16)(channel[c2].cdrl >> 30),
	         (unsigned long)channel[c2].cdrl & 07777777777);
#endif
      }
      printf ("\n");
      clearline ();
#ifdef DISPLAYCYCLES
      for (c2 = c1; c2 < c1 + 4 && c2 < numchan; c2++)
      {
	 printf ("%012ld   ", channel[c2].ccyc);
      }
      printf ("\n");
      clearline ();
#endif
   }
   screenposition (outputline,1);
   fflush (stdout);
}

/***********************************************************************
* pview - Display panel messages.
***********************************************************************/

void
pview (int spracode)
{
   int i;

   if (!panelmode) 
   {
      if (spracode != 9)
         putchar ('\n');
      if (prtview[prtviewlen-1][0])
	 printf ("%s", prtview[prtviewlen-1]);
      prtview[prtviewlen-1][0] = '\0';
      fflush (stdout);
      return;
   }
   
   screenposition (outputline,1);
   printf ("\n");
   clearline ();

   for (i = 0; i < prtviewlen; i++)
   {
      clearline ();
      printf ("%s\n", prtview[i]);
   }
}

/***********************************************************************
* help - Display the help messages.
***********************************************************************/

void
help ()
{
   int i = 0;

   strcpy (errview[i++], "Help:\n");
   strcpy (errview[i++], "a                 - toggle automatic mode\n");
   strcpy (errview[i++], "b [addr]          - set/clear breakpoint\n");
   strcpy (errview[i++], "c                 - clear memory and reset cpu\n");
   strcpy (errview[i++], "da                - display AC\n");
   strcpy (errview[i++], "di                - display SI\n");
   strcpy (errview[i++], "dk                - display keys\n");
   strcpy (errview[i++], "dl                - display SL\n");
   strcpy (errview[i++], "dm                - display MQ\n");
   strcpy (errview[i++], "dn                - display next storage location\n");
   strcpy (errview[i++], "dp                - display PC\n");
   strcpy (errview[i++], "dr                - display SR\n");
   strcpy (errview[i++], "ds addr           - display storage location\n");
   strcpy (errview[i++], "dx#               - display index register\n");
   strcpy (errview[i++], "dw                - display SSW\n");
   strcpy (errview[i++], "D start end [file]- Dump memory [to file]\n");
   strcpy (errview[i++], "ea val            - enter AC\n");
   strcpy (errview[i++], "ei val            - enter SI\n");
   strcpy (errview[i++], "ek val            - enter keys\n");
   strcpy (errview[i++], "em val            - enter MQ\n");
   strcpy (errview[i++], "en val            - enter next storage location\n");
   strcpy (errview[i++], "ep val            - enter PC\n");
   strcpy (errview[i++], "er val            - enter SR\n");
   strcpy (errview[i++], "es addr val       - enter storage\n");
   strcpy (errview[i++], "ex# val           - enter index register\n");
   strcpy (errview[i++], "h                 - help\n");
   strcpy (errview[i++], "lc                - load cards\n");
   strcpy (errview[i++], "ld cmu            - load disk\n");
   strcpy (errview[i++], "lf[n] [addr] file - load file\n");
   strcpy (errview[i++], "lt [cuu]          - load tape\n");
   strcpy (errview[i++], "m device=file     - mount file on device\n");
   strcpy (errview[i++], "p [windowsize]    - toggle panel mode\n");
   strcpy (errview[i++], "q                 - quit\n");
   strcpy (errview[i++], "r                 - reset cpu\n");
   strcpy (errview[i++], "sc                - step one cycle\n");
   strcpy (errview[i++], "ss                - single step\n");
   strcpy (errview[i++], "st                - start\n");
   strcpy (errview[i++], "sw#               - toggle SSW\n");
   strcpy (errview[i++], "t                 - print trace\n");
   strcpy (errview[i++], "tc                - close trace to file\n");
   strcpy (errview[i++], "tf[u] file        - trace instructions to file\n");
   strcpy (errview[i++], "tl                - print long format trace\n");
   strcpy (errview[i++], "u device          - unmount device\n");
   strcpy (errview[i++], "w [addr]          - set/clear watchpoint\n");
}

/***********************************************************************
* trace - Display the trace data.
***********************************************************************/

void
trace ()
{
   int32 i, j, tp;
   int errndx;

   errndx = 0;
   strcpy (errview[errndx++], "Instruction trace:\n");
   tp = itrc_idx;
   for (i = 0; i < (TRACE_SIZE/8); i++)
   {
      strcpy (errview[errndx], " ");
      for (j = 0; j < 8; j++)
      {
	 char temp[80];

         sprintf (temp, " %c %06o",
		  " ABCDEFGH"[itrc_buf[tp] >> 18],
		  itrc_buf[tp] & 0177777);
	 strcat (errview[errndx], temp);
         tp++;
         if (tp == TRACE_SIZE)
            tp = 0;
      }
      strcat (errview[errndx++], "\n");
   }
}

/***********************************************************************
* long_trace - Display the trace in long format.
***********************************************************************/

void
long_trace ()
{
   int i, j, tp;
   int errndx;

   errndx = 0;
   strcpy (errview[errndx++], "Instruction trace:\n");
   tp = itrc_idx;
   for (i = 0; i < (TRACE_SIZE/2); i++)
   {
      strcpy (errview[errndx], "  ");
      for (j = 0; j < 2; j++)
      {
	 char temp[80];
         if (j > 0)
            strcat (errview[errndx], "        ");
#ifdef USE64
         sprintf (temp, " %c %06o  %c %012llo",
		  " ABCDEFGH"[itrc_buf[tp] >> 18],
		  itrc_buf[tp] & 0177777,
		  (itrc[tp] & SIGN)? '-' : '+',
		  itrc[tp] & MAGMASK);
#else
         sprintf (temp, " %c %06o  %c %02o%010lo",
		  " ABCDEFGH"[itrc_buf[tp] >> 18],
		  itrc_buf[tp] & 0177777,
		  (itrc_h[tp] & SIGN)? '-' : '+',
		  ((itrc_h[tp] & HMSK) << 2) |
		   (short)(itrc_l[tp] >> 30),
		  (unsigned long)itrc_l[tp] & 07777777777);
#endif
	 strcat (errview[errndx], temp);
         tp++;
         if (tp == TRACE_SIZE)
            tp = 0;
      }
      strcat (errview[errndx++], "\n");
   }
}
