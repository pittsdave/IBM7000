/***********************************************************************
*
* patchdasd - Patch DASD file.
*
* Changes:
*   05/24/10   DGP   Original.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#include "sysdef.h"
#include "nativebcd.h"
#include "dasd.h"

static int recnum;
static int filenum;
static int altchars;

static char datbuf[MAXRECSIZE];

#include "octdump.h"

/***********************************************************************
* dskreadint - Read an integer.
***********************************************************************/

static int
dskreadint (FILE *fd)
{
   int r;
   int i;

   r = 0;
   for (i = 0; i < 4; i++)
   {
      int c;
      if ((c = fgetc (fd)) < 0) return (-1);
      r = (r << 8) | (c & 0xFF);
   }
   return (r);
}


/***********************************************************************
* atoo - Ascii to Octal.
***********************************************************************/

static int
atoo (char *bp)
{
   int val = 0;
   for (; *bp; bp++)
   {
      if (*bp >= '0' && *bp < '8')
         val = (val << 3) | (*bp - '0');
      else
         break;
   }
   return (val);
}

/***********************************************************************
* main - Main procedure.
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fd;
   char *bp;
   char *file;
   dasd_types *pdasd;
   int i;
   int parg;
   int size;
   int org;
   int off;
   int dasdloc;
   int cyls, heads, access, modules, byttrk;
   int cyl, acc, mod, head;

   /*
   ** Process arguments and file.
   */

   file = NULL;
   altchars = FALSE;
   recnum = 1;
   filenum = 1;
   cyl = -1;
   acc = -1;
   mod = -1;
   head = -1;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
      usage:
	 fprintf (stderr,
	     "usage: patchdasd file acc mod cyl head offset data [data...]\n");
	 exit (1);
      }
      else
      {
	 if (!file) 
	    file = bp;
	 else if (acc < 0)
	    acc = atoi (bp);
	 else if (mod < 0)
	    mod = atoi (bp);
	 else if (cyl < 0)
	    cyl = atoi (bp);
	 else if (head < 0)
	 {
	    head = atoi (bp);
	    parg = i + 1;
	    break;
	 }
      }
   }

   if (!file) goto usage;
   if (head < 0) goto usage;
   if (parg >= argc) goto usage;

   /*
   ** Open input file.
   */

   if ((fd = fopen (file, "r+b")) == NULL)
   {
      perror ("Can't open");
      exit (1);
   }

   /*
   ** Patch DASD track
   */

   cyls = dskreadint (fd);
   heads = dskreadint (fd);
   modules = dskreadint (fd);
   access = (modules >> 16) & 0xFFFF;
   modules = modules & 0xFFFF;
   byttrk = dskreadint (fd);

   for (i = 0; i < MAXDASDMODEL; i++)
   {
      if (dasd[i].cyls == cyls &&
	  dasd[i].heads == heads &&
	  dasd[i].access == access &&
	  dasd[i].modules == modules &&
	  dasd[i].byttrk == byttrk)
      {
	 break;
      }
   }
   if (i == MAXDASDMODEL)
   {
      fprintf (stderr, "Unsupported DASD file given\n");
      exit (1);
   }

   /*
   ** Print DASD geometry
   */

   pdasd = &dasd[i];
   printf ("DASD geometry: \n");
   printf ("   model     = %s\n", pdasd->model);
   printf ("   cyls      = %d\n", pdasd->cyls);
   printf ("   heads     = %d\n", pdasd->heads);
   printf ("   byttrk    = %d\n", pdasd->byttrk);
   printf ("   access    = %d\n", pdasd->access );
   printf ("   modules   = %d\n", pdasd->modules);
   printf ("   dasd size = %d bytes\n",
	       pdasd->byttrk * pdasd->heads * pdasd->cyls * 
	       pdasd->access * pdasd->modules);

   if (cyl >= pdasd->cyls || mod >= pdasd->modules ||
       acc >= pdasd->access || head >= pdasd->heads)
   {
      fprintf (stderr, "Invalid address info for device\n");
      exit (1);
   }

   dasdloc = (pdasd->byttrk * head) +
     (pdasd->byttrk * pdasd->heads *
     ((acc * pdasd->cyls) + (mod * (pdasd->cyls*pdasd->access)) + cyl)) +
     DASDOVERHEAD;

   fseek (fd, dasdloc, SEEK_SET);
   printf ("   dasdloc = %d\n", dasdloc);
   fread (datbuf, 1, pdasd->byttrk, fd);

   /*
   ** Dump the track
   */

   off = org = atoo (argv[parg++]);
   org = org / 022 * 022;
   fprintf (stdout, "Before:\n");
   octdump (stdout, &datbuf[org], 022, org);

   for (size = -1, i = parg; i < argc; i++)
   {
      datbuf[off++] = atoo (argv[i]) & 0xFF;
      size++;
   }
   fseek (fd, dasdloc, SEEK_SET);
   fwrite (datbuf, 1, pdasd->byttrk, fd);
   fprintf (stdout, "After:\n");
   octdump (stdout, &datbuf[org], 022+size, org);

   fclose (fd);
   return (0);
   
}
