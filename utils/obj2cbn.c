/***********************************************************************
*
* ob2cbn - Convert asm7090 assembler output to CBN format.
*
* Changes:
*   01/10/05   DGP   Original.
*	
***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <errno.h>

#include "sysdef.h"
#include "objdef.h"

static t_uint64 words[24];
static t_uint64 chksum;

static t_uint64 memory[MEMSIZE];
static char memctl[MEMSIZE];

/***********************************************************************
* punchblock - punch a block of cards.
***********************************************************************/

static void
punchblock (int loadaddr, int wrdcnt, FILE *fd)
{
   int i;

   words[0] = loadaddr | ((wrdcnt - 2) << DECRSHIFT);
   chksum = 0;
   for (i = 2; i < wrdcnt; i++)
   {
      chksum += words[i];
   }

#ifdef DEBUG
   printf ("punchblock: loadaddr = %05o, wrdcnt = %o\n", loadaddr, wrdcnt);
   printf ("   chksum = %12.12llo\n", chksum);
   for (i = 23; i >= 0; i-=2)
   {
      printf (" %12.12llo %12.12llo\n", words[i & 076], words[i]);
   }
#endif
   memset (&words, '\0', sizeof (words));
}

/***********************************************************************
* Main procedure
***********************************************************************/

main (int argc, char *argv[])
{
   FILE *infd;
   FILE *outfd;
   char *bp;
   char *infile;
   char *outfile;
   int loadaddr;
   int curraddr;
   int entaddr;
   int wrdcnt;
   int i;
   char inbuf[82];

   /*
   ** Process command line arguments
   */

   infile = NULL;
   outfile = NULL;
   loadaddr = 0200;
   curraddr = 0200;
   entaddr = -1;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
         bp++;
	 switch (*bp)
	 {
	 case 'l':
	    i++;
	    loadaddr = atoi (argv[i]);
	    break;

	 default:
	    goto usage;
	 }
      }
      else if (infile == NULL)
         infile = argv[i];
      else if (outfile == NULL)
         outfile = argv[i];
      else
      {
      usage:
         fprintf (stderr, "usage: obj2cbn [options] infile outfile\n");
	 fprintf (stderr, " options:\n");
         fprintf (stderr, "    -l addr      - Load address\n");
	 exit (1);
      }
   }

   if (infile == NULL || outfile == NULL) goto usage;

   /*
   ** Open files.
   */

   if ((infd = fopen (infile, "r")) == NULL)
   {
      fprintf (stderr, "obj2cbn: input open failed: %s",
	       strerror (errno));
      fprintf (stderr, "filename: %s", infile);
      exit (1);
   }
   if ((outfd = fopen (outfile, "w")) == NULL)
   {
      fprintf (stderr, "obj2cbn: output open failed: %s",
	       strerror (errno));
      fprintf (stderr, "filename: %s", outfile);
      exit (1);
   }

   /*
   ** Load the program into "memory"
   */

   memset (&memory, '\0', sizeof (memory));
   memset (memctl, '\0', sizeof (memctl));

   while (fgets (inbuf, sizeof (inbuf), infd))
   {
      char *op = inbuf;
      int relo;

      for (i = 0; i < 5; i++)
      {
	 char otag;
	 char item[16];
	 t_uint64 lldata;

	 otag = *op++;
	 if (otag == ' ') break;
	 lldata = 0;
	 strncpy (item, op, 12);
	 item[12] = '\0';
#if defined(WIN32)
	 sscanf (item, "%I64o", &lldata);
#else
	 sscanf (item, "%llo", &lldata);
#endif

#ifdef DEBUG
	 printf ("loadaddr = %05o, curraddr = %05o\n",
		  loadaddr, curraddr);
	 printf ("   otag = %c, item = %s\n", otag, item);
#if defined(WIN32)
	 printf ("   lldata = %12.12I64o\n", lldata);
#else
	 printf ("   lldata = %12.12llo\n", lldata);
#endif
#endif

         switch (otag)
	 {
	 case ABSORG_TAG:
	    curraddr = loadaddr = (int)(lldata & ADDRMASK);
	    break;

	 case RELORG_TAG:
	    curraddr = (int)(lldata & ADDRMASK) + loadaddr;
	    break;

	 case BSS_TAG:
	    curraddr += (int)(lldata & ADDRMASK);
	    break;

	 case ABSDATA_TAG:
	    memory[curraddr] = lldata;
	    memctl[curraddr] = otag;
	    curraddr++;
	    break;

	 case RELADDR_TAG:
	    memory[curraddr] = lldata + loadaddr;
	    memctl[curraddr] = otag;
	    curraddr++;
	    break;

	 case RELBOTH_TAG:
	    lldata += loadaddr;
	 case RELDECR_TAG:
	    relo = ((lldata & DECRMASK) >> DECRSHIFT) + loadaddr;
	    lldata &= ~DECRMASK;
	    lldata |= (relo << DECRSHIFT); 
	    memory[curraddr] = lldata;
	    memctl[curraddr] = otag;
	    curraddr++;
	    break;

	 case ABSXFER_TAG:
	 case ABSENTRY_TAG:
	    entaddr = (int)(lldata & ADDRMASK);
	    break;

	 case RELXFER_TAG:
	 case RELENTRY_TAG:
	    entaddr = (int)(lldata & ADDRMASK) + loadaddr;
	    break;

	 default: ;
	 }
	 op += 12;
      }
   }

   /*
   ** Punch it out.
   */

   wrdcnt = 2;
   loadaddr = -1;
   memset (&words, '\0', sizeof (words));

   for (i = 0; i < MEMSIZE; i++)
   {
      if (memctl[i])
      {
	 if (loadaddr < 0) loadaddr = i;
#ifdef DEBUG
         printf ("loadaddr = %05o, tag = %c\n", loadaddr, memctl[i]);
#endif
         switch (memctl[i])
	 {
	 case ABSDATA_TAG:
	 case RELADDR_TAG:
	 case RELDECR_TAG:
	 case RELBOTH_TAG:
	    words[wrdcnt++] = memory[i];
	    break;
	 case RELORG_TAG:
	 case ABSORG_TAG:
	    if (loadaddr > 0)
	    {
	       punchblock (loadaddr, wrdcnt, outfd);
	       wrdcnt = 2;
	    }
	    loadaddr = (int)(memory[i] & ADDRMASK);
	    break;
	 default: ;
	 }
      }
      if (wrdcnt == 24)
      {
	 punchblock (loadaddr, wrdcnt, outfd);
	 loadaddr += 24;
         wrdcnt = 2;
      }
   }

   fclose (infd);
   fclose (outfd);
   return (0);

}
