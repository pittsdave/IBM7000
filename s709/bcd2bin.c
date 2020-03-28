/***********************************************************************
*
* bcd2bin.c - IBM 7090 convert BCD tapes to BIN format.
*
* Changes:
*   ??/??/??   PRP   Original.
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "sysdef.h"
#include "cvtpar.h"
#include "prsf2.h"

char fin[300], fon[300];

int 
main (int argc, char **argv)
{
   int c;
   FILE *fi, *fo;
   int parerr = FALSE;

   parsefiles (argc, argv, "bcd", "bin", 0, 0);
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
   while ((c = fgetc (fi)) != EOF)
   {
      if (!parerr && evenpar[c & 077] != (c & 0177))
      {
         parerr = TRUE;
         fprintf (stderr, "Parity error in %s\n", fin);
      }
      fputc (binbcd[c & 077] | (c & 0200), fo);
   }

   fclose (fi);
   fclose (fo);

   return (0);
}
