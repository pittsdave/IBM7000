/***********************************************************************
*
* bsplit - Split BIN tape image into separate files.
*
* Changes:
*   01/25/05   DGP   Original.
*   03/15/05   DGP   Change file and record numbers to match IBEDT map.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#include "sysdef.h"

static int recmode;
static int recnum;
static int filenum;


int
main (int argc, char **argv)
{
   FILE *fd;
   FILE *ofd;
   char *bp;
   char *file;
   char *ofile;
   int i;
   int size;
   int org;
   int ch;
   char tape[MAXRECSIZE];

   /*
   ** Process program options.
   */

   file = NULL;
   ofile = NULL;
   recmode = FALSE;
   filenum = 1;
   recnum = 1;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
	 bp++;
         switch (*bp)
	 {
	 case 'r':
	    break;
	 default:
      usage:
	    fprintf (stderr, "usage: bsplit intape [outfileroot]\n");
	    exit (1);
	 }
      }
      else
      {
	 if (!file) 
	 {
	    file = bp;
	 }
	 else if (!ofile)
	 {
	    ofile = bp;
	 }
	 else goto usage;
      }
   }

   if (!file) goto usage;

   /*
   ** If no output file, set default to "abcd"
   */

   if (!ofile) ofile = "abcd";

   if ((fd = fopen (file, "rb")) == NULL)
   {
      perror ("Can't open input");
      exit (1);
   }

   /*
   ** Append file and record number to the outfile name
   */

   sprintf (tape, "%s_%03d%03d.bin", ofile, filenum, recnum);
   if ((ofd = fopen (tape, "wb")) == NULL)
   {
      perror ("Can't open output");
      exit (1);
   }


   /*
   ** Process file, put each record (possibly a different load image)
   ** into a new file. EOF gets a file, too.
   */

   size = 0;
   org = 0;
   printf ("Split of %s:\n", file);
   memset (tape, '\0', sizeof (tape));
   while ((ch = fgetc (fd)) != EOF)
   {
         if (ch & 0200 || ch == 072)
	 {
	    if (size)
	    {
	       printf ("File %d record %d:\n", filenum, recnum++);
	       fwrite (tape, 1, size, ofd);
	       fclose (ofd);
	       sprintf (tape, "%s_%03d%03d.bin", ofile, filenum, recnum);
	       if ((ofd = fopen (tape, "wb")) == NULL)
	       {
		  perror ("Can't open output");
		  exit (1);
	       }

	       memset (tape, '\0', sizeof (tape));
	       org = 0;
	       size = 0;
	       if (ch == 0217)
	       {
		  printf ("File %d EOF\n", filenum++);
		  recnum = 0;
	       }
	       tape[size++] = ch & 0xFF;
	    }
	    else 
	       tape[size++] = ch & 0xFF;
	 }
	 else
	    tape[size++] = ch & 0xFF;
   }
   if (size)
   {
      printf ("File %d record %d:\n", filenum, recnum++);
      fwrite (tape, 1, size, ofd);
   }

   fclose (fd);
   fclose (ofd);
   return (0);
   
}
