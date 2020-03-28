#include "nativebcd.h"

/***********************************************************************
* octdump - Octal dump routine.
***********************************************************************/

static void
octdump (FILE *file, void *ptr, int size, int offset)
{
   int jjj;
   int iii;
   int dmpmax;
   unsigned char *tp;
   unsigned char *cp;

   tp = (unsigned char *)ptr;
   cp = (unsigned char *)ptr;

   dmpmax = 9;

   for (iii = 0; iii < (size);)
   {
      fprintf (file, "%08o  ", iii+offset);

      for (jjj = 0; jjj < dmpmax; jjj++)
      {
	    if (jjj && (jjj % 3 == 0)) fprintf (file, " ");
	    if (cp < ((unsigned char *)ptr+size))
	    {
	       fprintf (file, "%02o", *cp++ & 077);
	       iii ++;
	       if (cp < ((unsigned char *)ptr+size))
	       {
		  fprintf (file, "%02o", *cp++ & 077);
		  iii ++;
	       }
	       else 
		  fprintf (file, "  ");
	    }
	    else 
	       fprintf (file, "    ");
      }
      fprintf ((file), "   ");
      for (jjj = 0; jjj < dmpmax; jjj++)
      {
	    if (jjj && (jjj % 3 == 0)) fprintf (file, " ");
	    if (tp < ((unsigned char *)ptr+size))
	    {
	       fprintf (file, "%c", tonative[*tp++ & 077]);
	       if (tp < ((unsigned char *)ptr+size))
	       {
		  fprintf (file, "%c", tonative[*tp++ & 077]);
	       }
	       else 
		  fprintf (file, " ");
	    }
	    else 
	       fprintf (file, "  ");
      }
      fprintf ((file), "\n");
   }
}
