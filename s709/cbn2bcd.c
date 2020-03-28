/***********************************************************************
*
* cbn2bin.c - IBM 7090 convert card binary to tape BIN format.
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
   short int col;
   register int num;
   register int row;
   int cards;
   int column;
   int maxcol;

   parsefiles (argc, argv, "cbn", "bcd", 0, 0);
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
   cards = 0;
   column = 0;
   maxcol = 0;
   while ((c = fgetc (fi)) != EOF)
   {
      if (c & 0200)
      {
         if (c == 0200)
            break;
         cards++;
         column = 0;
      }
      column++;
      if (column > maxcol)
         maxcol = column;
      if (!parerr && oddpar[c & 077] != (c & 0177))
      {
         parerr = TRUE;
	 fprintf (stderr,
		  "Parity error in %s, card %d, upper column %d, %03o\n",
		  fin, cards, column, c);
      }
      col = fgetc (fi);
      if (col == EOF || col & 0200)
      {
         fprintf (stderr, "Not cbn file: %s at card %d\n", fin, cards);
         exit (1);
      }
      if (!parerr && oddpar[col & 077] != (col & 0177))
      {
         parerr = TRUE;
	 fprintf (stderr,
		  "Parity error in %s, card %d, lower column %d, %03o\n",
		  fin, cards, column, col);
      }
      col = (col & 077) | ((c & 077) << 6);

      row = 00001;
      for (num = 10; --num; )
      {
         if (col & row)
            break;
         row <<= 1;
      }
      if (num == 8 && (col & 00174) != 0)
      {
         row = 00004;
         for (num = 16; --num > 10; )
	 {
            if (col & row)
               break;
            row <<= 1;
         }
      }
      else if (num == 0 && col & 01000)
         num = 10;
      if ((col & 01000) && num != 10)
         num |= 060;
      else if (col & 02000)
         num |= 040;
      else if (col & 04000)
         num |= 020;
      else if (num == 10)
         num = 0;
      else if (num == 0)
         num = 060;

      fputc (evenpar[num] | (c & 0200), fo);
   }
   printf ("%d cards, width %d columns\n", cards, maxcol);

   return (0);
}
