/***********************************************************************
*
* STRIPBSS - Strips BSS files.
*
* Changes:
*      04/04/11   DGP   Original
*
***********************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#define NORMAL		0
#define ABORT		16

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/***********************************************************************
* main
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *ifd;
   FILE *ofd;
   int ch;

   if (argc != 3)
   {
      fprintf (stderr, "usage: stripbss infile outfile\n");
      exit (ABORT);
   }
   
   if ((ifd = fopen (argv[1], "rb")) == NULL)
   {
      fprintf (stderr, "Open input %s failed: %s\n",
      	       argv[1], strerror(errno));
      exit (ABORT);
   }
   if ((ofd = fopen (argv[2], "wb")) == NULL)
   {
      fprintf (stderr, "Open output %s failed: %s\n",
      	       argv[2], strerror(errno));
      fclose (ifd);
      exit (ABORT);
   }

   while ((ch = fgetc (ifd)) != EOF)
   {
      if (ch == 0217)
         break;
      fputc (ch, ofd);
   }

   fputc (0217, ofd);
   fclose (ofd);
   fclose (ifd);

   return (NORMAL);
}
