/***********************************************************************
*
* chan7607.c - IBM 7090 emulator 7607 channel routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   02/07/05   DGP   Fixed max tape on a channel access.
*   05/19/05   DGP   Fixed multi-channel tape I/O.
*   01/03/06   DGP   Added alternate BCD support.
*   06/01/06   DGP   Added simh tape format support.
*   06/20/06   DGP   Moved 7289 (CTSS High speed drum) support here.
*   12/21/06   DGP   Added new functions to streamline I/O.
*   05/30/07   DGP   Changed channel operation, decouple from cycle_count.
*   02/19/08   DGP   Correct EOF detection on card reader.
*   02/28/08   DGP   Added SPRA and printer clock, ala Lisp.
*   02/29/08   DGP   Changed to use bit mask flags.
*   03/03/08   DGP   Added channel command stacking.
*   03/11/10   DGP   Fixed IOCx bugs.
*   05/05/10   DGP   Fixed EOF pending in check_reset.
*   05/18/10   DGP   Preserve device EOF in ioflags. Check in readword.
*   06/17/10   DGP   Short circuit BSR delay for CTSS.
*   06/28/10   DGP   Fill short DASD records with zero, not blanks.
*   12/06/10   DGP   Fixed parity checking.
*   03/31/11   DGP   Correct B core operation.
*   08/17/11   DGP   Handle SPRA printer codes.
*   11/17/11   DGP   Fix printer clock in 32 bit mode. SIGN not set.
*   03/18/15   DGP   Added real tape support.
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysdef.h"
#include "regs.h"
#include "s709.h"
#include "parity.h"
#include "io.h"
#include "trace.h"
#include "chan7607.h"
#include "chan7909.h"
#include "console.h"
#include "lights.h"
#include "dasd.h"

extern char *devstr();
extern int prtviewlen;
extern int panelmode;

extern char tonative[64];
extern char toaltnative[64];
extern char prtview[PRTVIEWLINENUM][PRTVIEWLINELEN+2];
extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];

extern uint32 datecols[10];

/*
* 7607 Channel commands.
*/

#define IOCD	00
#define TCH	01
#define IORP	02
#define IORT	03
#define IOCP	04
#define IOCT	05
#define IOSP	06
#define IOST	07

#if defined(DEBUGIO) && !defined(DEBUGCIO)
#define DEBUGCIO
#endif
#if defined(DEBUG7607) || defined(DEBUG7909) || defined(DEBUGCIO)
#define DEBUGCHAN 0
static char *opstr[] =
{
   "IOCD",  "TCH", "IORP", "IORT", "IOCP", "IOCT", "IOSP", "IOST"
};
#endif

static char *sopstr[] =
{
   "IDLE", "CMD ", "RDS ", "WRS ", "BSR ", "BSF ", "WEF ", "REW ",
   "RUN ", "SDN ", "SNS ", "CHAN", "WAIT"
};


/*
 * Card machine states:
 *      0:  9 left      1:  9 right
 *      2:  8 left      3:  8 right
 *      4:  7 left      5:  7 right
 *      6:  6 left      7:  6 right
 *      8:  5 left      9:  5 right
 *     10:  4 left     11:  4 right
 *     12:  3 left     13:  3 right
 *     14:  2 left     15:  2 right
 *     16:  1 left     17:  1 right
 *     18:  0 left     19:  0 right
 *     20: 11 left     21: 11 right
 *     22: 12 left     23: 12 right
 *     24: End of cycle
 *     47: Beginning of new cycle
 *
 * Read printer states:
 *      0:  9 left      1:  9 right
 *      2:  8 left      3:  8 right
 *      4:  7 left      5:  7 right
 *      6:  6 left      7:  6 right
 *      8:  5 left      9:  5 right
 *     10:  4 left     11:  4 right
 *     12:  3 left     13:  3 right
 *     14:  2 left     15:  2 right
 *     16:  1 left     17:  1 right
 *     18: 84 left     19: 84 right echo
 *     20:  0 left     21:  0 right
 *     22: 83 left     23: 83 right echo
 *     24: 11 left     25: 11 right
 *     26:  9 left     27:  9 right echo (clock)
 *     28: 12 left     29: 12 right
 *     30:  8 left     31:  8 right echo (clock)
 *     32:  7 left     33:  7 right echo (clock)
 *     34:  6 left     35:  6 right echo (clock)
 *     36:  5 left     37:  5 right echo (clock)
 *     38:  4 left     39:  4 right echo (clock)
 *     40:  3 left     41:  3 right echo (clock)
 *     42:  2 left     43:  2 right echo (clock)
 *     44:  1 left     45:  1 right echo (clock)
 *     46: End of cycle
 *     47: Beginning of new cycle
 */

uint16 crstate, crcol[80];	/* Card reader card code */
uint16 cpstate, cpcol[80];	/* Card punch card code */
uint16 prstate, prcol[120];	/* Printer card code */
uint8 cnvbuf[161];		/* Card data conversion buffer */
uint8 prtbuf[73];		/* Printer data conversion buffer */
uint16 pr_ws_rs[] = {		/* Convert from read state to write */
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 18, 18, 19, 20, 20, 20, 21, 22, 22, 22, 23,
   24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 47
};

/***********************************************************************
* readcard - Read a card.
***********************************************************************/

static void
readcard ()
{
   register int i;

   crstate = 0;
   for (i = 0; i < 80; i++)
      crcol[i] = 0;
   if ((i = readrec (0, cnvbuf, sizeof cnvbuf - 1)) < 0)
   {
      reset_chan (0);
      return;
   }
#ifdef DEBUG7607
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "readcard: ceof = %s(%s)\n",
	    channel[sysio[0].iochn].cflags & CHAN_EOF ? "Y" : "N",
	    channel[0].cflags & CHAN_EOF ? "Y" : "N");
#endif
   if (channel[sysio[0].iochn].cflags & CHAN_EOF)
      return;
   if (cpuflags & CPU_IOCHK)
      return;
   bincard (cnvbuf, crcol);
}

/***********************************************************************
* initpunch - Initialize card punch.
***********************************************************************/

static void
initpunch ()
{
   register int i;

   cpstate = 0;
   for (i = 0; i < 80; i++)
      cpcol[i] = 0;
}

/***********************************************************************
* writepunch - Punch a card.
***********************************************************************/

static void
writepunch ()
{

   cardbin (cpcol, cnvbuf);
   writerec (1, cnvbuf, sizeof cnvbuf - 1);
   initpunch ();
   return;
}

/***********************************************************************
* initprint - Initialize line printer.
***********************************************************************/

static void
initprint ()
{
   register int i;

   prstate = 0;
   for (i = 0; i < 120; i++)
      prcol[i] = 0;
}

/***********************************************************************
* writeprint - Print a line.
***********************************************************************/

static void
writeprint (int spracode)
{
   int i, j;
   int maxline = prtviewlen - 1;

#ifdef DEBUGPRINT
   fprintf (stderr, "\nwriteprint: spracode = %d\n", spracode);
#endif
   cardbcd (prcol, prtbuf, 72);
   if (spracode == 9)
   {
      j = 48;
   }
   else
   {
      j = 72;
      prtbuf[0] |= 0200;
   }
   writerec (2, prtbuf, j);

   if (maxline < 0) maxline = 0;
   for (i = 0; i < maxline; i++)
      strcpy (prtview[i], prtview[i + 1]);

   for (i = 0; i < j; i++)
      prtview[maxline][i] = tonative[prtbuf[i] & 077];
   prtview[maxline][j] = 0;

   if (spracode == 9)
   {
      for (j--; (j >= 0) && (prtview[maxline][j] == ' '); j--)
	 prtview[maxline][j] = 0;
   }

   logstr (prtview[maxline], spracode);
#if defined(DEBUGCIO)
   fprintf (stderr,
	    "write Channel A Printer  %05d %03o %03o %03o %03o %03o %03o\n",
	    strlen (prtview[maxline]),
	    prtview[maxline][0], prtview[maxline][1], prtview[maxline][2],
	    prtview[maxline][3], prtview[maxline][4], prtview[maxline][5]);
#endif
#if defined(DEBUG7607DATA) || defined(DEBUGCIODATA)
   fprintf (stderr, "%s\n", prtview[maxline]);
#endif
   lights ();
   pview (spracode);
   initprint ();
   return;
}

/***********************************************************************
* cvtdecimal - Convert decimal to BCD.
***********************************************************************/

static void
cvtdecimal (int val, uint8 *pb)
{
   *pb++ = val / 10;
   *pb = val % 10;
}

/***********************************************************************
* readchron - Read the chron clock.
***********************************************************************/

static int
readchron (uint8 *pb)
{
   t_uint64 clk;
   time_t curtim;
   struct tm *tptr;

   curtim = time (NULL);
   tptr = localtime (&curtim);

   cvtdecimal (tptr->tm_mon + 1, pb);
   cvtdecimal (tptr->tm_mday, pb + 2);
   cvtdecimal (tptr->tm_hour, pb + 4);
   cvtdecimal (tptr->tm_min, pb + 6);
   cvtdecimal (tptr->tm_sec, pb + 8);
#ifdef USE64
   clk = mem[CLOCK];
#else
   clk = (t_uint64)memh[CLOCK] << 32 | (t_uint64)meml[CLOCK];
#endif
   cvtdecimal ((uint32) (clk % 60), pb + 10);
   return (12);
}

/***********************************************************************
* readword - Read a word from the attached device.
***********************************************************************/

static void
readword (int ch)
{
   Tape_t *ptape;
   DASD_t *pdasd;
   uint8  *pbuf;
#ifdef USE64
   t_uint64 m;
#else
   uint32 m;
#endif
   int32  i, j;
   uint16 unit;
   uint16 col;
   uint16 dev;
   uint16 udev;
   uint8  dchr;

   unit = channel[ch].cunit;
#ifdef USE64
   channel[ch].cdr = 0;
#else
   channel[ch].cdrl = 0;
   channel[ch].cdrh = 0;
#endif

   if (unit == 0321)		/* Card reader */
   {
      if (sysio[0].ioflags & IO_ATEOF)
      {
	 channel[ch].cflags |= (CHAN_EOF | CHAN_EOR);
	 return;
      }
      if (crstate >= 24)
      {
         sysio[0].iochn = ch;
         readcard ();
         if (channel[ch].cflags & CHAN_EOF)
         {
            channel[ch].cflags |= CHAN_EOR;
            return;
         }
	 if (cpuflags & CPU_IOCHK)
            return;
      }

      j = (crstate & 1) ? 36 : 0;
      col = 1 << (crstate >> 1);
#ifdef USE64
      if (crcol[j++] & col)
         channel[ch].cdr |= 1;

      for (i = 0; i < 35; i++)
      {
         channel[ch].cdr <<= 1;
         if (crcol[j++] & col)
            channel[ch].cdr |= 1;
      }
#else
      if (crcol[j++] & col)
         channel[ch].cdrh |= SIGN;

      for (i = 04; i; i >>= 1)
         if (crcol[j++] & col)
            channel[ch].cdrh |= i;

      for (i = 0; i < 32; i++)
      {
         channel[ch].cdrl <<= 1;
         if (crcol[j++] & col)
            channel[ch].cdrl |= 1;
      }
#endif

      crstate++;
      if (crstate == 24)
	 channel[ch].cflags |= CHAN_EOR;
      return;
   }

   if (unit == 0361)		/* BCD printer */
   {
      if (prstate >= 46)
         initprint ();
      j = (prstate & 1)? 36 : 0;

#ifdef DEBUGPCLK
      fprintf (stderr, "readword: prstate = %d, j = %d, ioclk = %s\n",
               prstate, j, sysio[2].ioflags & IO_PRINTCLK ? "Y" : "N");
#endif
      col = 0;
      switch (prstate)
      {
      case  0:   /*  9 left  */
      case  1:   /*  9 right */
      case  2:   /*  8 left  */
      case  3:   /*  8 right */
      case  4:   /*  7 left  */
      case  5:   /*  7 right */
      case  6:   /*  6 left  */
      case  7:   /*  6 right */
      case  8:   /*  5 left  */
      case  9:   /*  5 right */
      case 10:   /*  4 left  */
      case 11:   /*  4 right */
      case 12:   /*  3 left  */
      case 13:   /*  3 right */
      case 14:   /*  2 left  */
      case 15:   /*  2 right */
      case 16:   /*  1 left  */
      case 17:   /*  1 right */
         col = 1 << (prstate >> 1);
         goto write_row;

      case 20:   /*  0 left  */
      case 21:   /*  0 right */
         col = 01000;
         goto write_row;

      case 24:   /* 11 left  */
      case 25:   /* 11 right */
         col = 02000;
         goto write_row;

      case 28:   /* 12 left  */
      case 29:   /* 12 right */
         col = 04000;
write_row:
#ifdef USE64
	 channel[ch].cdr = mem[channel[ch].car | channel[ch].dcore];

         for (m = P; m; m >>= 1)
	 {
            if (channel[ch].cdr & m)
               prcol[j] |= col;
            j++;
         }
#else
         channel[ch].cdrh = memh[channel[ch].car| channel[ch].dcore];
         channel[ch].cdrl = meml[channel[ch].car| channel[ch].dcore];
         if (channel[ch].cdrh & SIGN)
            prcol[j] |= col;
         j++;

         for (i = 04; i; i >>= 1)
	 {
            if (channel[ch].cdrh & i)
               prcol[j] |= col;
            j++;
         }
         for (m = 020000000000; m; m >>= 1)
	 {
            if (channel[ch].cdrl & m)
               prcol[j] |= col;
            j++;
         }
#endif
         break;

      case 18:   /* 84 left  echo */
      case 19:   /* 84 right echo */
         col = 00042;
         goto read_8row;

      case 22:   /* 83 left  echo */
      case 23:   /* 83 right echo */
         col = 00102;
         goto read_8row;


      case 27:   /*  9 right echo */
         if (channel[ch].cflags & CHAN_PRINTCLK)
	 {
#ifdef USE64
            channel[ch].cdr = (t_uint64)datecols[9] << 6;
#else
	    channel[ch].cdrh = (datecols[9] >> 26) & 017;
	    if (channel[ch].cdrh & 010) channel[ch].cdrh |= SIGN;
	    channel[ch].cdrl = (datecols[9] & 0377777777) << 6;
#endif
#ifdef DEBUGPCLK
	    fprintf (stderr, "   datecols[9] = %012o\n", datecols[9]);
#ifdef USE64
	    fprintf (stderr, "   cdr         = %012llo\n", channel[ch].cdr);
#else
	    fprintf (stderr, "   cdr         = %02o%010lo\n",
	      ((channel[ch].cdrh & 017) << 2) | (short)(channel[ch].cdrl >> 30),
	      (unsigned long)channel[ch].cdrl & 07777777777);
#endif
#endif
	    break;
	 }
      case 26:   /*  9 left  echo */
         col = 00001;
         goto read_row;

      case 31:   /*  8 right echo */
         if (channel[ch].cflags & CHAN_PRINTCLK)
	 {
#ifdef USE64
            channel[ch].cdr = (t_uint64)datecols[8] << 6;
#else
	    channel[ch].cdrh = (datecols[8] >> 26) & 017;
	    if (channel[ch].cdrh & 010) channel[ch].cdrh |= SIGN;
	    channel[ch].cdrl = (datecols[8] & 0377777777) << 6;
#endif
#ifdef DEBUGPCLK
	    fprintf (stderr, "   datecols[8] = %012o\n", datecols[8]);
#ifdef USE64
	    fprintf (stderr, "   cdr         = %012llo\n", channel[ch].cdr);
#else
	    fprintf (stderr, "   cdr         = %02o%010lo\n",
	      ((channel[ch].cdrh & 017) << 2) | (short)(channel[ch].cdrl >> 30),
	      (unsigned long)channel[ch].cdrl & 07777777777);
#endif
#endif
	    break;
	 }
      case 30:   /*  8 left  echo */
         col = 00002;
read_8row:
#ifdef USE64
         if ((prcol[j++] & 00142) == col)
            channel[ch].cdr |= 1;
         for (i = 0; i < 35; i++)
	 {
            channel[ch].cdr <<= 1;
            if ((prcol[j++] & 00142) == col)
               channel[ch].cdr |= 1;
         }
#else
         if ((prcol[j++] & 00142) == col)
            channel[ch].cdrh |= SIGN;
         for (i = 04; i; i >>= 1)
            if ((prcol[j++] & 00142) == col)
               channel[ch].cdrh |= i;
         for (i = 0; i < 32; i++)
	 {
            channel[ch].cdrl <<= 1;
            if ((prcol[j++] & 00142) == col)
               channel[ch].cdrl |= 1;
         }
#endif
         break;

      case 33:   /*  7 right echo */
         col++;
      case 35:   /*  6 right echo */
         col++;
      case 37:   /*  5 right echo */
         col++;
      case 39:   /*  4 right echo */
         col++;
      case 41:   /*  3 right echo */
         col++;
      case 43:   /*  2 right echo */
         col++;
      case 45:   /*  1 right echo */
         col++;
         if (channel[ch].cflags & CHAN_PRINTCLK)
	 {
#ifdef USE64
            channel[ch].cdr = (t_uint64)datecols[col] << 6;
#else
	    channel[ch].cdrh = (datecols[col] >> 26) & 017;
	    if (channel[ch].cdrh & 010) channel[ch].cdrh |= SIGN;
	    channel[ch].cdrl = (datecols[col] & 0377777777) << 6;
#endif
#ifdef DEBUGPCLK
	    fprintf (stderr, "   datecols[%d] = %012o\n", col, datecols[col]);
#ifdef USE64
	    fprintf (stderr, "   cdr         = %012llo\n", channel[ch].cdr);
#else
	    fprintf (stderr, "   cdr         = %02o%010lo\n",
	      ((channel[ch].cdrh & 017) << 2) | (short)(channel[ch].cdrl >> 30),
	      (unsigned long)channel[ch].cdrl & 07777777777);
#endif
#endif
	    break;
	 }
      case 32:   /*  7 left  echo */
      case 34:   /*  6 left  echo */
      case 36:   /*  5 left  echo */
      case 38:   /*  4 left  echo */
      case 40:   /*  3 left  echo */
      case 42:   /*  2 left  echo */
      case 44:   /*  1 left  echo */
         col = 1 << ((prstate - 28) >> 1);

read_row:
#ifdef USE64
         if (prcol[j++] & col)
            channel[ch].cdr |= 1;
         for (i = 0; i < 35; i++)
	 {
            channel[ch].cdr <<= 1;
            if (prcol[j++] & col)
               channel[ch].cdr |= 1;
         }
#else
         if (prcol[j++] & col)
            channel[ch].cdrh |= SIGN;
         for (i = 04; i; i >>= 1)
            if (prcol[j++] & col)
               channel[ch].cdrh |= i;
         for (i = 0; i < 32; i++)
	 {
            channel[ch].cdrl <<= 1;
            if (prcol[j++] & col)
               channel[ch].cdrl |= 1;
         }
#endif
      }
      prstate++;
      if (prstate == 46)
      {
	 channel[ch].cflags |= CHAN_EOR;
         writeprint (0);
         channel[ch].spracode = 0;
      }
      return;
   }

   if (channel[ch].ctype == CHAN_7607) /* 7607 == Tape */
   {
      udev = (unit & 017) - 1;
      dev = udev + TAPEOFFSET + 10*ch;
      ptape = &channel[ch].devices.tapes[udev];
      pbuf = ptape->buf;

#ifdef DEBUG76071
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "read  %s        state = %d, pos = %10ld\n",
	       devstr (dev), ptape->state, sysio[dev].iopos);
#endif

      if (ptape->state == TAPEIDLE)
      {
	 long startpos;

	 /*
	 ** If CTSS and tape is A7, then Cronlog clock
	 */

	 if (cpumode == 7096 && ch == 0 && udev == 6)
	 {
	    ptape->reclen = readchron (pbuf);
	    startpos = sysio[dev].iopos = 0;
	    channel[ch].cunit |= 020;
	 }

	 /*
	 ** Normal tape...
	 */

	 else
	 {
	    startpos = sysio[dev].iopos;
	    sysio[dev].iochn = ch;
	    if ((ptape->reclen = readrec (dev, pbuf, MAXRECLEN)) == -1)
	    {
	       reset_chan (ch);
	       return;
	    }
	    channel[ch].ccyc = 2000;
	 }

#if defined(DEBUG7607) || defined(DEBUG7607RW)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr,
		  "read  %s  %05d %03o %03o %03o %03o %03o %03o %10ld\n",
		  devstr (dev), ptape->reclen,
		  pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
		  startpos);
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "read  %s completed                      %10ld\n",
		  devstr (dev), sysio[dev].iopos);
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "read  %s flags = %08o\n",
	 	  devstr (dev), channel[ch].cflags);
#endif
#if defined(DEBUGCIO)
	 fprintf (stderr,
		  "read  %s  %05d %03o %03o %03o %03o %03o %03o\n",
		  devstr (dev), ptape->reclen,
		  pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5]);
#endif
#if defined(DEBUG7607DATA) || defined(DEBUGCIODATA)
	 {
	    int iii, jjj, kkk;
	    jjj = ptape->reclen;
	    for (iii = 0; iii < jjj; )
	    {
	       fprintf (stderr, "   ");
	       if (channel[ch].cunit & 020)
	       {
		  if (jjj > 234) jjj = 234;
		  for (kkk = 0; kkk < 12; kkk++)
		     fprintf (stderr, "%03o ", pbuf[iii+kkk]);
		  fputs ("   ", stderr);
		  for (kkk = 0; kkk < 12; kkk++)
		     fputc (tonative[pbuf[iii+kkk] & 077], stderr);
		  iii += 12;
		  fputc ('\n', stderr);
	       }
	       else
	       {
		  if (altbcd || (sysio[dev].ioflags & IO_ALTBCD))
		     fputc (toaltnative[pbuf[iii] & 077], stderr);
		  else
		     fputc (tonative[pbuf[iii] & 077], stderr);
		  iii++;
	       }
	    }
	    fputc ('\n', stderr);
	 }
#endif
	 ptape->curloc = 0;
	 ptape->state = TAPEREAD;

	 if (pbuf[0] == 0217)	/* end of file marker */
	 {
	    if (pbuf[1] == 0)
	    {
	       channel[ch].cflags |= CHAN_EOF | CHAN_EOR;
	       ptape->state = TAPEIDLE;
	       return;
	    }
	    if (pbuf[1] == 017)
	    {
	       pbuf[0] = 0100;
	       pbuf[1] = 0100;
	    }
	 }
      }
      for (i = 0; i < 6; i++)
      {
         int kk, kb;
#ifdef USE64
	 channel[ch].cdr = (channel[ch].cdr & CHAR5MSK) << 6;
#else
	 channel[ch].cdrh = 0;
	 if (channel[ch].cdrl &                  004000000000)
	    channel[ch].cdrh |= SIGN;
	 channel[ch].cdrh |= (channel[ch].cdrl & 003400000000) >> 26;
	 channel[ch].cdrl =  (channel[ch].cdrl & 000377777777) << 6;
#endif

	 kk = j = pbuf[ptape->curloc++] & 0177;
	 if (unit & 020)
	 {
	    dchr = j & 077;
	    kb = 0;
	    j ^= oddpar[dchr];
	 }
	 else
	 {
	    kb = 1;
	    if (altbcd || (sysio[dev].ioflags & IO_ALTBCD))
	       dchr = binbcd[j & 077];
	    else
	       dchr = /*binbcd[j & 077]*/ j & 077;
	    j ^= evenpar[j & 077];
	 }
	 if (j) {
#ifdef DEBUG7607
	    fprintf (stderr,
	   "CHAN_CHECK: i = %d, chr = %03o, dchr = %03o, kb = %d, kc = %03o\n",
	           i, kk, dchr, kb, kb ? evenpar[kk&077] : oddpar[kk&77]);
	    fprintf (stderr, "   curloc = %d, unit = %o, dev = %s\n",
		     ptape->curloc-1, unit, devstr(dev));
#endif
	    channel[ch].cflags |= CHAN_CHECK;
	 }
#ifdef USE64
	 channel[ch].cdr |= dchr & 077;
#else
	 channel[ch].cdrl |= dchr & 077;
#endif
      }

      if (ptape->curloc >= ptape->reclen)
      {
	 channel[ch].cflags |= CHAN_EOR;
	 ptape->state = TAPEIDLE;
      }
   }
   else /* 7909 == DASD */
   {
      pdasd = &channel[ch].devices.dasd[unit];
      udev = (pdasd->unit & 07);
      dev = udev + DASDOFFSET + 10*ch;
      pbuf = pdasd->dbuf;

#ifdef DEBUG79091
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "%d: %05o: ", inst_count, ic);
	 fprintf (stderr, "read  %s        state = %d, pos = %10ld\n",
		  devstr (dev), pdasd->state, sysio[dev].iopos);
      }
#endif

      if (pdasd->state == DASDIDLE)
      {
	 long startpos;

	 startpos = sysio[dev].iopos;
	 sysio[dev].iochn = ch;
	 if ((pdasd->reclen = readrec (dev, pbuf, sysio[dev].iobyttrk)) == -1)
	 {
	    reset_7909 (ch);
	    return;
	 }

#if defined(DEBUG7909) || defined(DEBUG7909DATA)
	 if (ch == DEBUGCHAN)
	 {
	       fprintf (stderr, "%d: %05o: ", inst_count, ic);
	       fprintf (stderr,
			"read  %s  %05d %03o %03o %03o %03o %03o %03o %10ld\n",
			devstr (dev), pdasd->reclen,
			pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
			startpos);

#ifdef DEBUG7909DATA
	       {
		  int iii, jjj, kkk;
		  jjj = pdasd->reclen;
		  for (iii = 0; iii < jjj; )
		  {
		     /*if (jjj > 80) jjj = 80;*/
		     fprintf (stderr, "   ");
		     for (kkk = 0; kkk < 12; kkk++)
			fprintf (stderr, "%03o ", pbuf[iii+kkk]);
		     fputs ("   ", stderr);
		     for (kkk = 0; kkk < 12; kkk++)
			fputc (tonative[pbuf[iii+kkk] & 077], stderr);
		     iii += 12;
		     fputc ('\n', stderr);
		  }
		  fputc ('\n', stderr);
	       }
#endif
	 }
#endif

	 pdasd->curloc = 0;
	 pdasd->state = DASDREAD;
      }

      for (i = 0; i < 6; i++)
      {
#ifdef USE64
	 channel[ch].cdr = (channel[ch].cdr & CHAR5MSK) << 6;
#else
	 channel[ch].cdrh = 0;
	 if (channel[ch].cdrl &                  004000000000)
	    channel[ch].cdrh |= SIGN;
	 channel[ch].cdrh |= (channel[ch].cdrl & 003400000000) >> 26;
	 channel[ch].cdrl =  (channel[ch].cdrl & 000377777777) << 6;
#endif

	 j = pbuf[pdasd->curloc++] & 0177;
	 dchr = j & 077;
#ifdef USE64
	 channel[ch].cdr |= dchr & 077;
#else
	 channel[ch].cdrl |= dchr & 077;
#endif
      }

      if (pdasd->curloc >= pdasd->reclen)
	 channel[ch].cflags |= CHAN_EOR;
   }
}

/***********************************************************************
* writeword - Write a word to the attached device.
***********************************************************************/

static void
writeword (int ch)
{
   Tape_t *ptape;
   DASD_t *pdasd;
   uint8  *pbuf;
   int32  i, j;
   uint16 unit;
   uint16 udev;
   uint16 dev;
   uint16 col;
   uint8  dchr;

   unit = channel[ch].cunit;
   channel[ch].cflags &= ~CHAN_EOR;

   if (unit == 0341)		/* Card punch */
   {
      if (cpstate >= 24)
         initpunch ();
      j = (cpstate & 1)? 36 : 0;
      col = 1 << (cpstate >> 1);

#ifdef USE64
      if (channel[ch].cdr & SIGN)
         cpcol[j] |= col;
      j++;

      for (i = 0; i < 35; i++)
      {
         if (channel[ch].cdr & BIT1)
            cpcol[j] |= col;
         j++;
         channel[ch].cdr <<= 1;
      }
#else
      if (channel[ch].cdrh & SIGN)
         cpcol[j] |= col;
      j++;

      for (i = 04; i; i >>= 1)
      {
         if (channel[ch].cdrh & i)
            cpcol[j] |= col;
         j++;
      }
      for (i = 0; i < 32; i++)
      {
         if (channel[ch].cdrl & 020000000000)
            cpcol[j] |= col;
         j++;
         channel[ch].cdrl <<= 1;
      }
#endif

      cpstate++;
      if (cpstate == 24)
      {
	 channel[ch].cflags |= CHAN_EOR;
         writepunch ();
      }
      return;
   }

   if (unit == 0361)		/* BCD printer */
   {
      if (prstate >= 24)
         initprint ();
      j = (prstate & 1) ? 36 : 0;
      col = 1 << (prstate >> 1);

#ifdef USE64
      if (channel[ch].cdr & SIGN)
         prcol[j] |= col;
      j++;

      for (i = 0; i < 35; i++)
      {
         if (channel[ch].cdr & BIT1)
            prcol[j] |= col;
         j++;
         channel[ch].cdr <<= 1;
      }
#else
      if (channel[ch].cdrh & SIGN)
         prcol[j] |= col;
      j++;

      for (i = 04; i; i >>= 1)
      {
         if (channel[ch].cdrh & i)
            prcol[j] |= col;
         j++;
      }

      for (i = 0; i < 32; i++)
      {
         if (channel[ch].cdrl & 020000000000)
            prcol[j] |= col;
         j++;
         channel[ch].cdrl <<= 1;
      }
#endif

      prstate++;
      if (prstate == 24)
      {
	 channel[ch].cflags |= CHAN_EOR;
         writeprint (channel[ch].spracode);
         channel[ch].spracode = 0;
      }
      return;
   }

   if (channel[ch].ctype == CHAN_7607) /* 7607 == Tape */
   {
      udev = (unit & 017) - 1;
      dev = udev + TAPEOFFSET + 10*ch;
      ptape = &channel[ch].devices.tapes[udev];
      pbuf = ptape->buf;

#ifdef DEBUG76071
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "write  %s        state = %d, pos = %10ld\n", devstr (dev),
	       ptape->state, sysio[dev].iopos);
#endif
      if (ptape->state == TAPEIDLE)
      {
	 ptape->curloc = 0;
	 ptape->state = TAPEWRITE;
      }
#ifdef USE64
      dchr = (channel[ch].cdr >> 30) & 077;
#else
      dchr = ((channel[ch].cdrh & SIGN) >> 2) |
	     ((channel[ch].cdrh & HMSK) << 2) |
	     ((channel[ch].cdrl >> 30) & 03);
#endif

      if (unit & 020)
	 pbuf[ptape->curloc++] = oddpar[dchr & 077];
      else
      {
	 if (altbcd || (sysio[dev].ioflags & IO_ALTBCD))
	    pbuf[ptape->curloc++] = bcdbin[dchr & 077];
	 else
	    pbuf[ptape->curloc++] = evenpar[dchr & 077];
      }

      for (i = 1; i < 6; i++)
      {
#ifdef USE64
	 dchr = ((channel[ch].cdr >> 24) & 077);
#else
	 dchr = ((channel[ch].cdrl >> 24) & 077);
#endif
	 if (unit & 020)
	    pbuf[ptape->curloc++] = oddpar[dchr & 077];
	 else
	 {
	    if (altbcd || (sysio[dev].ioflags & IO_ALTBCD))
	       pbuf[ptape->curloc++] = bcdbin[dchr & 077];
	    else
	       pbuf[ptape->curloc++] = evenpar[dchr & 077];
	 }
#ifdef USE64
	 channel[ch].cdr <<= 6;
#else
	 channel[ch].cdrl <<= 6;
#endif
      }
   }
   else /* 7909 == DASD */
   {
      pdasd = &channel[ch].devices.dasd[unit];
      udev = (pdasd->unit & 07);
      dev = udev + DASDOFFSET + 10*ch;
      pbuf = pdasd->dbuf;

#ifdef DEBUG79091
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "%d: %05o: ", inst_count, ic);
	 fprintf (stderr,
		  "write  %s        state = %d, pos = %10ld\n", devstr (dev),
		  pdasd->state, sysio[dev].iopos);
      }
#endif
      if (pdasd->state == DASDIDLE)
      {
	 pdasd->curloc = 0;
	 pdasd->state = DASDWRITE;
      }
#ifdef USE64
      dchr = (channel[ch].cdr >> 30) & 077;
#else
      dchr = ((channel[ch].cdrh & SIGN) >> 2) |
	     ((channel[ch].cdrh & HMSK) << 2) |
	     ((channel[ch].cdrl >> 30) & 03);
#endif

      pbuf[pdasd->curloc++] = dchr & 077;

      for (i = 1; i < 6; i++)
      {
#ifdef USE64
	 dchr = ((channel[ch].cdr >> 24) & 077);
#else
	 dchr = ((channel[ch].cdrl >> 24) & 077);
#endif
	 pbuf[pdasd->curloc++] = dchr & 077;
#ifdef USE64
	 channel[ch].cdr <<= 6;
#else
	 channel[ch].cdrl <<= 6;
#endif
      }
   }
}

/***********************************************************************
* rw_chan - Perform channel Read/Write.
***********************************************************************/

static void
rw_chan (int ch)
{
   int timeset = FALSE;
   uint32 coreaddr;

   channel[ch].ccyc = 20;

   coreaddr = channel[ch].car | channel[ch].dcore;
#if defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "rw_chan %-18.18s   %s     op = %o cwr = %o\n",
	    devstr (whatdev (ch)),
	    channel[ch].csel == READ_SEL ?  "READ " : "WRITE",
	    channel[ch].cop, channel[ch].cwr);
   fprintf (stderr, "   core = %c, car = %o, flags = %08o\n",
	    channel[ch].dcore ? 'B' : 'A', coreaddr, channel[ch].cflags);
#endif

   switch (channel[ch].cop & 07)
   {

   case IOCD: /* 00 */
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
	       channel[ch].ccyc = 1000;
	       timeset = TRUE;
               goto done1;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
	 {
	    if (channel[ch].cflags & CHAN_EOR)
	       channel[ch].cflags &= ~CHAN_EOR;
            return;
	 }
      }
done1:
      endrecord (ch, 0);
      channel[ch].csel = NOT_SEL;
      channel[ch].cact = CHAN_IDLE;
      chan_in_op &= ~(1 << ch);
#ifdef DEBUG7607
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "rw_chan: done1: cact = %d\n", channel[ch].cact);
#endif
      return;

   case IORP: /* 02 */
      channel[ch].cflags &= ~CHAN_IOCX;
      if (channel[ch].cwr)
      {
         if (channel[ch].cflags & CHAN_EOR)
	 {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
            fprintf (stderr, "   EOR\n");
#endif
	    channel[ch].ccyc = 1000;
	    timeset = TRUE;
            goto done2;
	 }
         if (channel[ch].csel == READ_SEL)
	 {
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
               goto done2;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
            return;
      }
done2:
      endrecord (ch, 1);
      if (channel[ch].cflags & CHAN_EOF)
      {
	 channel[ch].ccyc = 600;
         channel[ch].csel = NOT_SEL;
         channel[ch].cact = CHAN_IDLE;
         chan_in_op &= ~(1 << ch);
#ifdef DEBUG7607
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "rw_chan: done2: cact = %d\n", channel[ch].cact);
#endif
         return;
      }
      break;

   case IORT: /* 03 */
      channel[ch].cflags &= ~CHAN_IOCX;
      if (channel[ch].cwr)
      {
         if (channel[ch].cflags & CHAN_EOR)
	 {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
            fprintf (stderr, "   EOR\n");
#endif
	    channel[ch].ccyc = 1000;
	    channel[ch].cact = CHAN_IDLE;
	    timeset = TRUE;
            goto done3;
	 }
         if (channel[ch].csel == READ_SEL)
	 {
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
               goto done3;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
            return;
      }
done3:
      if (channel[ch].cflags & CHAN_LOADPEND)
      {
         channel[ch].ccyc = 60;
	 channel[ch].cact = CHAN_WAIT;
	 return;
      }
      endrecord (ch, 1);
      if (channel[ch].cflags & CHAN_EOF)
      {
	 channel[ch].ccyc = 600;
         channel[ch].csel = NOT_SEL;
	 channel[ch].cact = CHAN_IDLE;
	 chan_in_op &= ~(1 << ch);
      }
      else if (!timeset)
      {
         channel[ch].ccyc = 60;
	 channel[ch].cact = CHAN_IDLE;
      }
#ifdef DEBUG7607
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "rw_chan: done3: cact = %d\n", channel[ch].cact);
#endif
      return;

   case IOCP: /* 04 */
      channel[ch].cflags |= CHAN_IOCX;
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
               goto done4;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
	 {
	    if (channel[ch].cflags & CHAN_EOR)
	       channel[ch].cflags &= ~CHAN_EOR;
            return;
	 }
      }
done4:
      if (channel[ch].cflags & CHAN_EOF)
      {
	 channel[ch].ccyc = 600;
         channel[ch].csel = NOT_SEL;
         channel[ch].cact = CHAN_IDLE;
#ifdef DEBUG7607
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "rw_chan: done4: cact = %d\n", channel[ch].cact);
#endif
         chan_in_op &= ~(1 << ch);
         return;
      }
      break;

   case IOCT: /* 05 */
      channel[ch].cflags |= CHAN_IOCX;
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
	    if (channel[ch].cflags & CHAN_EOR)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOR\n");
#endif
	       channel[ch].ccyc = 1000;
	       channel[ch].cact = CHAN_WAIT;
	       timeset = TRUE;
	       goto done5;
	    }
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
               goto done5;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
	 {
	    channel[ch].cflags &= ~CHAN_EOR;
            return;
	 }
      }
done5:
      if (channel[ch].cflags & CHAN_LOADPEND)
      {
         channel[ch].ccyc = 60;
	 channel[ch].cact = CHAN_WAIT;
	 return;
      }
      if (channel[ch].csel == WRITE_SEL && (channel[ch].cunit & 0700) == 0200)
	 endrecord (ch, 0);
      if (channel[ch].cflags & CHAN_EOF)
      {
	 channel[ch].ccyc = 600;
         channel[ch].csel = NOT_SEL;
	 channel[ch].cact = CHAN_IDLE;
	 chan_in_op &= ~(1 << ch);
      }
      else if (!timeset)
      {
         channel[ch].ccyc = 60;
	 channel[ch].cact = CHAN_WAIT;
      }
#ifdef DEBUG7607
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "rw_chan: done5: cact = %d\n", channel[ch].cact);
#endif
      return;

   case IOSP: /* 06 */
      channel[ch].cflags &= ~CHAN_IOCX;
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
               goto done6;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
            return;
      }
done6:
      if (channel[ch].cflags & CHAN_EOF)
      {
	 channel[ch].ccyc = 600;
         channel[ch].csel = NOT_SEL;
         channel[ch].cact = CHAN_IDLE;
         chan_in_op &= ~(1 << ch);
#ifdef DEBUG7607
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "rw_chan: done6: cact = %d\n", channel[ch].cact);
#endif
         return;
      }
      break;

   case IOST: /* 07 */
      channel[ch].cflags &= ~CHAN_IOCX;
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
	    if (channel[ch].cflags & CHAN_EOR)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOR\n");
#endif
	       channel[ch].ccyc = 1000;
	       channel[ch].cact = CHAN_WAIT;
	       timeset = TRUE;
	       goto done7;
	    }
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF)
	    {
#if defined(DEBUG7607RW) || defined(DEBUGCIO)
	       fprintf (stderr, "   EOF\n");
#endif
               goto done7;
	    }
            if ((channel[ch].cop & 010) == 0)
	    {
#ifdef USE64
               mem[coreaddr] = channel[ch].cdr;
#else
               memh[coreaddr] = channel[ch].cdrh;
               meml[coreaddr] = channel[ch].cdrl;
#endif
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	       if (watchstop && watchloc == coreaddr)
	       {
		  run = CPU_STOP;
		  atbreak = TRUE;
	       }
            }
         }
	 else
	 {
            writeword (ch);
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
            channel[ch].cdrh = memh[coreaddr];
            channel[ch].cdrl = meml[coreaddr];
#endif
            if (channel[ch].cwr > 1)
               channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
         }
         channel[ch].cwr--;
         if (channel[ch].cwr)
            return;
      }
done7:
      if (channel[ch].cflags & CHAN_LOADPEND)
      {
         channel[ch].ccyc = 60;
	 channel[ch].cact = CHAN_WAIT;
	 return;
      }
      if (channel[ch].csel == WRITE_SEL && (channel[ch].cunit & 0700) == 0200)
	 endrecord (ch, 0);
      if (channel[ch].cflags & CHAN_EOF)
      {
	 channel[ch].ccyc = 600;
         channel[ch].csel = NOT_SEL;
	 channel[ch].cact = CHAN_IDLE;
	 chan_in_op &= ~(1 << ch);
      }
      else if (!timeset)
      {
         channel[ch].ccyc = 60;
	 channel[ch].cact = CHAN_WAIT;
      }
#ifdef DEBUG7607
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "rw_chan: done7: cact = %d\n", channel[ch].cact);
#endif
      return;


   default:   /* invalid ops, TCH handled in load_chan */
      cpuflags |= CPU_MACHCHK;
      run = CPU_STOP;
      channel[ch].cact = CHAN_IDLE;
      chan_in_op &= ~(1 << ch);
      sprintf (errview[0], "I/O MACHINE check: cop = %o\n",
	       channel[ch].cop & 07);
#ifdef DEBUG7607
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr, "rw_chan: %s", errview[0]);
#endif
      return;
   }

   channel[ch].cact = CHAN_LOAD;
#ifdef DEBUG7607
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "rw_chan: exit: cact = %d\n", channel[ch].cact);
#endif
   channel[ch].ccyc = 1;
}

/***********************************************************************
* load_chan_cycle - Load channel and start cycle.
***********************************************************************/

static void
load_chan_cycle (int ch)
{
   int addr;
#ifdef USE64
   t_uint64 chsr;
#else
   uint8 chsrh; uint32 chsrl;
#endif

   addr = channel[ch].clr | channel[ch].ccore;
#ifdef USE64
   chsr = mem[addr];
#else
   chsrh = memh[addr];
   chsrl = meml[addr];
#endif
   channel[ch].clr = (channel[ch].clr + 1) & MEMLIMIT;

#ifdef USE64
   itrc[itrc_idx] = chsr;
#else
   itrc_h[itrc_idx] = chsrh;
   itrc_l[itrc_idx] = chsrl;
#endif
   itrc_buf[itrc_idx++] = channel[ch].clr + ((long)(ch+1) << 18);
   if (itrc_idx == TRACE_SIZE)
      itrc_idx = 0;

#ifdef USE64
   channel[ch].cop = ((chsr >> 33 ) & 007) | ((chsr & NBITMASK) >> 13);
   channel[ch].cwr = (chsr & DECRMASK) >> 18;
   channel[ch].car = chsr & ADDRMASK;
   channel[ch].dcore = chsr & BCORE;
#if defined(DEBUG7607) || defined(DEBUGCIO)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "load_chan_cycle: op = %c%012llo\n",
	    (chsr & SIGN)? '-' : ' ',
	    chsr & MAGMASK);
   fprintf (stderr,
   "   cop = %02o(%s%c), cwr = %05o, car = %05o, clr = %05o, unit = %02o%03o\n",
	    channel[ch].cop, opstr[channel[ch].cop & 7],
	    channel[ch].cop & 010 ? 'N' : ' ',
	    channel[ch].cwr, channel[ch].car, addr,
	    ch+1, channel[ch].cunit);
   fprintf (stderr, "   core = %c, flags = %08o\n",
            channel[ch].dcore ? 'B' : 'A', channel[ch].cflags);
#endif
   if (chsr & IBITMASK)
   {
      channel[ch].car = mem[channel[ch].car | channel[ch].dcore] & ADDRMASK;
      cycle_count++;
   }
#else
   channel[ch].cop = ((chsrh & SIGN) >> 5) | ((chsrh & 06) >> 1) |
        ((chsrl & NBITMASK) >> 13);
   channel[ch].cwr = ((chsrh & 1) << 14) | ((chsrl & DECRMASK) >> 18);
   channel[ch].car = chsrl & ADDRMASK;
   channel[ch].dcore = chsrl & BCORE;
#if defined(DEBUG7607) || defined(DEBUGCIO)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "load_chan_cycle: op = %c%02o%010lo\n",
	    (chsrh & SIGN)? '-' : ' ',
	    ((chsrh & 017) << 2) | (short)(chsrl >> 30),
	    (unsigned long)chsrl & 07777777777);
   fprintf (stderr,
   "   cop = %02o(%s%c), cwr = %05o, car = %05o, clr = %05o, unit = %02o%03o\n",
	    channel[ch].cop, opstr[channel[ch].cop & 7],
	    channel[ch].cop & 010 ? 'N' : ' ',
	    channel[ch].cwr, channel[ch].car, addr,
	    ch+1, channel[ch].cunit);
   fprintf (stderr, "   core = %c, flags = %08o\n",
            channel[ch].dcore ? 'B' : 'A', channel[ch].cflags);
#endif
   if (chsrl & IBITMASK)
   {
      channel[ch].car = meml[channel[ch].car | channel[ch].dcore] & ADDRMASK;
      cycle_count++;
   }
#endif 

   if ((channel[ch].cop & 07) == TCH)	/* 01 */
   {
      channel[ch].clr = channel[ch].car;
      channel[ch].ccore = channel[ch].dcore;
      channel[ch].cact = CHAN_LOAD;
   }
   else if (channel[ch].cwr && channel[ch].csel == WRITE_SEL)
      channel[ch].cact = CHAN_LOADDATA;
   else
      channel[ch].cact = CHAN_RUN;
#ifdef DEBUG7607
   fprintf (stderr, "   load_chan_cycle: cact = %d, ccyc = %d\n",
	    channel[ch].cact, channel[ch].ccyc);
#endif
   chan_in_op |= 1 << ch;
   channel[ch].ccyc = 10;
}

/***********************************************************************
* cycle_chan - Cycle the channel.
***********************************************************************/

static void
cycle_chan (int ch)
{
#if defined(DEBUG7607RW) || defined(DEBUGIOSTACK)
   int oldsel = channel[ch].csel;
#endif

#ifdef DEBUG7607RW
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "cycle_chan: Channel %c, cact = %d, csel = %d, ccyc = %d\n",
	    ch+'A', channel[ch].cact, oldsel, channel[ch].ccyc);
#endif
   cycle_count++;
   if (cycle_count >= next_lights)
   {
      lights ();
      next_lights = cycle_count + NEXTLIGHTS;
      next_steal = next_lights;
      check_intr ();
   }

NEXT_LOAD:
   switch (channel[ch].cact)
   {

   case CHAN_RUN:
      channel[ch].ccyc = 10;
      switch (channel[ch].csel)
      {

      case READ_SEL:
      case WRITE_SEL:
         rw_chan (ch);
	 if (channel[ch].csel == NOT_SEL)
	 {
#ifdef DEBUGIOSTACK
	    fprintf (stderr,
	 "cycle_chan: %s %-17s, cact = %d, xfer = %s, flags = %08o, eof = %s\n",
		     sopstr[oldsel], devstr(whatdev(ch)),
		     channel[ch].cact, "Y", channel[ch].cflags,
		     channel[ch].cflags & CHAN_EOF ? "Y" : "N");
#endif
	    channel[ch].cflags &= ~CHAN_DXFER;
	    if (channel[ch].cflags & CHAN_SNDXFER)
	    {
#ifdef DEBUGIOSTACK
	       fprintf (stderr,
			"   unstack non-data request: cmd = %s, unit = %06o\n",
			sopstr[channel[ch].csndcmd], channel[ch].csndunit);
#endif
	       channel[ch].cflags &= ~CHAN_SNDXFER;
	       channel[ch].cflags |= CHAN_NDXFER;
	       channel[ch].cact = CHAN_RUN;
	       channel[ch].csel = channel[ch].csndcmd;
	       channel[ch].cunit = channel[ch].csndunit;
	       chan_in_op |= (1 << ch);
	    }
	 }
         break;

      case SDN_SEL:
	 setdensity (ch);
         channel[ch].cact = CHAN_END;
         break;

      case BSR_SEL:
	 channel[ch].ccyc = 600;
         bsr (ch);
         channel[ch].cact = CHAN_END;
         break;

      case BSF_SEL:
	 channel[ch].ccyc = 600;
         bsf (ch);
         channel[ch].cact = CHAN_END;
         break;

      case WEF_SEL:
	 channel[ch].ccyc = 600;
         wef (ch);
         channel[ch].cact = CHAN_END;
         break;

      case REW_SEL:
         dorewind (whatdev(ch));
	 channel[ch].cflags |= CHAN_BOT;
         channel[ch].cact = CHAN_END;
         break;

      case RUN_SEL:
         unload (whatdev(ch));
	 channel[ch].cunit = 0;
         channel[ch].cact = CHAN_END;
         break;

      default: ;
      }
      break;

   case CHAN_LOAD:
      load_chan_cycle (ch);
      goto NEXT_LOAD;

   case CHAN_LOADDATA:
#ifdef USE64
      channel[ch].cdr = mem[channel[ch].car | channel[ch].dcore];
#else
      channel[ch].cdrh = memh[channel[ch].car | channel[ch].dcore];
      channel[ch].cdrl = meml[channel[ch].car | channel[ch].dcore];
#endif
      channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
      channel[ch].cact = CHAN_RUN;
#ifdef DEBUG7607RW
      fprintf (stderr, "   cycle_chan: LOADDATA: cact = %d\n",
	       channel[ch].cact);
#endif
      break;

   case CHAN_WAIT:
      if (channel[ch].cflags & CHAN_LOADPEND)
      {
	 channel[ch].cact = CHAN_LOAD;
	 channel[ch].cflags &= ~CHAN_LOADPEND;
      }
      else
      {
	 Tape_t *ptape;
	 uint16 unit;
	 uint16 udev;
#ifdef DEBUGIOSTACK
	 fprintf (stderr,
	 "cycle_chan: %s %-17s, cact = %d, xfer = %s, flags = %08o, eof = %s\n",
		  sopstr[channel[ch].csel], devstr(whatdev(ch)),
		  channel[ch].cact, "Y", channel[ch].cflags,
		  channel[ch].cflags & CHAN_EOF ? "Y" : "N");
#endif
	 unit = channel[ch].cunit;
	 if ((unit & 0700) == 0200)
	 {
	    udev = (unit & 017) - 1;
	    ptape = &channel[ch].devices.tapes[udev];
	    ptape->state = TAPEIDLE;
	 }
	 channel[ch].cflags &= ~(CHAN_DXFER | CHAN_IOCX);
	 channel[ch].cact = CHAN_IDLE;
	 channel[ch].csel = NOT_SEL;
	 channel[ch].cflags |= CHAN_TRAPPEND;
	 chan_in_op &= ~(1 << ch);
	 if (channel[ch].cflags & CHAN_SNDXFER)
	 {
	       channel[ch].cflags &= ~CHAN_SNDXFER;
	       channel[ch].cflags |= CHAN_NDXFER;
	       channel[ch].cact = CHAN_RUN;
	       channel[ch].csel = channel[ch].csndcmd;
	       channel[ch].cunit = channel[ch].csndunit;
	       channel[ch].ccyc = 10;
	       chan_in_op |= (1 << ch);
	 }
      }
#ifdef DEBUG7607
      fprintf (stderr, "   cycle_chan: WAIT: cact = %d, run = %d\n",
	       channel[ch].cact, run);
#endif
      break;

   case CHAN_END:
#ifdef DEBUGIOSTACK
      fprintf (stderr,
	 "cycle_chan: %s %-17s, cact = %d, xfer = %s, flags = %08o, eof = %s\n",
	       sopstr[channel[ch].csel], devstr(whatdev(ch)),
	       channel[ch].cact, "N", channel[ch].cflags,
	       channel[ch].cflags & CHAN_EOF ? "Y" : "N");
#endif
      channel[ch].cflags &= ~(CHAN_NDXFER | CHAN_IOCX);
      channel[ch].cact = CHAN_IDLE;
      channel[ch].csel = NOT_SEL;
      chan_in_op &= ~(1 << ch);
      if (channel[ch].cflags & CHAN_SDXFER)
      {
	 channel[ch].cflags &= ~CHAN_SDXFER;
#ifdef DEBUGIOSTACK
	 fprintf (stderr,
		  "   unstack data request: cmd = %s, unit = %06o\n",
		  sopstr[channel[ch].csdcmd], channel[ch].csdunit);
#endif
	 channel[ch].cflags |= CHAN_DXFER;
	 channel[ch].cflags &= ~(CHAN_TRAPPEND | CHAN_PRINTCLK);
	 channel[ch].cact = CHAN_UNSTACK;
	 channel[ch].csel = channel[ch].csdcmd;
	 channel[ch].cunit = channel[ch].csdunit;
	 startrec (ch);
	 channel[ch].ccyc = 10;
	 chan_in_op |= (1 << ch);
      }
#ifdef DEBUG7607
      fprintf (stderr, "   cycle_chan: END: cact = %d\n", channel[ch].cact);
#endif
      break;

   default: ;
   }
}

/***********************************************************************
* unitcheck - Check for I/O errors.
***********************************************************************/

int
unitcheck (int ch, int wr)
{
   uint16 unit;

   unit = channel[ch].cunit;
#if defined(DEBUGIO) || defined(DEBUG7607)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "unitcheck: Channel %c, unit = %o, wr = %d\n",
	    ch+'A', unit, wr);
#endif
   if (ch == 0)
   {
      if (unit == 0321 && wr == 1) /* Chan A Card reader */
      {
         sysio[0].iochn = ch;
         readcard ();
         channel[ch].ccyc = 4600;
         return 0;
      }
      if (unit == 0341 && wr == 0) /* Chan A Card punch */
      {
         initpunch ();
         channel[ch].ccyc = 8000;
         return 1;
      }
      if (unit == 0361)		/* Chan A BCD printer */
      {
         initprint ();
         channel[ch].ccyc = 5000;
         return 2;
      }
      if (unit == 0362)		/* Chan A Binary printer */
      {
         initprint ();
         channel[ch].ccyc = 5000;
         return 2;
      }
   }

   if (channel[ch].ctype == CHAN_7909) /* 7909 == DASD */
   {
      if (unit > 300)
	 unit = unit - 0330 + DASDOFFSET + 10*ch;
      else
         unit = (unit & 007) + DASDOFFSET + 10*ch;
#ifdef DEBUGIO
      fprintf (stderr, "   unit = %d, dev = %s\n",
	       unit, devstr (unit));
#endif
      return unit;
   }
   else if (channel[ch].ctype == CHAN_7750) /* 7750 == COMM */
   {
         unit = (unit & 007) + COMMOFFSET + 10*ch;
#ifdef DEBUGIO
      fprintf (stderr, "   unit = %d, dev = %s\n",
	       unit, devstr (unit));
#endif
      return unit;
   }

   if (channel[ch].cflags & CHAN_BOT)
      channel[ch].ccyc = 4000;
   else
      channel[ch].ccyc = 600;

   if (unit >= 0201 && unit <= 0212)	/* BCD tape */
      return unit - 0201 + TAPEOFFSET + 10*ch;
   if (unit >= 0221 && unit <= 0232)	/* Binary tape */
      return unit - 0221 + TAPEOFFSET + 10*ch;

   cpuflags |= CPU_IOCHK;
   sprintf (errview[0],
	    "unitcheck: I/O check: Channel %c, cop = %o, cunit = %o\n",
	    ch+'A', channel[ch].cop & 07, channel[ch].cunit);
#if defined(DEBUG7607) || defined(DEBUGTRAP) || defined(DEBUGIO)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "%s", errview[0]);
#endif
   run = CPU_STOP;
   return -1;
}

/***********************************************************************
* whatdev - Returns the device number for a specified unit number.
***********************************************************************/

int
whatdev (int ch)
{
   uint16 unit;

   unit = channel[ch].cunit;
   if (ch == 0)
   {
      if (unit == 0321)		/* Chan A Card reader */
         return 0;
      if (unit == 0341)		/* Chan A Card punch */
         return 1;
      if (unit == 0361)		/* Chan A BCD printer */
         return 2;
      if (unit == 0362)		/* Chan A Binary printer */
         return 2;
   }

   if (unit >= 0201 && unit <= 0212) /* BCD tape */
      return unit - 0201 + TAPEOFFSET + 10*ch;
   if (unit >= 0221 && unit <= 0232) /* Binary tape */
      return unit - 0221 + TAPEOFFSET + 10*ch;

   if (channel[ch].ctype == CHAN_7909) /* 7909 */
   {
      if (unit >= 0330 && unit <= 0337)	/* DASD */
         unit = unit - 0330 + DASDOFFSET + 10*ch;
      else
         unit = (unit & 007) + DASDOFFSET + 10*ch;
#ifdef DEBUGIO
      fprintf (stderr, "   unit = %d\n",
	       unit);
#endif
      return unit;
   }
   else if (channel[ch].ctype == CHAN_7750) /* 7750 */
   {
      unit = (unit & 007) + COMMOFFSET + 10*ch;
#ifdef DEBUGIO
      fprintf (stderr, "   unit = %d\n",
	       unit);
#endif
      return unit;
   }

   cpuflags |= CPU_IOCHK;
   sprintf (errview[0],
	    "whatdev: I/O check: Channel %c, cop = %o, cunit = %o\n",
	    ch+'A', channel[ch].cop & 07, channel[ch].cunit);
#if defined(DEBUG7607) || defined(DEBUGTRAP) || defined(DEBUGIO)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "%s", errview[0]);
#endif
   run = CPU_STOP;
   return -1;
}

/***********************************************************************
* startrec - Start a record for the specified device.
***********************************************************************/

void
startrec (int ch)
{
   uint16 unit;

   unit = channel[ch].cunit;

   if (unit == 0321)		/* Card reader */
   {
      if (crstate >= 24)
      {
         sysio[0].iochn = ch;
         readcard ();
         if (channel[ch].cflags & CHAN_EOF)
         {
            channel[ch].cflags |= CHAN_EOR;
            return;
         }
      }

   }

   else if (unit == 0341)	/* Card punch */
   {
      if (cpstate == 0)
         cpstate = 25;

   }

   else if (unit == 0361)	/* BCD printer */
   {
      if (prstate == 0)
         prstate = 47;
   }

   if (isdrum (ch))
      channel[ch].cflags |= CHAN_CTSSDRUM;
   else
      channel[ch].cflags &= ~CHAN_CTSSDRUM;
}

/***********************************************************************
* endrecord - Perform record termination specific to the device.
***********************************************************************/

void
endrecord (int ch, int skiptoend)
{
   Tape_t *ptape;
   DASD_t *pdasd;
   uint8  *pbuf;
   uint16 unit;
   uint16 dev;
   uint16 udev;

#ifdef USE64
   channel[ch].cdr = 0;
#else
   channel[ch].cdrh = 0;
   channel[ch].cdrl = 0;
#endif
   unit = channel[ch].cunit;

   if (unit == 0321)		/* Card reader */
   {
      if (sysio[0].ioflags & IO_ATEOF)
         return;
      while (crstate < 24)
         readword (ch);
   }
   
   else if (unit == 0341)	/* Card punch */
   {
      if (cpstate != 0)
      {
         if (cpstate == 25)
            cpstate = 0;
         do {
            writeword (ch);
         } while (cpstate != 0);
      }
   }

   else if (unit == 0361)	/* BCD printer */
   {
      if (channel[ch].csel == READ_SEL)
         prstate = pr_ws_rs[prstate];
      if (prstate != 0)
      {
         if (prstate == 47)
            prstate = 0;
         do {
            writeword (ch);
         } while (prstate != 0);
      }
   }

   else if (channel[ch].ctype == CHAN_7607)	/* 7607 == Tape */
   {
      udev = (unit & 017) - 1;
      dev = udev + TAPEOFFSET + 10*ch;
      ptape = &channel[ch].devices.tapes[udev];
      pbuf = ptape->buf;

      if (ptape->state == TAPEWRITE)
      {

#if defined(DEBUG7607) || defined(DEBUG7607RW)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
         fprintf (stderr,
		  "write %s  %05d %03o %03o %03o %03o %03o %03o %10ld\n",
		  devstr (dev), ptape->curloc,
		  pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
		  sysio[dev].iopos);

#endif
#if defined(DEBUGCIO)
	 fprintf (stderr,
		  "write %s  %05d %03o %03o %03o %03o %03o %03o\n",
		  devstr (dev), ptape->curloc,
		  pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5]);
#endif
#if defined(DEBUG7607DATA) || defined(DEBUGCIODATA)
	 {
	    int32  iii, jjj, kkk;
	    jjj = ptape->curloc;
	    for (iii = 0; iii < jjj; )
	    {
	       fprintf (stderr, "   ");
	       if (unit & 020)
	       {
		  if (jjj > 234) jjj = 234;
		  for (kkk = 0; kkk < 12; kkk++)
		     fprintf (stderr, "%03o ", pbuf[iii+kkk]);
		  fputs ("   ", stderr);
		  for (kkk = 0; kkk < 12; kkk++)
		     fputc (tonative[pbuf[iii+kkk] & 077], stderr);
		  iii += 12;
		  fputc ('\n', stderr);
	       }
	       else
	       {
		  if (altbcd || (sysio[dev].ioflags & IO_ALTBCD))
		     fputc (toaltnative[pbuf[iii] & 077], stderr);
		  else
		     fputc (tonative[pbuf[iii] & 077], stderr);
		  iii++;
	       }
	    }
	    fputc ('\n', stderr);
	 }
#endif

         writerec (dev, pbuf, ptape->curloc);
#if defined(DEBUG7607) || defined(DEBUG7607RW)
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "write %s completed                      %10ld\n",
		  devstr (dev), sysio[dev].iopos);
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "write %s flags = %08o\n",
	 	  devstr (dev), channel[ch].cflags);
#endif
         ptape->curloc = 0;
      }
      else if (skiptoend)
      {
#ifdef DEBUG7607
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr, "endrecord: skiptoend \n");
#endif
         while (!(channel[ch].cflags & CHAN_EOR))
         {
            readword (ch);
            if (channel[ch].cflags & CHAN_EOF) break;
         }
	 channel[ch].cflags &= ~CHAN_EOR;
      }
      ptape->state = TAPEIDLE;
   }

   else		/* 7909 == DASD */
   {

      pdasd = &channel[ch].devices.dasd[unit];
      dev = pdasd->unit + DASDOFFSET + 10*ch;
      pbuf = pdasd->dbuf;

#if defined(DEBUGDASD)
      fprintf (stderr,
    "endrecord: Channel %c, module = %d, curloc = %d, state = %d, pos = %ld\n",
	       ch+'A', unit, pdasd->curloc, pdasd->state, sysio[dev].iopos);
#endif
      if (pdasd->state == DASDWRITE)
      {
	 uint8 fchr;
	 uint8 *pfmt;

	 pfmt = pdasd->fbuf;

#if defined(DEBUG7909) || defined(DEBUG7909DATA)
	 if (ch == DEBUGCHAN)
	 {
	       fprintf (stderr, "%d: %05o: ", inst_count, ic);
	       fprintf (stderr,
		      "write9W %s  %05d %03o %03o %03o %03o %03o %03o %10ld\n",
			devstr (dev), pdasd->curloc,
			pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
			sysio[dev].iopos);

#ifdef DEBUG7909DATA
	       {
		  int32  iii, jjj, kkk;
		  jjj = pdasd->curloc;
		  for (iii = 0; iii < jjj; )
		  {
		     /*if (jjj > 80) jjj = 80;*/
		     fprintf (stderr, "   ");
		     fprintf (stderr, "%05o: ", iii);
		     for (kkk = 0; kkk < 12; kkk++)
			fprintf (stderr, "%03o ", pbuf[iii+kkk]);
		     fputs ("   ", stderr);
		     for (kkk = 0; kkk < 12; kkk++)
			if (altbcd || (sysio[dev].ioflags & IO_ALTBCD))
			   fputc (toaltnative[pbuf[iii+kkk] & 077], stderr);
			else
			   fputc (tonative[pbuf[iii+kkk] & 077], stderr);
		     iii += 12;
		     fputc ('\n', stderr);
		  }
		  fputc ('\n', stderr);
	       }
#endif
	 }
#endif

	 /*
	 ** Pad out buffer as needed
	 */

	 if (!(pdasd->flags & DASD_CTSSDRUM)) switch (channel[ch].cdcmd)
	 {
	 case DVHA:
	    while (pdasd->curloc < sysio[dev].iobyttrk)
	       pbuf[pdasd->curloc++] = 000;
	    break;

	 case DVTN:
	 case DVTA:
	 case DVSR:
	    while ((fchr = getfmtchar (pfmt, pdasd->curloc)) == FMT_DATA)
	       pbuf[pdasd->curloc++] = 000;
	    break;

	 default: ;
	 }
	 else
	    channel[ch].cflags |= CHAN_TRAPPEND;

         writerec (dev, pbuf, sysio[dev].iobyttrk);
      }
      else if (pdasd->state == DASDREAD)
      {
	 if (pdasd->flags & DASD_CTSSDRUM) 
	    channel[ch].cflags |= CHAN_TRAPPEND;
      }
      pdasd->curloc = 0;
      pdasd->state = DASDIDLE;
   }
}

/***********************************************************************
* active_chan - Cycle the channel while it is active.
***********************************************************************/

void
active_chan (int ch)
{
#ifdef DEBUG7607RW
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "active_chan: Channel %c, cact = %d, csel = %d\n",
	    ch+'A', channel[ch].cact, channel[ch].csel);
#endif
   cycle_chan (ch);

#ifdef DEBUG7607aa
   if (i == 1 && channel[i].csel)
   {
      fprintf (stderr,
	       "CYCLEe: cact = %d, csel = %d, ccyc = %d, cycle_count = %d\n",
	       channel[i].cact, channel[i].csel,
	       channel[i].ccyc, cycle_count);
      fprintf (stderr,
	       "   trap_inhibit = %d, ctrap_ind = %s, trap_enb = %o\n",
	       trap_inhibit, cpuflags & CPU_CHTRAP ? "Y" : "N", trap_enb);
   }
#endif

   if (channel[ch].csel && !channel[ch].cact)
   {
#ifdef DEBUGIOSTACK
      fprintf (stderr,
	 "activ_chan: %s %-17s, cact = %d, xfer = %s, flags = %08o, eof = %s\n",
	       sopstr[channel[ch].csel], devstr(whatdev(ch)),
	       channel[ch].cact, "Y", channel[ch].cflags,
	       channel[ch].cflags & CHAN_EOF ? "Y" : "N");
#endif
      channel[ch].csel = NOT_SEL;
      channel[ch].cflags &= ~CHAN_DXFER;
      channel[ch].cflags |= CHAN_TRAPPEND;
   }
}

/***********************************************************************
* load_chan - Load and activate channel.
***********************************************************************/

void
load_chan (int ch)
{
#ifdef DEBUG7607
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "load_chan: Channel %c\n", ch+'A');
#endif
   cycle_count++;
   load_chan_cycle (ch);
}

/***********************************************************************
* reset_chan - Reset 7607 channel.
***********************************************************************/

void
reset_chan (int ch)
{
#ifdef DEBUG7607
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "reset_chan: Channel %c, cact = %d, csel = %d\n",
	    ch+'A', channel[ch].cact, channel[ch].csel);
#endif
#ifdef USE64
   channel[ch].csns[0] = 0;
   channel[ch].csns[1] = 0;
   channel[ch].casr = 0;
#else
   channel[ch].csnsh[0] = 0;
   channel[ch].csnsl[0] = 0;
   channel[ch].csnsh[1] = 0;
   channel[ch].csnsl[1] = 0;
   channel[ch].casrh = 0;
   channel[ch].casrl = 0;
#endif
   channel[ch].cind = 0;
   channel[ch].cccr = 0;
   channel[ch].cflags = 0;
   channel[ch].clr = 0;
   channel[ch].cwr = 0;
   channel[ch].car = 0;
   channel[ch].csms = 0;
   channel[ch].ccore = 0;
   channel[ch].dcore = 0;
   channel[ch].cact = CHAN_IDLE;
   channel[ch].csel = NOT_SEL;
#ifdef DEBUG7607
   fprintf (stderr, "   reset_chan: cact = %d\n", channel[ch].cact);
#endif
}

/***********************************************************************
* load_drum - Load 7289 CTSS drum channel.
***********************************************************************/

void
load_drum (int ch)
{
   DASD_t *pdasd;
   uint32 drumloc;
   uint16 addr;
   uint16 drumaddr;
   uint16 dev;
   uint8 pdrum, ldrum;
#ifdef USE64
   t_uint64 chsr;
#else
   uint8 chsrh; uint32 chsrl;
#endif

#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "load_drum: Channel %c\n", ch+'A');
   }
#endif

   /*
   ** Get Drum address word.
   */

   cycle_count++;
   addr = channel[ch].clr | channel[ch].ccore;
#ifdef USE64
   chsr = mem[addr];
#else
   chsrh = memh[addr];
   chsrl = meml[addr];
#endif
   channel[ch].clr = (channel[ch].clr + 1) & MEMLIMIT;
#ifdef USE64
   itrc[itrc_idx] = chsr;
#else
   itrc_h[itrc_idx] = chsrh;
   itrc_l[itrc_idx] = chsrl;
#endif
   itrc_buf[itrc_idx++] = channel[ch].clr + ((long)(ch+1) << 18);
   if (itrc_idx == TRACE_SIZE)
      itrc_idx = 0;

#ifdef USE64
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   dop = %c%012llo\n",
	       (chsr & SIGN) ? '-' : ' ',
	       chsr & MAGMASK);
   }
#endif
   pdrum = (chsr >> 30) & 07;
   ldrum = (chsr >> 18) & 07;
   drumaddr = chsr & 0177777;
#else
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   dop = %c%02o%010lo\n",
	       (chsrh & SIGN) ? '-' : ' ',
	       ((chsrh & 017) << 2) | (short)(chsrl >> 30),
	       (unsigned long)chsrl & 07777777777);
   }
#endif
   pdrum = (chsrh << 2) | ((chsrl >> 30) & 07);
   ldrum = (chsrl >> 18) & 07;
   drumaddr = chsrl & 0177777;
#endif

   channel[ch].cunit = pdrum;
   pdasd = &channel[ch].devices.dasd[pdrum];
   pdasd->unit = pdrum;
   pdasd->state = DASDIDLE;
   pdasd->flags |= DASD_CTSSDRUM;
   pdasd->curloc = 0;
   dev = pdrum + DASDOFFSET + 10*ch;

   drumloc = ((ldrum-1) * (sysio[dev].ioheads * sysio[dev].iobyttrk))
	    + (drumaddr * 6) + DASDOVERHEAD;
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr,
	       "   pdrum = %d, ldrum = %d, drumaddr = %06o\n",
	       pdrum, ldrum, drumaddr);
      fprintf (stderr, "   unit = %d, dev = %s\n", dev, devstr (dev));
      fprintf (stderr, "   drumloc = %d\n", drumloc);
   }
#endif

   fseek (sysio[dev].iofd, drumloc, SEEK_SET);
   sysio[dev].iopos = drumloc;

   /*
   ** Get Drum channel command word.
   */

   addr = channel[ch].clr | channel[ch].ccore;
#ifdef USE64
   chsr = mem[addr];
#else
   chsrh = memh[addr];
   chsrl = meml[addr];
#endif
   channel[ch].clr = (channel[ch].clr + 1) & MEMLIMIT;
#ifdef USE64
   itrc[itrc_idx] = chsr;
#else
   itrc_h[itrc_idx] = chsrh;
   itrc_l[itrc_idx] = chsrl;
#endif
   itrc_buf[itrc_idx++] = channel[ch].clr + ((long)(ch+1) << 18);
   if (itrc_idx == TRACE_SIZE)
      itrc_idx = 0;
#ifdef USE64
   channel[ch].cop = ((chsr >> 33 ) & 007) | ((chsr & NBITMASK) >> 13);
   channel[ch].cwr = (chsr & DECRMASK) >> 18;
   channel[ch].car = chsr & ADDRMASK;
   channel[ch].dcore = chsr & BCORE;
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   op = %c%012llo\n",
	       (chsr & SIGN)? '-' : ' ',
	       chsr & MAGMASK);
      fprintf (stderr,
	"   cop = %02o, cwr = %05o, car = %05o, clr = %05o, unit = %02o%03o\n",
	       channel[ch].cop, channel[ch].cwr, channel[ch].car, addr,
	       ch+1, channel[ch].cunit);
      fprintf (stderr, "   core = %c, flags = %08o\n",
	       channel[ch].dcore ? 'B' : 'A', channel[ch].cflags);
   }
#endif
   if (chsr & IBITMASK)
   {
      channel[ch].car = mem[channel[ch].car | channel[ch].dcore] & ADDRMASK;
      cycle_count++;
   }
#else /* !USE64 */
   channel[ch].cop = ((chsrh & SIGN) >> 5) | ((chsrh & 06) >> 1) |
        ((chsrl & NBITMASK) >> 13);
   channel[ch].cwr = ((chsrh & 1) << 14) | ((chsrl & DECRMASK) >> 18);
   channel[ch].car = chsrl & ADDRMASK;
   channel[ch].dcore = chsrl & BCORE;
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   op = %c%02o%010lo\n",
	       (chsrh & SIGN)? '-' : ' ',
	       ((chsrh & 017) << 2) | (short)(chsrl >> 30),
	       (unsigned long)chsrl & 07777777777);
      fprintf (stderr,
    "   cop = %02o(%s), cwr = %05o, car = %05o, clr = %05o, unit = %02o%03o\n",
	       channel[ch].cop, opstr[channel[ch].cop & 7],
	       channel[ch].cwr, channel[ch].car, addr,
	       ch+1, channel[ch].cunit);
      fprintf (stderr, "   core = %c, flags = %08o\n",
	       channel[ch].dcore ? 'B' : 'A', channel[ch].cflags);
   }
#endif
   if (chsrl & IBITMASK)
   {
      channel[ch].car = meml[channel[ch].car| channel[ch].dcore] & ADDRMASK;
      cycle_count++;
   }
#endif /* !USE64 */

   if (channel[ch].cwr && channel[ch].csel == WRITE_SEL)
      channel[ch].cact = CHAN_LOADDATA;
   else
      channel[ch].cact = CHAN_RUN;
#ifdef DEBUG7607
   fprintf (stderr, "   load_drum: cact = %d\n", channel[ch].cact);
#endif
   chan_in_op |= 1 << ch;
   channel[ch].ccyc = 5;
}

/***********************************************************************
* check_reset - Check if 7607/7909 channel is ready to reset and load.
***********************************************************************/

void
check_reset (int chan)
{
   char op[10];

   sprintf (op, "%s%c", channel[chan].ctype ? "RSC" : "RCH", chan+'A');

#if defined(DEBUGIO) || defined(DEBUG7607) || defined(DEBUG7909)
   if (chan == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "%s Channel %c, ic = %05o, trap_enb = %o, eof = %s, drum = %s\n",
	       channel[chan].ctype ? "RSC" : "RCH",
	       'A' + chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N",
	       channel[chan].cflags & CHAN_CTSSDRUM ? "Y" : "N");
      fprintf (stderr, "   core = %s %s\n",
	       bcoreinst ? "B" : "A", bcoredata ? "B" : "A");
   }
#endif

   switch (channel[chan].cact)
   {
   case CHAN_IDLE:
   case CHAN_WAIT:
   case CHAN_UNSTACK:
      channel[chan].cflags &= ~(CHAN_IOCX | CHAN_EOR | CHAN_CHECK |
      				CHAN_EOF | CHAN_TRAPPEND);
      channel[chan].clr = y;
      channel[chan].ccore = bcoredata;

      if (channel[chan].ctype) /* 7909/7750 */
      {
	 if (channel[chan].cflags & CHAN_CTSSDRUM) 
	 {
	    load_drum (chan);
	 }
	 else
	 {
	    load_7909 (chan);
	 }
      }
      else /* 7607 */
      {
	 if (channel[chan].csel == READ_SEL || channel[chan].csel == WRITE_SEL)
	 {
	    load_chan (chan);
	 }
	 else
	 {
	    cpuflags |= CPU_IOCHK;
#ifdef DEBUGIO
	    sprintf (errview[0], "%s iochk: csel = %d, eof = %s\n",
		     op, channel[chan].csel, 
		     channel[chan].cflags & CHAN_EOF ? "Y" : "N");
	    fprintf (stderr, "%s\n", errview[0]);
#endif
	 }
      }
      break;

   default:
#if defined(DEBUGIO) || defined(DEBUG7607)
      if (chan == DEBUGCHAN)
	 fprintf (stderr, "   cact = %d, reset waiting...\n", channel[chan].cact);
#endif
      ic--;
   }
}

/***********************************************************************
* check_load - Check if 7607/7909 channel is ready to load.
***********************************************************************/

void
check_load (chan)
{
#if defined(DEBUGIO) || defined(DEBUG7607) || defined(DEBUG7909)
   static int load_printed = 0;

   if (!load_printed && (chan == DEBUGCHAN))
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "%s Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	       channel[chan].ctype ? "STC" : "LCH",
	       'A' + chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, "   core = %s %s\n",
	       bcoreinst ? "B" : "A", bcoredata ? "B" : "A");
   }
#endif

   if (channel[chan].ctype) /* 7909/7750 */
   {
      if (channel[chan].cflags & CHAN_INWAIT)
	 start_7909 (chan);
      else ic--;
   }
   else /* 7607 */
   {
      if (channel[chan].csel == READ_SEL || channel[chan].csel == WRITE_SEL)
      {
	 switch (channel[chan].cact)
	 {
	 case CHAN_WAIT:
	 case CHAN_LOAD:
	 case CHAN_END:
	 case CHAN_IDLE:
#if defined(DEBUGIO) || defined(DEBUG7607)
	    if (load_printed && (chan == DEBUGCHAN))
	    {
	       fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	       fprintf (stderr,
			"%s Channel %c, ic = %05o, waited %d times\n",
			channel[chan].ctype ? "STC" : "LCH",
			'A' + chan, ic-1, load_printed);
	    }
	    load_printed = 0;
#endif
	    channel[chan].cflags &= ~CHAN_LOADPEND;
	    channel[chan].clr = y;
	    channel[chan].ccore = bcoredata;
	    load_chan (chan);
	    break;

	 default:
#if defined(DEBUGIO) || defined(DEBUG7607)
	    if (!load_printed)
	    {
	       fprintf (stderr, "   cact = %d, load waiting...\n",
			channel[chan].cact);
	    }
	    load_printed++;
#endif
	    channel[chan].cflags |= CHAN_LOADPEND;
	    channel[chan].cloady = y;
	    ic--;
	    return;
	 }
      }
      else
      {
	 cpuflags |= CPU_IOCHK;
	 sprintf (errview[0], "%s%c iochk-1: eof = %s\n",
		  channel[chan].ctype ? "STC" : "LCH", chan+'A',
		  channel[chan].cflags & CHAN_EOF ? "Y" : "N");
#if defined(DEBUGIO) || defined(DEBUG7607)
	 fprintf (stderr, "%s\n", errview[0]);
#endif
      }
   }
}

/***********************************************************************
* check_chan - Check if 7607 channel is active.
***********************************************************************/

int 
check_chan (int chan, int mode, int xfer, int sel)
{
   int dev;

#if defined(DEBUGIO) || defined(DEBUG7607)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr,
  "check_chan: %s Channel %c, ic = %05o, trap_enb = %o, eof = %s, drum = %s\n",
	    sopstr[sel], 'A' + chan, ic-1, trap_enb,
	    channel[chan].cflags & CHAN_EOF ? "Y" : "N",
	    channel[chan].cflags & CHAN_CTSSDRUM ? "Y" : "N");
   fprintf (stderr, "   mode = %d, xfer = %d, csel = %d, cact = %d\n",
            mode, xfer, channel[chan].csel, channel[chan].cact);
#endif

   if (chan >= 0 && chan < numchan)
   {
#ifdef DEBUGIOSTACK
      if (!channel[chan].ctype) /* 7607 */
      {
	 int temp = channel[chan].cunit;
	 channel[chan].cunit = (iaddr - getxr (FALSE)) & 0777;
	 fprintf (stderr,
	 "check_chan: %s %-17s, cact = %d, xfer = %s, flags = %08o, eof = %s\n",
		  sopstr[sel], devstr(whatdev(chan)),
		  channel[chan].cact, xfer ? "Y" : "N", channel[chan].cflags,
		  channel[chan].cflags & CHAN_EOF ? "Y" : "N");
	 channel[chan].cunit = temp;
      }
#endif
      /*
      ** On a 7607 channel, stack the command if busy
      */
      if (channel[chan].cact != CHAN_IDLE)
      {
	 if (!channel[chan].ctype) /* 7607 */
	 {
	    if (xfer)
	    {
	       if (channel[chan].cflags & CHAN_NDXFER) 
	       {
		  if (!(channel[chan].cflags & CHAN_SDXFER) &&
		      (channel[chan].csel != WEF_SEL))
		  {
#ifdef DEBUGIOSTACK
		     fprintf (stderr,"   stack data request\n");
#endif
		     channel[chan].cflags |= CHAN_SDXFER;
		     channel[chan].csdcmd = sel;
		     channel[chan].csdunit = (iaddr - getxr (FALSE)) & 0777;
		     return (-1);
		  }
	       }
	    }
	    else
	    {
	       if (channel[chan].cflags & CHAN_DXFER)
	       {
		  if (!(channel[chan].cflags & CHAN_SNDXFER))
		  {
#ifdef DEBUGIOSTACK
		     fprintf (stderr,"   stack non-data request\n");
#endif
		     channel[chan].cflags |= CHAN_SNDXFER;
		     channel[chan].csndcmd = sel;
		     channel[chan].csndunit = (iaddr - getxr (FALSE)) & 0777;
		     return (-1);
		  }
	       }
	    }
	 }
#if defined(DEBUGIO) || defined(DEBUG7607)
	 fprintf (stderr, "   cact = %d, chan waiting...\n",
		  channel[chan].cact);
#endif
	 ic--;
	 return (-2);
      }

      if (!channel[chan].ctype) /* 7607 */
      {
	 if (xfer)
	 {
	    channel[chan].cflags |= CHAN_DXFER;
	 }
	 else
	 {
	    channel[chan].cflags |= CHAN_NDXFER;
	 }
      }

      channel[chan].cunit = (iaddr - getxr (FALSE)) & 0777;
      dev = unitcheck (chan, mode);

      switch (sel)
      {
      case READ_SEL:
      case WRITE_SEL:
	 startrec (chan);
	 break;

      default:
	 /*
	 ** Short circuit channel delay for CTSS.
	 ** CTSS uses a 25 inst spin.... yuk.
	 */
	 if (sel == BSR_SEL && cpumode == 7096)
	 {
	    channel[chan].ccyc = 10;
	 }
	 channel[chan].cact = CHAN_RUN;
	 chan_in_op |= 1 << chan;
      }
      channel[chan].csel = sel;
      trap_inhibit = 1;
   }
   else
   {
      dev = -1;
      cpuflags |= CPU_MACHCHK;
      sprintf (errview[0], "%s MACHINE check: Channel %c\n",
	       sopstr[sel], chan+'A');
      run = CPU_STOP;
   }
   return (dev);
}

/***********************************************************************
* store_chan - Store 7607/7909 channel info.
***********************************************************************/

void
store_chan (int chan)
{
   if (channel[chan].ctype) /* 7909/7750 */
   {
#if defined(DEBUG7909)
      if (chan == DEBUGCHAN)
      {
	 fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
	 fprintf (stderr,
		  "SCH Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
		  'A' + chan, ic-1, trap_enb,
		  channel[chan].cflags & CHAN_EOF ? "Y" : "N");
	 fprintf (stderr, "   core = %s %s\n",
		  bcoreinst ? "B" : "A", bcoredata ? "B" : "A");
      }
#endif

#ifdef USE64
      sr = ((channel[chan].car & ADDRMASK) << DECRSHIFT) |
	    (channel[chan].clr & ADDRMASK);
#if defined(DEBUG7909)
      if (chan == DEBUGCHAN)
      {
	 fprintf (stderr, "   sr = %012llo\n", sr);
      }
#endif
#else
      srh = ((channel[chan].car >> 14) & 1);
      srl = (((uint32)channel[chan].car & ADDRMASK) << DECRSHIFT) |
	     (channel[chan].clr & ADDRMASK);
#if defined(DEBUG7909)
      if (chan == DEBUGCHAN)
      {
	 fprintf (stderr, "   sr = %02o%010lo\n",
	       ((srh & SIGN) >> 2) | ((srh & HMSK) << 2) | (uint16)(srl >> 30),
		  (unsigned long)srl & 07777777777);
      }
#endif
#endif
   }
   else /* 7607 */
   {
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "SCH Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	       'A' + chan, ic-1, trap_enb,
	       channel[chan].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, "   core = %s %s\n",
	       bcoreinst ? "B" : "A", bcoredata ? "B" : "A");
#endif
#ifdef USE64
      sr = (((t_uint64)channel[chan].cop & 07) << PRESHIFT) |
	   (((t_uint64)channel[chan].clr & ADDRMASK) << DECRSHIFT) |
	   (((t_uint64)channel[chan].cop & 010) << 13) |
	    ((t_uint64)channel[chan].car & ADDRMASK);
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "   sr = %012llo\n", sr);
#endif
#else
      srh = ((channel[chan].cop << 5) & SIGN) |
	    ((channel[chan].cop << 1) & 06) |
	    ((channel[chan].clr >> 14) & 1);
      srl = (((uint32)channel[chan].clr & ADDRMASK) << DECRSHIFT) |
	    ((channel[chan].cop & 010) << 13) |
	     (channel[chan].car & ADDRMASK);
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "   sr = %02o%010lo\n",
	       ((srh & SIGN) >> 2) | ((srh & HMSK) << 2) | (uint16)(srl >> 30),
	       (unsigned long)srl & 07777777777);
#endif
#endif
   }
#ifdef USE64
   STORE (y, sr);
#else
   STORE (y, srh, srl);
#endif
}

/***********************************************************************
* check_eof - Check if channel is at EOF for TEF.
***********************************************************************/

void
check_eof (int chan)
{

#if defined(DEBUGIO) || defined(DEBUG7607)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr,
	 "TEF Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	    'A' + chan, ic-1, trap_enb,
	    channel[chan].cflags & CHAN_EOF ? "Y" : "N");
#endif

   if (trap_enb & (1 << chan))
      return;

   if (cpuflags & CPU_TRAPMODE)
      traptrace ();

   if (channel[chan].cflags & CHAN_EOF)
   {
      channel[chan].cflags &= ~CHAN_EOF;
      if (cpuflags & CPU_TRAPMODE)
	 ic = 00001;
      else
	 ic = y;
   }
}

/***********************************************************************
* check_cchk - Check if channel has had a channel check.
***********************************************************************/

void
check_cchk (int chan)
{

   if (trap_enb & (01000000 << chan))
      return;

   if (cpuflags & CPU_TRAPMODE)
      traptrace ();

   if (channel[chan].cflags & CHAN_CHECK)
   {
#if defined(DEBUGIO) || defined(DEBUG7607)
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "TRC Channel %c, trap_enb = %o, CHECK = %s\n",
	       'A' + chan, trap_enb,
	       channel[chan].cflags & CHAN_CHECK ? "Y" : "N");
#endif
      channel[chan].cflags &= ~CHAN_CHECK;
      if (cpuflags & CPU_TRAPMODE)
	 ic = 00001;
      else
	 ic = y;
   }
}
