/***********************************************************************
*
* bin2bcd.c - IBM 7090 convert BIN tapes to BCD format.
*
* Changes:
*   ??/??/??   PRP   Original.
*
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "cvtpar.h"
#include "prsf2.h"

char fin[300], fon[300];

int
main (int argc, char **argv)
{
   FILE *fi, *fo;
   int c;
   int loc;
   int parerr = FALSE;

   parsefiles (argc, argv, "bin", "bcd", 0, 0);
   if ((fi = fopen (fin, "rb")) == NULL)
   {
      perror (fin);
      exit (1);
   }
   if ((fo = fopen (fon, "wb")) == NULL)
   {
      perror (fon);
      exit (1);
   }
   loc = 0;
   while ((c = fgetc (fi)) != EOF)
   {
      if ((c != 0217) && (oddpar[c & 077] != (c & 0177)))
      {
         parerr = TRUE;
         fprintf (stderr, "Parity error in %s, location = %10d (%12o)\n",
		  fin, loc, loc);
      }
      fputc (bcdbin[c & 077] | (c & 0200), fo);
      loc++;
   }

   fclose (fi);
   fclose (fo);

   return (0);
}
