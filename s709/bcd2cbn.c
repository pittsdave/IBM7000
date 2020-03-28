/***********************************************************************
*
* bcd2cbn.c - IBM 7090 BCD to Card Code converter.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/28/05   DGP   Corrected card codes to match IBSYS.
*   04/01/11   DGP   Correct EOF processing.
*   
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "cvtpar.h"
#include "prsf2.h"


uint16 bcdcbn[64] = {
/* 00  '0',   '1',   '2',   '3',   '4',   '5',   '6',   '7', */
       01000, 00400, 00200, 00100, 00040, 00020, 00010, 00004,
/* 10  '8',   '9',   ' ',   '=',   '\'',  ' ',   ' ',   ' ', */
       00002, 00001, 01000, 00102, 00042, 00000, 00000, 00006,
/* 20  '+',   'A',   'B',   'C',   'D',   'E',   'F',   'G', */
       04000, 04400, 04200, 04100, 04040, 04020, 04010, 04004,
/* 30  'H',   'I',   ' ',   '.',   '(',   ' ',   ' ',   ' ', */
       04002, 04001, 00000, 04102, 04042, 00000, 00000, 00000,
/* 40  '-',   'J',   'K',   'L',   'M',   'N',   'O',   'P', */
       02000, 02400, 02200, 02100, 02040, 02020, 02010, 02004,
/* 50  'Q',   'R',   ' ',   '$',   '*',   ' ',   ' ',   ' ', */
       02002, 02001, 00000, 02102, 02042, 00000, 00000, 00000,
/* 60  ' ',   '/',   'S',   'T',   'U',   'V',   'W',   'X', */
       00000, 01400, 01200, 01100, 01040, 01020, 01010, 01004,
/* 70  'Y',   'Z',   ' ',   ',',   ')',   ' ',   ' ',   ' '  */
       01002, 01001, 00000, 01102, 01042, 00000, 00000, 00000
};


char fin[300], fon[300];

int
main (int argc, char **argv)
{
   int c;
   FILE *fi, *fo;
   int parerr = FALSE;
   int colerr = FALSE;
   short int col;
   int recnum;
   int colnum;

   parsefiles (argc, argv, "bcd", "cbn", 0, 0);
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
   recnum = 0;
   colnum = 0;
   while ( (c = fgetc (fi)) != EOF )
   {
      if ((c & 0200) != 0)
      {
	 if (c == 0217)
	    break;
         recnum++;
         colnum = 0;
      }
      colnum++;
      if (colnum > 80)
      {
         if (!colerr)
	 {
            colerr = TRUE;
            fprintf (stderr, "Too many columns in %s, record %d\n",
		     fin, recnum);
         }
         continue;
      }
      if (!parerr && evenpar[c & 077] != (c & 0177))
      {
         parerr = TRUE;
         fprintf (stderr, "Parity error in %s, %03o column %d record %d\n",
		  fin, c, colnum, recnum);
      }

      col = bcdcbn[c & 077];
      fputc (oddpar[col >> 6] | (c & 0200), fo);
      fputc (oddpar[col & 077], fo);
   }
   return (0);
}
