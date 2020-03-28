/************************************************************************
*
* lnkpunch - Punchs out memory for IBM 7090 linker.
*
* Changes:
*   05/21/03   DGP   Original.
*   12/28/04   DGP   New object tags.
*   02/14/05   DGP   Revamped operation to allow stacked objects and new
*                    link map listing format.
*	
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "lnkdef.h"

extern FILE *lstfd;
extern int listmode;
extern int pc;
extern int errcount;
extern int absentry;
extern int relentry;
extern int symbolcount;

extern char deckname[MAXSYMLEN+2];
extern Memory memory[MEMSIZE];
extern struct tm *timeblk;
SymNode *symbols[MAXSYMBOLS];

static int objcnt = 0;
static int objrecnum = 0;
static char objbuf[16];
static char objrec[82];

/***********************************************************************
* punchfinish - Punch a record with sequence numbers.
***********************************************************************/

static void
punchfinish (FILE *outfd)
{
   if (objcnt)
   {
      sprintf (&objrec[SEQUENCENUM], OBJSEQFORMAT, ++objrecnum);
      if (deckname[0])
	 strncpy (&objrec[LBLSTART], deckname, strlen(deckname));
      fputs (objrec, outfd);
      memset (objrec, ' ', sizeof(objrec));
      objcnt = 0;
   }
}

/***********************************************************************
* punchrecord - Punch an object value into record.
***********************************************************************/

static void 
punchrecord (FILE *outfd)
{
   if (objcnt+WORDTAGLEN >= CHARSPERREC)
   {
      punchfinish (outfd);
   }
   strncpy (&objrec[objcnt], objbuf, WORDTAGLEN);
   objbuf[0] = '\0';
   objcnt += WORDTAGLEN;
}

/***********************************************************************
* puncheof - Punch EOF mark.
***********************************************************************/

static void
puncheof (FILE *outfd)
{
   char temp[80];

   punchfinish (outfd);
   strncpy (objrec, "$EOF", 4);
   sprintf (temp, "%-8.8s  %02d/%02d/%02d  %02d:%02d:%02d    LNK7090 %s",
	    deckname,
	    timeblk->tm_mon+1, timeblk->tm_mday, timeblk->tm_year - 100,
	    timeblk->tm_hour, timeblk->tm_min, timeblk->tm_sec,
	    VERSION);
   strncpy (&objrec[7], temp, strlen(temp));
   objcnt = 1;
   punchfinish (outfd);
}

/***********************************************************************
* punchmemory - Punch out memory.
***********************************************************************/

int
lnkpunch (FILE *outfd)
{
   t_uint64 ldata;
   int i;

   memset (objrec, ' ', sizeof(objrec));

#ifdef DEBUGPUNCH
   printf ("lnkpunch: deckname = %s, pc = %o\n", deckname, pc);
#endif

   sprintf (objbuf, OBJSYMFORMAT, IDT_TAG, deckname, pc);
   punchrecord (outfd);
   ldata = 0;
   sprintf (objbuf, OBJFORMAT, RELORG_TAG, ldata);
   punchrecord (outfd);

   for (i = 0; i < pc; )
   {
      if (memory[i].tag)
      {
#ifdef DEBUGPUNCH
	 printf ("   i = %5.5o, tag = %c, word = %12.12llo\n", i, 
		  memory[i].tag, memory[i].word);
#endif

         sprintf (objbuf, OBJFORMAT,
		  memory[i].tag, memory[i].word & WORDMASK);
         punchrecord (outfd);
	 if (memory[i].tag == RELORG_TAG)
	    i = (int)(memory[i].word & ~ADDRMASK);
	 else if (memory[i].tag == BSS_TAG)
	    i += (int)(memory[i].word & ~ADDRMASK);
	 else i++;
      }
      else
      {
	 ldata = 0;
         sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, ldata);
         punchrecord (outfd);
	 i++;
      }
   }

   if (relentry >= 0)
   {
      ldata = relentry;
#ifdef DEBUGPUNCH
	 printf ("   i = %5.5o, tag = %c, word = %12.12llo\n", i, 
		  RELENTRY_TAG, ldata);
#endif
      sprintf (objbuf, OBJFORMAT, RELENTRY_TAG, ldata);
      punchrecord (outfd);
   }

   puncheof (outfd);
   return (0);
}

