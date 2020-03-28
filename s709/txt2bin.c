/***********************************************************************
*
* txt2bin.c - IBM 7090 convert text files to tape BIN format.
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
   long rcnt = 0;
   long ccnt = 0;
   int c;
   int fmterr = FALSE;
   int recmark = TRUE;

   parsefiles (argc, argv, "txt", "bin", 0, 0);
   if ((fi = fopen (fin, "r")) == NULL)
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
      if (c == '\n')
      {
         recmark = TRUE;
         continue;
      }
      if (c == '|')
         continue;
      if (!fmterr && c != '~' && (c < ' ' || c > '_'))
      {
         fmterr = TRUE;
         fprintf (stderr, "Format error in %s\n", fin);
      }
      if (c == '~')
         c = 017;
      else
         c = (c - ' ') & 077;
      if (recmark)
      {
         c |= 0200;
         recmark = FALSE;
         rcnt++;
      }
      fputc (oddpar[c & 077] | (c & 0200), fo);
      ccnt++;
   }
   fputc (0200, fo);
   printf ("%ld records, %ld characters\n", rcnt, ccnt);
   return (0);
}
