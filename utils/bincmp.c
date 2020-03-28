/***********************************************************************
*
* bincmp - Compare binary IBM 7090 image files.
*
* Changes:
*   03/01/05   DGP   Original.
*   12/15/05   RMS   MINGW changes.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>

#include "sysdef.h"


#if defined(WIN32) && !defined(MINGW)
#define IOCP     0400000000000I64
#define IOCT     0500000000000I64
#define OPMASK   0700000000000I64
#define ADDRMASK 0000000077777I64
#else
#define IOCP     0400000000000ULL
#define IOCT     0500000000000ULL
#define OPMASK   0700000000000ULL
#define ADDRMASK 0000000077777ULL
#endif

static t_uint64 memory1[MEMSIZE];
static t_uint64 memory2[MEMSIZE];

/***********************************************************************
* readword - read a word.
***********************************************************************/

static t_uint64
readword (FILE *fd)
{
   t_uint64 word;
   int i;
   int ch;

   word = 0;

   for (i = 0; i < 6; i++)
   {
      ch = fgetc (fd);
      word = word << 6 | (t_uint64)(ch & 077);
   }
#ifdef DEBUG
   printf ("readword: word = %12.12llo\n", word);
#endif

   return word;
}

/***********************************************************************
* loadmem - Load memory.
***********************************************************************/

static void
loadmem (t_uint64 *memory, FILE *infd)
{
   t_uint64 ctlword;
   int ldaddr;
   int wrdcnt;
   int done;
   int i;

   memset (memory, '\0', MEMSIZE);

   done = FALSE;
   while (!done)
   {
      ctlword = readword (infd);
      if ((ctlword & IOCT) == IOCT)
         done = TRUE;
      else
      {
         ldaddr = (int)(ctlword & ADDRMASK);
         wrdcnt = (int)((ctlword >> DECRSHIFT) & ADDRMASK);
#ifdef DEBUG
	 printf ("loadmem: ldaddr = %05o, wrdcnt = %05o(%d)\n",
	       ldaddr, wrdcnt, wrdcnt);
#endif
	 for (i = 0; i < wrdcnt; i++)
	 {
	    memory[ldaddr] = readword (infd);
	    ldaddr++;
	 }
      }
   }
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *infd1;
   FILE *infd2;
   char *infile1;
   char *infile2;
   char *bp;
   int status;
   int i;

   /*
   ** Process command line arguments
   */

   infile1 = NULL;
   infile2 = NULL;
   status = 0;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
	 goto usage;
      }
      else if (infile1 == NULL)
         infile1 = argv[i];
      else if (infile2 == NULL)
         infile2 = argv[i];
      else
      {
      usage:
         fprintf (stderr, "usage: bincmp infile1 infile2\n");
	 exit (1);
      }
   }

   if (infile1 == NULL || infile2 == NULL) goto usage;
#ifdef DEBUG
   printf ("bincmp: infile1 = %s, infile2 = %s\n", infile1, infile2);
#endif

   /*
   ** Open the files.
   */

   if ((infd1 = fopen (infile1, "rb")) == NULL)
   {
      fprintf (stderr, "bincmp: input open failed: %s\n",
	       strerror (errno));
      fprintf (stderr, "filename: %s\n", infile1);
      exit (1);
   }
   if ((infd2 = fopen (infile2, "rb")) == NULL)
   {
      fprintf (stderr, "bincmp: input open failed: %s\n",
	       strerror (errno));
      fprintf (stderr, "filename: %s\n", infile2);
      exit (1);
   }

   /*
   ** Load the memory areas
   */

#ifdef DEBUG
   printf ("bincmp: Load memory1\n");
#endif
   loadmem (memory1, infd1);
#ifdef DEBUG
   printf ("bincmp: Load memory2\n");
#endif
   loadmem (memory2, infd2);

   /*
   ** Compare the memory areas.
   */

#ifdef DEBUG
   printf ("bincmp: Compare\n");
#endif

   for (i = 0; i < MEMSIZE; i++)
   {
      if (memory1[i] != memory2[i])
      {
#ifdef WIN32
         printf ("%05o: %12.12I64o != %12.12I64o\n", i, memory1[i], memory2[i]);
#else
         printf ("%05o: %12.12llo != %12.12llo\n", i, memory1[i], memory2[i]);
#endif
         status = 1;
      }
   }

   fclose (infd1);
   fclose (infd2);

   return status;

}
