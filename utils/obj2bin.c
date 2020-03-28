/***********************************************************************
*
* ob2bin - Convert asm7090 assembler output to scatter load tape binary
*         format.
*
* Changes:
*   01/20/05   DGP   Original.
*   03/10/05   DGP   Change to preserve load order of object.
*   03/15/05   DGP   Add back ctlbits, oops.
*   03/16/05   DGP   Added Segment option.
*   12/15/05   RMS   MINGW changes.
*   03/19/07   DGP   Include parity tables directly.
*   03/31/10   DGP   Correct DECREMENT relocation.
*   10/20/10   DGP   Added -b, NO BOM, option.
*   01/03/12   DGP   Fixed ctlbits at start of block.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#include "sysdef.h"
#include "objdef.h"

#if defined(WIN32) && !defined(MINGW)
#define IOCP 0400000000000I64
#define IOCT 0500000000000I64
#define TRA  0002000000000I64
#else
#define IOCP 0400000000000ULL
#define IOCT 0500000000000ULL
#define TRA  0002000000000ULL
#endif

static t_uint64 memory[MEMSIZE];

static int nobom;
static uint8 word[12];
static uint8 ctlbits;

#include "parity.h"

/***********************************************************************
* writeword - write a word.
***********************************************************************/

static void
writeword (t_uint64 wrd, FILE *fd)
{
   int i;

   for (i = 0; i < 6; i++)
   {
      word[i] = oddpar[(uint8)((wrd >> 30) & 077)];
      if (i == 0) word[i] |= ctlbits;
      else ctlbits = 0;
      wrd <<= 6;
   }
   fwrite (word, 1, 6, fd);
}

/***********************************************************************
* writeblock - write a block of words with address.
***********************************************************************/

static void
writeblock (int loadaddr, int wrdcnt, FILE *fd)
{
   t_uint64 wrd1;
   int i;

#ifdef DEBUG
   printf ("writeblock: loadaddr = %05o, wrdcnt = %o\n", loadaddr, wrdcnt);
#endif

   /*
   ** Write out loader control word
   */

   if (!nobom)
   {
      wrd1 = IOCP | ((t_uint64)wrdcnt << DECRSHIFT) |
	    (t_uint64)loadaddr;
      writeword (wrd1, fd);
   }

   /*
   ** Write the corresponding block
   */

   for (i = 0; i < wrdcnt; i++)
   {
      writeword (memory[loadaddr+i], fd);
   }

}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char *argv[])
{
   FILE *infd;
   FILE *outfd;
   char *bp;
   char *infile;
   char *outfile;
   int objindex;
   int loadaddr;
   int startaddr;
   int curraddr;
   int entaddr;
   int wrdcnt;
   int noeom;
   int overlay;
   int verbose;
   int i;
   char inbuf[82];

   /*
   ** Process command line arguments
   */

   noeom = FALSE;
   nobom = FALSE;
   overlay = FALSE;
   verbose = FALSE;

   infile = NULL;
   outfile = NULL;

   objindex = 0;
   wrdcnt = 0;
   startaddr = 0;

   loadaddr = 0200;
   curraddr = 0200;
   ctlbits = 0200;

   entaddr = -1;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
         for (bp++; *bp; bp++) switch (*bp)
	 {
	 case 'b': /* No BOM */
	    nobom = TRUE;
	    break;

	 case 'e': /* No EOM */
	    noeom = TRUE;
	    break;

	 case 'l': /* Load address, for relocatable images */
	    i++;
	    curraddr = loadaddr = atoi (argv[i]);
	    break;

	 case 'o': /* Overlay */
	    overlay = TRUE;
	    break;

	 case 's': /* Object segment number */
	    i++;
	    objindex = atoi (argv[i]);
	    break;

	 case 'v': /* Verbose */
	    verbose = TRUE;
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
	 fprintf (stderr, "usage: obj2bin [options] infile outfile\n");
	 fprintf (stderr, " options:\n");
	 fprintf (stderr, "    -b           - No BOM word\n");
	 fprintf (stderr, "    -e           - No EOM word\n");
	 fprintf (stderr, "    -l addr      - Load address\n");
	 fprintf (stderr, "    -o           - Add as overlay\n");
	 fprintf (stderr, "    -s objnum    - Object segment number\n");
	 exit (1);
      }
   }

   if (infile == NULL || outfile == NULL) goto usage;

   /*
   ** Open the files.
   */

   if ((infd = fopen (infile, "r")) == NULL)
   {
      fprintf (stderr, "obj2bin: input open failed: %s\n",
	       strerror (errno));
      fprintf (stderr, "filename: %s\n", infile);
      exit (1);
   }

   if ((outfd = fopen (outfile, overlay ? "ab" : "wb")) == NULL)
   {
      fprintf (stderr, "obj2bin: output open failed: %s\n",
	       strerror (errno));
      fprintf (stderr, "filename: %s\n", outfile);
      exit (1);
   }

   /*
   ** Position to the object segment.
   */

   for (i = 0; i < objindex; i++)
   {
      while ((bp = fgets (inbuf, sizeof (inbuf), infd)) != NULL)
      {
         if (*bp == IBSYSSYM) break;
      }
      if (bp == NULL) break;
   }

   if (bp == NULL) 
   {
      fprintf (stderr,"Requested object segment not found\n");
      exit (1);
   }

   /*
   ** Load "memory" with the program, writing it out on location change.
   */

   memset (&memory, '\0', sizeof (memory));

   while (fgets (inbuf, sizeof (inbuf), infd))
   {
      char *op = inbuf;

      if (*op == IBSYSSYM) break; /* $EOF */

      for (i = 0; i < 5; i++)
      {
	 char otag;
	 char item[16];
	 t_uint64 lldata, dectmp;

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
	    if (wrdcnt)
	    {
	       writeblock (startaddr, wrdcnt, outfd);
	       wrdcnt = 0;
	    }
	    curraddr = loadaddr = (int)(lldata & ADDRMASK);
	    startaddr = curraddr;
	    if (verbose)
	    {
	       printf ("obj2bin: ABSORG = %05o\n", curraddr);
	    }
	    break;

	 case RELORG_TAG:
	    if (wrdcnt)
	    {
	       writeblock (startaddr, wrdcnt, outfd);
	       wrdcnt = 0;
	    }
	    curraddr = (int)((lldata + loadaddr) & ADDRMASK);
	    startaddr = curraddr;
	    if (verbose)
	    {
	       printf ("obj2bin: RELORG = %05o\n", curraddr);
	    }
	    break;

	 case BSS_TAG:
	    if (wrdcnt)
	    {
	       writeblock (startaddr, wrdcnt, outfd);
	       wrdcnt = 0;
	    }
	    curraddr += (int)(lldata & ADDRMASK);
	    curraddr &= ADDRMASK;
	    startaddr = curraddr;
	    if (verbose)
	    {
	       printf ("obj2bin: BSS = %05o\n", curraddr);
	    }
	    break;

	 case ABSDATA_TAG:
	    memory[curraddr] = lldata;
	    curraddr++;
	    wrdcnt++;
	    break;

	 case RELADDR_TAG:
	    memory[curraddr] = lldata + loadaddr;
	    curraddr++;
	    wrdcnt++;
	    break;

	 case RELBOTH_TAG:
	    lldata += loadaddr;
	 case RELDECR_TAG:
	    dectmp = ((lldata & DECRMASK) >> DECRSHIFT) + loadaddr;
	    lldata &= ~DECRMASK;
	    lldata |= (dectmp << DECRSHIFT); 
	    memory[curraddr] = lldata;
	    curraddr++;
	    wrdcnt++;
	    break;

	 case ABSXFER_TAG:
	    goto LOADED;
	 case ABSENTRY_TAG:
	    entaddr = (int)(lldata & ADDRMASK);
	    if (verbose)
	    {
	       printf ("obj2bin: ABSENTRY = %05o\n", entaddr);
	    }
	    break;

	 case RELXFER_TAG:
	    goto LOADED;
	 case RELENTRY_TAG:
	    entaddr = (int)(lldata & ADDRMASK) + loadaddr;
	    if (verbose)
	    {
	       printf ("obj2bin: RELENTRY = %05o\n", entaddr);
	    }
	    break;

	 default: ;
	 }
	 op += 12;
      }
   }

LOADED:

   if (wrdcnt)
   {
      writeblock (startaddr, wrdcnt, outfd);
   }

   /*
   ** Write End of Module word.
   */

   if (!noeom)
   {
      writeword (IOCT | (entaddr >= 0 ? entaddr : 0), outfd);
   }

   fclose (infd);
   fclose (outfd);

   return (0);
}
