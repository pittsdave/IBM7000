/***********************************************************************
*
* binloader.c - IBM 7090 emulator binary loader routines for ASM7090
*            object files.
*
* Changes:
*   10/20/03   DGP   Original.
*   12/28/04   DGP   Changed for new object formats.
*   02/14/05   DGP   Detect IBSYSSYM for EOF.
*   02/19/08   DGP   Set ic on entry/xfer.
*   02/29/08   DGP   Changed to use bit mask flags.
*   
***********************************************************************/

#define EXTERN extern

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#include "sysdef.h"
#include "regs.h"
#include "binloader.h"

extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
extern int errno;

int
binloader (char *file, int loadpt)
{
   FILE *fd;
#ifdef DEBUGLOADER
   FILE *lfd;
#endif
   int transfer = FALSE;
   int loadaddr = 0200;
   int curraddr = 0200;
   char inbuf[82];

#ifdef DEBUGLOADER
   lfd = fopen ("load.log", "w");
   fprintf (lfd, "binloader: file = '%s', loadpt = %d\n", file, loadpt);
#endif

   if (loadpt > 0)
   {
      loadaddr = loadpt;
      curraddr = loadpt;
   }

   if ((fd = fopen (file, "r")) == NULL)
   {
      sprintf (errview[0], "binloader: open failed: %s\n",
	       strerror (errno));
      sprintf (errview[1], "filename: %s\n", file);
      cpuflags |= CPU_IOCHK;
      run = CPU_STOP;
#ifdef DEBUGLOADER
      fprintf (lfd, "error: %s", errview[0]);
      fprintf (lfd, "       %s", errview[1]);
      fclose (lfd);
#endif
      return (-1);
   }

   while (fgets (inbuf, sizeof (inbuf), fd))
   {
      char *op = inbuf;
      int i;

      if (*op == IBSYSSYM)
      {
         break;
      }

      for (i = 0; i < 5; i++)
      {
	 char otag;
	 char item[16];
	 union 
	 {
	    uint32 ldata[2];
	    t_uint64 lldata;
	 } d;

	 otag = *op++;
	 if (otag == ' ') break;
	 strncpy (item, op, 12);
	 item[12] = '\0';
#ifdef WIN32
	 sscanf (item, "%I64o", &d.lldata);
#else
	 sscanf (item, "%llo", &d.lldata);
#endif

#ifdef DEBUGLOADER
	 fprintf (lfd, "loadaddr = %5.5o, curraddr = %5.5o\n",
		  loadaddr, curraddr);
	 fprintf (lfd, "   otag = %c, item = %s\n", otag, item);
	 fprintf (lfd, "   d.ldata = %2.2o %11.11o\n",
		  d.ldata[MSL], d.ldata[LSL]);
	 fprintf (lfd, "   d.lldata = %12.12llo\n", d.lldata);
#endif

         switch (otag)
	 {
	 case IDT_TAG:
	    break;

	 case ABSORG_TAG:
	    curraddr = loadaddr = d.ldata[LSL] & ADDRMASK;
	    break;

	 case RELORG_TAG:
	    curraddr = ((d.ldata[LSL] & ADDRMASK) + loadaddr) & ADDRMASK;
	    break;

	 case BSS_TAG:
	    curraddr = (curraddr + (d.ldata[LSL] & ADDRMASK)) & ADDRMASK;
	    break;

	 case RELADDR_TAG:
	    d.ldata[LSL] = d.ldata[LSL] + loadaddr;

	 case ABSDATA_TAG:
#ifdef USE64
	    mem[curraddr] = d.lldata;
#else
	    memh[curraddr] = ((d.ldata[MSL] << 4) & SIGN) 
			    | (d.ldata[MSL] & HMSK);
	    meml[curraddr] = d.ldata[LSL];
#endif
	    curraddr++;
	    break;

	 case ABSXFER_TAG:
	    transfer = TRUE;
	 case ABSENTRY_TAG:
#ifdef USE64
	    sr = TRA | d.lldata;
	    ic = d.lldata;
#else
	    srh = TRAh; /* TRA   addr */
	    srl = TRAl | d.ldata[LSL];
	    ic = d.ldata[LSL];
#endif
	    if (transfer) goto GOSTART;
	    break;

	 case RELXFER_TAG:
	    transfer = TRUE;
	 case RELENTRY_TAG:
#ifdef USE64
	    sr = TRA | (d.lldata + loadaddr);
	    ic = d.lldata;
#else
	    srh = TRAh; /* TRA   addr */
	    srl = TRAl | (d.ldata[LSL] + loadaddr);
	    ic = d.ldata[LSL];
#endif
	    ic += loadaddr;
	    if (transfer) goto GOSTART;
	    break;

	 default: ;
	 }
	 op += 12;
      }
   }

GOSTART:
#if 0
   {
      int i;

      for (i = 0; i < 3; i++)
         fprintf (stderr, "memh[%04o] = %04o; meml[%04o] = %012lo;\n", 
            i, memh[i], i, meml[i]);
      for (i = 0101; i < 0126; i++)
         fprintf (stderr, "memh[%04o] = %04o; meml[%04o] = %012lo;\n", 
            i, memh[i], i, meml[i]);
   }
#endif
#ifdef DEBUGLOADER
   fclose (lfd);
#endif
   fclose (fd);

   return (0);
}
