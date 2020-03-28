/***********************************************************************
*
* MKDASD - Make a dasd image file for IBM 7000.
*
* Changes:
*      06/03/05   DGP   Original
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

#include "dasd.h"

/***********************************************************************
* dskwriteint - Write an integer.
***********************************************************************/

static int
dskwriteint (FILE *fd, int num)
{

   if (fputc ((num >> 24) & 0xFF, fd) < 0) return (-1);
   if (fputc ((num >> 16) & 0xFF, fd) < 0) return (-1);
   if (fputc ((num >> 8) & 0xFF, fd) < 0) return (-1);
   if (fputc (num & 0xFF, fd) < 0) return (-1);
   return (0);
}

/***********************************************************************
* main
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *dfd;
   char *bp;
   char *dasdfile;
   char *dasdname;
   int i;
   int dasdndx;
   int dasdsize;
   int verbose;

   verbose = FALSE;
   dasdfile = NULL;
   dasdname = NULL;

   /*
   ** Scan off args 
   */

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];

      if (*bp == '-')
      {
         for (bp++; *bp; bp++) switch (*bp)
         {
	 case 'd':
	    i++;
	    dasdname = argv[i];
	    break;

	 case 'v':
	    verbose = TRUE;
	    break;

         default:
      USAGE:
	    printf ("usage: mkdasd [-options] dasd.fil\n");
            printf (" options:\n");
	    printf ("    -d model  - Disk/Drum model\n");
            printf ("    -v        - Verbose output\n");
	    return (ABORT);
         }
      }

      else
      {
         if (dasdfile) goto USAGE;
         dasdfile = argv[i];
      }

   }

   if (!dasdfile) goto USAGE;
   if (!dasdname) dasdname = "1301-1";

   /*
   ** Check dasd model
   */

   for (dasdndx = 0; dasdndx < MAXDASDMODEL; dasdndx++)
   {
      if (!strcmp (dasdname, dasd[dasdndx].model)) break;
   }
   if (dasdndx == MAXDASDMODEL)
   {
      printf ("mkdasd: Unknown device model: %s\n", dasdname);
      exit (ABORT);
   }

   /*
   ** Calulate the size of the dasd
   */

   dasdsize = dasd[dasdndx].byttrk *
	      dasd[dasdndx].heads *
	      dasd[dasdndx].cyls *
	      dasd[dasdndx].access *
	      dasd[dasdndx].modules;

   if (verbose)
   {
      double size;

      size = dasdsize / 1048576.0;
      printf ("mkdasd: create device %s on file %s\n", dasdname, dasdfile);
      printf (
 "Geometry: cyls = %d, heads = %d, byte/trk = %d, access = %d, modules = %d\n",
	       dasd[dasdndx].cyls, dasd[dasdndx].heads,
	       dasd[dasdndx].byttrk, dasd[dasdndx].access,
	       dasd[dasdndx].modules);
      printf (" Device size = %6.2f MB\n", size);
   }

   /*
   ** Open the dasd image file
   */

   if ((dfd = fopen (dasdfile, "wb")) == NULL)
   {
      perror ("mkdasd: Can't open dasdfile");
      exit (ABORT);
   }

   /*
   ** Write the dasd geometry information
   */

   dskwriteint (dfd, dasd[dasdndx].cyls);
   dskwriteint (dfd, dasd[dasdndx].heads);
   dskwriteint (dfd, dasd[dasdndx].access << 16 | dasd[dasdndx].modules);
   dskwriteint (dfd, dasd[dasdndx].byttrk);

   /*
   ** Write the dasd image
   */

   for (i = 0; i < dasdsize; i++)
   {
      fputc ('\0', dfd);
   }

   fclose (dfd);
   return (0);
}
