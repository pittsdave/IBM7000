/***********************************************************************
*
* bd - BCD Dump of file.
*
* Changes:
*   01/20/05   DGP   Original.
*   03/15/06   DGP   Change file and record numbers to match IBEDT map.
*   03/29/06   DGP   Added DASD support.
*   05/31/06   DGP   Added simh format support.
*   10/24/06   DGP   Remove length mask in tapereadint.
*   05/25/10   DGP   Change DASD formatting.
*   06/28/10   DGP   Added binary switch.
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

static int recmode;
static int recnum;
static int filenum;
static int altchars;
static int verbose;
static int dasdflg;
static int binmode;
static int alldasd;

static char fmtbuf[MAXTRACKLEN/4];
static char datbuf[MAXRECSIZE];

/***********************************************************************
* octdump - Octal dump routine.
***********************************************************************/

static void
octdump (FILE *file, void *ptr, int size,int offset)
{
   int jjj;
   int iii;
   int kkk;
   int dmpmax;
   unsigned char *tp;
   unsigned char *cp;
   char fill[4];

   tp = (unsigned char *)ptr;
   cp = (unsigned char *)ptr;

   if (recmode) dmpmax = 9;
   else if (dasdflg) dmpmax = 9;
   else dmpmax = 4;
   if (dasdflg || binmode)
      strcpy (fill, "  ");
   else
      strcpy (fill, "   ");

   kkk = 0;
   for (iii = 0; iii < (size);)
   {
      if (binmode)
	 fprintf (file, "%08o  ", kkk+offset);
      else
	 fprintf (file, "%08o  ", iii+offset);

      for (jjj = 0; jjj < dmpmax; jjj++)
      {
	 if (recmode)
	 {
	    if (jjj && (jjj % 3 == 0)) fputs (" ", file);
	    if (cp < ((unsigned char *)ptr+size))
	    {
	       if (dasdflg || binmode)
		  fprintf (file, "%02o", *cp++ & 077);
	       else
		  fprintf (file, "%03o", *cp++);
	       iii ++;
	       if (cp < ((unsigned char *)ptr+size))
	       {
		  if (dasdflg || binmode)
		     fprintf (file, "%02o", *cp++ & 077);
		  else
		     fprintf (file, "%03o", *cp++);
		  iii ++;
	       }
	       else 
		  fputs (fill, file);
	    }
	    else 
	    {
	       fputs (fill, file);
	       fputs (fill, file);
	    }
	 }
	 else
	 {
	    if (dasdflg)
	    {
	       if (jjj && (jjj % 3 == 0)) fputs (" ", file);
	       fprintf (file, "%02o", *cp++);
	       fprintf (file, "%02o", *cp++);
	    }
	    else
	    {
	       fprintf (file, "%3.3o ", *cp++);
	       fprintf (file, "%3.3o ", *cp++);
	       fprintf (file, " ");
	    }
	    iii += 2;
	 }
      }
      fputs ("   ", file);
      for (jjj = 0; jjj < dmpmax; jjj++)
      {
	 if (recmode)
	 {
	    if (jjj && (jjj % 3 == 0)) fputs (" ", file);
	    if (tp < ((unsigned char *)ptr+size))
	    {
	       if (altchars)
		  fprintf (file, "%c", toaltnative[*tp++ & 077]);
	       else
		  fprintf (file, "%c", tonative[*tp++ & 077]);
	       if (tp < ((unsigned char *)ptr+size))
	       {
		  if (altchars)
		     fprintf (file, "%c", toaltnative[*tp++ & 077]);
		  else
		     fprintf (file, "%c", tonative[*tp++ & 077]);
	       }
	       else 
		  fputs (" ", file);
	    }
	    else 
	       fputs (" ", file);
	 }
	 else
	 {
	    if (dasdflg)
	    {
	       if (jjj && (jjj % 3 == 0)) fputs (" ", file);
	       fprintf (file, "%c", tonative[*tp++ & 077]);
	       fprintf (file, "%c", tonative[*tp++ & 077]);
	    }
	    else
	    {
	       if (altchars)
	       {
		  fprintf (file, "%c", toaltnative[*tp++ & 077]);
		  fprintf (file, "%c", toaltnative[*tp++ & 077]);
	       }
	       else
	       {
		  fprintf (file, "%c ", tonative[*tp++ & 077]);
		  fprintf (file, "%c ", tonative[*tp++ & 077]);
	       }
	       fputs (" ", file);
	    }
	 }
      }
      fputs ("\n", file);
      kkk += 3;
   }
}


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
* getfmtchar - Returns a format char from the format track.
***********************************************************************/

static char
getfmtchar (int curloc)
{
   char chr;

   chr = (fmtbuf[curloc/4] >> ((curloc % 4) * 2)) & 03;
   return (chr);
}

/***********************************************************************
* loadformat - Loads DASD format.
***********************************************************************/

static int
loadformat (FILE *fd, dasd_types *pdasd, int cyl, int mod, int acc)
{
   size_t j;
   int dasdloc;

   dasdloc = (pdasd->byttrk * pdasd->heads *
	   ((acc * pdasd->cyls) + (mod * (pdasd->cyls*pdasd->access)) + cyl))
	   + DASDOVERHEAD;
   
   if (verbose)
   {
      printf ("Load format: dasdloc = %d\n", dasdloc);
   }

   if (fseek (fd, dasdloc, SEEK_SET) < 0)
      return -1;

   j = pdasd->byttrk/4;
   if (fread (fmtbuf, 1, j, fd) != j)
      return -1;

   if (getfmtchar (0) != FMT_HA2)
      return -2;

   return 0;
}

static void
dumptrack (FILE *fd, dasd_types *pdasd, int dasdloc, int mod, int acc, int cyl,
int head)
{
   int i;

   fseek (fd, dasdloc, SEEK_SET);
   printf ("cyl = %d, head = %d, dasdloc = %d\n", cyl, head, dasdloc);
   fread (datbuf, 1, pdasd->byttrk, fd);

   /*
   ** Dump the records.
   */

   if (recmode)
   {
      int fieldsize;
      int curloc;
      char field[20];
      char recbuf[MAXTRACKLEN];

      if ((i = loadformat (fd, pdasd, cyl, mod, acc)) < 0)
      {
	 if (i == -2) goto NOT_FORMATTED;
      }

      curloc = 0;
      fieldsize = 0;

      while (getfmtchar (curloc) == FMT_HA2)
      {
	 char chr;
	 chr = datbuf[curloc++] & 077;
	 field[fieldsize++] = toaltnative[chr];
      }
      field[fieldsize] = '\0';
      if (verbose)
      {
	 printf ("HA2 = %s, length = %d (%d words)\n",
	       field, fieldsize, fieldsize/6);
      }

      while (getfmtchar (curloc) != FMT_END)
      {
	 int recndx;
	 int recnum;
	 char chr;

	 recnum = 0;
	 fieldsize = 0;
	 while (getfmtchar (curloc) == FMT_HDR)
	 {
	    field[fieldsize++] = toaltnative[datbuf[curloc] & 077];
	    if ((chr = datbuf[curloc++]) == 012) chr = 0;
	    recnum = recnum * 10 + chr;
	 }
	 field[fieldsize] = '\0';
	 if (verbose)
	 {
	    printf ("HDR = %s, length = %d (%d words)\n",
	       field, fieldsize, fieldsize/6);
	 }
	 printf ("Record %d:\n", recnum);

	 recndx = 0;
	 fieldsize = 0;
	 while (getfmtchar (curloc) == FMT_DATA)
	 {
	    recbuf[recndx++] = datbuf[curloc++];
	    fieldsize++;
	 }
	 if (verbose)
	 {
	    printf ("DATA length = %d (%d words)\n",
	       fieldsize, fieldsize/6);
	 }
	 octdump (stdout, recbuf, recndx, 0);
      }
   }

   /*
   ** Dump the track
   */

   else
   {
   NOT_FORMATTED:
      octdump (stdout, datbuf, pdasd->byttrk, 0);
   }
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
   int size;
   int org;
   int ch;
   int simhfmt;
   int tapemark;
   int cyls, heads, access, modules, byttrk;
   int cyl, acc, mod, head;
   size_t curpos;

   /*
   ** Process arguments and file.
   */

   file = NULL;
   verbose = FALSE;
   recmode = FALSE;
   altchars = FALSE;
   dasdflg = FALSE;
   simhfmt = FALSE;
   binmode = FALSE;
   alldasd = FALSE;
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
	 for (bp++; *bp; bp++) switch (*bp)
	 {
	 case 'a':
	    altchars = TRUE;
	    break;
	 case 'b':
	    recmode = TRUE;
	    binmode = TRUE;
	    break;
	 case 'D':
	    alldasd = TRUE;
	 case 'd':
	    dasdflg = TRUE;
	    break;
	 case 'r':
	    recmode = TRUE;
	    break;
	 case 's':
	    simhfmt = TRUE;
	    break;
	 case 'v':
	    verbose = TRUE;
	    break;
	 default:
      usage:
	    fprintf (stderr, "usage: bd [options] file [acc mod cyl head]\n");
	    fprintf (stderr, " options:\n");
	    fprintf (stderr, "    -a           - Use Alt charset\n");
	    fprintf (stderr, "    -b           - Binary record mode\n");
	    fprintf (stderr, "    -d           - DASD file\n");
	    fprintf (stderr, "    -r           - Process Records\n");
	    fprintf (stderr, "    -s           - Process simh format\n");
	    fprintf (stderr, "    -v           - Verbose\n");
	    exit (1);
	 }
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
	    head = atoi (bp);
         else
	    goto usage;
      }
   }

   if (!file) goto usage;
   if (dasdflg && !alldasd && head < 0) goto usage;

   /*
   ** Open input file.
   */

   if ((fd = fopen (file, "rb")) == NULL)
   {
      perror ("Can't open");
      exit (1);
   }

   /*
   ** Process according to type
   */

   if (dasdflg)
   {
      int dasdloc;

      /*
      ** Dump DASD track
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

      if (mod >= pdasd->modules || acc >= pdasd->access ||
	 (alldasd ? FALSE : (cyl >= pdasd->cyls || head >= pdasd->heads)))
      {
         fprintf (stderr, "Invalid address info for device\n");
	 exit (1);
      }

      if (alldasd)
      {
	 for (cyl = 0; cyl < pdasd->cyls; cyl++)
	    for (head = 1; head < pdasd->heads; head++)
	    {
	       dasdloc = (pdasd->byttrk * head) +
		 (pdasd->byttrk * pdasd->heads *
		 ((acc * pdasd->cyls) + (mod * (pdasd->cyls*pdasd->access)) +
		 cyl)) + DASDOVERHEAD;
	       dumptrack (fd, pdasd, dasdloc, mod, acc, cyl, head);
	    }
      }
      else
      {
	 dasdloc = (pdasd->byttrk * head) +
	   (pdasd->byttrk * pdasd->heads *
	   ((acc * pdasd->cyls) + (mod * (pdasd->cyls*pdasd->access)) + cyl)) +
	   DASDOVERHEAD;
         dumptrack (fd, pdasd, dasdloc, mod, acc, cyl, head);
      }
#if 0
      fseek (fd, dasdloc, SEEK_SET);
      printf ("   dasdloc = %d\n", dasdloc);
      fread (datbuf, 1, pdasd->byttrk, fd);

      /*
      ** Dump the records.
      */

      if (recmode)
      {
	 int fieldsize;
	 int curloc;
	 char field[20];
	 char recbuf[MAXTRACKLEN];

         if ((i = loadformat (fd, pdasd, cyl, mod, acc)) < 0)
	 {
	    if (i == -2) goto NOT_FORMATTED;
	 }

	 curloc = 0;
	 fieldsize = 0;

	 while (getfmtchar (curloc) == FMT_HA2)
	 {
	    char chr;
	    chr = datbuf[curloc++] & 077;
	    field[fieldsize++] = toaltnative[chr];
	 }
	 field[fieldsize] = '\0';
	 if (verbose)
	 {
	    printf ("HA2 = %s, length = %d (%d words)\n",
		  field, fieldsize, fieldsize/6);
	 }

	 while (getfmtchar (curloc) != FMT_END)
	 {
	    int recndx;
	    int recnum;
	    char chr;

	    recnum = 0;
	    fieldsize = 0;
	    while (getfmtchar (curloc) == FMT_HDR)
	    {
	       field[fieldsize++] = toaltnative[datbuf[curloc] & 077];
	       if ((chr = datbuf[curloc++]) == 012) chr = 0;
	       recnum = recnum * 10 + chr;
	    }
	    field[fieldsize] = '\0';
	    if (verbose)
	    {
	       printf ("HDR = %s, length = %d (%d words)\n",
		  field, fieldsize, fieldsize/6);
	    }
	    printf ("Record %d:\n", recnum);

	    recndx = 0;
	    fieldsize = 0;
	    while (getfmtchar (curloc) == FMT_DATA)
	    {
	       recbuf[recndx++] = datbuf[curloc++];
	       fieldsize++;
	    }
	    if (verbose)
	    {
	       printf ("DATA length = %d (%d words)\n",
		  fieldsize, fieldsize/6);
	    }
	    octdump (stdout, recbuf, recndx, 0);
	 }
      }

      /*
      ** Dump the track
      */

      else
      {
      NOT_FORMATTED:
	 octdump (stdout, datbuf, pdasd->byttrk, 0);
      }
#endif
   }

   else
   {

      /*
      ** Dump the Tape file.
      */

      size = 0;
      org = 0;
      printf ("Dump of %s:\n", file);
      memset (datbuf, '\0', sizeof (datbuf));
      if (verbose)
	 curpos = ftell (fd);

      tapemark = 0;
      while ((ch = fgetc (fd)) != EOF)
      {
	 if (recmode)
	 {

	    if (simhfmt)
	    {
	       ungetc (ch, fd);
	       if (verbose)
		  curpos = ftell (fd);
	       if ((size = tapereadint (fd)) > 0)
	       {
		  tapemark = 0;
		  for (i = 0; i < size; i++)
		  {
		     datbuf[i] = fgetc (fd);
		  }
		  size = tapereadint (fd);
		  ch = 0200;
	       }
	       else
	       {
		  printf ("File %d EOF", filenum++);
		  if (verbose)
		     printf ("  pos = %d(%o)",
			   curpos, curpos);
		  printf ("\n");
		  if (tapemark) break;
		  recnum = 1;
		  tapemark = 1;
		  continue;
	       }
	    }

	    if ((ch & 0200) || (!altchars && !binmode && (ch == 072)))
	    {
	       if (size)
	       {
		  printf ("File %d record %d:", filenum, recnum++);
		  if (verbose)
		     printf ("  pos = %d(%o), reclen = %d(%o), words = %d(%o)",
			      curpos, curpos, size, size, size/6, size/6);
		  printf ("\n");
		  octdump (stdout, datbuf, size, org);
		  memset (datbuf, '\0', sizeof (datbuf));
		  org = 0;
		  size = 0;
		  if (verbose)
		     curpos = ftell (fd);
		  if (ch == 0217)
		  {
		     printf ("File %d EOF", filenum++);
		     if (verbose)
			printf ("  pos = %d(%o)", curpos, curpos);
		     printf ("\n");
		     recnum = 1;
		     tapemark = 1;
		  }
		  else
		  {
		     curpos--;
		     datbuf[size++] = ch & 0xFF;
		  }
	       }
	       else
	       {
		  if (ch == 0217)
		  {
		     printf ("File %d EOF", filenum++);
		     if (verbose)
			printf ("  pos = %d(%o)", curpos, curpos);
		     printf ("\n");
		     if (tapemark) break;
		     recnum = 1;
		  }
		  else
		  {
		     curpos--;
		     datbuf[size++] = ch & 0xFF;
		  }
	       }
	    }
	    else
	    {
	       datbuf[size++] = ch & 0xFF;
	       tapemark = 0;
	    }
	 }

	 else
	 {
	    datbuf[size++] = ch & 0xFF;
	    if (size == MAXRECSIZE)
	    {
	       octdump (stdout, datbuf, size, org);
	       memset (datbuf, '\0', sizeof (datbuf));
	       org += MAXRECSIZE;
	       size = 0;
	    }
	 }
      }

      if (size)
      {
	 if (recmode)
	 {
	    printf ("File %d record %d:", filenum, recnum++);
	    if (verbose)
	       printf ("  pos = %d(%o), reclen = %d(%o), words = %d(%o)",
		       curpos, curpos, size, size, size/6, size/6);
	    printf ("\n");
	 }
	 octdump (stdout, datbuf, size, org);
      }
   }


   fclose (fd);
   return (0);
   
}
