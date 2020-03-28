/***********************************************************************
*
* console.c - IBM 7090 emulator console routines.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   02/07/05   DGP   Added command file.
*   12/18/05   DGP   Added fflush for MINGW.
*   12/07/06   DGP   Added non-panel mode display of Keys and DS/DN.
*                    And added display of all the other registers.
*   12/08/06   DGP   Revamped commands.
*   02/19/08   DGP   Added 'cuu' to 'lt' and addr to 'es' commands.
*   02/29/08   DGP   Changed to use bit mask flags.
*   08/17/11   DGP   Handle SPRA printer codes.
*   
***********************************************************************/

#define EXTERN extern

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "sysdef.h"
#include "regs.h"
#include "trace.h"
#include "lights.h"
#include "io.h"
#include "s709.h"
#include "chan7607.h"
#include "chan7909.h"
#include "console.h"
#include "screen.h"
#include "binloader.h"

long total_cycles = 0;

extern int prtviewlen;
extern int panelmode;
extern int outputline;
extern int traceinsts;
extern int traceuser;
extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
extern char prtview[PRTVIEWLINENUM][PRTVIEWLINELEN+2];
extern char tonative[64];

static char iline[256];
static char *ibp = 0;
static char *lastcmd = 0;
static int  loadaddr = 0;

#define VIEW_CLEAR   -1
#define VIEW_NONE    0
#define VIEW_HELP    1
#define VIEW_MOUNT   2
#define VIEW_PRINT   3
#define VIEW_TRACE   4
#define VIEW_ERROR   5

int view = VIEW_NONE;
int newview = VIEW_NONE;

static int ctlc = FALSE;

#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
static t_uint64 RCHx[8] =
{
   0054000000000I64, 0454000000000I64, 0054100000000I64, 0454100000000I64, 
   0054200000000I64, 0454200000000I64, 0054300000000I64, 0454300000000I64 
};
static t_uint64 LCHx[8] =
{
   0054400000000I64, 0454400000000I64, 0054500000000I64, 0454500000000I64, 
   0054600000000I64, 0454600000000I64, 0054700000000I64, 0454700000000I64 
};
#else
static t_uint64 RCHx[8] =
{
   0054000000000ULL, 0454000000000ULL, 0054100000000ULL, 0454100000000ULL, 
   0054200000000ULL, 0454200000000ULL, 0054300000000ULL, 0454300000000ULL 
};
static t_uint64 LCHx[8] =
{
   0054400000000ULL, 0454400000000ULL, 0054500000000ULL, 0454500000000ULL, 
   0054600000000ULL, 0454600000000ULL, 0054700000000ULL, 0454700000000ULL 
};
#endif
#else
static uint8 RCHx[8] = 
{
   0001, 0201, 0001, 0201, 0001, 0201, 0001, 0201
};
static uint32 RCHxl[8] = 
{
   014000000000, 014000000000, 014100000000, 014100000000,
   014200000000, 014200000000, 014300000000, 014300000000
};

static uint8 LCHx[8] = 
{
   0001, 0201, 0001, 0201, 0001, 0201, 0001, 0201
};
static uint32 LCHxl[8] = 
{
   014400000000, 014400000000, 014500000000, 014500000000,
   014600000000, 014600000000, 014700000000, 014700000000
};
#endif

/***********************************************************************
* errorview - Display the error/help messages in the panel.
***********************************************************************/

void
errorview ()
{
   int32 i;
   int line;

   if (panelmode)
   {
      screenposition (outputline,1);
      clearline ();
   }
   printf ("\n");

   line = 0;
   for (i = 0; errview[i][0] && (i < ERRVIEWLINENUM); i++)
   {
      if (panelmode)
	 clearline ();
      printf ("%s", errview[i]);
      errview[i][0] = '\0';
      line++;
      if (panelmode && line == prtviewlen)
      {
	 int c;

	 line = 0;
	 printf ("press enter to continue:");
	 c = fgetc (stdin);
	 screenposition (outputline+prtviewlen,1);
	 clearline ();
	 if (c == 'q') break;
	 screenposition (outputline,1);
	 clearline ();
	 printf ("\n");
      }
   }
   if (line && panelmode)
      for (; line < prtviewlen; line++) { clearline (); printf ("\n"); }
   fflush (stdout);
}

/***********************************************************************
* sigintr - Process user interrupt.
***********************************************************************/

void
sigintr (int sig)
{
   run = CPU_STOP;
   atbreak = TRUE;
   ctlc = TRUE;
   signal (SIGINT, sigintr);
}

/***********************************************************************
* check_intr - Check for user interrupt.
***********************************************************************/

void
check_intr ()
{
#if defined(WIN32)
   if (kbhit ())
   {
      cpuflags &= ~CPU_AUTO;
      run = CPU_STOP;
      ctlc = TRUE;
   }
#endif
}


/***********************************************************************
* keys - Position on screen for keyed input.
***********************************************************************/

static void
keys ()
{
   if (panelmode)
   {
      screenposition (windowlen, 1);
      clearline ();
   }
   else
      printf ("\n");
   fflush (stdout);
}

/***********************************************************************
* setssw - Set the Sense Switch.
***********************************************************************/

static void
setssw (int c)
{

   switch (c)
   {

   case 1:
      ssw ^= 040;
      break;

   case 2:
      ssw ^= 020;
      break;

   case 3:
      ssw ^= 010;
      break;

   case 4:
      ssw ^= 004;
      break;

   case 5:
      ssw ^= 002;
      break;

   case 6:
      ssw ^= 001;
      break;
   }
}

/***********************************************************************
* reset - System reset.
***********************************************************************/

static void
reset ()
{
   int32 i;

#ifdef USE64
   sr = 0;
   ac = 0;
   mq = 0;
#else
   srh = 0;
   srl = 0;
   ach = 0;
   acl = 0;
   mqh = 0;
   mql = 0;
#endif
   ic = 0;
   for (i = 0; i < 8; i++)
      xr[i] = 0;

   run = CPU_STOP;

   sl = 0;
   op = 0;
   spill = 0;
   cycle_count = 0;
   single_cycle = 0;
   prog_reloc = 0;
   prog_base = 0;
   prog_limit = 0;
   bcoredata = 0;
   bcoreinst = 0;

   cpuflags = CPU_AUTO | CPU_MULTITAG | CPU_FPTRAP;

   for (i = 0; i < numchan; i++)
   {
      int j;

      if (sysio[i*10+COMMOFFSET].iofd == NULL)
      {
	 memset (&channel[i], '\0', sizeof (Channel_t));
	 for (j = i*10+DASDOFFSET; j < i*10+DASDOFFSET+MAXDASD; j++)
	 {
	    if (sysio[j].iofd != NULL)
	    {
	       int unit;

	       channel[i].ctype = CHAN_7909;
	       for (unit = 0; unit < MAXDASD; unit++)
		  channel[i].devices.dasd[unit].fcyl = 9999;
		  channel[i].devices.dasd[unit].fmod = 9999;
	    }
	 }
      }
   }

   trap_enb = 0;
   chan_in_op = 0;
   view = VIEW_NONE;
}

/***********************************************************************
* entershort - Get a short, 16 bit, number from the input line.
***********************************************************************/

static uint16
entershort (char *bp)
{
   int num;

   sscanf (bp, "%o", &num);
   return ((uint16)(num & 077777));
}

/***********************************************************************
* enterlong - Get a long, 36 bit, number from the input line.
***********************************************************************/

#ifdef USE64
static void
enterlong (char *bp, t_uint64 *addr)
{
   int sign = FALSE;

   if (*bp == '-')
   {
      sign = TRUE;
      bp++;
   }
   else if (*bp == '+') bp++;

#ifdef WIN32
   sscanf (bp, "%I64o", addr);
#else
   sscanf (bp, "%llo", addr);
#endif
   if (sign) *addr |= SIGN;
}
#else
static void
enterlong (char *bp, uint8 *addrh, uint32 *addrl)
{
   t_uint64 temp;
   int sign = FALSE;

   if (*bp == '-')
   {
      sign = TRUE;
      bp++;
   }
   else if (*bp == '+') bp++;

#ifdef WIN32
   sscanf (bp, "%I64o", &temp);
#else
   sscanf (bp, "%llo", &temp);
#endif
   *addrh = (sign ? SIGN : 0) | (uint8) ((temp >> 32) & HMSK);
   *addrl = (uint32)(temp & 037777777777);
}
#endif

/***********************************************************************
* octdump - Octal dump routine.
***********************************************************************/

static void
octdump (FILE *file, uint32 begaddr, uint32 endaddr)
{
   int i, j;
   int dmpmax;

   dmpmax = 4;

   for (i = begaddr; i < endaddr;)
   {
      fprintf (file, "%05o  ", i);

      for (j = 0; j < dmpmax; j++)
      {
#ifdef USE64
	 dsr = mem[i];
	 fprintf (file, "%c%012llo ",
		 (dsr & SIGN)? '-' : '+', dsr & MAGMASK);
#else
	 uint32 tch;

	 dsrh = memh[i]; dsrl = meml[i];
	 tch = ((dsrh & SIGN) >> 2) | ((dsrh & HMSK) << 2) | (dsrl >> 30);
	 fprintf (file, "%c%02o%010lo ",
		 (dsrh & SIGN)? '-' : '+',
		 tch & 037,
		 (unsigned long)dsrl & 07777777777);
#endif
	  i++;
       }
      fprintf (file, "\n");
   }
}

/***********************************************************************
* docommand - Process user commands.
***********************************************************************/

static int
docommand (int inscript)
{
#ifdef USE64
   t_uint64 lclreg;
#else
   uint8 lclregh; uint32 lclregl;
   uint32 tch;
#endif
   FILE *fd;
   int retval;
   int ch;
   int start;
   uint32 cnv;
   uint32 begaddr, endaddr;
   int32 kp;
   uint16 c;
   char regname[10];

   start = TRUE;
   c = *ibp++;
   switch (c)
   {

   case 'a':
      if (cpuflags & CPU_AUTO)
	 cpuflags &= ~CPU_AUTO;
      else
	 cpuflags |= CPU_AUTO;
      return (1);

   case 'b':
      while (*ibp == ' ') ibp++;
      c = *ibp;
      if (c >= '0' && c <= '7')
      {
	 addrstop = TRUE;
	 stopic = 0;
	 while (c >= '0' && c <= '7')
	 {
	    stopic = (stopic << 3) + (c - '0');
	    c = *++ibp;
	 }
	 newview = VIEW_NONE;
      }
      else
      {
	 addrstop = FALSE;
	 newview = VIEW_NONE;
      }
      return (1);

   case 'c':
      clear ();
      return (1);

   case 'd':
      c = *ibp++;
      switch (c)
      {
      case 'a':
         strcpy (regname, "AC");
#ifdef USE64
         lclreg = ((ac & ACSIGN) ? SIGN : 0) | (ac & DATAMASK);
#else
         lclregh = ach; lclregl = acl;
#endif

      DISPLAY_REGISTER:
#ifdef USE64
	 printf ("%s: %c%012llo\n",
	         regname,
	         (lclreg & SIGN)? '-' : '+', lclreg & MAGMASK);
#else
	 printf ("%s: %c%02o%010lo\n",
	         regname,
	         (lclregh & SIGN)? '-' : '+',
	         ((lclregh & HMSK) << 2) | (short)(lclregl >> 30),
	         (unsigned long)lclregl & 07777777777);
#endif
	 break;

      case 'i':
         strcpy (regname, "SI");
#ifdef USE64
         lclreg = si;
#else
         lclregh = sih; lclregl = sil;
#endif
	 goto DISPLAY_REGISTER;

      case 'k':
         strcpy (regname, "KEYS");
#ifdef USE64
         lclreg = ky;
#else
         lclregh = kyh; lclregl = kyl;
#endif
	 goto DISPLAY_REGISTER;

      case 'l':
	 printf ("SL %o%o%o%o\n",
	         (sl >> 3) & 1,
	         (sl >> 2) & 1,
	         (sl >> 1) & 1,
	         sl & 1);
         return (1);

      case 'm':
         strcpy (regname, "MQ");
#ifdef USE64
         lclreg = mq;
#else
         lclregh = mqh; lclregl = mql;
#endif
	 goto DISPLAY_REGISTER;

      case 'n':
	 dsra = (dsra + 1) & (cpumode == 7096 ? (BCORE|ADDRMASK) : ADDRMASK);
#ifdef USE64
         dsr = mem[dsra];
#else
         dsrh = memh[dsra]; dsrl = meml[dsra];
#endif
         lastcmd = "dn";
         goto DISPLAY_STORAGE;

      case 'p':
         strcpy (regname, "PC");
         c = ic;

      DISPLAY_SHORT:
         printf ("%s: %05o\n", regname, c);
	 break;
         
      case 'r':
         strcpy (regname, "SR");
#ifdef USE64
         lclreg = sr;
#else
         lclregh = srh; lclregl = srl;
#endif
	 goto DISPLAY_REGISTER;

      case 's':
         while (*ibp == ' ') ibp++;
         sscanf (ibp, "%o", &cnv);
         dsra = cnv & (cpumode == 7096 ? (BCORE|ADDRMASK) : ADDRMASK);
         ibp = 0;
#ifdef USE64
         dsr = mem[dsra];
#else
         dsrh = memh[dsra]; dsrl = meml[dsra];
#endif
         lastcmd = "dn";

      DISPLAY_STORAGE:
	 if (!panelmode && !inscript)
	 {
#ifdef USE64
	    printf ("%05o: %c%012llo",
		    dsra,
		    (dsr & SIGN)? '-' : '+', dsr & MAGMASK);
	    printf (" '%c%c%c%c%c%c'\n",
                    tonative[(dsr >> 30) & 077],
                    tonative[(dsr >> 24) & 077],
                    tonative[(dsr >> 18) & 077],
                    tonative[(dsr >> 12) & 077],
                    tonative[(dsr >>  6) & 077],
                    tonative[ dsr        & 077]);
#else
            tch = ((dsrh & SIGN) >> 2) | ((dsrh & HMSK) << 2) | (dsrl >> 30);
	    printf ("%05o: %c%02o%010lo",
		    dsra,
		    (dsrh & SIGN)? '-' : '+',
		    tch & 037,
		    (unsigned long)dsrl & 07777777777);
	    printf ("  '%c%c%c%c%c%c'\n",
                    tonative[ tch         & 077],
                    tonative[(dsrl >> 24) & 077],
                    tonative[(dsrl >> 18) & 077],
                    tonative[(dsrl >> 12) & 077],
                    tonative[(dsrl >>  6) & 077],
                    tonative[ dsrl        & 077]);
#endif
	 }
         return (1);

      case 'w':
	 printf ("SSW %o%o%o%o%o%o\n",
	         (ssw >> 5) & 1,
	         (ssw >> 4) & 1,
	         (ssw >> 3) & 1,
	         (ssw >> 2) & 1,
	         (ssw >> 1) & 1,
	          ssw & 1);
         return (1);

      case 'x':
	 c = *ibp++;
	 if (c > '0' && c < '8')
	 {
	    sprintf (regname, "XR%c", c);
	    c -= '0';
	    c = xr[c];
	    goto DISPLAY_SHORT;
	 }

      default:
         return (-1);
      }
      return (1);

   case 'D':
      while (*ibp && *ibp == ' ') ibp++;
      if (*ibp)
      {
	 sscanf (ibp, "%o", &begaddr);
	 while (*ibp && *ibp != ' ') ibp++;
	 while (*ibp && *ibp == ' ') ibp++;
	 endaddr = begaddr + 1;
	 if (*ibp)
	 {
	    sscanf (ibp, "%o", &endaddr);
	    while (*ibp && *ibp != ' ') ibp++;
	    while (*ibp && *ibp == ' ') ibp++;
	 }
	 if (*ibp && ((fd = fopen (ibp, "w")) != NULL))
	 {
	    octdump (fd, begaddr, endaddr);
	    fclose (fd);
	 }
	 else
	 {
	    octdump (stdout, begaddr, endaddr);
	 }
      }
      ibp = 0;
      return (1);

   case 'e':
      c = *ibp++;
      switch (c)
      {

      case 'a':
         while (*ibp == ' ') ibp++;
#ifdef USE64
	 enterlong (ibp, &ac);
#else
	 enterlong (ibp, &ach, &acl);
#endif
	 ibp = 0;
         return (1);

      case 'i':
         while (*ibp == ' ') ibp++;
#ifdef USE64
	 enterlong (ibp, &si);
#else
	 enterlong (ibp, &sih, &sil);
#endif
	 ibp = 0;
         return (1);

      case 'k':
         while (*ibp == ' ') ibp++;
#ifdef USE64
	 enterlong (ibp, &ky);
#else
	 enterlong (ibp, &kyh, &kyl);
#endif
	 ibp = 0;
         return (1);

      case 'm':
         while (*ibp == ' ') ibp++;
#ifdef USE64
	 enterlong (ibp, &mq);
#else
	 enterlong (ibp, &mqh, &mql);
#endif
	 ibp = 0;
         return (1);

      case 'n':
         while (*ibp == ' ') ibp++;
	 dsra = (dsra + 1) & ADDRMASK;
#ifdef USE64
	 enterlong (ibp, &mem[dsra]);
#else
	 enterlong (ibp, &memh[dsra], &meml[dsra]);
#endif
	 ibp = 0;
	 return (1);

      case 'p':
         while (*ibp == ' ') ibp++;
	 ic = entershort (ibp);
#ifdef USE64
         sr = mem[ic];
#else
         srh = memh[ic];
         srl = meml[ic];
#endif
	 ibp = 0;
	 return (1);

      case 'r':
         while (*ibp == ' ') ibp++;
         run = CPU_EXEC;
#ifdef USE64
	 enterlong (ibp, &sr);
#else
	 enterlong (ibp, &srh, &srl);
#endif
         lastcmd = "st";
         break;

      case 's':
         while (*ibp == ' ') ibp++;
	 dsra = entershort (ibp);
	 while (*ibp != ' ') ibp++;
         while (*ibp == ' ') ibp++;
#ifdef USE64
	 enterlong (ibp, &mem[dsra]);
#else
	 enterlong (ibp, &memh[dsra], &meml[dsra]);
#endif
	 ibp = 0;
	 return (1);

      case 'x':
         c = *ibp++;
         while (*ibp == ' ') ibp++;
	 if (c > '0' && c < '8')
	 {
	    c -= '0';
	    xr[c] = entershort (ibp);
	    ibp = 0;
	    return (1);
	 }
	 return (-1);

      default:
         return (-1);
      }
      break;

   case '?':
   case 'h':
      help ();
      newview = VIEW_HELP;
      return (2);

   case 'l':
      c = *ibp++;
      switch (c)
      {

      case 'c':
         if (!(cpuflags & CPU_AUTO))
            break;
         reset ();
         channel[0].cunit = 01321;
	 ch = 0;
	 atbreak = FALSE;
         goto load_key;

      case 'd':
         while (*ibp == ' ') ibp++;
         reset ();
         if ((retval = boot_7909 (ibp)) < 0)
         {
            ibp = 0;
            return (1);
         }
	 ic = retval & ADDRMASK;
         run = CPU_RUN;
	 atbreak = FALSE;
	 ibp = 0;
         goto loaded;

      case 'f':
	 if (*ibp == 'n' || *ibp == 'N')
	 {
	    start = FALSE;
	    ibp++;
	 }
         while (*ibp == ' ') ibp++;
         reset ();
         loadaddr = 0;
         while (isdigit (*ibp)) 
            loadaddr = (loadaddr << 3) | (*ibp++ - '0');
         if (loadaddr == 0) loadaddr = -1;
         while (*ibp == ' ') ibp++;
         if (binloader (ibp, loadaddr) < 0)
         {
            newview = VIEW_ERROR;
            ibp = 0;
            return (1);
         }
	 ibp = 0;
	 newview = VIEW_NONE;
	 if (!start)
	 {
	    lastcmd = "st";
	    break;
	 }
         if (!(cpuflags & CPU_AUTO))
            break;
         run = CPU_RUN;
	 atbreak = FALSE;
         goto loaded;

      case 't':
      case '\0':
         while (*ibp == ' ') ibp++;
         if (!(cpuflags & CPU_AUTO))
            break;
         reset ();
	 channel[0].cunit = 01221;
	 ch = 0;
	 if (islower (*ibp)) *ibp = toupper (*ibp);
	 if (*ibp >= 'A' && *ibp <= 'H' && isdigit (*(ibp+1)))
	 {
	    int unit;
	    ch = *ibp++ - 'A';
	    unit = *ibp++ - '0';
	    if (isdigit (*ibp))
	       unit = (unit * 10) + (*ibp - '0');
	    channel[ch].cunit = ((ch + 1) << 9) | 0220 | unit;
	 }

      load_key:
         ic = 01000;
#ifdef USE64
#if defined(WIN32) && !defined(MINGW)
	 mem[ic+0] = 0076200000000I64 | channel[ch].cunit;	/* RDS */
	 mem[ic+1] = 0000000001004I64 | RCHx[ch];		/* RCHx *+3 */
	 mem[ic+2] = 0000000000000I64 | LCHx[ch];		/* LCHx 0 */
	 mem[ic+3] = 0002100000001I64;				/* TTR 1 */
	 mem[ic+4] = 0500003000000I64;			 	/* IOCT 0,,3 */
#else
	 mem[ic+0] = 0076200000000ULL | channel[ch].cunit;	/* RDS */
	 mem[ic+1] = 0000000001004ULL | RCHx[ch];		/* RCHx *+3 */
	 mem[ic+2] = 0000000000000ULL | LCHx[ch];		/* LCHx 0 */
	 mem[ic+3] = 0002100000001ULL;				/* TTR 1 */
	 mem[ic+4] = 0500003000000ULL;			 	/* IOCT 0,,3 */
#endif
#else
	 memh[ic+0] = 0001;      meml[ic+0] = 036200000000 | channel[ch].cunit;
	 memh[ic+1] = RCHx[ch];  meml[ic+1] = 000000001004 | RCHxl[ch];
	 memh[ic+2] = LCHx[ch];  meml[ic+2] = 000000000000 | LCHxl[ch];
	 memh[ic+3] = 0000;      meml[ic+3] = 002100000001;
	 memh[ic+4] = 0202;      meml[ic+4] = 000003000000;
#endif
	 run = CPU_RUN;
	 atbreak = FALSE;
	 ibp = 0;

   loaded:
         lastcmd = "st";
         return (0);

      default:
         return (-1);
      }
      break;

   case 'm':
      while (*ibp == ' ') ibp++;
      if (isalpha (*ibp))
      {
	 if (mount (ibp) < 0)
	 {
	    newview = VIEW_ERROR;
	 }
      }
      else
      {
         listmount ();
      }
      ibp = 0;
      return (2);

   case 'p':
      while (*ibp == ' ') ibp++;
      if (isdigit (*ibp))
      {
	 panelmode = TRUE;
	 if ((windowlen = atoi (ibp)) < 25)
	    windowlen = 25;
	 prtviewlen = windowlen - outputline - 1;
	 if (prtviewlen <= 0) prtviewlen = 1;
      }
      else
	 panelmode = panelmode ? FALSE : TRUE;
      if (panelmode)
         clearscreen ();
      break;

   case 'P':
      panelmode = TRUE;
      lights ();
      panelmode = FALSE;
      break;

   case 'q':
      lights ();
      iofin ();
      printf ("\n");
      if (panelmode)
         screenposition (windowlen, 1);
      fflush (stdout);
      exit (0);

   case 'r':
      reset ();
      return (1);

   case 's':
      c = *ibp++;
      switch (c)
      {
      case 'c':
         single_cycle = 1 - single_cycle;
         lastcmd = "sc";
	 atbreak = FALSE;
         return (1);

      case 's':
         run = CPU_CYCLE;
         lastcmd = "ss";
	 atbreak = FALSE;
         break;

      case 't':
	 pview (0);
         run = CPU_RUN;
	 atbreak = FALSE;
         lastcmd = "st";
         break;

      case 'w':
         c = *ibp++;
         if (c >= '1' && c <= '6')
	 {
            setssw (c - '0');
            return (1);
         }
	 return (-1);

      default:
         return (-1);
      }
      break;

   case 't':
      {
	 c = *ibp++;
	 if (c == 'c')
	 {
	    trace_close();
	    traceinsts = FALSE;
	    traceuser = FALSE;
	 }
	 else if (c == 'f')
	 {
	    if (*ibp == 'u')
	    {
	       traceuser = TRUE;
	       ibp++;
	    }
	    if (trace_open (ibp) == 0)
	    {
	       traceinsts = TRUE;
	       ibp = 0;
	    }
	 }
	 else if (c == 'l')
	 {
	    long_trace ();
	    return (2);
	 }
	 else
	 {
	    trace ();
	    return (2);
	 }
      }
      return (1);

   case 'u':
      while (*ibp == ' ') ibp++;
      if (dismount (ibp) < 0)
      {
	 newview = VIEW_ERROR;
	 ibp = 0;
	 return (1);
      }
      ibp = 0;
      return (1);

   case '-':
#ifdef USE64
      ky |= SIGN;
#else
      kyh |= SIGN;
#endif
      c = *ibp++;
      goto setkey;

   case '+':
      c = *ibp++;
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '.':
#ifdef USE64
      ky &= ~SIGN;
#else
      kyh &= ~SIGN;
#endif
   setkey:
      kp = 11;
      for (;;)
      {
         if (c >= '0' && c <= '7')
	 {
            uint32 v;

            v = c - '0';
#ifdef USE64
            if (kp < 0)
	       ky = ((ky << 3) & DATAMASK) | v;
	    else
	    {
	       if (kp == 11 && (v & 04))
	       {
	          v &= 03;
		  ky |= SIGN;
	       }
               ky = (ky & SIGN) | (ky & ~(03 << (3*kp))) | (v << (3*kp));
	    }
#else
            if (kp < 0)
	    {
               kyl =  (kyl & 037777700000) |
                     ((kyl & 000000007777) << 3) | v;
            }
	    else if (kp == 11)
	    {
               kyh = (kyh & (SIGN | 001)) | (v << 1);
            }
	    else if (kp == 10)
	    {
               kyh = (kyh & (SIGN | 006)) | (v >> 2);
               kyl = (kyl & 007777777777) | (v << 30);
            }
	    else
	    {
               kyl = (kyl & ~(07L << (3*kp))) |
                     (v << (3*kp));
            }
#endif
         }
	 else if (c == '.')
	 {
#ifdef USE64
            if (kp == 11)
               ky = ky & SIGN;
	    else
               ky = (ky & ((DATAMASK << (3*(kp+1))) & DATAMASK));
#else
            if (kp == 11)
	    {
               kyh = (kyh & SIGN);
               kyl = 0;
            }
	    else if (kp == 10)
	    {
               kyh = (kyh & (SIGN | 006));
               kyl = 0;
            }
	    else
	    {
               kyl = (kyl &
                      (037777777777 << (3*(kp+1))));
            }
#endif
            kp = -1;
         }
	 else
	 {
            ibp--;
            break;
         }
         if (kp == 0)
            break;
         kp--;
         c = *ibp++;
      }

   case 'w':
      while (*ibp == ' ') ibp++;
      c = *ibp;
      if (c >= '0' && c <= '7')
      {
	 watchstop = TRUE;
	 watchloc = 0;
	 while (c >= '0' && c <= '7')
	 {
	    watchloc = (watchloc << 3) + (c - '0');
	    c = *++ibp;
	 }
	 newview = VIEW_NONE;
      }
      else
      {
	 watchstop = FALSE;
	 newview = VIEW_NONE;
      }
      return (1);

   case 0:
      return (1);

   default:
      newview = VIEW_HELP;
      return (-1);

   }
   return (0);

}


/***********************************************************************
* commands - Read commands from the command file.
***********************************************************************/

int 
commands (FILE *cmdfd)
{

   while ((ibp = fgets (iline, sizeof (iline), cmdfd)) != NULL)
   {
      if (ctlc) break;
      if (iline[0] == '#') continue;
      if (docommand (TRUE) < 0)
      {
         fprintf (stderr, "Syntax error in command file: %s\n", iline);
	 return (-1);
      }
      return (0);
   }
   return (-1);
}

/***********************************************************************
* console - Run the console.
***********************************************************************/

void
console ()
{
   int i;

#if defined(UNIX)
   signal (SIGINT, sigintr);
#endif

redraw:
   if (newview == VIEW_NONE)
   {
      if (errview[0][0])
         newview = VIEW_ERROR;
      else if (prtview[prtviewlen][0])
         newview = VIEW_PRINT;
      else if (addrstop || watchstop)
         newview = VIEW_NONE;
      else
         newview = VIEW_NONE;
   }

   if (view != newview)
      view = newview;

   lights ();

   switch (view)
   {

   case VIEW_HELP:
      newview = VIEW_NONE;
      break;

   case VIEW_TRACE:
      trace ();
      break;

   case VIEW_ERROR:
      errorview ();
      newview = VIEW_NONE;
      break;

   case VIEW_PRINT:
      pview (0);

   default: ;
   }

   keys ();

   total_cycles += cycle_count;
   cycle_count = 0;
   next_lights = NEXTLIGHTS;

   if (ibp == 0 || *ibp == 0)
   {
      if (panelmode)
	 screenposition (windowlen,1);

      if (lastcmd != 0)
	 printf (" %s                                                      \r.",
	         lastcmd);
      else
	 printf (
	       "                                                         \r.");
      fflush (stdout);
      ibp = (char *)fgets (iline, sizeof (iline), stdin);
      *(strchr (iline,'\n')) = '\0';
      if (ibp == 0)
      {
         iofin ();
         exit (0);
      }
      if (*ibp == 0)
      {
         ibp = lastcmd;
         if (ibp == 0)
            goto redraw;
      }
      else
         lastcmd = 0;
   }

   if ((i = docommand (FALSE)) < 0) goto syntax;
   if (i == 1) goto redraw;
   if (i == 2)
   {
      newview = VIEW_ERROR;
      goto redraw;
   }

   lights ();
   if (single_cycle)
   {
      next_steal = 0;
   }
   else
   {
      if (next_steal > NEXTLIGHTS)
         next_steal = NEXTLIGHTS;
   }
   return;

syntax:
   printf ("%c", 7);
   fflush (stdout);
   ibp = 0;
   goto redraw;
}

/***********************************************************************
* clear - Clear out the system.
***********************************************************************/

void
clear ()
{
   uint32 i;

   for (i = 0; i <= MEMLIMIT; i++)
   {
#ifdef USE64
      mem[i] = 0;
#else
      memh[i] = 0;
      meml[i] = 0;
#endif
   }
#ifdef USE64
   si = 0;
#else
   sih = 0;
   sil = 0;
#endif
   reset ();
   memset (prtview, '\0', sizeof (prtview));
   memset (errview, '\0', sizeof (errview));
}
