/***********************************************************************
*
* bcd2bss - Convert BCD image tape(s) to a BSS library.
*
* Changes:
*   06/28/10   DGP   Original.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#include "sysdef.h"

static int verbose;

static uint8 datbuf[MAXRECSIZE];

/***********************************************************************
* tapereadint - Read an integer.
***********************************************************************/

static int
tapereadint (FILE *fd)
{
   int r;

   r = fgetc (fd);
   r = r | (fgetc (fd) << 8);
   r = r | (fgetc (fd) << 16);
   r = r | (fgetc (fd) << 24);
   if (feof (fd)) return (-2);
   return (r);
}

/***********************************************************************
* tapewriteint - Write an integer.
***********************************************************************/

static void
tapewriteint (FILE *fd, int v)
{
   fputc (v & 0xFF, fd);
   fputc ((v >> 8) & 0xFF, fd);
   fputc ((v >> 16) & 0xFF, fd);
   fputc ((v >> 24) & 0xFF, fd);
}

/***********************************************************************
* main - Main procedure.
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *infd, *outfd;
   char *bp;
   char *outfile;
   int i;
   int simhfmt;

   /*
   ** Process arguments and file.
   */

   outfile = NULL;
   verbose = FALSE;
   simhfmt = FALSE;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
	 for (bp++; *bp; bp++) switch (*bp)
	 {
	 case 'o':
	    i++;
	    if (i >= argc) goto usage;
	    outfile = argv[i];
	    if ((outfd = fopen (outfile, "wb")) == NULL)
	    {
	       sprintf ((char*)datbuf, "Can't open output: %s", argv[i]);
	       perror ((char*)datbuf);
	       exit (1);
	    }
	    break;
	 case 's':
	    simhfmt = TRUE;
	    break;
	 case 'v':
	    verbose = TRUE;
	    break;
	 default:
      usage:
	    fprintf (stderr, "usage: bcd2bss [options] BCDimage...\n");
	    fprintf (stderr, " options:\n");
	    fprintf (stderr, "    -o BSSimage  - Output BSS image\n");
	    fprintf (stderr, "    -s           - Process simh format\n");
	    fprintf (stderr, "    -v           - Verbose\n");
	    exit (1);
	 }
      }
      else
      {
         int ch;
	 int size;

         if (!outfile) goto usage;

	 /*
	 ** Open input file.
	 */

	 if (verbose)
	    printf ("bcd2bss: Input file: %s\n", argv[i]);

	 if ((infd = fopen (argv[i], "rb")) == NULL)
	 {
	    sprintf ((char*)datbuf, "Can't open input: %s", argv[i]);
	    perror ((char*)datbuf);
	    exit (1);
	 }
	 while ((ch = fgetc (infd)) != EOF)
	 {
	    int j;

	    ungetc (ch, infd);
	    if (simhfmt)
	    {
	       if ((size = tapereadint (infd)) > 0)
	       {
		  tapewriteint (outfd, size);
		  for (j = 0; j < size; j++)
		  {
		     datbuf[j] = fgetc (infd);
		     fputc (datbuf[j], outfd);
		  }
		  size = tapereadint (infd);
		  tapewriteint (outfd, size);
	       }
	    }
	    else
	    {
	       size = 168;
	       for (j = 0; j < size; j++)
	       {
		  datbuf[j] = fgetc (infd);
		  if (datbuf[j] == 0217)
		  {
		     break;
		  }
		  fputc (datbuf[j], outfd);
	       }
	    }
	 }
	 fclose (infd);
      }
   }

   if (outfd)
      fclose (outfd);

   return (0);
   
}
