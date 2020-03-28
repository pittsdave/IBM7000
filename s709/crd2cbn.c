/***********************************************************************
*
* crd2cbn.c - IBM 7090 convert card to card binary.
*
* Changes:
*   ??/??/??   PRP   Original.
*
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "prsf2.h"

uint8 oddpar[64] = {
   0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
   0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
   0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
   0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
   0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100,
   0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
   0100, 0000, 0000, 0100, 0000, 0100, 0100, 0000,
   0000, 0100, 0100, 0000, 0100, 0000, 0000, 0100
};

char fin[300], fon[300];

int
main (int argc, char **argv)
{
   FILE *fi, *fo;
   int i, j;
   int n;
   uint8 buf[160];

   parsefiles (argc, argv, "crd", "cbn", 0, 0);
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
   for (n = 0; ; n++)
   {
      if ((j = fread (buf, 1, 160, fi)) != 160)
      {
         if (j == 0)
	 {
            printf ("%d cards\n", n);
            exit (0);
         }
         fprintf (stderr, "Out of sync: %d\n", j);
         exit (1);
      }
      for (i = 160; i; i--)
      {
         if (buf[i-1] != 0)
            break;
      }
      if (i & 1)
         i++;
      if (i == 0)
         i = 2;
      for (j = 0; j < i; j++)
      {
         if (buf[j] & 0300)
	 {
            fprintf (stderr, "Not a column binary file\n");
            break;
         }
         buf[j] |= oddpar[buf[j]];
      }
      buf[0] |= 0200;
      fwrite (buf, 1, i, fo);
   }

   return (0);
}
