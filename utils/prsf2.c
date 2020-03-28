/***********************************************************************
*
* prsf2.c - Parse file parm input.
*
* Changes:
*   ??/??/??   PRP   Original.
*   05/31/06   DGP   Added block length and rewrite.
*   05/06/10   DGP   Fixed digit scanning.
*   
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "sysdef.h"

char fin[300], fon[300];

/***********************************************************************
* dofile - Process file names.
***********************************************************************/

static void
dofile (char *arg, char *fn, char *ex)
{
   int extfnd = FALSE;

   while (*arg)
   {
      if (*arg == '.') extfnd = TRUE;
      *fn++ = *arg++;
   }

   if (!extfnd)
   {
      *fn++ = '.';
      while (*ex) *fn++ = *ex++;
   }
   *fn = '\0';
}

/***********************************************************************
* parsefiles - Parse the files and add extensions with defaults.
***********************************************************************/

void
parsefiles (int argc, char **argv, char *iex, char *oex, int *parm1, int *parm2)
{
   int i;
   char *f1p, *f2p;
   char *p1p, *p2p;

   f1p = f2p = p1p = p2p = NULL;

   if (argc < 2 || argc > 5 || (parm1 == 0 && argc > 3) ||
   	(parm2 == 0 && argc > 4))
   {
   USAGE:
      if (parm1 == 0)
      {
         fprintf (stderr, "Usage: %s2%s infile [outfile]\n",
		  iex, oex);
      }
      else if (parm2 == 0)
      {
         fprintf (stderr,
		  "Usage: %s2%s infile [outfile] [reclen, default %d]\n",
		  iex, oex, *parm1);
      }
      else
      {
         fprintf (stderr,
   "Usage: %s2%s infile [outfile] [reclen, default %d [blklen, default %d]]\n",
		  iex, oex, *parm1, *parm2);
      }
      exit (1);
   }

   for (i = 1; i < argc; i++)
   {
      char *cp;
      int alldigits = TRUE;

      cp = argv[i];
      while (*cp) 
      {
         if (!isdigit (*cp))
	 {
	    alldigits = FALSE;
	    break;
	 }
	 cp++;
      }
      if (alldigits)
      {
         if (!p1p) p1p = argv[i];
	 else if (!p2p) p2p = argv[i];
	 else goto USAGE;
      }
      else
      {
         if (!f1p) f1p = argv[i];
	 else if (!f2p) f2p = argv[i];
	 else goto USAGE;
      }
   }

   if (f1p) dofile (f1p, fin, iex);
   else     goto USAGE;
   if (f2p) dofile (f2p, fon, oex);
   else     dofile (f1p, fon, oex);
   if (p1p && parm1) *parm1 = atoi (p1p);
   if (p2p && parm2) *parm2 = atoi (p2p);

}
