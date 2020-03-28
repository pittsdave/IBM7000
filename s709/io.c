/***********************************************************************
*
* io.c - IBM 7090 emulator I/O routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   05/18/05   DGP   Added Disk channels.
*   06/01/06   DGP   Added simh tape format support.
*   06/05/07   DGP   Changed DASD assignment to be like IBSYS ATTACH format.
*   02/29/08   DGP   Changed to use bit mask flags.
*   02/12/10   DGP   Check fseek/fread return values.
*   05/18/10   DGP   Preserve device EOF in ioflags.
*   06/17/10   DGP   Short circuit BSR delay for CTSS.
*   08/24/10   DGP   Added KSR37/KSR33 support.
*   08/17/11   DGP   Handle SPRA printer codes.
*   03/18/15   DGP   Added real tape support.
*   
***********************************************************************/

#define EXTERN extern

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/mtio.h>

#include "sysdef.h"
#include "regs.h"
#include "parity.h"
#include "nativebcd.h"
#include "chan7607.h"
#include "io.h"
#include "screen.h"
#include "dasd.h"
#include "comm.h"

extern int errno;
extern int prtviewlen;
extern int panelmode;

extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];

static FILE *logfd;
static uint8 bsbuf[300];

static dasd_types dasd[MAXDASDMODEL] =
{
   { "1301-1",  250, 1, 1,  41,  2840, 4 },
   { "1301-2",  250, 2, 1,  41,  2840, 4 },
   { "1302-1",  250, 1, 2,  41,  5902, 7 },
   { "1302-2",  250, 2, 2,  41,  5902, 7 },
   { "7320",      1, 1, 1, 401,  2836, 4 },
   { "7289",      6, 1, 1,  16, 12288, 0 },
};

comm_types comm_data[MAXCOMMMODEL] =
{
   { "KSR-35",  85, FALSE, COM_KSR35,    TRUE },
   { "1050",    80, TRUE,  COM_1050,     FALSE },
   { "TELEX",   72, FALSE, COM_TELEX,    FALSE },
   { "TWX",     72, FALSE, COM_TWX,      TRUE },
   { "KSR-33",  72, FALSE, COM_KSR33,    TRUE },
   { "ASR-35",  85, FALSE, COM_ASR35,    TRUE },
   { "KSR-37",  88, TRUE,  COM_KSR37,    TRUE },
   { "HSLINE", 130, TRUE,  COM_2741,     FALSE },
   { "ESL",     80, TRUE,  COM_ESLSCOPE, FALSE },
};

static struct mtop mt_weof   = { MTWEOF, 1 };
static struct mtop mt_rew    = { MTREW, 1 };
static struct mtop mt_bsr    = { MTBSR, 1 };
static struct mtop mt_bsf    = { MTBSF, 1 };
static struct mtop mt_unload = { MTUNLOAD, 1 };
static struct mtop mt_setblk = { MTSETBLK, 0 }; /* blockize = 0 (variable) */

/***********************************************************************
* tapereadint - Read an integer.
***********************************************************************/

static int
tapereadint (FILE *fd)
{
   int r;

   r = fgetc (fd);
   r = r | (fgetc (fd) << 8);
   r = r | (fgetc (fd) << 16);
   r = r | (fgetc (fd) << 24);
#ifdef DEBUGIOSIMH
   fprintf (stderr, "tapereadint: r = %d\n", r);
#endif
   if (feof (fd)) return (EOF);
   if (ferror (fd)) return (EOF-1);
   return (r);
}

/***********************************************************************
* tapewriteint - Write an integer.
***********************************************************************/

static void
tapewriteint (FILE *fd, int v)
{
   fputc (v & 0xFF, fd);
   fputc ((v >> 8) & 0xFF, fd);
   fputc ((v >> 16) & 0xFF, fd);
   fputc ((v >> 24) & 0xFF, fd);
}

/***********************************************************************
* dasdreadint - Read an integer.
***********************************************************************/

static int
dasdreadint (FILE *fd)
{
   int r;
   int i;

   r = 0;
   for (i = 0; i < 4; i++)
   {
      int c;
      if ((c = fgetc (fd)) < 0)
      {
	 sprintf (errview[0], "dasdreadint: read failed: %s\n",
	       strerror (errno));
	 cpuflags |= CPU_IOCHK;
	 run = CPU_STOP;
         return -1;
      }
      r = (r << 8) | (c & 0xFF);
   }
   return (r);
}

/***********************************************************************
* isdrum - Returns TRUE if the device is CTSS high speed drum.
***********************************************************************/

int
isdrum (int ch)
{
   uint16 unit;

   unit = channel[ch].cunit;
#ifdef DEBUGIO
   fprintf (stderr, "isdrum: Channel %c, unit = %o\n", ch+'A', unit);
#endif

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
      return sysio[unit].ioflags & IO_CTSSDRUM ? TRUE : FALSE;
   }

   return 0;
}

/***********************************************************************
* devstr - Returns a text string that describes the device.
***********************************************************************/

char *
devstr (int dev)
{
   int ch, unit;
   static char s[20];

#ifdef DEBUGIO1
   fprintf (stderr, "devstr: dev = %d\n", dev);
#endif

   switch (dev)
   {

   case 0:
      return "Channel A Reader";

   case 1:
      return "Channel A Punch";

   case 2:
      return "Channel A Printer";

   default:
      if (dev >= 200)
      {
	 ch = (dev - COMMOFFSET)/10;
	 unit = (dev - COMMOFFSET) % 10 + 1;

	 sprintf (s, "Channel %c Comm %2d",
		  ch + 'A',
		  unit);
      }
      else if (dev > 100)
      {
	 ch = (dev - DASDOFFSET)/10;
	 unit = (dev - DASDOFFSET) % 10 + 1;

	 sprintf (s, "Channel %c %s %2d",
		  ch + 'A',
		  sysio[dev].ioflags & IO_DISK ? "Disk" : "Drum",
		  unit);
      }
      else
      {
	 ch = (dev - TAPEOFFSET)/10;
	 unit = (dev - TAPEOFFSET) % 10 + 1;
	 sprintf (s, "Channel %c Tape %2d",
		  ch + 'A', 
		  unit);
      }
      return s;
   }
}

/***********************************************************************
* parsedev - Parses a device specification.
***********************************************************************/

static char *
parsedev (char *s, int *devp, int *typep)
{
   char  c;
   int   ch, unit;
   int   dev;
   int   dasd;
   int   disk;
   int   comm;
   int   cdrum;
   int   modstart;

#ifdef DEBUGIO
   fprintf (stderr, "parsedev: s = %s\n", s);
#endif
   disk = FALSE;
   dasd = FALSE;
   comm = FALSE;
   cdrum = FALSE;
   *typep = 0;
   c = *s++;
   switch (c)
   {

   case 'r':
      dev = 0;
      memset (&sysio[dev], '\0', sizeof (IO_t));
      sysio[dev].iorw = IO_READ;
      break;

   case 'u':
      dev = 1;
      memset (&sysio[dev], '\0', sizeof (IO_t));
      sysio[dev].iorw = IO_WRITE;
      break;

   case 'p':
      dev = 2;
      memset (&sysio[dev], '\0', sizeof (IO_t));
      sysio[dev].iorw = IO_WRITE;
      if (*s == 'c')
      {
         sysio[dev].ioflags |= IO_PRINTCLK;
	 s++;
      }
      break;

   case 'a':
   case 'b':
   case 'c':
   case 'd':
   case 'e':
   case 'f':
   case 'g':
   case 'h':
      ch = c - 'a';
      if (ch >= numchan)
      {
	 cpuflags |= CPU_IOCHK;
	 sprintf (errview[0],
	       "parsedev: Channel %c not configured for device '%c%s'\n",
	       islower (c) ? toupper (c) : c, c, s);
	 run = CPU_STOP;
	 return (char *)0;
      }

      dev = 10*ch + 2;
      unit = -1;
      modstart = 0;
      c = *s++;

      if (c == 'c') /* Comm */
      {
	 *typep = channel[ch].ctype = CHAN_7750;
	 comm = TRUE;
	 dev = COMMOFFSET + (ch * 10);
         unit++;
	 c = *s++;
	 if (isdigit (c))
	 {
	    modstart = c - '0';
	    c = *s++;
	 }
      }
      else if (c == 'd') /* Disk */
      {
	 *typep = channel[ch].ctype = CHAN_7909;
	 disk = TRUE;
	 dasd = TRUE;
	 dev += DASDOFFSET - 2;
         unit++;
	 c = *s++;
	 if (isdigit (c))
	 {
	    modstart = c - '0';
	    c = *s++;
	 }
      }
      else if (c == 'n') /* NEXT (Drum) */
      {
	 *typep = channel[ch].ctype = CHAN_7909;
	 disk = FALSE;
	 dasd = TRUE;
	 dev += DASDOFFSET - 2;
         unit++;
	 c = *s++;
	 if (isdigit (c))
	 {
	    modstart = c - '0';
	    c = *s++;
	 }
      }
      else if (c == 'h') /* High speed CTSS drum */
      {
	 *typep = channel[ch].ctype = CHAN_7909;
	 cdrum = TRUE;
	 disk = FALSE;
	 dasd = TRUE;
	 dev += DASDOFFSET - 2;
         unit++;
	 c = *s++;
	 if (isdigit (c))
	 {
	    modstart = c - '0';
	    c = *s++;
	 }
      }

      if (isdigit (c))
      {
	 if (comm)
	 {
	    modstart = (modstart * 10) + (c - '0');
	    if (modstart > MAXCOMM)
	    {
	       cpuflags |= CPU_IOCHK;
	       sprintf (errview[0],
			"parsedev: Too many comm lines: lines = %d, MAX = %d\n",
			modstart, MAXCOMM);
#ifdef DEBUGIO
	       fputs (errview[0], stderr);
#endif
	       run = CPU_STOP;
	       return (char *)0;
	    }
	 }
         else if (c == '1' && *s == '0')
	 {
	    unit += 10;
            dev += 10;
            s++;
         }
	 else
	 {
	    unit += c - '0';
            dev += c - '0';
         }
	 memset (&sysio[dev], '\0', sizeof (IO_t));
         sysio[dev].iorw = IO_RDWRT;
	 if (comm)
	 {
	    sysio[dev].iochntype = channel[ch].ctype;
	    sysio[dev].ioflags |= IO_COMM;
            sysio[dev].iomodstart = FIRSTTTY;
            sysio[dev].iomodend = modstart + FIRSTTTY;
#ifdef DEBUGIO
            fprintf (stderr,
	    "   COMM device: Channel %c dev = %d, unit = %d, modend = %d\n",
		     ch+'A', dev, unit, sysio[dev].iomodend);
#endif
	 }
	 else if (dasd)
	 {
	    sysio[dev].iochntype = channel[ch].ctype;
	    sysio[dev].ioflags |= IO_DASD;
	    sysio[dev].ioflags |= disk ? IO_DISK : 0;
	    sysio[dev].ioflags |= cdrum ? IO_CTSSDRUM : 0;
            sysio[dev].iomodstart = modstart ? modstart : channel[ch].cmodstart;
#ifdef DEBUGIO
            fprintf (stderr,
	    "   DASD device: Channel %c dev = %d, unit = %d, modstart = %d\n",
		     ch+'A', dev, unit, sysio[dev].iomodstart);
            fprintf (stderr, "   disk = %d, cdrum = %d", disk, cdrum);
#endif
	 }
	 else
	 {
	    sysio[dev].ioflags |= IO_TAPE;
#ifdef DEBUGIO
            fprintf (stderr,
		     "   TAPE device: Channel %c dev = %d, unit = %d\n",
		     ch+'A', dev, unit);
#endif
	    while (*s && *s != '=' && *s != ' ')
	    {
               if (*s == 'a')
               {
#ifdef DEBUGIO
		  fputs ("   alternate format\n", stderr);
#endif
		  sysio[dev].ioflags |= IO_ALTBCD;
		  s++;
               }

	       /*
	       ** Check if read only
	       */

	       else if (*s == 'r')
	       {
#ifdef DEBUGIO
		  fputs ("   read only\n", stderr);
#endif
		  sysio[dev].iorw = IO_READ;
		  s++;
	       }

	       /*
	       ** Check if simh format
	       */

	       else if (*s == 's')
	       {
		  sysio[dev].ioflags |= (IO_ALTBCD | IO_SIMH);
		  s++;
	       }
	       else
	          goto PARSE_ERROR;
	    }
	 }
         break;
      }

   default:
   PARSE_ERROR:
      cpuflags |= CPU_IOCHK;
      sprintf (errview[0], "parsedev: I/O check: s = %c%s\n",
	       c, s);
#ifdef DEBUGIO
      fputs (errview[0], stderr);
#endif
      run = CPU_STOP;
      return (char *)0;
   }

   while (*s && (*s == ' ' || *s == '='))
      s++;

   /*
   ** Check if we have a real device.
   */

   if (!strncmp (s, "/dev/", 5))
   {
      sysio[dev].ioflags |= IO_REALDEV;
      if (sysio[dev].ioflags & IO_TAPE)
	 sysio[dev].ioflags |= IO_ALTBCD;
   }

#ifdef DEBUGIO
   if (sysio[dev].ioflags & IO_TAPE)
   {
      if (sysio[dev].ioflags & IO_REALDEV)
	 fputs ("   REAL TAPE\n", stderr);
      else if (sysio[dev].ioflags & IO_SIMH)
	 fputs ("   simh format\n", stderr);
      else
	 fputs ("   p7b format\n", stderr);
   }
   fprintf (stderr, "   dev = %d, channel = %d\n", dev,
	    sysio[dev].iochntype ? 7909 : 7607);
#endif
   *devp = dev;
   return s;
}

/***********************************************************************
* opendev - Opens the specified device.
***********************************************************************/

static int
opendev (char *s, int dev)
{
   register int i;
   int dot;

#ifdef DEBUGIO
   fprintf (stderr, "opendev: dev = %d, s = %s\n", dev, s);
#endif

   if (*s == '@')
   {
      dorewind (dev);
      return 0;
   }
   else if (*s == '#')
   {
      if (dev <= 2)
      {
	 cpuflags |= CPU_IOCHK;
         sprintf (errview[0],
		  "opendev: I/O check: s = %s\n", s);
#ifdef DEBUGIO
	 fputs (errview[0], stderr);
#endif
         run = CPU_STOP;
         return -1;
      }
      sysio[dev].iorw = IO_READ;
      return 0;
   }

   if (sysio[dev].iofd != NULL)
   {
      fclose (sysio[dev].iofd);
      sysio[dev].iofd = NULL;
      sysio[dev].iostr[0] = '\0';
   }
   if (*s == '\0' || *s == '\n')
   {
      return 0;
   }

   dot = FALSE;
   i = 0;

doname:
   for (; i < IO_MAXNAME; i++)
   {
      if ((sysio[dev].iostr[i] = *s++) == '\0')
         goto nlong;
      if (sysio[dev].iostr[i] == '\n')
      {
         sysio[dev].iostr[i] = '\0';
         goto nlong;
      }
      if (sysio[dev].iostr[i] == '@')
      {
         sysio[dev].iostr[i] = '\0';
         goto nlong;
      }
      if (sysio[dev].iostr[i] == '#')
      {
         if (dev <= 2)
	 {
	    cpuflags |= CPU_IOCHK;
            sprintf (errview[0],
		     "opendev: I/O check: s = %s, iostr = %s\n",
		     s, sysio[dev].iostr);
#ifdef DEBUGIO
	    fputs (errview[0], stderr);
#endif
            run = CPU_STOP;
            sysio[dev].iostr[0] = '\0';
            return -1;
         }
         sysio[dev].iorw = IO_READ;
         sysio[dev].iostr[i] = '\0';
         goto nlong;
      }
      if (sysio[dev].iostr[i] == '.')
      {
         dot = TRUE;
      }
      if (sysio[dev].iostr[i] == '\\')
      {
         dot = FALSE;
      }
   }
   cpuflags |= CPU_IOCHK;
   sprintf (errview[0],
	    "opendev: I/O check: s = %s, iostr = %s\n",
	    s, sysio[dev].iostr);
#ifdef DEBUGIO
   fputs (errview[0], stderr);
#endif
   run = CPU_STOP;
   sysio[dev].iostr[0] = '\0';
   return -1;

nlong:
   if (!dot && dev <= 2)
   {
      if (dev < 2)
         s = ".cbn";
      else
         s = ".bcd";
      goto doname;
   }

   switch (sysio[dev].iorw)
   {
   case IO_READ:
      sysio[dev].iofd = fopen (sysio[dev].iostr, "rb");
      break;

   case IO_WRITE:
      sysio[dev].iofd = fopen (sysio[dev].iostr, "wb");
      break;

   case IO_RDWRT:
      sysio[dev].iofd = fopen (sysio[dev].iostr, "r+b");
      if (sysio[dev].iofd == NULL && !(sysio[dev].ioflags & IO_DASD))
         sysio[dev].iofd = fopen (sysio[dev].iostr, "w+b");
      break;

   }
   if (sysio[dev].iofd == NULL)
   {
      sprintf (errview[0], "%s: open failed: %s\n",
	       devstr (dev), strerror (errno));
      sprintf (errview[1], "filename: %s\n", sysio[dev].iostr);
      cpuflags |= CPU_IOCHK;
#ifdef DEBUGIO
      fputs (errview[0], stderr);
      fputs (errview[1], stderr);
#endif
      sysio[dev].iostr[0] = '\0';
      run = CPU_STOP;
      return -1;
   }
   sysio[dev].iopos = 0;

   if (sysio[dev].ioflags & IO_DASD)
   {
      int chan;

      /*
      ** Seek to beginning of the DASD device
      */

      chan = (dev - DASDOFFSET)/10;
      if (fseek (sysio[dev].iofd, 0, SEEK_SET) < 0)
      {
      DASD_SEEK_FAILURE:
	 sprintf (errview[0], "%s: DASD seek failed: %s\n",
		  devstr (dev), strerror (errno));
	 cpuflags |= CPU_IOCHK;
	 run = CPU_STOP;
	 return -1;
      }

      /*
      ** Read the DASD device geometry
      */

      if ((sysio[dev].iocyls  = dasdreadint (sysio[dev].iofd)) < 0)
      {
      DASD_READ_FAILURE:
	 sprintf (errview[0], "%s: DASD geometry read failed: %s\n",
		  devstr (dev), strerror (errno));
	 cpuflags |= CPU_IOCHK;
	 run = CPU_STOP;
	 return (-1);
      }
      if ((sysio[dev].ioheads = dasdreadint (sysio[dev].iofd)) < 0)
         goto DASD_READ_FAILURE;
      if ((sysio[dev].iomodules = dasdreadint (sysio[dev].iofd)) < 0)
         goto DASD_READ_FAILURE;
      sysio[dev].ioaccess = (sysio[dev].iomodules >> 16) & 0xFFFF;
      sysio[dev].iomodules &= 0xFFFF;
      channel[chan].cmodstart += sysio[dev].iomodules;
      sysio[dev].iomodend = sysio[dev].iomodules + sysio[dev].iomodstart;
      if ((sysio[dev].iobyttrk  = dasdreadint (sysio[dev].iofd)) < 0)
         goto DASD_READ_FAILURE;

      for (i = 0; i < MAXDASDMODEL; i++)
      {
         if (dasd[i].cyls == sysio[dev].iocyls &&
	     dasd[i].heads == sysio[dev].ioheads &&
	     dasd[i].access == sysio[dev].ioaccess &&
	     dasd[i].modules == sysio[dev].iomodules &&
	     dasd[i].byttrk == sysio[dev].iobyttrk)
         {
	    break;
	 }
      }
      if (i == MAXDASDMODEL)
      {
         sprintf (errview[0], "%s: Unsupported DASD file given\n",
		  devstr (dev));
	 cpuflags |= CPU_IOCHK;
	 run = CPU_STOP;
	 return (-1);
      }
      sysio[dev].iooverhead = dasd[i].overhead;
      sysio[dev].iomodel = i;

#ifdef DEBUGDASD
      fprintf (stderr, "DASD geometry:\n");
      fprintf (stderr, "   device    = %s(%d)\n", devstr (dev), dev);
      fprintf (stderr, "   model     = %s\n", dasd[i].model);
      fprintf (stderr, "   file      = %s\n", sysio[dev].iostr);
      fprintf (stderr, "   cyls      = %d\n", sysio[dev].iocyls);
      fprintf (stderr, "   heads     = %d\n", sysio[dev].ioheads);
      fprintf (stderr, "   byttrk    = %d\n", sysio[dev].iobyttrk);
      fprintf (stderr, "   overhead  = %d\n", sysio[dev].iooverhead);
      fprintf (stderr, "   access    = %d\n", sysio[dev].ioaccess );
      fprintf (stderr, "   modules   = %d\n", sysio[dev].iomodules);
      fprintf (stderr, "   modstart  = %d\n", sysio[dev].iomodstart);
      fprintf (stderr, "   modend    = %d\n", sysio[dev].iomodend);
      fprintf (stderr, "   dasd size = %d bytes\n",
	       sysio[dev].iobyttrk * sysio[dev].ioheads *
	       sysio[dev].iocyls * sysio[dev].ioaccess *
	       sysio[dev].iomodules);
#endif

      if (fseek (sysio[dev].iofd, DASDOVERHEAD, SEEK_SET) < 0)
         goto DASD_SEEK_FAILURE;
   }

   /*
   ** if a real tape, set default characteristics.
   */

   else if ((sysio[dev].ioflags & IO_REALDEV) &&
	    (sysio[dev].ioflags & IO_TAPE))
   {
      struct mtop mt_setden = { MTSETDENSITY, 3 }; 

      ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_setden);
      ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_setblk);
   }
   return 0;
}

/***********************************************************************
* mounterr - Prints an error message for mount failures.
***********************************************************************/

static void
mounterr ()
{
    strcpy (errview[1], "mount file on unit: r=cr, u=pch, p=prt\n");
    strcpy (errview[2], "                    a-h=chan + 1-10=tape\n");
    strcpy (errview[3], "                    a-h=chan + d + m + 0-9=disk\n");
    strcpy (errview[4], "                    a-h=chan + n + m + 0-9=drum\n");
    strcpy (errview[5], "                    # fpt, @ rwd, m module\n");
#ifdef DEBUGIO
      fputs (errview[0], stderr);
      fprintf (stderr, "%d: ", inst_count);
      fputs (errview[1], stderr);
      fprintf (stderr, "%d: ", inst_count);
      fputs (errview[2], stderr);
      fprintf (stderr, "%d: ", inst_count);
      fputs (errview[3], stderr);
      fprintf (stderr, "%d: ", inst_count);
      fputs (errview[4], stderr);
      fprintf (stderr, "%d: ", inst_count);
      fputs (errview[5], stderr);
#endif
}

/***********************************************************************
* mount - Mounts the specified device.
***********************************************************************/

int
mount (char *s)
{
   int type;
   int dev;

   if (*s == '\n' || *s == '\0')
      return 0;
   if ((s = parsedev (s, &dev, &type)) == (char *)0)
   {
      mounterr ();
      return -1;
   }
   if (type == CHAN_7750)
      return commopen (s, dev);
   return opendev (s, dev);
}

/***********************************************************************
* listmount - Lists the currently mounted devices.
***********************************************************************/

void
listmount ()
{
   register int i, j;
   int errndx;

   errndx = 0;
   strcpy (errview[errndx++], "Channel  Unit     R F  File\n");

   if (sysio[0].iofd)
      sprintf (errview[errndx++], "   A  Card Reader #    %s\n",
	       sysio[0].iostr);
   if (sysio[1].iofd)
      sprintf (errview[errndx++], "   A  Card Punch  *    %s\n",
	       sysio[1].iostr);
   if (sysio[2].iofd)
      sprintf (errview[errndx++], "   A  Printer     *    %s\n",
	       sysio[2].iostr);

   for (i = 0; i < numchan; i++)
   {
      int iondx;

      for (j = 0; j < MAXTAPE; j++)
      {
	 iondx = i*10+j+TAPEOFFSET;

         if (sysio[iondx].iofd != NULL)
            sprintf (errview[errndx++], "   %c  Tape %2d     %c %c  %s\n",
		     'A' + i, j + 1,
		     sysio[iondx].iorw == IO_READ ? '#' : ' ',
		     sysio[iondx].ioflags & IO_SIMH ? 's' : 'p',
		     sysio[iondx].iostr);
      }
      for (j = 0; j < MAXDASD; j++)
      {
         iondx = i*10+j+DASDOFFSET;

         if (sysio[iondx].iofd != NULL)
            sprintf (errview[errndx++], "   %c  %s %2d     %c    %s (%s)\n",
		     'A' + i, sysio[iondx].ioflags & IO_DISK ? "Disk" : "Drum",
		     j+1, sysio[iondx].iorw == IO_READ ? '#' : ' ',
		     sysio[iondx].iostr, dasd[sysio[iondx].iomodel].model);
      }

      iondx = i*10+COMMOFFSET;
      if (sysio[iondx].iofd != NULL)
      {
	 sprintf (errview[errndx++], "   %c  Comm %2d     %c    %s\n",
		  'A' + i, sysio[iondx].iomodend,
		  sysio[iondx].iorw == IO_READ ? '#' : ' ',
		  sysio[iondx].iostr);
	 for (j = 0; j < sysio[iondx].iomodend; j++)
	 {
	    COMM_t *line = &channel[i].devices.comlines[j];
	    int qdepth;

	    if (line->tail >= line->head)
	    	qdepth = line->tail - line->head;
	    else
	       qdepth = (sizeof(line->ring) - line->head) + line->tail;
	    sprintf (errview[errndx++], "        Line %02d: %s - %s (%d)\n",
		     j, comm_data[line->model-1].model, line->who, qdepth);
	 }
      }
   }
}

/***********************************************************************
* readrec - Reads a record from the attached device.
***********************************************************************/

int
readrec (int dev, uint8 *buf, int len)
{
   register int i, j;
   int n;

   if (sysio[dev].iofd == NULL)
   {
      sprintf (errview[0], "%s: No file mounted\n", devstr (dev));
      goto READ_ERRMSG;
   }
   if ((sysio[dev].iorw & IO_READ) == 0)
   {
      sprintf (errview[0], "%s(%s): File is write-only\n",
	       devstr (dev), sysio[dev].iostr);
      goto READ_ERRMSG;
   }

#ifdef DEBUGIOSIMH
   fprintf (stderr, "readrec: %s: len = %d\n", devstr (dev), len);
#endif

   if (dev >= 0 && dev < 100) /* Card & Tape */
   {
      if (sysio[dev].ioflags & IO_REALDEV)
      {
         if ((i = read (fileno(sysio[dev].iofd), buf, len)) == 0)
	 {
	    sysio[dev].iopos++;
	    goto TAPE_EOF;
	 }
	 if (i < 0)
	    goto READ_ERR;
	 sysio[dev].iopos += i;
	 sysio[dev].iobyttrk = i;
	 goto TAPE_READ;
      }
      else if (sysio[dev].ioflags & IO_SIMH)
      {
	 if ((i = tapereadint (sysio[dev].iofd)) < 0)
	 {
	 SIMH_READ_ERR:
	    if (i == EOF)
	    {
	       clearerr (sysio[dev].iofd);
	       channel[sysio[dev].iochn].cflags |= CHAN_EOT;
	       goto SIMH_EOF;
	    }
	    goto READ_ERR;
	 }
	 if (i == 0)
	 {
	 SIMH_EOF:
	    sysio[dev].iopos = ftell (sysio[dev].iofd);
	 TAPE_EOF:
	    buf[0] = (char)0217;
	    for (i = 1; i < len; i++)
	       buf[i] = 0;
	    if (dev == 0 && feof (sysio[dev].iofd))
	    {
#ifdef DEBUG7607
	       fprintf (stderr, "%d: ", inst_count);
	       fprintf (stderr, "EOF on dev %d, chan %d\n",
			dev, sysio[dev].iochn);
#endif
	       channel[sysio[dev].iochn].cflags |= CHAN_EOF;
	    }
	    return (0);
	 }
	 if (fread (buf, 1, i, sysio[dev].iofd) != i) goto SIMH_READ_ERR;
	 if (tapereadint (sysio[dev].iofd) != i) goto SIMH_READ_ERR;
	 sysio[dev].iopos = ftell (sysio[dev].iofd);
      TAPE_READ:
	 buf[0] |= 0200;
	 for (j = i; j < len; j++)
	    buf[j] = 0;
      }
      else
      {
      readmore:
	 n = fread (buf, 1, len, sysio[dev].iofd);
	 if (n == 0)
	 {
	    buf[0] = (char)0217;
	    for (i = 1; i < len; i++)
	       buf[i] = 0;
	    if (feof (sysio[dev].iofd))
	    {
#ifdef DEBUG7607
	       fprintf (stderr, "%d: ", inst_count);
	       fprintf (stderr, "EOF on dev %d, chan %d\n",
			dev, sysio[dev].iochn);
#endif
	       channel[sysio[dev].iochn].cflags |= CHAN_EOF;
	       if (dev > 0)
		  channel[sysio[dev].iochn].cflags |= CHAN_EOT;
	       sysio[dev].ioflags |= IO_ATEOF;
	       return (0);
	    }
	    goto READ_ERR;
	 }

	 sysio[dev].iopos += n;
	 for (j = n; j < len; j++)
	    buf[j] = 0;
	 for (i = 0; i < n; i++)
	 {
	    if (buf[i] & 0200)
	    {
#ifdef DEBUGIO1
	       fprintf (stderr, "readrec-1: i = %d, buf[i] = %o\n",
			i, (uint8)buf[i]);
#endif
	       break;
	    }
	 }
	 if (i == n)
	    goto readmore;
	 if (i > 0)
	 {
	    for (j = 0; i < n; )
	       buf[j++] = buf[i++];
	    n = fread (&buf[j], 1, len - j, sysio[dev].iofd);
	    if (n < 0)
	       goto READ_ERR;
	    sysio[dev].iopos += n;
	    for (j = j + n; j < len; j++)
	       buf[j] = 0;
	 }
	 for (i = 1; i < n; i++)
	 {
	    if (buf[i] & 0200)
	    {
#ifdef DEBUGIO1
	       fprintf (stderr, "readrec-2: i = %d, buf[i] = %o\n",
			i, (uint8)buf[i]);
#endif
	       for (j = i; j < len; j++)
		  buf[j] = 0;
	       sysio[dev].iopos = sysio[dev].iopos - n + i;
	       if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
		  goto READ_ERR;
	       break;
	    }
	 }
      }
      if (dev > 2)
	 channel[sysio[dev].iochn].cflags &= ~CHAN_BOT;
   }
   else  /* DASD */
   {
      n = fread (buf, 1, len, sysio[dev].iofd);
      if (n != len)
      {
	 if (n < 0)
	    goto READ_ERR;
      }
      i = n;
   }
   return (i);

READ_ERR:
   sprintf (errview[0], "%s(%s): Read error %d\n",
	    devstr (dev), sysio[dev].iostr, errno);

READ_ERRMSG:
   cpuflags |= CPU_IOCHK;
#ifdef DEBUGIO
   fputs (errview[0], stderr);
#endif
   run = CPU_STOP;
   return (-1);
}

/***********************************************************************
* bsr - Backspace record.
***********************************************************************/

void
bsr (int ch)
{
   register int i;
   int dev;
   int n;

   dev = whatdev (ch);
   if (sysio[dev].iofd == NULL)
   {
      sprintf (errview[0], "%s: No file mounted\n", devstr (dev));
      goto BSR_ERRMSG;
   }
   if ((sysio[dev].iorw & IO_READ) == 0)
   {
      sprintf (errview[0], "%s(%s): File is write-only\n",
	       devstr (dev), sysio[dev].iostr);
      goto BSR_ERRMSG;
   }
#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "bsr   %s                                %10ld\n",
	    devstr (dev), sysio[dev].iopos);
#endif

   /*
   ** Short circuit channel delay for CTSS.
   ** CTSS uses a 25 inst spin.... yuk.
   */
   if (cpumode == 7096)
   {
      channel[ch].ccyc = 10;
   }
   sysio[dev].ioflags &= ~IO_ATEOF;
   if (sysio[dev].ioflags & IO_REALDEV)
   {
      struct mtget mt_status;

      if (ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_bsr) < 0)
	 sysio[dev].iopos--; /* back over an EOF */
      else
	 sysio[dev].iopos -= sysio[dev].iobyttrk;
      if (sysio[dev].iopos < 0)
	 sysio[dev].iopos = 0;

      if (ioctl (fileno(sysio[dev].iofd), MTIOCGET, &mt_status) < 0)
         goto BSR_ERR;
      if (GMT_BOT(mt_status.mt_gstat))
	 sysio[dev].iopos = 0;
         
      goto BSR_DONE;
   }

   sysio[dev].iopos = ftell (sysio[dev].iofd);
   while (sysio[dev].iopos > 0)
   {
      if (sysio[dev].ioflags & IO_SIMH)
      {
         sysio[dev].iopos -= 4;
	 if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
	    goto BSR_ERR;
	 n = tapereadint (sysio[dev].iofd);
	 if (n > 0)
	    sysio[dev].iopos -= n + 4;
	 if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
	    goto BSR_ERR;
	 goto BSR_DONE;
      }
      else
      {
	 if (sysio[dev].iopos > sizeof bsbuf)
	 {
	    n = sizeof bsbuf;
	    sysio[dev].iopos -= sizeof bsbuf;
	 }
	 else
	 {
	    n = sysio[dev].iopos;
	    sysio[dev].iopos = 0;
	 }
	 if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
	    goto BSR_ERR;
	 if (fread (bsbuf, 1, n, sysio[dev].iofd) != n)
	    goto BSR_ERR;
	 for (i = n - 1; i >= 0; i--)
	 {
	    if (bsbuf[i] & 0200)
	    {
	       sysio[dev].iopos += i;
	       if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
		  goto BSR_ERR;
	       goto BSR_DONE;
	    }
	 }
      }
   }
   fseek (sysio[dev].iofd, 0L, SEEK_SET);

BSR_DONE:

#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "bsr   %s completed                      %10ld\n",
	    devstr (dev), sysio[dev].iopos);
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "bsr   %s flags = %08o\n",
	    devstr (dev), channel[ch].cflags);
#endif
   if (sysio[dev].iopos == 0)
      channel[ch].cflags |= CHAN_BOT;
   return;

BSR_ERR:
   sprintf (errview[0], "%s: bsr failed: %s\n", devstr(dev), strerror(ERRNO));

BSR_ERRMSG:
   cpuflags |= CPU_IOCHK;
#ifdef DEBUGIO
   fputs (errview[0], stderr);
#endif
   run = CPU_STOP;
   return;
}

/***********************************************************************
* bsf - Backspace file.
***********************************************************************/

void
bsf (int ch)
{
   int   dev;
   register int i;
   int n;

   dev = whatdev (ch);
   if (sysio[dev].iofd == NULL)
   {
      sprintf (errview[0], "%s: No file mounted\n", devstr (dev));
      goto BSF_ERRMSG;
   }
   if ((sysio[dev].iorw & IO_READ) == 0)
   {
      sprintf (errview[0], "%s(%s): File is write-only\n",
	       devstr (dev), sysio[dev].iostr);
      goto BSF_ERRMSG;
   }

#if defined(DEBUG7607) || defined(DEBUGBSF) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "bsf   %s                                %10ld\n",
	    devstr (dev), sysio[dev].iopos);
#endif

   sysio[dev].ioflags &= ~IO_ATEOF;
   if (sysio[dev].ioflags & IO_REALDEV)
   {
      struct mtget mt_status;

      if (ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_bsf) < 0)
         goto BSF_ERR;

      if (ioctl (fileno(sysio[dev].iofd), MTIOCGET, &mt_status) < 0)
         goto BSF_ERR;
      if (GMT_BOT(mt_status.mt_gstat))
	 sysio[dev].iopos = 0;
         
      goto BSF_DONE;;
   }

   if (sysio[dev].iopos == 0)
   {
      channel[ch].ccyc = 10;
   }
   sysio[dev].iopos = ftell (sysio[dev].iofd);
   while (sysio[dev].iopos > 0)
   {
      if (sysio[dev].ioflags & IO_SIMH)
      {
	 while (sysio[dev].iopos > 0)
	 {
	    sysio[dev].iopos -= 4;
	    if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
	       goto BSF_ERR;
	    n = tapereadint (sysio[dev].iofd);
	    if (n == 0)
	    {
	       if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
		  goto BSF_ERR;
	       goto BSF_DONE;
	    }
	    if (n > 0)
	       sysio[dev].iopos -= n + 4;
	    if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
	       goto BSF_ERR;
	 }
      }
      else
      {
	 if (sysio[dev].iopos > sizeof bsbuf)
	 {
	    n = sizeof bsbuf;
	    sysio[dev].iopos -= sizeof bsbuf;
	 }
	 else
	 {
	    n = sysio[dev].iopos;
	    sysio[dev].iopos = 0;
	 }
	 if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
	    goto BSF_ERR;
	 if (fread (bsbuf, 1, n, sysio[dev].iofd) != n)
	    goto BSF_ERR;
	 for (i = n - 1; i >= 0; i--)
	 {
	    if (bsbuf[i] == 0217 && (i == n - 1 || bsbuf[i + 1] & 0200))
	    {
	       sysio[dev].iopos += i;
	       if (fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET) < 0)
		  goto BSF_ERR;
	       goto BSF_DONE;
	    }
	 }
      }
   }
   fseek (sysio[dev].iofd, 0L, SEEK_SET);

BSF_DONE:

#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "bsf   %s completed                      %10ld\n",
	   devstr (dev), sysio[dev].iopos);
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "bsf   %s flags = %08o\n",
	    devstr (dev), channel[ch].cflags);
#endif
   if (sysio[dev].iopos == 0)
      channel[ch].cflags |= CHAN_BOT;
   return;

BSF_ERR:
   sprintf (errview[0], "%s: bsf failed: %s\n", devstr(dev), strerror(ERRNO));

BSF_ERRMSG:
   cpuflags |= CPU_IOCHK;
#ifdef DEBUGIO
   fputs (errview[0], stderr);
#endif
   run = CPU_STOP;
   return;
}

/***********************************************************************
* writerec - Writes a record to the attached device.
***********************************************************************/

void
writerec (int dev, uint8 *buf, int len)
{
   int n;

#ifdef DEBUGIO
   fprintf (stderr, "writerec: dev = %d, len = %d\n", dev, len);
#endif

   n = 0;
   if (sysio[dev].iofd == NULL)
   {
      if (dev == 2)
         return;
      sprintf (errview[0], "%s: No file mounted\n", devstr (dev));
      goto WRITE_ERRMSG;
   }
   if ((sysio[dev].iorw & IO_WRITE) == 0)
   {
      sprintf (errview[0], "%s(%s): File is read-only\n",
	       devstr (dev), sysio[dev].iostr);
      goto WRITE_ERRMSG;
   }

   if (dev > 2 && dev < 100) /* Tape */
   {
      if (sysio[dev].ioflags & IO_REALDEV)
      {
	 if ((n = write (fileno(sysio[dev].iofd), buf, len)) != len)
	       goto WRITE_ERR;
	 sysio[dev].iopos += len;
	 sysio[dev].iobyttrk = len;
      }
      else if (sysio[dev].ioflags & IO_SIMH)
      {
         tapewriteint (sysio[dev].iofd, len);
	 if (len)
	 {
	    if ((n = fwrite (buf, 1, len, sysio[dev].iofd)) != len)
	       goto WRITE_ERR;
	    tapewriteint (sysio[dev].iofd, len);
	 }
	 sysio[dev].iopos = ftell (sysio[dev].iofd);
      }
      else
      {
	 buf[0] |= 0200;
	 buf[len] = (char)0217;
	 if ((n = fwrite (buf, 1, len + 1, sysio[dev].iofd)) != len + 1)
	    goto WRITE_ERR;
	 sysio[dev].iopos += len;
	 if ((n = fseek (sysio[dev].iofd, sysio[dev].iopos, SEEK_SET)) < 0)
	    goto WRITE_ERR;
      }

      if (dev > 2)
      {
	 if (sysio[dev].iopos > TAPESIZE)
	    channel[sysio[dev].iochn].cflags |= CHAN_EOT;
	 if (sysio[dev].iopos > 0)
	    channel[sysio[dev].iochn].cflags &= ~CHAN_BOT;
      }
   }
   else /* DASD, Print, Punch */
   {
      if ((n = fwrite (buf, 1, len, sysio[dev].iofd)) != len)
	 goto WRITE_ERR;
   }
   return;

WRITE_ERR:
   if (n < 0)
      sprintf (errview[0], "%s(%s): Write error %d\n",
	       devstr (dev), sysio[dev].iostr, errno);
   else
      sprintf (errview[0],
	       "%s(%s): Out of disk space, wrote %d of %d bytes\n",
	       devstr(dev), sysio[dev].iostr, n, len);

WRITE_ERRMSG:
   cpuflags |= CPU_IOCHK;
#ifdef DEBUGIO
   fputs (errview[0], stderr);
#endif
   run = CPU_STOP;
   return;
}

/***********************************************************************
* dorewind - Rewind the attached device.
***********************************************************************/

void
dorewind (int dev)
{

#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "rew   %s                                %10ld\n",
	    devstr (dev), sysio[dev].iopos);
#endif

   if (dev < TAPEOFFSET)
   {
      fflush (sysio[dev].iofd);
      return;
   }
   if (sysio[dev].iofd == NULL)
   {
      sprintf (errview[0], "rewind: %s: dev %d is not open\n",
	       devstr(dev), dev);
      goto REW_ERRMSG;
   }

   sysio[dev].ioflags &= ~IO_ATEOF;
   sysio[dev].iopos = 0;
   channel[sysio[dev].iochn].cflags |= CHAN_BOT;

   if (sysio[dev].ioflags & IO_REALDEV)
   {
      if (ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_rew) < 0)
         goto REW_ERR;
   }
   else
   {
      if (fseek (sysio[dev].iofd, 0L, SEEK_SET) < 0)
         goto REW_ERR;
   }

#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "rew   %s completed                      %10ld\n",
	    devstr (dev), sysio[dev].iopos);
#endif
   return;

REW_ERR:
   sprintf (errview[0], "%s: rew failed: %s\n", devstr(dev), strerror(ERRNO));

REW_ERRMSG:
#ifdef DEBUGIO
   fputs (errview[0], stderr);
#endif
   cpuflags |= CPU_IOCHK;
   run = CPU_STOP;
   return;
}

/***********************************************************************
* setdensity - Unload the attached device.
***********************************************************************/

void
setdensity (int ch)
{
   int dev;
   int cunit;

   dev = whatdev(ch);
   cunit = channel[ch].cunit;

#if defined(DEBUG7607) || defined(DEBUG7607RW) || defined(DEBUGRUN)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "sdn%s %s                                %10ld\n",
	    (cunit & 020) ? "hi" : "lo", devstr (dev), sysio[dev].iopos);
   fprintf (stderr, "   cunit = %05o\n", cunit);
#endif

   if (dev < TAPEOFFSET)
   {
      return;
   }
   if (sysio[dev].iofd == NULL)
   {
      return;
   }

   if (sysio[dev].ioflags & IO_REALDEV)
   {
      struct mtop mt_setden = { MTSETDENSITY, 2 }; 

      if (cunit & 020)
	 mt_setden.mt_count = 3; /* High density - 6250 */
      else
	 mt_setden.mt_count = 2; /* Low density - 1600 */

      ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_setden);
   }
}

/***********************************************************************
* unload - Unload the attached device.
***********************************************************************/

void
unload (int dev)
{
#if defined(DEBUG7607) || defined(DEBUG7607RW) || defined(DEBUGRUN)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "rwunl %s                                %10ld\n",
	    devstr (dev), sysio[dev].iopos);
#endif

   if (dev < TAPEOFFSET)
   {
      fflush (sysio[dev].iofd);
      return;
   }
   if (sysio[dev].iofd == NULL)
   {
      return;
   }

   if (sysio[dev].ioflags & IO_REALDEV)
   {
      ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_rew);
      ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_unload);
   }

   fclose (sysio[dev].iofd);
   sysio[dev].iofd = NULL;
}

/***********************************************************************
* dismount - Dismount the sttached device.
***********************************************************************/

int
dismount (char *s)
{
   int type;
   int dev;

   if (*s == '\n' || *s == '\0')
      return 1;
   if (parsedev (s, &dev, &type) == NULL)
      return -1;

   if (type < 2)
      unload (dev);
   return 0;
}

/***********************************************************************
* wef - Write EOF on attached device.
***********************************************************************/

void
wef (int ch)
{
   int dev;
   uint8 tapemark[2];

   dev = whatdev (ch);
#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "wef   %s                                %10ld\n",
	    devstr (dev), sysio[dev].iopos);
#endif

   if (dev < TAPEOFFSET)
      return;

   if (sysio[dev].ioflags & IO_REALDEV)
   {
      ioctl (fileno(sysio[dev].iofd), MTIOCTOP, &mt_weof);
      sysio[dev].iopos++;
   }
   else if (sysio[dev].ioflags & IO_SIMH)
   {
      writerec (dev, tapemark, 0);
   }
   else
   {
      tapemark[0] = 0217;
      writerec (dev, tapemark, 1);
   }

   sysio[dev].ioflags |= IO_ATEOF;
#if defined(DEBUG7607) || defined(DEBUG7607RW)
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "wef   %s completed                      %10ld\n",
	    devstr (dev), sysio[dev].iopos);
   fprintf (stderr, "%d: %05o: ", inst_count, ic-1);
   fprintf (stderr, "wef   %s flags = %08o\n",
	    devstr (dev), channel[ch].cflags);
#endif
}


/***********************************************************************
* bincard - Process a binary card image.
***********************************************************************/

void
bincard (uint8 *cnvbuf, uint16 *crcol)
{
   register int i;

   for (i = 0; i < 80; i++)
      crcol[i] = ((uint16 )(cnvbuf[2*i] & 077) << 6) |
            (cnvbuf[2*i + 1] & 077);
}

/***********************************************************************
* cardbin - Translate card image to binary.
***********************************************************************/

void
cardbin (uint16 *crcol, uint8 *cnvbuf)
{
   register int i;

   for (i = 0; i < 80; i++)
   {
      cnvbuf[2*i] = oddpar[(crcol[i] >> 6) & 077];
      cnvbuf[2*i + 1] = oddpar[crcol[i] & 077];
   }
   cnvbuf[0] |= 0200;
}

/***********************************************************************
* cardbcd - Translate card image to BCD.
***********************************************************************/

void
cardbcd (uint16 *cbuf, uint8 *bbuf, int len)
{
   register int num;
   register int row;

   for (; len; len--)
   {
      row = 00001;
      for (num = 10; --num; )
      {
         if (*cbuf & row)
            break;
         row <<= 1;
      }
      if (num == 8 && (*cbuf & 00174) != 0)
      {
         row = 00004;
         for (num = 16; --num > 10; )
	 {
            if (*cbuf & row)
               break;
            row <<= 1;
         }
      }
      else if (num == 0 && *cbuf & 01000)
         num = 10;
      if ((*cbuf & 01000) && num != 10)
         num |= 060;
      else if (*cbuf & 02000)
         num |= 040;
      else if (*cbuf & 04000)
         num |= 020;
      else if (num == 10)
         num = 0;
      else if (num == 0)
         num = 060;
      cbuf++;
      *bbuf++ = evenpar[num];
   }
}

/***********************************************************************
* logstr - Log a message.
***********************************************************************/

void
logstr (char *s, int spracode)
{
   if (spracode == 1)
      fputs ("\n\f", logfd);
   else if (spracode == 4)
      fputs ("\n\n\n", logfd);
   else if (spracode != 9)
      fputc ('\n', logfd);
   fprintf (logfd, "%s", s);
}

/***********************************************************************
* ioinit - Initialize device tables.
***********************************************************************/

void
ioinit ()
{
   register int i;

   for (i = 0; i < IODEV; i++)
   {
      memset (&sysio[i], '\0', sizeof (IO_t));
   }
   sysio[0].iorw = IO_READ;
   sysio[1].iorw = IO_WRITE;
   sysio[2].iorw = IO_WRITE;
   logfd = fopen ("printlog.lst", "w");

   for (i = 0; i < ERRVIEWLINENUM; i++)
      errview[i][0] = '\0';
}

/***********************************************************************
* iofin - Finish up (close) attached devices.
***********************************************************************/

void
iofin ()
{
   register int i;

   for (i = 0; i < IODEV; i++)
   {
      if (sysio[i].iofd != NULL && sysio[i].iofd != IO_COMMFD)
         fclose (sysio[i].iofd);
      sysio[i].iofd = NULL;
   }

   fputc ('\n', logfd);
   fclose (logfd);
}
