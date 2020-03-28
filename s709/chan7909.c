/***********************************************************************
*
* chan7909.c - IBM 7090 emulator 7909 channel routines.
*
* Changes:
*   05/24/05   DGP   Original.
*   05/30/07   DGP   Changed channel operation, decouple from cycle_count.
*   02/29/08   DGP   Changed to use bit mask flags.
*   03/12/10   DGP   Added COMM support.
*   05/15/10   DGP   Fix SIGN bit in getrecord and getrack.
*   03/30/11   DGP   Fix DVCY-W (Cylinder Write).
*   03/31/11   DGP   Correct B core operation.
*
* The DASD is formatted as follows:
*    4 bytes (big-endian): # cylinders
*    4 bytes (big-endian): # heads
*    4 bytes (big-endian): # access << 16 | # modules
*    4 bytes (big-endian): # bytes/track
*    N bytes: raw data for each track - the order is:
*      cylinder 0 head 0 track 0   - contains track format
*      cylinder 0 head 0 track 1   - data track 0
*      cylinder 0 head 0 track N+1 - data track N
*      cylinder 0 head 1 track 0   - contains track format
*      cylinder 0 head 1 track 1   - data track 0
*      cylinder 0 head 1 track N+1 - data track N
*      ...
*      cylinder 0 head H track 0   - contains track format
*      cylinder 0 head H track 1   - data track 0
*      cylinder 0 head H track N+1 - data track N
*      ...
*      cylinder C head H track 0   - contains track format
*      cylinder C head H track 1   - data track 0
*      cylinder C head H track N+1 - data track N
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>

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
#include "comm.h"

extern char *devstr();
extern int prtviewlen;

extern char tonative[64];
extern char toaltnative[64];
extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
extern char prtview[PRTVIEWLINENUM][PRTVIEWLINELEN+2];

#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
#define CHANOPMASK    00740000000000I64
#define CHANOPNMASK   00000000200000I64
#define CHANASYMASK   00007777777777I64

#define SNS_INVSEQ    00042000000000I64
#define SNS_INVCODE   00040400000000I64
#define SNS_FMTCHK    00040200000000I64
#define SNS_NOREC     00040100000000I64
#define SNS_INVADDR   00040020000000I64
#else
#define CHANOPMASK    00740000000000ULL
#define CHANOPNMASK   00000000200000ULL
#define CHANASYMASK   00007777777777ULL

#define SNS_INVSEQ    00042000000000ULL
#define SNS_INVCODE   00040400000000ULL
#define SNS_FMTCHK    00040200000000ULL
#define SNS_NOREC     00040100000000ULL
#define SNS_INVADDR   00040020000000ULL
#endif
#else
#define SNS_PGMCHK    001
#define SNS_INVSEQ    00002000000000UL
#define SNS_INVCODE   00000400000000UL
#define SNS_FMTCHK    00000200000000UL
#define SNS_NOREC     00000100000000UL
#define SNS_INVADDR   00000020000000UL
#endif
#define SNS_RESPONSE  00020004000000UL
#define SNS_CHECK     00020002000000UL
#define SNS_PARITY    00020001000000UL
#define SNS_OFFLINE   00010000200000UL
#define SNS_NOTRDY    00010000040000UL
#define SNS_DSKCHK    00010000020000UL
#define SNS_FILECHK   00010000010000UL
#define SNS_SIXBIT    00000000000400UL

typedef struct {
   int wrd;
   int shft;
} snsshft_t;

static snsshft_t snsshft[20] =
{
 /* access 0 */
  {0,4},  {0,2},  {0,1},  {0,0}, {1,34}, {1,32}, {1,31}, {1,30}, {1,28}, {1,26},
 /* access 1 */
 {1,25}, {1,24}, {1,22}, {1,20}, {1,19}, {1,18}, {1,16}, {1,14}, {1,13}, {1,12}
};

static int snsndx;

#ifdef DEBUG7909
#define DEBUGCHAN 4
static char *opstr[] =
{
   "WTR",  "XMT",  "WTR",  "XMT",  "TCH",  "LIPT", "TCH",  "LIPT",
   "CTL",  "CTLR", "CTLW", "SNS",  "LAR",  "SAR",  "TWT",  "",
   "CPYP", "",     "CPYP", "",     "CPYD", "TCM",  "CPYD", "TCM",
   "TWT",  "LIP",  "TDC",  "LCC",  "SMS",  "ICC",  "",     "ICC"
};
#endif

#ifdef DEBUGDASD
static char *fmtstr[] = { "DATA", "HDR", "HA2", "END" };
static char chsel[] = " CRWBFMSC";
#endif

#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
static t_uint64 RSCx[8] =
{
   0054000000000I64, 0454000000000I64, 0054100000000I64, 0454100000000I64, 
   0054200000000I64, 0454200000000I64, 0054300000000I64, 0454300000000I64 
};
static t_uint64 SCDx[8] =
{
   0064400000000I64, 0464400000000I64, 0064500000000I64, 0464500000000I64, 
   0064600000000I64, 0464600000000I64, 0064700000000I64, 0464700000000I64 
};
#else
static t_uint64 RSCx[8] =
{
   0054000000000ULL, 0454000000000ULL, 0054100000000ULL, 0454100000000ULL, 
   0054200000000ULL, 0454200000000ULL, 0054300000000ULL, 0454300000000ULL 
};
static t_uint64 SCDx[8] =
{
   0064400000000ULL, 0464400000000ULL, 0064500000000ULL, 0464500000000ULL, 
   0064600000000ULL, 0464600000000ULL, 0064700000000ULL, 0464700000000ULL 
};
#endif
#else
static uint8 RSCxh[8] = 
{
   0001, 0201, 0001, 0201, 0001, 0201, 0001, 0201
};
static uint32 RSCxl[8] = 
{
   014000000000, 014000000000, 014100000000, 014100000000,
   014200000000, 014200000000, 014300000000, 014300000000
};

static uint8 SCDxh[8] = 
{
   0001, 0201, 0001, 0201, 0001, 0201, 0001, 0201
};
static uint32 SCDxl[8] = 
{
   024400000000, 024400000000, 024500000000, 024500000000,
   024600000000, 024600000000, 024700000000, 024700000000
};
#endif

/***********************************************************************
* cvtzero - Convert value with BCD zero packing.
***********************************************************************/

static uint32
cvtzero (uint32 val)
{
   uint32 r;
   uint32 v;
   uint32 m;
   int i;

   r = 0;
   for (i = 0; i < 4; i++)
   {
      m = (077 << (i*6));
      v = (012 << (i*6));
      if ((val & m) != v)
	 r =  r | (val & m);
   }
   return (r);
}

/***********************************************************************
* cvtdecimal - Convert value as decimal BCD.
***********************************************************************/

static uint32
cvtdecimal (uint32 val)
{
   uint32 r;
   uint32 m;
   int i;

   r = 0;
   for (i = 3; i >= 0; i--)
   {
      m = (val >> (i*6)) & 077;
      if (m == 012) m = 0;
         r = (r * 10) + m;
   }
   return (r);
}

/***********************************************************************
* cvtoctal - Convert value as octal BCD.
***********************************************************************/

static uint32
cvtoctal (uint32 val)
{
   uint32 r;
   uint32 m;
   int i;

   r = 0;
   for (i = 3; i >= 0; i--)
   {
      m = (val >> (i*6)) & 017;
      if (m == 012) m = 0;
         r = (r << 4) | m;
   }
   return (r);
}

/***********************************************************************
* setsnsattn - Set SNS attention bits.
***********************************************************************/

static void
setsnsattn (int ch, int module)
{
   int i;

#if defined(DEBUG7909) || defined(DEBUGSATTN)
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "setsnsattn: Channel %c, module = %d, access = %d\n",
	       ch+'A', module, channel[ch].devices.dasd[module].access);
   }
#endif

   i = module;
   if (channel[ch].devices.dasd[module].access) i += 10;
#if defined(DEBUG7909) || defined(DEBUGSATTN)
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   sns[%d] |= 1 << %d\n",
	       snsshft[i].wrd, snsshft[i].shft);
   }
#endif

#ifdef USE64
   channel[ch].csns[snsshft[i].wrd] |= 1 << snsshft[i].shft;
#if defined(DEBUG7909) || defined(DEBUGSATTN)
   if (ch == DEBUGCHAN)
   {
      for (i = 0; i < 3; i++)
	 fprintf (stderr, "   csns[%d] = %12.12llo\n", i, channel[ch].csns[i]);
   }
#endif
#else
   if (snsshft[i].shft < 33)
      channel[ch].csnsl[snsshft[i].wrd] |= 1 << snsshft[i].shft;
   else
      channel[ch].csnsh[snsshft[i].wrd] |= 1 << (snsshft[i].shft - 33);
#if defined(DEBUG7909) || defined(DEBUGSATTN)
   if (ch == DEBUGCHAN)
   {
      for (i = 0; i < 3; i++)
	 fprintf (stderr, "   csns[%d] = %02o%010lo\n", i,
		  ((channel[ch].csnsh[i] & SIGN) >> 2) |
		  ((channel[ch].csnsh[i] & HMSK) << 2) |
		  (uint16)(channel[ch].csnsl[i] >> 30),
		  (unsigned long)channel[ch].csnsl[i] & 07777777777);
   }
#endif
#endif
}

/***********************************************************************
* getmodule - Returns module number for device and sets access and unit.
***********************************************************************/

static int
getmodule (int ch)
{
   int module;
   int unit;
   int i;
   int modulefound;
   uint32 t, mod;

#ifdef USE64
   mod = (mem[channel[ch].car | channel[ch].dcore] >> 12) & 07777;
#else
   mod = (meml[channel[ch].car | channel[ch].dcore] >> 12) & 07777;
#endif
   t = cvtzero (mod);
   module = t & 017;
#ifdef DEBUGDASD
   fprintf (stderr, "getmodule: Channel %c, module = %d\n", ch+'A', module);
   fprintf (stderr, "   mod = %o\n", mod);
#endif

   modulefound = FALSE;
   for (i = DASDOFFSET + ch * 10; i < DASDOFFSET + MAXDASD + ch * 10; i++)
   {
      if ((module >= sysio[i].iomodstart && module < sysio[i].iomodend) &&
          sysio[i].iochntype && sysio[i].iofd)
      {
         modulefound = TRUE;
	 break;
      }
   }
   if (!modulefound)
   {
#ifdef DEBUGDASD
      fprintf (stderr, "   module not found\n");
#endif
      setsnsattn (ch, module);
#ifdef USE64
      channel[ch].csns[0] = SNS_OFFLINE;
#else
      channel[ch].csnsh[0] = 0;
      channel[ch].csnsl[0] = SNS_OFFLINE;
#endif
      channel[ch].cind = CIND_UNEND;
      channel[ch].cflags |= CHAN_INTRPEND;
      return (-1);
   }

   unit = (i - DASDOFFSET) % 10;
   channel[ch].cunit = module;
   channel[ch].caccess = (t >> 6) & 017;
   channel[ch].devices.dasd[module].access = (t >> 6) & 017;
   channel[ch].devices.dasd[module].module = module - sysio[i].iomodstart;
   channel[ch].devices.dasd[module].unit = unit;

#ifdef DEBUGDASD
   fprintf (stderr,
	    "getmodule: access = %d, module = %d(%d), unit = %d, index = %d\n",
	    channel[ch].devices.dasd[module].access,
	    channel[ch].devices.dasd[module].module, module,
	    channel[ch].devices.dasd[module].unit,
	    i);
#endif
   return (module);
}

/***********************************************************************
* gettrack - Returns track number.
***********************************************************************/

static uint32
gettrack (int ch, int module)
{
   uint32 t;
   uint32 coreaddr;

   coreaddr = channel[ch].car | channel[ch].dcore;
#ifdef USE64
   t = ((mem[coreaddr] & 07777) << 12) |
       ((mem[coreaddr+1] >> 24) & 07777);
#else
   t = ((meml[coreaddr] & 07777) << 12) |
	 ((memh[coreaddr+1] & SIGN) << 4) |
	 ((memh[coreaddr+1] & 007) << 8) | 
	 ((meml[coreaddr+1] >> 24) & 00377);
#endif
#ifdef DEBUGDASD
   fprintf (stderr, "gettrack: t = 0%08o\n", t);
#endif
   if ((t & 000770000) == 000130000) /* Check for CE track */
   {
      t &= 000007777;
      t += CETRACK;
   }
   else
      t = cvtdecimal (t);

   channel[ch].devices.dasd[module].track = t;

#ifdef DEBUGDASD
   fprintf (stderr,
	    "gettrack: track = %d\n",
	    channel[ch].devices.dasd[module].track);
#endif

   return (t);
}

/***********************************************************************
* getrecord - Returns record number.
***********************************************************************/

static uint32
getrecord (int ch, int module)
{
   uint32 s, ds;
   uint32 t, dt;
   uint32 coreaddr;

   coreaddr = channel[ch].car | channel[ch].dcore;
#ifdef USE64
   t = ((mem[coreaddr] & 07777) << 12) |
       ((mem[coreaddr+1] >> 24) & 07777);
   s = (mem[coreaddr+1] >> 12) & 07777;
#else
   t = ((meml[coreaddr] & 07777) << 12) |
	 ((memh[coreaddr+1] & SIGN) << 4) |
	 ((memh[coreaddr+1] & 007) << 8) | 
	 ((meml[coreaddr+1] >> 24) & 00377);
   s = ((meml[coreaddr+1] >> 12) & 07777);
#endif
#ifdef DEBUGDASD
   fprintf (stderr, "getrecord: t = 0%o, s = 0%o\n", t, s);
#endif
   dt = cvtdecimal (t);
   t = cvtoctal (t);
   ds = cvtdecimal (s);
   s = cvtoctal (s);
#ifdef DEBUGDASD
   fprintf (stderr, "    t = 0%08o, s = 0%04o\n", t, s);
   fprintf (stderr, "    dt = %d, ds = %d\n", dt, ds);
#endif
   channel[ch].devices.dasd[module].record = t << 8 | s;
   dt = dt * 100 + ds;

#ifdef DEBUGDASD
   fprintf (stderr,
	    "getrecord: record = 0%o, drec = %d\n",
	    channel[ch].devices.dasd[module].record, dt);
#endif
   channel[ch].devices.dasd[module].record = dt; //ADD//
   return (dt);
}

/***********************************************************************
* getha2 - Returns Home address field.
***********************************************************************/

static uint32
getha2 (int ch, int module)
{
   uint32 s;
   uint32 coreaddr;

   coreaddr = channel[ch].car | channel[ch].dcore;
#ifdef USE64
   s = (mem[coreaddr+1] >> 12) & 07777;
#else
   s = ((meml[coreaddr+1] >> 12) & 07777);
#endif
   s = cvtoctal (s);
   channel[ch].devices.dasd[module].ha2id = s;

#ifdef DEBUGDASD
   fprintf (stderr,
	    "getha2: ha2id = %04o\n",
	    channel[ch].devices.dasd[module].ha2id);
#endif
   return (s);
}

/***********************************************************************
* getcmd - Returns DASD command.
***********************************************************************/

static uint32
getcmd (int ch)
{
   uint32 t;
   uint32 coreaddr;
#ifdef DEBUGDASD
#ifdef USE64
   t_uint64 w;
#else
   uint8 wh;
   uint32 wl;
#endif
#endif

   coreaddr = channel[ch].car | channel[ch].dcore;
#ifdef USE64
   t = (mem[coreaddr] >> 24) & 07777;
#else
   t = ((memh[coreaddr] & 017) << 8) | 
	 ((meml[coreaddr] >> 24) & 00377);
#endif
   t = cvtdecimal (t & 01717);

#ifdef DEBUGDASD
   fprintf (stderr,
	    "getcmd: cmd = %d\n", t);
#ifdef USE64
   w = mem[coreaddr];
   fprintf (stderr, "   w0 = %012llo\n", w);
   w = mem[coreaddr+1];
   fprintf (stderr, "   w1 = %012llo\n", w);
#else
   wh = memh[coreaddr];
   wl = meml[coreaddr];
   fprintf (stderr, "   w0 = %02o%010lo\n",
	 ((wh & SIGN) >> 2) | ((wh & HMSK) << 2) | (uint16)(wl >> 30),
	 (unsigned long)wl & 07777777777);
   wh = memh[coreaddr+1];
   wl = meml[coreaddr+1];
   fprintf (stderr, "   w1 = %02o%010lo\n",
	 ((wh & SIGN) >> 2) | ((wh & HMSK) << 2) | (uint16)(wl >> 30),
	 (unsigned long)wl & 07777777777);
#endif
#endif

   return (t);
}

/***********************************************************************
* chkformat - Check if doing format; if so, convert buffer to format data.
*    Liberally borrowed from Richard Cornwell ;)
***********************************************************************/

static int
chkformat (int ch)
{
   DASD_t *pdasd;
   IO_t   *pio;
   uint8  *pbuf;
   uint8  *fbuf;
   int    i;
   int    j;
   int    out;
   uint16 unit;
   uint16 dev;
   uint8  chr;
   uint8  fmt[MAXTRACKLEN];

   unit = channel[ch].cunit;
   pdasd = &channel[ch].devices.dasd[unit];

   if (!(pdasd->flags & DASD_INFORMAT)) return (0);
   
   unit = pdasd->unit;
   pbuf = pdasd->dbuf;
   fbuf = pdasd->fbuf;
   dev = unit + DASDOFFSET + 10*ch;
   pio = &sysio[dev];

#ifdef DEBUGDASDFMT
   fprintf (stderr, "chkformat: Channel %c\n", ch+'A');
#endif

   /*
   ** Scan over new format 
   ** Convert format specification in pbuf 
   */

   out = 0;

   /* Skip initial gap */
   for (i = 4; i < MAXTRACKLEN && (pbuf[i] & 077) == 04; i++);
   if (i == MAXTRACKLEN) return (-1);	/* Failed if we hit end */
   /* HA1 Gap */
   for (j = i; i < MAXTRACKLEN && pbuf[i] == 03; i++);
   if ((i - j) > 12) return (-1);	/* HA1 too big */
   if (pbuf[i++] != 04) return (-1);	/* Not gap */
   for (j = i; i < MAXTRACKLEN && pbuf[i] == 03; i++);
   if (i == MAXTRACKLEN) return (-1);	/* Failed if we hit end */
   if (pbuf[i++] != 04) return (-1);	/* Not gap */
   /* Size up HA2 gap */
   for ( j = i; i < MAXTRACKLEN && (pbuf[i] == 03 || pbuf[i] == 01);i++);
   j = i - j;
   if (j < 6) return (-1);
   j -= pio->iooverhead;	/* Remove overhead */
#ifdef DEBUGDASDFMT
   fprintf (stderr, "   HA2 len = %d\n", j);
#endif
   for (; j > 0; j--) fmt[out++] = FMT_HA2;

   /* Now grab records */
   while (i < MAXTRACKLEN)
   {
      chr = pbuf[i++];
      if (chr == 0x80) break;			/* End of record */
      if (chr != 04 && chr != 02) return (-1);	/* Not a gap */
      for (j = i; i < MAXTRACKLEN && pbuf[i] == chr; i++);
      chr = pbuf[i];		       /* Should be RA */
					       /* Gap not long enough or eor */
      if (chr == 0x80 || (i - j) < 11) break;	
      /* Size up RA gap */
      if (chr != 01 && chr != 03) return (-1);	/* Not header */
      for (j = i; i < MAXTRACKLEN && pbuf[i] == chr; i++);
      j = i - j;
      if (j < 10) return (-1);
      j -= pio->iooverhead;	/* Remove overhead */
#ifdef DEBUGDASDFMT
      fprintf (stderr, "   RECNUM len = %d\n", j);
#endif
      for (; j > 0; j--) fmt[out++] = FMT_HDR;
      chr = pbuf[i++];
      if (chr != 04 && chr != 02) return (-1);	/* End of RA Field */
      chr = pbuf[i];
      if (chr != 01 && chr != 03) return (-1);	/* Not gap */
      for (j = i; i < MAXTRACKLEN && pbuf[i] == chr; i++);
      if ((i - j) < 10) return (-1);	/* Gap not large enough */
      chr = pbuf[i++];
      if (chr != 04 && chr != 02) return (-1);	/* End of RA Gap */
      chr = pbuf[i];		/* Should be RA */
      if (chr != 01 && chr != 03) return (-1);	/* Not Record data */
      for (j = i; i < MAXTRACKLEN && pbuf[i] == chr; i++);
      j = i - j;
      if (j < 10) return (-1);
      j -= pio->iooverhead;	/* Remove overhead */
#ifdef DEBUGDASDFMT
      fprintf (stderr, "   DATA len = %d (%d words)\n", j, j/6);
#endif
      for (; j > 0; j--) fmt[out++] = FMT_DATA;
   }

   /* Put four END chars to end of buffer */
   for (j = 4; j > 0; j--) fmt[out++] = FMT_END;

   /* Make sure we did not pass size of track */
   if (out > pio->iobyttrk) return (-1);	/* Too big for track */
   
   /* Now grab every four characters and place them in next format location */
   for (j = i = 0; j < out; i++) {
	uint8	temp;
	temp = (fmt[j++] & 03);
	temp |= (fmt[j++] & 03) << 2;
	temp |= (fmt[j++] & 03) << 4;
	temp |= (fmt[j++] & 03) << 6;
	pbuf[i] = temp;
   }
   memcpy (fbuf, pbuf, i);
   memset (&fbuf[i+1], '\0', (MAXTRACKLEN/4) - i - 1);
   memset (&pbuf[i+1], '\0', MAXTRACKLEN - i - 1);

   pdasd->curloc = i;
   return (i);
}

/***********************************************************************
* getfmtchar - Returns a format char from the format track.
***********************************************************************/

uint8
getfmtchar (uint8 *pfmt, uint32 curloc)
{
   uint8 chr;

   chr = (pfmt[curloc/4] >> ((curloc % 4) * 2)) & 03;
#ifdef DEBUGDASDFMT
   if (chr != FMT_DATA)
      fprintf (stderr, "getfmtchar: curloc = %d, chr = %o(%s)\n",
	       curloc, chr, fmtstr[chr]);
#endif
   return (chr);
}

/***********************************************************************
* loadformat - Loads DASD format.
***********************************************************************/

static int
loadformat (int ch, int module, uint16 cyl)
{
   DASD_t *pdasd;
   IO_t   *pio;
   int32  len;
   uint32 dasdloc;
   uint16 unit;
   uint16 dev;
   uint16 acc;
   uint16 mod;

   pdasd = &channel[ch].devices.dasd[module];
#ifdef DEBUGDASD
      fprintf (stderr,
      "loadformat: Channel %c, module = %d, cyl = %d, fcyl = %d, fmod = %d\n",
	       ch+'A', module, cyl, pdasd->fcyl, pdasd->fmod);
#endif
   if (pdasd->fcyl == cyl && pdasd->fmod == module)
      return (0);

   unit = pdasd->unit;
   dev = unit + DASDOFFSET + 10*ch;
   pio = &sysio[dev];
   acc = pdasd->access;
   mod = pdasd->module;

   dasdloc = (pio->iobyttrk * pio->ioheads *
	   ((acc * pio->iocyls) + (mod * (pio->iocyls*pio->ioaccess)) + cyl))
	      + DASDOVERHEAD;
   
#ifdef DEBUGDASD
   fprintf (stderr,
	    "   loading format: dasdloc = %d\n",
	    dasdloc);
#endif

   if (fseek (pio->iofd, dasdloc, SEEK_SET) < 0)
   {
      sprintf (errview[0],
	    "loadformat: I/O check: seek: Channel %c, cop = %o, cunit = %o\n",
	    ch+'A', channel[ch].cop & 07, channel[ch].cunit);
      cpuflags |= CPU_IOCHK;
      run = CPU_STOP;
      return (-1);
   }
   len = pio->iobyttrk/4;
   if (fread (pdasd->fbuf, 1, len, pio->iofd) != len)
   {
      sprintf (errview[0],
	    "loadformat: I/O check: read: Channel %c, cop = %o, cunit = %o\n",
	    ch+'A', channel[ch].cop & 07, channel[ch].cunit);
      cpuflags |= CPU_IOCHK;
      run = CPU_STOP;
      return (-1);
   }

#ifdef DEBUG7909DATA1
   if (ch == DEBUGCHAN)
   {
      int iii, jjj, kkk;
      jjj = len;
      for (iii = 0; iii < jjj; )
      {
	 /*if (jjj > 80) jjj = 80;*/
	 fprintf (stderr, "   ");
	 fprintf (stderr, "%05o: ", iii);
	 for (kkk = 0; kkk < 12; kkk++)
	    fprintf (stderr, "%03o ", pdasd->fbuf[iii+kkk]);
	 fputs ("   ", stderr);
	 for (kkk = 0; kkk < 12; kkk++)
	    fputc (tonative[pdasd->fbuf[iii+kkk] & 077], stderr);
	 iii += 12;
	 fputc ('\n', stderr);
      }
      fputc ('\n', stderr);
   }
#endif

   pdasd->fcyl = cyl;
   pdasd->fmod = module;

   return (0);
}

/***********************************************************************
* dasdseek - Seeks to location on DASD.
***********************************************************************/

static int
dasdseek (int ch, int module, uint32 track, int fmt, int reseek)
{
   DASD_t *pdasd;
   IO_t   *pio;
   uint32 dasdloc;
   uint16 unit;
   uint16 head;
   uint16 cyl;
   uint16 dev;
   uint16 acc;
   uint16 mod;

   pdasd = &channel[ch].devices.dasd[module];
   unit = pdasd->unit;
   dev = unit + DASDOFFSET + 10*ch;
   pio = &sysio[dev];

   acc = pdasd->access;
   mod = pdasd->module;
#ifdef DEBUGDASD
   fprintf (stderr,
      "dasdseek: Channel %c, module = %d, track = %d, fmt = %d, reseek = %d\n",
            ch+'A', module, track, fmt, reseek);
#endif

   if (!reseek)
   {
      if (pio->ioflags & IO_DISK)
      {
	 uint16 heads = pio->ioheads - 1;

#ifdef DEBUGDASD
         fprintf (stderr, "   disk: heads = %d\n", heads);
#endif
	 cyl = track / heads;

	 if (fmt)
	    head = 0;
	 else
	    head = (track % heads) + 1;
      }
      else
      {
#ifdef DEBUGDASD
         fprintf (stderr, "   drum: \n");
#endif
	 cyl = 0;
	 if (fmt)
	    head = 0;
	 else
	    head = track + 1;
      }

      if (!fmt)
	 loadformat (ch, module, cyl);

      dasdloc = (pio->iobyttrk * head) +
		(pio->iobyttrk * pio->ioheads *
	      ((acc * pio->iocyls) + (mod * (pio->iocyls*pio->ioaccess)) + cyl))
		 + DASDOVERHEAD;
   }
   else
   {
      cyl = 0;
      head = 0;
      dasdloc = pio->iopos;
   }

#ifdef DEBUGDASD
   fprintf (stderr,
	   "   dev = %d, acc = %d, mod = %d, cyl = %d, head = %d, unit = %d\n",
	    dev, acc, mod, cyl, head, unit);
   fprintf (stderr, "   dasdloc = %d\n", dasdloc);
#endif
   if (fseek (pio->iofd, dasdloc, SEEK_SET) < 0)
   {
      sprintf (errview[0],
	    "dasdseek: I/O check: seek: Channel %c, cop = %o, cunit = %o\n",
	    ch+'A', channel[ch].cop & 07, channel[ch].cunit);
      cpuflags |= CPU_IOCHK;
      run = CPU_STOP;
      return (-1);
   }
   pio->iopos = dasdloc;
   return (cyl);
}

/***********************************************************************
* readword - Reads a word from the specified device.
***********************************************************************/

static int
readword (int ch)
{
   DASD_t *pdasd;
   uint8  *pbuf;
   uint8  *pfmt;
   int    module;
   int    found;
   int    readok;
   int32  i, j;
   uint32 recnum;
   uint16 unit;
   uint16 dev;
   uint8  dchr;

#ifdef USE64
   channel[ch].cdr = 0;
#else
   channel[ch].cdrl = 0;
   channel[ch].cdrh = 0;
#endif
   channel[ch].cflags &= ~(CHAN_EOF | CHAN_EOR);

   module = channel[ch].cunit;
   pdasd = &channel[ch].devices.dasd[module];
   unit = pdasd->unit;
   dev = unit + DASDOFFSET + 10*ch;
   pbuf = pdasd->dbuf;
   pfmt = pdasd->fbuf;

#ifdef DEBUGDASD1
   fprintf (stderr,
	    "readword: Channel %c, module = %d, state = %d, pos = %10ld\n",
	    ch+'A', module, pdasd->state, sysio[dev].iopos);
#endif

NEXT_TRACK:
   readok = FALSE;
   if (pdasd->state == DASDIDLE)
   {
      long startpos;

      startpos = sysio[dev].iopos;
      sysio[dev].iochn = ch;
      if ((pdasd->reclen = readrec (dev, pbuf, sysio[dev].iobyttrk)) == -1)
         return (-1);
      if (dasdseek (ch, module, 0, 0, 1) < 0)
         return (-1);

#if defined(DEBUG7909) || defined(DEBUG7909DATA)
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "%d: %05o: ", inst_count, ic);
	 fprintf (stderr,
		  "read9  %s  %05d %03o %03o %03o %03o %03o %03o %10ld\n",
		  devstr (dev), pdasd->reclen,
		  pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
		  startpos);
      }

#endif
#ifdef DEBUG7909DATA
      if (ch == DEBUGCHAN)
      {
	    int iii, jjj, kkk;
	    jjj = pdasd->reclen;
	    for (iii = 0; iii < jjj; )
	    {
	       /*if (jjj > 80) jjj = 80;*/
	       fprintf (stderr, "   ");
	       fprintf (stderr, "%05o: ", iii);
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

      pdasd->curloc = 0;
      if (!(pdasd->flags & DASD_INFORMAT))
      {
	 switch (channel[ch].cdcmd)
	 {
         case DVCY:
	 case DVTN:

	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HA2) pdasd->curloc++;
	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HDR) pdasd->curloc++;
	    break;

	 case DVTA:

	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HA2) pdasd->curloc++;
	    break;
	    
	 case DVSR:

	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HA2) pdasd->curloc++;

	    recnum = 0;
	    found = FALSE;
	    while (!found)
	    {
	       uint32 drec;
	       uint8 chr;

	       drec = 0;
	       while (getfmtchar (pfmt, pdasd->curloc) == FMT_HDR)
	       {
		  if ((chr = pbuf[pdasd->curloc++] & 017) == 012) chr = 0;
		  drec = drec * 10 + chr;
		  recnum = (recnum << 4) | chr;
#ifdef DEBUGDASDREC
		  fprintf (stderr, "   chr = 0%o\n", chr);
#endif
	       }
#ifdef DEBUGDASDREC
	       fprintf (stderr, "   recnum = 0%o, drec = %d\n", recnum, drec);
	       fprintf (stderr, "   record = %d\n", pdasd->record);
#endif
	       if (drec == pdasd->record)
	          found = TRUE;
	       else
	       {
	          while ((chr = getfmtchar (pfmt, pdasd->curloc)) == FMT_DATA)
		     pdasd->curloc++;
	          if (chr == FMT_END)
		  {
#ifdef USE64
		     channel[ch].csns[0] = SNS_NOREC;
#else
		     channel[ch].csnsh[0] = SNS_PGMCHK;
		     channel[ch].csnsl[0] = SNS_NOREC;
#endif
                     channel[ch].cind = CIND_UNEND;
		     channel[ch].cflags |= CHAN_INTRPEND;
#ifdef DEBUGDASD
		     fprintf (stderr, "   No Record Found\n");
#endif
		     return (-1);
		  }
	       }
	    }
	    break;

	 default: ;
	 }
      }
      pdasd->state = DASDREAD;
      readok = TRUE;
   }

   else
   {
      dchr = getfmtchar (pfmt, pdasd->curloc);

      switch (channel[ch].cdcmd)
      {
      case DVTA:
	 if (dchr == FMT_END)
            channel[ch].cflags |= CHAN_EOR;
	 else
	    readok = TRUE;
	 break;

      case DVTN:
	 if (dchr == FMT_END)
            channel[ch].cflags |= CHAN_EOR;
	 else 
	 {
	    if (dchr == FMT_HDR)
	       while (getfmtchar (pfmt, pdasd->curloc) == FMT_HDR)
		  pdasd->curloc++;
	    readok = TRUE;
	 }
	 break;

      case DVSR:
	 if (dchr == FMT_END || dchr == FMT_HDR)
            channel[ch].cflags |= CHAN_EOR;
	 else
	    readok = TRUE;
	 break;

      case DVCY:
	 while (dchr != FMT_DATA)
	 {
	    if (dchr == FMT_END)
	    {
	       pdasd->track++;
	       if (dasdseek (ch, module, pdasd->track, 0, 0) < 0)
		  return (-1);
	       pdasd->state = DASDIDLE;
	       goto NEXT_TRACK;
	    }
	    pdasd->curloc++;
	    dchr = getfmtchar (pfmt, pdasd->curloc);
	 }
         readok = TRUE;
         break;

      default: 
         readok = TRUE;
      }
   }

   if (readok)
   {
      for (i = 0; i < 6; i++)
      {
#ifdef USE64
	 channel[ch].cdr = (channel[ch].cdr & CHANASYMASK) << 6;
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
   }

   return (0);
}

/***********************************************************************
* writeword - Writes a word to the specified device.
***********************************************************************/

static int
writeword (int ch)
{
   DASD_t *pdasd;
   uint8  *pbuf;
   uint8  *pfmt;
   int    module;
   int    found;
   int    writeok;
   int32  i;
   uint32 recnum;
   uint16 unit;
   uint16 dev;
   uint8  dchr;
   uint8  dstrt;

   module = channel[ch].cunit;
   pdasd = &channel[ch].devices.dasd[module];
   unit = pdasd->unit;

   channel[ch].cflags &= ~(CHAN_EOF | CHAN_EOR);

   dev = unit + DASDOFFSET + 10*ch;
   pbuf = pdasd->dbuf;
   pfmt = pdasd->fbuf;

#ifdef DEBUGDASD1
   fprintf (stderr,
     "writeword: Channel %c, module = %d, curloc = %d, state = %d, pos = %ld\n",
	    ch+'A', module, pdasd->curloc, pdasd->state, sysio[dev].iopos);
#endif

NEXT_TRACK:
   writeok = FALSE;
   dstrt = 0;
   if (pdasd->state == DASDIDLE)
   {
      pdasd->curloc = 0;
      if (!(pdasd->flags & DASD_INFORMAT))
      {
	 uint32 drec;
	 uint8 chr;

	 if ((pdasd->reclen = readrec (dev, pbuf, sysio[dev].iobyttrk)) == -1)
	    return (-1);
	 if (dasdseek (ch, module, 0, 0, 1) < 0)
	    return (-1);

#if defined(DEBUG7909)
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr,
		     "write9R  %s  %05d %03o %03o %03o %03o %03o %03o %10ld\n",
		     devstr (dev), pdasd->reclen,
		     pbuf[0], pbuf[1], pbuf[2], pbuf[3], pbuf[4], pbuf[5],
		     sysio[dev].iopos);

#ifdef DEBUG7909DATA
	    {
	       int iii, jjj, kkk;
	       jjj = pdasd->reclen;
	       for (iii = 0; iii < jjj; )
	       {
		  /*if (jjj > 80) jjj = 80;*/
		  fprintf (stderr, "   ");
		  fprintf (stderr, "%05o: ", iii);
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

	 switch (channel[ch].cdcmd)
	 {
	 case DVCY:
	 case DVTN:

	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HA2) pdasd->curloc++;
	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HDR) pdasd->curloc++;
	    break;

	 case DVTA:

	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HA2) pdasd->curloc++;
	    break;
	    
	 case DVSR:

	    while (getfmtchar (pfmt, pdasd->curloc) == FMT_HA2) pdasd->curloc++;

	    found = FALSE;
	    while (!found)
	    {

	       recnum = 0;
	       drec = 0;
	       while (getfmtchar (pfmt, pdasd->curloc) == FMT_HDR)
	       {
		  if ((chr = pbuf[pdasd->curloc++] & 017) == 012) chr = 0;
		  recnum = (recnum << 4) | chr;
		  drec = drec * 10 + chr;

#ifdef DEBUGDASDREC
		  fprintf (stderr, "   chr = 0%o\n", chr);
#endif
	       }
#ifdef DEBUGDASDREC
	       fprintf (stderr, "   recnum = 0%o, drec = %d\n", recnum, drec);
	       fprintf (stderr, "   record = %d\n", pdasd->record);
#endif
	       if (drec == pdasd->record)
	          found = TRUE;
	       else
	       {
	          while ((chr = getfmtchar (pfmt, pdasd->curloc)) == FMT_DATA)
		     pdasd->curloc++;
	          if (chr == FMT_END)
		  {
#ifdef USE64
		     channel[ch].csns[0] = SNS_NOREC;
#else
		     channel[ch].csnsh[0] = SNS_PGMCHK;
		     channel[ch].csnsl[0] = SNS_NOREC;
#endif
                     channel[ch].cind = CIND_UNEND;
		     channel[ch].cflags |= CHAN_INTRPEND;
#ifdef DEBUGDASD
		     fprintf (stderr, "   No Record Found\n");
#endif
		     return (-1);
		  }
	       }
	    }
	    break;

	 default: ;
	 }
      }
      pdasd->state = DASDWRITE;
      writeok = TRUE;
   }
   
   else
   {
      if (!(pdasd->flags & DASD_INFORMAT))
      {
	 dchr = getfmtchar (pfmt, pdasd->curloc);

	 switch (channel[ch].cdcmd)
	 {
	 case DVTA:
	    if (dchr == FMT_END)
	       channel[ch].cflags |= CHAN_EOR;
	    else
	       writeok = TRUE;
	    break;

	 case DVTN:
	    if (dchr == FMT_END)
	       channel[ch].cflags |= CHAN_EOR;
	    else 
	    {
	       if (dchr == FMT_HDR)
		  while (getfmtchar (pfmt, pdasd->curloc) == FMT_HDR)
		     pdasd->curloc++;
	       writeok = TRUE;
	    }
	    break;

	 case DVSR:
	    if (dchr == FMT_END || dchr == FMT_HDR)
	       channel[ch].cflags |= CHAN_EOR;
	    else
	       writeok = TRUE;
	    break;

         case DVCY:
	    while (dchr != FMT_DATA)
	    {
	       if (dchr == FMT_END)
	       {
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
		  writerec (dev, pbuf, sysio[dev].iobyttrk);

		  pdasd->track++;
		  if (dasdseek (ch, module, pdasd->track, 0, 0) < 0)
		     return (-1);
		  pdasd->state = DASDIDLE;
		  goto NEXT_TRACK;
	       }
	       pdasd->curloc++;
	       dchr = getfmtchar (pfmt, pdasd->curloc);
	    }
            writeok = TRUE;
            break;

	 default: 
	    writeok = TRUE;
	 }
      }
      else
         writeok = TRUE;
   }

   if (writeok)
   {

#ifdef USE64
      dchr = (channel[ch].cdr >> 30) & 077;
#else
      dchr = ((channel[ch].cdrh & SIGN) >> 2) |
	     ((channel[ch].cdrh & HMSK) << 2) |
	     ((channel[ch].cdrl >> 30) & 03);
#endif

#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   curloc = %05o(%d), data = %03o ",
		  pdasd->curloc, pdasd->curloc, dchr & 077);
      }
#endif
      pbuf[pdasd->curloc++] = (dchr & 077) | dstrt;

      for (i = 1; i < 6; i++)
      {
#ifdef USE64
	 dchr = ((channel[ch].cdr >> 24) & 077);
#else
	 dchr = ((channel[ch].cdrl >> 24) & 077);
#endif
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "%03o ", dchr & 077);
	 }
#endif
	 pbuf[pdasd->curloc++] = dchr & 077;
#ifdef USE64
	 channel[ch].cdr <<= 6;
#else
	 channel[ch].cdrl <<= 6;
#endif
      }
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "\n");
      }
#endif
   }

   return (0);
}

/***********************************************************************
* godasd - Process DASD orders.
***********************************************************************/

static void
godasd (int ch, uint32 cmd)
{
   DASD_t *pdasd;
   int module;
   int cyl;
   uint32 track;
   uint32 record;
   uint32 ha2;

   channel[ch].cind &= ~CIND_INTMASK;
   switch (cmd)
   {
   case DNOP:  /* 0 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DNOP-%c\n", chsel[channel[ch].csel]);
#endif
      break;

   case DREL: /* 4 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DREL-%c\n", chsel[channel[ch].csel]);
#endif
      break;

   case DEBM: /* 8 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DEBM-%c\n", chsel[channel[ch].csel]);
#endif
      channel[ch].cflags |= CHAN_EIGHTBIT;
#ifdef USE64
      channel[ch].csns[0] &= ~SNS_SIXBIT;
#else
      channel[ch].csnsl[0] &= ~SNS_SIXBIT;
#endif
      break;

   case DSBM: /* 9 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DSBM-%c\n", chsel[channel[ch].csel]);
#endif
      channel[ch].cflags &= ~CHAN_EIGHTBIT;
#ifdef USE64
      channel[ch].csns[0] |= SNS_SIXBIT;
#else
      channel[ch].csnsl[0] |= SNS_SIXBIT;
#endif
      break;

   case DSEK: /* 80 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DSEK-%c\n", chsel[channel[ch].csel]);
#endif
      channel[ch].ccyc = 200;
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      track = gettrack (ch, module);
      if ((cyl = dasdseek (ch, module, track, 0, 0)) < 0)
         return;
      channel[ch].cind |= CIND_ATTN1;
#ifdef DEBUGDASD
      fprintf (stderr, "   cind = %o\n", channel[ch].cind);
#endif
      setsnsattn (ch, module);
      pdasd->flags = 0;
      break;

   case DVSR: /* 82 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DVSR-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      record = getrecord (ch, module);
      pdasd->flags = 0;
      if ((cyl = dasdseek (ch, module, 0, 0, 1)) < 0)
         return;
      break;

   case DWRF: /* 83 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DWRF-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      track = gettrack (ch, module);
      if ((cyl = dasdseek (ch, module, track, 1, 0)) < 0)
         return;
      pdasd->fcyl = cyl;
      pdasd->flags = DASD_INFORMAT;
      break;

   case DVTN: /* 84 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DVTN-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      track = gettrack (ch, module);
      ha2 = getha2 (ch, module);
      if ((cyl = dasdseek (ch, module, track, 0, 0)) < 0)
         return;
      pdasd->flags = 0;
      break;

   case DVCY: /* 85 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DVCY-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      track = gettrack (ch, module);
      ha2 = getha2 (ch, module);
      if ((cyl = dasdseek (ch, module, track, 0, 0)) < 0)
         return;
      pdasd->flags = 0;
      break;

   case DWRC: /* 86 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DWRC-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      record = getrecord (ch, module);
      pdasd->flags = 0;
      if ((cyl = dasdseek (ch, module, 0, 0, 1)) < 0)
         return;
      break;

   case DSAI: /* 87 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DSAI-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      pdasd->flags = 0;
      break;

   case DVTA: /* 88 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DVTA-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      track = gettrack (ch, module);
      ha2 = getha2 (ch, module);
      if ((cyl = dasdseek (ch, module, track, 0, 0)) < 0)
         return;
      pdasd->flags = 0;
      break;

   case DVHA: /* 89 */
#ifdef DEBUGDASD
      fprintf (stderr, "godasd: cmd = DVHA-%c\n", chsel[channel[ch].csel]);
#endif
      if ((module = getmodule (ch)) < 0)
         return;
      pdasd = &channel[ch].devices.dasd[module];
      track = gettrack (ch, module);
      if ((cyl = dasdseek (ch, module, track, 0, 0)) < 0)
         return;
      pdasd->flags = 0;
      break;

   default: ;
   }
}

/***********************************************************************
* rw_7909 - Process 7909 commands.
***********************************************************************/

static void
rw_7909 (int ch)
{
   uint32 cmd;
   uint32 coreaddr;
   uint8  cnt;
   uint8  msk;
   uint8  chr;
   uint8  rev;

   channel[ch].ccyc = 10;
   coreaddr = channel[ch].car | channel[ch].dcore;

#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "rw_7909:  op = %s(%02o) cwr = %o, car = %o, clr = %o\n",
	       opstr[channel[ch].cop],
	       channel[ch].cop, channel[ch].cwr, channel[ch].car,
	       channel[ch].clr);
      fprintf (stderr, "   core = %c, car = %o\n",
	       channel[ch].dcore ? 'B' : 'A', coreaddr);
   }
#endif

   if (!channel[ch].csel)
      channel[ch].csel = CHAN_SEL;

   switch (channel[ch].cop)
   {

   case 016: /* TWT */
   case 030:
      channel[ch].clr --;
      chan_in_op &= ~(1 << ch);
      channel[ch].cact = CHAN_WAIT;
      channel[ch].cflags |= CHAN_INWAIT;
      channel[ch].csms |= SMS_INHIBINT;
      return;

   case 000: /* WTR */
   case 002:
      channel[ch].clr --;
      if (!(channel[ch].csms & (SMS_INHIBINT|SMS_INHIBAT1)) &&
	   (channel[ch].cind & CIND_INTMASK) ) 
      {
	 int intadr;

	 intadr = 042 + (ch << 1);
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr,
		  "rw_7909: Channel %c interrupt: cind = %04o, intadr = %o\n",
		  ch+'A', (channel[ch].cind & CIND_INTMASK), intadr);
	 }
#endif
	 
#ifdef USE64
	 STOREA (intadr, channel[ch].car << 18 |
	 		 channel[ch].clr | channel[ch].ccore);
#else
	 STOREA (intadr, (channel[ch].car >> 14) & 01, 
		 channel[ch].car << 18 | channel[ch].clr | channel[ch].ccore);
#endif
	 channel[ch].csms |= SMS_INHIBINT;
	 channel[ch].clr = intadr + 1;
	 channel[ch].ccore = 0;
      }
      else
      {
	 chan_in_op &= ~(1 << ch);
	 channel[ch].csel = NOT_SEL;
	 channel[ch].cact = CHAN_IDLE;
	 channel[ch].cflags |= CHAN_INWAIT;
	 return;
      }
      break;

   case 001: /* XMT */
   case 003:
      while (channel[ch].cwr)
      {
#ifdef USE64
	 mem[coreaddr] = mem[channel[ch].clr | channel[ch].dcore];
#else
	 memh[coreaddr] = memh[channel[ch].clr | channel[ch].dcore];
	 meml[coreaddr] = meml[channel[ch].clr | channel[ch].dcore];
#endif
	 channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	 channel[ch].clr = (channel[ch].clr + 1) & MEMLIMIT;
         channel[ch].cwr--;
      }
      break;

   case 004: /* TCH */
   case 006:
      channel[ch].clr = channel[ch].car;
      channel[ch].ccore = channel[ch].dcore;
      break;

   case 005: /* LIPT */
   case 007:
      channel[ch].csms &= ~SMS_INHIBINT;
      channel[ch].cind &= ~CIND_INTMASK;
      channel[ch].cflags &= ~CHAN_INTRPEND;
      channel[ch].clr = channel[ch].car;
      break;

   case 010: /* CTL */
      channel[ch].cflags &= ~CHAN_TRAPPEND;
      channel[ch].csel = CMD_SEL;
      if (channel[ch].ctype == CHAN_7750)
      {
	 cmd = COMM_CTL;
	 channel[ch].cdcmd = 0;
	 commgo (ch, cmd);
      }
      else
      {
	 cmd = getcmd (ch);
	 channel[ch].clcmd = channel[ch].cdcmd;
	 channel[ch].cdcmd = cmd;
	 godasd (ch, cmd);
      }
      break;

   case 011: /* CTLR */
      channel[ch].cflags &= ~CHAN_TRAPPEND;
      channel[ch].csel = READ_SEL;
      if (channel[ch].ctype == CHAN_7750)
      {
	 cmd = COMM_READ;
	 channel[ch].cdcmd = 0;
         commgo (ch, cmd);
      }
      else
      {
	 cmd = getcmd (ch);
	 channel[ch].clcmd = channel[ch].cdcmd;
	 channel[ch].cdcmd = cmd;
	 godasd (ch, cmd);
      }
      break;

   case 012: /* CTLW */
      channel[ch].cflags &= ~CHAN_TRAPPEND;
      channel[ch].csel = WRITE_SEL;
      if (channel[ch].ctype == CHAN_7750)
      {
	 cmd = COMM_WRITE;
	 channel[ch].cdcmd = 0;
         commgo (ch, cmd);
      }
      else
      {
	 cmd = getcmd (ch);
	 channel[ch].clcmd = channel[ch].cdcmd;
	 channel[ch].cdcmd = cmd;
	 godasd (ch, cmd);
      }
      break;

   case 013: /* SNS */
      channel[ch].csel = SNS_SEL;
      snsndx = 0;
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
#ifdef USE64
	 fprintf (stderr, "   sns[0] = %012llo\n", channel[ch].csns[0]);
	 fprintf (stderr, "   sns[1] = %012llo\n", channel[ch].csns[1]);
#else
	 fprintf (stderr, "   sns[0] = %02o%010lo \n",
		  ((channel[ch].csnsh[0] & SIGN) >> 2) |
		  ((channel[ch].csnsh[0] & HMSK) << 2) |
		  (uint16)(channel[ch].csnsl[0] >> 30),
		  (unsigned long)channel[ch].csnsl[0] & 07777777777);
	 fprintf (stderr, "   sns[1] = %02o%010lo \n",
		  ((channel[ch].csnsh[1] & SIGN) >> 2) |
		  ((channel[ch].csnsh[1] & HMSK) << 2) |
		  (uint16)(channel[ch].csnsl[1] >> 30),
		  (unsigned long)channel[ch].csnsl[1] & 07777777777);
#endif
      }
#endif
      break;

   case 014: /* LAR */
#ifdef USE64
      channel[ch].casr = mem[coreaddr];
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   casr = %012llo = mem[%o]\n",
		  channel[ch].casr, channel[ch].car);
      }
#endif
#else
      channel[ch].casrh = memh[coreaddr];
      channel[ch].casrl = meml[coreaddr];
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   casr = %02o%010lo = mem[%o]\n",
	       ((channel[ch].casrh & SIGN) >> 2) |
	       ((channel[ch].casrh & HMSK) << 2) |
	       (uint16)(channel[ch].casrl >> 30),
	       (unsigned long)channel[ch].casrl & 07777777777,
	       channel[ch].car);
      }
#endif
#endif
      break;

   case 015: /* SAR */
#ifdef USE64
      mem[coreaddr] = channel[ch].casr;
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   mem[%o] = %012llo\n",
		  channel[ch].car, mem[channel[ch].car]);
      }
#endif
#else
      memh[coreaddr] = channel[ch].casrh;
      meml[coreaddr] = channel[ch].casrl;
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   mem[%o] = %02o%010lo\n",
		  channel[ch].car,
		  ((memh[channel[ch].car] & SIGN) >> 2) |
		  ((memh[channel[ch].car] & HMSK) << 2) |
		  (uint16)(meml[channel[ch].car] >> 30),
		  (unsigned long)meml[channel[ch].car] & 07777777777);
      }
#endif
#endif
      break;

   case 020: /* CPYP */
   case 022:
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
	    int wordstat;
	    if (channel[ch].ctype == CHAN_7750)
	    {
	       wordstat = commread (ch);
	    }
	    else
	    {
	       wordstat = readword (ch);
	    }
	    if (wordstat < 0)
	       break;
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
	 else if (channel[ch].csel == WRITE_SEL)
	 {
	    int wordstat;
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
	    channel[ch].cdrl = meml[coreaddr];
	    channel[ch].cdrh = memh[coreaddr];
#endif
	    if (channel[ch].cwr > 1)
	       channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;

	    if (channel[ch].ctype == CHAN_7750)
	    {
	       wordstat = commwrite (ch);
	    }
	    else if (channel[ch].cdcmd != DWRC)
	    {
	       wordstat = writeword (ch);
	    }
	    if (wordstat < 0)
	       break;
	 }
	 else if (channel[ch].csel == SNS_SEL && snsndx < 2)
	 {
#ifdef USE64
	    mem[coreaddr] = channel[ch].csns[snsndx];
#else
	    memh[coreaddr] = channel[ch].csnsh[snsndx];
	    meml[coreaddr] = channel[ch].csnsl[snsndx];
#endif
	    channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	    if (snsndx == 0)
	    {
#ifdef USE64
	       channel[ch].csns[snsndx] &= SNS_SIXBIT;
#else
	       channel[ch].csnsh[snsndx] = 0;
	       channel[ch].csnsl[snsndx] &= SNS_SIXBIT;
#endif
            }
	    else
	    {
#ifdef USE64
	       channel[ch].csns[snsndx] = 0;
#else
	       channel[ch].csnsh[snsndx] = 0;
	       channel[ch].csnsl[snsndx] = 0;
#endif
	    }
	    snsndx ++;
	 }
	 channel[ch].cwr--;
	 if (channel[ch].cwr)
	    return;
      }
      break;

   case 024: /* CPYD */
   case 026:
      if (channel[ch].cwr)
      {
         if (channel[ch].csel == READ_SEL)
	 {
	    int wordstat;
	    if (channel[ch].ctype == CHAN_7750)
	    {
	       wordstat = commread (ch);
	    }
	    else
	    {
	       wordstat = readword (ch);
	    }
	    if (wordstat < 0)
	       goto CPYD_FAILURE;
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
	 else if (channel[ch].csel == WRITE_SEL)
	 {
	    int wordstat;
#ifdef USE64
            channel[ch].cdr = mem[coreaddr];
#else
	    channel[ch].cdrh = memh[coreaddr];
	    channel[ch].cdrl = meml[coreaddr];
#endif
	    if (channel[ch].cwr > 1)
	       channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	    if (channel[ch].ctype == CHAN_7750)
	    {
	       wordstat = commwrite (ch);
	    }
	    else if (channel[ch].cdcmd != DWRC)
	    {
	       wordstat = writeword (ch);
	    }
	    if (wordstat < 0)
	       goto CPYD_FAILURE;
	 }
	 else if (channel[ch].csel == SNS_SEL && snsndx < 2)
	 {
#ifdef USE64
	    mem[coreaddr] = channel[ch].csns[snsndx];
#else
	    memh[coreaddr] = channel[ch].csnsh[snsndx];
	    meml[coreaddr] = channel[ch].csnsl[snsndx];
#endif
	    channel[ch].car = (channel[ch].car + 1) & MEMLIMIT;
	    if (snsndx == 0)
	    {
#ifdef USE64
	       channel[ch].csns[snsndx] &= SNS_SIXBIT;
#else
	       channel[ch].csnsh[snsndx] = 0;
	       channel[ch].csnsl[snsndx] &= SNS_SIXBIT;
#endif
            }
	    else
	    {
#ifdef USE64
	       channel[ch].csns[snsndx] = 0;
#else
	       channel[ch].csnsh[snsndx] = 0;
	       channel[ch].csnsl[snsndx] = 0;
#endif
	    }
	    snsndx ++;
	 }
	 channel[ch].cwr--;
	 if (channel[ch].cwr)
	    return;
      }

      if (channel[ch].ctype == CHAN_7750)
      {
         commdone (ch);
      }
      else
      {
	 chkformat (ch);
	 endrecord (ch, 0);
      }
   CPYD_FAILURE:
      break;

   case 025: /* TCM */
   case 027:
      msk = channel[ch].cwr & 00077;
      rev = channel[ch].cwr & 00100;
      cnt = (channel[ch].cwr >> 12) & 07;
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "%d: %05o: ", inst_count, ic);
	 fprintf (stderr, "TCM: msk = %o, rev = %o, cnt = %o\n",
		  msk, rev, cnt);
      }
#endif
      if (cnt == 0) 
      {
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "   cind = %o\n", channel[ch].cind >> 4);
	 }
#endif
	 if (rev)
	 {
	    if (((channel[ch].cind >> 4) & msk) == msk)
	       channel[ch].clr = channel[ch].car;
	 }
	 else
	 {
	    if ((channel[ch].cind >> 4) == msk)
	       channel[ch].clr = channel[ch].car;
	 }
      }
      else if (cnt == 7)
      {
         if (msk == 000)
	    channel[ch].clr = channel[ch].car;
      }
      else
      {
#ifdef USE64
	 chr = channel[ch].casr >> ((6 - cnt) * 6);
#else
	 if (cnt == 1)
	    chr = ((channel[ch].casrh & SIGN) >> 2) |
		  ((channel[ch].casrh & HMSK) << 2) |
		  ((channel[ch].casrl >> 30) & 003);
	 else
	    chr = channel[ch].casrl >> ((6 - cnt) * 6);
#endif
         chr &= 077;
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "   chr = %o\n", chr);
	 }
#endif
	 if (rev)
	 {
	    if ((chr & msk) == msk)
	       channel[ch].clr = channel[ch].car;
	 }
	 else
	 {
	    if (chr == msk)
	       channel[ch].clr = channel[ch].car;
	 }
      }
      break;

   case 031: /* LIP */
      channel[ch].csms &= ~SMS_INHIBINT;
      channel[ch].cind &= ~CIND_INTMASK;
      channel[ch].cflags &= ~CHAN_INTRPEND;
#ifdef USE64
      channel[ch].clr = mem[042 + (ch << 1)] &
      		(channel[ch].dcore ? 0177777 : 077777);
      channel[ch].casr = 0;
#else
      channel[ch].clr = meml[042 + (ch << 1)] &
      		(channel[ch].dcore ? 0177777 : 077777);
      channel[ch].casrh = 0;
      channel[ch].casrl = 0;
#endif
      break;

   case 032: /* TDC */
      if (channel[ch].cccr)
      {
         channel[ch].cccr--;
	 channel[ch].clr = channel[ch].car;
      }
      break;

   case 033: /* LCC */
      channel[ch].cccr = channel[ch].car & 077;
      break;

   case 034: /* SMS */
      channel[ch].csms &= SMS_INHIBINT;
      channel[ch].csms = channel[ch].car & SMS_MASK;
      if (!(channel[ch].csms & SMS_INHIBAT1) &&
           (channel[ch].cind & CIND_INTMASK))
      {
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr, "rw_7909: Channel %c SMS interrupt: cind = %04o\n",
		     ch+'A', channel[ch].cind & CIND_INTMASK);
	 }
#endif
	 channel[ch].cflags |= CHAN_INTRPEND;
#ifdef USE64
	 channel[ch].casr = 0;
#else
	 channel[ch].casrh = 0;
	 channel[ch].casrl = 0;
#endif
      }
      else
	 channel[ch].cflags &= ~CHAN_INTRPEND;
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "%d: %05o: ", inst_count, ic);
	 fprintf (stderr, "rw_7909: csms = %04o\n", channel[ch].csms);
      }
#endif
      break;

   case 035: /* ICC */
   case 037:
      cnt = (channel[ch].cwr >> 12) & 07;
      if (cnt == 0)
      {
#ifdef USE64
         channel[ch].casr = channel[ch].csms & 0137;
#else
	 channel[ch].casrh = 0;
         channel[ch].casrl = channel[ch].csms & 0137;
#endif
      }
      else if (cnt > 0 && cnt < 7)
      {
         int sc = (6 - cnt) * 6;
#ifdef USE64
         channel[ch].casr = (channel[ch].casr & ~(((t_uint64)077) << sc)) |
	 	((t_uint64)channel[ch].cccr << sc);
#else
	 if (cnt == 1)
	 {
	    channel[ch].casrh = ((channel[ch].cccr & 040) << 2) |
		  ((channel[ch].cccr >> 2) & 07);
	    channel[ch].casrl = (channel[ch].casrl & ~(03 << sc)) | 
		  ((channel[ch].cccr & 003) << sc);
	 }
	 else
	 {
	    channel[ch].casrl = (channel[ch].casrl & ~(077 << sc)) |
		   (channel[ch].cccr << sc);
	 }
#endif
      }
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
#ifdef USE64
	 fprintf (stderr, "   casr = %012llo\n", channel[ch].casr);
#else
	 fprintf (stderr, "   casr = %02o%010lo\n",
		  ((channel[ch].casrh & SIGN) >> 2) |
		  ((channel[ch].casrh & HMSK) << 2) |
		  (uint16)(channel[ch].casrl >> 30),
		  (unsigned long)channel[ch].casrl & 07777777777);
#endif
      }
#endif
      break;

   default:   /* invalid ops */
      cpuflags |= CPU_MACHCHK;
      run = CPU_STOP;
      channel[ch].cact = CHAN_IDLE;
      channel[ch].csel = NOT_SEL;
      chan_in_op &= ~(1 << ch);
      sprintf (errview[0], "I/O MACHINE check: cop = %o\n",
	       channel[ch].cop);
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "rw_7909: %s", errview[0]);
      }
#endif
      return;
   }

   channel[ch].cact = CHAN_LOAD;
}

/***********************************************************************
* load_cycle_7909 - Load and cycle channel.
***********************************************************************/

static void
load_cycle_7909 (int ch)
{
   int addr;
   int temp;
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
   channel[ch].cop = ((chsr & CHANOPMASK) >> 31) | ((chsr & CHANOPNMASK) >> 16);
   temp = ((chsr & CHANOPMASK) >> 30) | ((chsr & CHANOPNMASK) >> 16);
   channel[ch].cwr = (chsr & DECRMASK) >> 18;
   channel[ch].car = chsr & ADDRMASK;
   channel[ch].dcore = chsr & BCORE;
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "load_cycle_7909: op = %c%012llo\n",
	       (chsr & SIGN)? '-' : ' ',
	       chsr & MAGMASK);
      fprintf (stderr,
"   cop = %s(%02o)%02o, cwr = %05o, car = %05o, clr = %05o, unit = %02o%03o\n",
	       opstr[channel[ch].cop],
	       channel[ch].cop, temp, channel[ch].cwr, channel[ch].car, addr,
	       ch+1, channel[ch].cunit);
      fprintf (stderr, "   core = %c\n", channel[ch].dcore ? 'B' : 'A');
   }
#endif
   /* Check for indirect address */
   if (chsr & IBITMASK)
   {
      channel[ch].car = mem[channel[ch].car | channel[ch].dcore] & ADDRMASK;
      cycle_count++;
   }
#else
   channel[ch].cop = ((chsrh & SIGN) >> 3) | ((chsrh & 07) << 1) |
        ((chsrl & 000000200000) >> 16);
   temp = ((chsrh & SIGN) >> 2) | ((chsrh & 07) << 2) |
        ((chsrl & 000000200000) >> 16);
   channel[ch].cwr = ((chsrh & 1) << 14) | ((chsrl & DECRMASK) >> 18);
   channel[ch].car = chsrl & ADDRMASK ;
   channel[ch].dcore = chsrl & BCORE;
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "load_cycle_7909: op = %c%02o%010lo\n",
	       (chsrh & SIGN)? '-' : ' ',
	       ((chsrh & 017) << 2) | (short)(chsrl >> 30),
	       (unsigned long)chsrl & 07777777777);
      fprintf (stderr,
"   cop = %s(%02o)%02o, cwr = %05o, car = %05o, clr = %05o, unit = %02o%03o\n",
	       opstr[channel[ch].cop],
	       channel[ch].cop, temp, channel[ch].cwr, channel[ch].car, addr,
	       ch+1, channel[ch].cunit);
      fprintf (stderr, "   core = %c\n", channel[ch].dcore ? 'B' : 'A');
   }
#endif
   /* Check for indirect address */
   if (chsrl & IBITMASK)
   {
      channel[ch].car = meml[channel[ch].car | channel[ch].dcore] & ADDRMASK;
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   car = %05o\n", 
		  channel[ch].car);
      }
#endif
      cycle_count++;
   }
#endif

   channel[ch].cact = CHAN_RUN;
   chan_in_op |= 1 << ch;
}

/***********************************************************************
* cycle_7909 - Cycle channel.
***********************************************************************/

static void
cycle_7909 (int ch)
{
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "cycle_7909: Channel %c, cact = %d, csel = %d\n",
	       ch+'A', channel[ch].cact, channel[ch].csel);
   }
#endif
   cycle_count++;
   if (cycle_count >= next_lights)
   {
      lights ();
      next_lights = cycle_count + NEXTLIGHTS;
      next_steal = next_lights;
      check_intr ();
   }

   switch (channel[ch].cact)
   {
   case CHAN_RUN:
      rw_7909 (ch);
      break;

   case CHAN_LOAD:
      load_cycle_7909 (ch);
      break;

   case CHAN_WAIT:
      channel[ch].cact = CHAN_IDLE;
      channel[ch].csel = NOT_SEL;
      channel[ch].cflags |= CHAN_TRAPPEND;
      break;

   default: ;
   }
}

/***********************************************************************
* check_7909 - Check channel for interrupts.
***********************************************************************/

void
check_7909 (int ch)
{
   int addr;

   /*
   ** Interrupts honored when the channel is not running.
   */

   switch (channel[ch].cact)
   {
   case CHAN_IDLE:
   case CHAN_END:
      if (channel[ch].cflags & CHAN_INTRPEND)
      {
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "%d: %05o: ", inst_count, ic);
	    fprintf (stderr, "check_7909: Channel %c, cact = %d, csel = %d\n",
		     ch+'A', channel[ch].cact, channel[ch].csel);
	 }
#endif

	 addr = 042 + (ch << 1);
#ifdef DEBUG7909
	 if (ch == DEBUGCHAN)
	 {
	    fprintf (stderr, "   INTERRUPT: addr = %05o\n", addr);
	 }
#endif
	 if (channel[ch].cact == CHAN_IDLE)
	 {
#ifdef USE64
	    STOREA (addr, channel[ch].car << 18 | channel[ch].clr);
#else
	    STOREA (addr, (channel[ch].car >> 14) & 01, 
		    channel[ch].car << 18 | channel[ch].clr);
#endif
	 }
	 else
	 {
#ifdef USE64
	    STOREA (addr, channel[ch].car << 18 | 
		    ((channel[ch].clr + 1) & ADDRMASK));
#else
	    STOREA (addr, (channel[ch].car >> 14) & 01, 
		    channel[ch].car << 18 |
		    ((channel[ch].clr + 1) & ADDRMASK));
#endif
	 }
	 channel[ch].cflags &= ~CHAN_INWAIT;
	 channel[ch].csms &= ~SMS_INHIBINT;
#ifdef USE64
	 channel[ch].casr = 0;
#else
	 channel[ch].casrh = 0;
	 channel[ch].casrl = 0;
#endif

	 channel[ch].clr = addr + 1;
	 load_cycle_7909 (ch);
	 cycle_7909 (ch);
      }
      break;

   default: ;

   }
}

/***********************************************************************
* start_7909 - Process channel.
***********************************************************************/

void
start_7909 (int ch)
{
   channel[ch].clr = channel[ch].car;
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "%s Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	       channel[ch].ctype ? "STC" : "LCH",
	       'A' + ch, ic-1, trap_enb,
	       channel[ch].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "start_7909: Channel %c, cact = %d, csel = %d\n",
	       ch+'A', channel[ch].cact, channel[ch].csel);
   }
#endif

   channel[ch].cflags &= ~CHAN_INWAIT;
   channel[ch].csms &= ~SMS_INHIBINT;

   load_cycle_7909 (ch);
   cycle_7909 (ch);
}

/***********************************************************************
* active_7909 - Process active channel.
***********************************************************************/

void
active_7909 (int ch)
{
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "active_7909: Channel %c, cact = %d, csel = %d\n",
	       ch+'A', channel[ch].cact, channel[ch].csel);
   }
#endif
   cycle_7909 (ch);
}

/***********************************************************************
* load_7909 - Load channel.
***********************************************************************/

void
load_7909 (int ch)
{
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "%s Channel %c, ic = %05o, trap_enb = %o, eof = %s, drum = %s\n",
	       channel[ch].ctype ? "RSC" : "RCH",
	       'A' + ch, ic-1, trap_enb,
	       channel[ch].cflags & CHAN_EOF ? "Y" : "N",
	       channel[ch].cflags & CHAN_CTSSDRUM ? "Y" : "N");
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "load_7909: Channel %c, cact = %d, csel = %d\n",
	       ch+'A', channel[ch].cact, channel[ch].csel);
   }
#endif
   cycle_count++;

   channel[ch].cflags &= ~CHAN_INWAIT;
   channel[ch].csms &= ~SMS_INHIBINT;

   load_cycle_7909 (ch);
   cycle_7909 (ch);
}

/***********************************************************************
* diag_7909 - Store 7909 channel diagnostic.
***********************************************************************/

void
diag_7909 (int ch, uint16 addr)
{
#if defined(DEBUG7909)
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
      fprintf (stderr,
	       "SCD Channel %c, ic = %05o, trap_enb = %o, eof = %s\n",
	       'A' + ch, ic-1, trap_enb,
	       channel[ch].cflags & CHAN_EOF ? "Y" : "N");
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr,
	       "diag_7909: Channel %c, cact = %d, csel = %d, addr = %05o\n",
	       ch+'A', channel[ch].cact, channel[ch].csel, addr);
      fprintf (stderr, "   cccr = %o, cind = %o, cint = %s\n",
	       channel[ch].cccr, channel[ch].cind,
	       channel[ch].cflags & CHAN_INTRPEND ? "Y" : "N");
   }
#endif

#ifdef USE64
   sr = (((t_uint64)channel[ch].cccr & 077) << 30) | 
	  ((t_uint64)channel[ch].cind << 20) |
	  ((t_uint64)channel[ch].cflags & CHAN_INTRPEND ? (1 << 19) : 0);
#if defined(DEBUG7909)
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   sr = %012llo\n", sr);
   }
#endif
   STORE (addr, sr);
#else
   srh = (((channel[ch].cccr & 040) << 2) & SIGN) | 
	  ((channel[ch].cccr & 034) >> 2);
   srl = ((channel[ch].cccr & 003) << 30) | 
	  (channel[ch].cind ? (channel[ch].cind << 20) : 0) |
	  (channel[ch].cflags & CHAN_INTRPEND ? (1 << 19) : 0);
#if defined(DEBUG7909)
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   sr = %02o%010lo\n",
	       ((srh & SIGN) >> 2) | ((srh & HMSK) << 2) | (uint16)(srl >> 30),
	       (unsigned long)srl & 07777777777);
   }
#endif
   STORE (addr, srh, srl);
#endif
}

/***********************************************************************
* reset_7909 - Reset 7909 channel.
***********************************************************************/

void
reset_7909 (int ch)
{
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "%d: %05o: ", inst_count, ic);
      fprintf (stderr, "reset_7909: Channel %c, cact = %d, csel = %d\n",
	       ch+'A', channel[ch].cact, channel[ch].csel);
   }
#endif
#ifdef USE64
   channel[ch].csns[0] = SNS_SIXBIT;
   channel[ch].csns[1] = 0;
   channel[ch].casr = 0;
#else
   channel[ch].csnsh[0] = 0;
   channel[ch].csnsl[0] = SNS_SIXBIT;
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
}

/***********************************************************************
* boot_7909 - Boot 7909 channel DASD.
***********************************************************************/

int
boot_7909 (char *pch)
{
#ifdef USE64
   t_uint64 accmod;
#else
   uint32 accmod;
#endif
   int i;
   int modulefound;
   int validchannel;
   int module;
   int access;
   int ch;
   
#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "boot_7909: pch = %s\n", pch);
   }
#endif

   if (islower (*pch)) ch = *pch - 'a';
   else                ch = *pch - 'A';
   if (ch < 0 || ch > numchan)
   {
   BOOT_INVCHANNEL:
      sprintf (errview[0],
	    "boot_7909: I/O check: Invalid channel\n");
   BOOT_IOCHK:
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "%s", errview[0]);
      }
#endif
      cpuflags |= CPU_IOCHK;
      return (-1);
   }

   pch++;
   if (islower (*pch)) *pch = toupper (*pch);
   if (*pch != 'D' && *pch != 'N')
   {
      sprintf (errview[0],
	    "boot_7909: I/O check: Invalid boot device\n");
      goto BOOT_IOCHK;
   }

   pch++;
   access = *pch - '0';
   module = *(pch + 1) - '0';

   modulefound = FALSE;
   validchannel = FALSE;
   for (i = DASDOFFSET + ch*10; i < DASDOFFSET + MAXDASD + ch*10; i++)
   {
#ifdef DEBUG7909
      if (ch == DEBUGCHAN)
      {
	 fprintf (stderr, "   i = %d\n", i);
      }
#endif
      if (!sysio[i].iochntype) continue;
      validchannel = TRUE;
      if ((module >= sysio[i].iomodstart && module < sysio[i].iomodend) &&
          sysio[i].iofd)
      {
         modulefound = TRUE;
	 break;
      }
   }
   if (!validchannel)
      goto BOOT_INVCHANNEL;
   if (!modulefound)
   {
      sprintf (errview[0],
	    "boot_7909: I/O check: Invalid module\n");
      goto BOOT_IOCHK;
   }

   accmod = (access << 18) | (module << 12);

#ifdef DEBUG7909
   if (ch == DEBUGCHAN)
   {
      fprintf (stderr, "   Channel %c, access = %d, module = %d\n",
	       ch+'A', access, module);
   }
#endif

   if (cpumode == 7096)
   {
      /* CTSS loader - ctss/s.util/cylod.fap */

#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
      mem[0000] = 0377777000100I64;
      mem[0001] = 0006000000001I64;
      mem[0002] = 0007400400100I64;

      mem[0100] = 0076000000350I64 | ((ch + 1) << 9);
      mem[0101] = 0000000000120I64 | RSCx[ch];
      mem[0102] = 0006000000102I64 | (ch << 24);
      mem[0103] = 0476100000042I64;
      mem[0104] = 0450000000000I64;
      mem[0105] = 0036100477777I64;
      mem[0106] = 0200001400105I64;
      mem[0107] = 0476100000041I64;
      mem[0110] = 0032200000131I64;
      mem[0111] = 0450100000046I64;
      mem[0112] = 0010000000132I64;
      mem[0113] = 0000000000002I64;
      mem[0114] = 0101200001212I64 | accmod;
      mem[0115] = 0121212121212I64;
      mem[0116] = 0100500001212I64 | accmod;
      mem[0117] = 0121267671212I64;  
      mem[0120] = 0700000000004I64; // | sel;
      mem[0121] = 0200000000114I64;
      mem[0122] = 0500000200122I64;
      mem[0123] = 0200000200116I64;
      mem[0124] = 0400007000125I64;
#else
      mem[0000] = 0377777000100LL;
      mem[0001] = 0006000000001LL;
      mem[0002] = 0007400400100LL;

      mem[0100] = 0076000000350LL | ((ch + 1) << 9);
      mem[0101] = 0000000000120LL | RSCx[ch];
      mem[0102] = 0006000000102LL | (ch << 24);
      mem[0103] = 0476100000042LL;
      mem[0104] = 0450000000000LL;
      mem[0105] = 0036100477777LL;
      mem[0106] = 0200001400105LL;
      mem[0107] = 0476100000041LL;
      mem[0110] = 0032200000131LL;
      mem[0111] = 0450100000046LL;
      mem[0112] = 0010000000132LL;
      mem[0113] = 0000000000002LL;
      mem[0114] = 0101200001212LL | accmod;
      mem[0115] = 0121212121212LL;
      mem[0116] = 0100500001212LL | accmod;
      mem[0117] = 0121267671212LL;  
      mem[0120] = 0700000000004LL; // | sel;
      mem[0121] = 0200000000114LL;
      mem[0122] = 0500000200122LL;
      mem[0123] = 0200000200116LL;
      mem[0124] = 0400007000125LL;
#endif
#else
      memh[0000] = 0007;             meml[0000] = 037777000100;
      memh[0001] = 0000;             meml[0001] = 006000000001;
      memh[0002] = 0000;             meml[0002] = 007400400100;

      memh[0100] = 0001;             meml[0100] = 036000000350 | ((ch+1) << 9);
      memh[0101] = 0000 | RSCxh[ch]; meml[0101] = 000000000120 | RSCxl[ch];
      memh[0102] = 0000;             meml[0102] = 006000000102 | (ch << 24);
      memh[0103] = 0201;             meml[0103] = 036100000042;
      memh[0104] = 0201;             meml[0104] = 010000000000;
      memh[0105] = 0000;             meml[0105] = 036100477777;
      memh[0106] = 0004;             meml[0106] = 000001400105;
      memh[0107] = 0201;             meml[0107] = 036100000041;
      memh[0110] = 0000;             meml[0110] = 032200000131;
      memh[0111] = 0201;             meml[0111] = 010100000046;
      memh[0112] = 0000;             meml[0112] = 010000000132;
      memh[0113] = 0000;             meml[0113] = 000000000002;
      memh[0114] = 0002;             meml[0114] = 001200001212 | accmod;
      memh[0115] = 0002;             meml[0115] = 021212121212;
      memh[0116] = 0002;             meml[0116] = 000500001212 | accmod;
      memh[0117] = 0002;             meml[0117] = 021267671212;  
      memh[0120] = 0206;             meml[0120] = 000000000004; // | sel;
      memh[0121] = 0004;             meml[0121] = 000000000114;
      memh[0122] = 0202;             meml[0122] = 000000200122;
      memh[0123] = 0004;             meml[0123] = 000000200116;
      memh[0124] = 0200;             meml[0124] = 000007000125;
#endif
      return (2);
   }
   else
   {
#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
      mem[0000] = 0000025000101I64;
      mem[0001] = 0006000000001I64;
      mem[0002] = 0002000000101I64;

      mem[0101] = 0000000000115I64 | RSCx[ch];
      mem[0102] = 0000000000000I64 | SCDx[ch];
      mem[0103] = 0044100000000I64;
      mem[0104] = 0405400007100I64;
      mem[0105] = 0002000000110I64;
      mem[0106] = 0006000000102I64 | (ch << 24);
      mem[0107] = 0002000000003I64;
      mem[0110] = 0076000000350I64 | ((ch+1) << 9);
      mem[0111] = 0500500001212I64 | accmod;
      mem[0112] = 0121222440000I64;
      mem[0113] = 0501200000000I64 | accmod;
      mem[0114] = 0000000000000I64;
      mem[0115] = 0700000000016I64;
      mem[0116] = 0200000000113I64;
      mem[0117] = 0500000200117I64;
      mem[0120] = 0200000200111I64;
      mem[0121] = 0400001000122I64;
      mem[0122] = 0000000000122I64;
      mem[0123] = 0100000000121I64;
      mem[0124] = 0500000000000I64;
      mem[0125] = 0340000000125I64;
#else
      mem[0000] = 0000025000101ULL;
      mem[0001] = 0006000000001ULL;
      mem[0002] = 0002000000101ULL;

      mem[0101] = 0000000000115ULL | RSCx[ch];
      mem[0102] = 0000000000000ULL | SCDx[ch];
      mem[0103] = 0044100000000ULL;
      mem[0104] = 0405400007100ULL;
      mem[0105] = 0002000000110ULL;
      mem[0106] = 0006000000102ULL | (ch << 24);
      mem[0107] = 0002000000003ULL;
      mem[0110] = 0076000000350ULL | ((ch+1) << 9);
      mem[0111] = 0500500001212ULL | accmod;
      mem[0112] = 0121222440000ULL;
      mem[0113] = 0501200000000ULL | accmod;
      mem[0114] = 0000000000000ULL;
      mem[0115] = 0700000000016ULL;
      mem[0116] = 0200000000113ULL;
      mem[0117] = 0500000200117ULL;
      mem[0120] = 0200000200111ULL;
      mem[0121] = 0400001000122ULL;
      mem[0122] = 0000000000122ULL;
      mem[0123] = 0100000000121ULL;
      mem[0124] = 0500000000000ULL;
      mem[0125] = 0340000000125ULL;
#endif

#else
      memh[0000] = 0000;             meml[0000] = 000025000101;
      memh[0001] = 0000;             meml[0001] = 006000000001;
      memh[0002] = 0000;             meml[0002] = 002000000101;

      memh[0101] = 0000 | RSCxh[ch]; meml[0101] = 000000000115 | RSCxl[ch];
      memh[0102] = 0000 | SCDxh[ch]; meml[0102] = 000000000000 | SCDxl[ch];
      memh[0103] = 0001;             meml[0103] = 004100000000;
      memh[0104] = 0200;             meml[0104] = 005400007100;
      memh[0105] = 0000;             meml[0105] = 002000000110;
      memh[0106] = 0000;             meml[0106] = 006000000102 | (ch << 24);
      memh[0107] = 0000;             meml[0107] = 002000000003;
      memh[0110] = 0001;             meml[0110] = 036000000350 | ((ch+1) << 9);
      memh[0111] = 0202;             meml[0111] = 000500001212 | accmod;
      memh[0112] = 0002;             meml[0112] = 021222440000;
      memh[0113] = 0202;             meml[0113] = 001200000000 | accmod;
      memh[0114] = 0000;             meml[0114] = 000000000000;
      memh[0115] = 0206;             meml[0115] = 000000000016;
      memh[0116] = 0004;             meml[0116] = 000000000113;
      memh[0117] = 0202;             meml[0117] = 000000200117;
      memh[0120] = 0004;             meml[0120] = 000000200111;
      memh[0121] = 0200;             meml[0121] = 000001000122;
      memh[0122] = 0000;             meml[0122] = 000000000122;
      memh[0123] = 0002;             meml[0123] = 000000000121;
      memh[0124] = 0202;             meml[0124] = 000000000000;
      memh[0125] = 0007;             meml[0125] = 000000000125;
#endif
      return (0101);
   }
}
