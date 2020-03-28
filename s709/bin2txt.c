/***********************************************************************
*
* bin2txt.c - IBM 7090 convert BIN tapes to text format.
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
   int parerr = FALSE;
   long rcnt = 0;
   long ccnt = 0;

   parsefiles (argc, argv, "bin", "txt", 0, 0);
   if ((fi = fopen (fin, "rb")) == NULL)
   {
      perror (fin);
      exit (1);
   }
   if ((fo = fopen (fon, "w")) == NULL)
   {
      perror (fon);
      exit (1);
   }
   while ((c = fgetc (fi)) != EOF)
   {
      if (!parerr && (c & 077) != 017 && oddpar[c & 077] != (c & 0177))
      {
         parerr = TRUE;
         fprintf (stderr, "Parity error in %s\n", fin);
      }
      if (c == 0200)
         break;
      if ((c & 0200) && (rcnt || ccnt))
      {
         fputc ('|', fo);
         fputc ('\n', fo);
         rcnt++;
      }
      c &= 077;
      if (c == 017)
         c = '~';
      else
         c += ' ';
      fputc (c, fo);
      ccnt++;
   }
   fputc ('|', fo);
   fputc ('\n', fo);
   rcnt++;
   printf ("%ld records, %ld characters\n", rcnt, ccnt);

   return (0);
}
