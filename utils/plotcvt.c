/***********************************************************************
*
* pltcvt.c - CTSS plot tape processor.
*
* Changes:
*   12/07/11   DGP   Original.
*   
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#ifdef USS
#include <_Ieee754.h>
#endif

#include "sysdef.h"
#include "nativebcd.h"

static char *pfin;

static int altchars;
static int reclen;
static int simhfmt;
static int verbose;
static int verdump;

static uint8 record[32767];

extern void plots (int i, int j, int ldev);
extern void plot (float x, float y, int pen);

#include "octdump.h"

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
* readrecord - Read record from input.
***********************************************************************/

static int
readrecord (FILE *fd, uint8 *record, char *name)
{
   int reclen;
   int done;
   int c;

   reclen = 0;
   if (simhfmt)
   {
      if ((reclen = tapereadint (fd)) < 0)
         goto READ_ERR;
      if (reclen == 0)
      {
	 if (verdump)
	    fprintf (stderr, "EOF: %s\n", name);
	 return EOF;
      }
      if (fread (record, 1, reclen, fd) != reclen)
         goto READ_ERR;
      if (tapereadint (fd) != reclen)
         goto READ_ERR;
      record[0] |= 0200;
   }
   else
   {
      if ((c = fgetc (fd)) == EOF)
      {
	 if (verdump)
	    fprintf (stderr, "EOF: %s\n", name);
         return EOF;
      }
      if (c & 0200)
      {
	 if (c == 0217)
	 {
	    if (verdump)
	       fprintf (stderr, "EOF: %s\n", name);
	    return EOF;
	 }
	 done = FALSE;
	 while (!done)
	 {
	    record[reclen++] = c;
	    if ((c = fgetc (fd)) == EOF)
	    {
	       if (verdump)
		  fprintf (stderr, "EOF: %s\n", name);
	       return EOF;
	    }
	    if (c & 0200) done = TRUE;
	 } 
	 ungetc (c, fd);
      }
      else
         goto READ_ERR;
   }

   if (verdump)
   {
      fprintf (stderr, "RECORD: %s: reclen = %d(0%o)\n",
	      name, reclen, reclen);
      octdump (stderr, record, reclen, 0);
   }

   return reclen;

READ_ERR:
   fprintf (stderr, "Can't read: %s: %s\n", name, strerror (errno));
   return -1;
}

/***********************************************************************
* unmapchar - Map BCD char to local charset.
***********************************************************************/

static uint8
unmapchar (uint8 ch)
{
   return altchars ? toaltnative[ch & 077] : tonative[ch & 077];
}

/***********************************************************************
* bcdword - Map BCD word (6 chars) to local charset.
***********************************************************************/

static char *
bcdword (uint8 *ibuf, char *wbuf)
{
   int i;

   for (i = 0; i < 6; i++)
   {
      uint8 c;

      c = unmapchar (*ibuf++);
      if (c != ' ')
	 *wbuf++ = c;
   }
   *wbuf = '\0';
   return ibuf;
}

/***********************************************************************
* binword - get a word from the buffer.
***********************************************************************/

static char *
binword (uint8 *ibuf, t_uint64 *obuf)
{
   t_uint64 wd;
   int i;

   wd = 0;
   for (i = 0; i < 6; i++)
   {
      wd = (wd << 6) | (t_uint64)(*ibuf++ & 077);
   }
   *obuf = wd;
   return ibuf;
}

/***********************************************************************
* cvtfloat - Convert floating point number.
*   This routine, currently, supports conversion to IEEE format.
*   For IBM z/OS we then use a library routine.
***********************************************************************/

static float 
cvtfloat (t_uint64 in)
{
   union {
      uint32 ii;
      float ix;
   } cv;

   cv.ii = 0;
   if (in)
   {
      uint32 frac;
      uint32 exp;

      /* Convert to IEEE format */

      exp = (uint32)(in >> EXPSHIFT) & 0777;
      if (exp & 0400)
	 cv.ii = 0x80000000;
      exp = ((exp & 0377) - 2) << 23;
      frac = (uint32)(in & 0000377777777) >> 3;
      cv.ii |= exp | frac;

#ifdef USS
      /* Now convert IEEE to IBM Hex floating point. */
      __fp_btoh (&cv.ix, _FP_FLOAT, &cv.ix, _FP_FLOAT, _FP_BH_NR);
#endif
   }

   if (verbose)
      fprintf (stderr, "      in = %012llo, ii = %08X, ix = %10.4f\n",
	       in, cv.ii, cv.ix);

   return (cv.ix);
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fin;
   char *bp;
   int status;
   int i, j;
   int done;
   int fileno;
   int rotate;
   int resolution;

   /*
    * Process the arguments.
    */

   simhfmt = FALSE;
   altchars = FALSE;
   verbose = FALSE;
   verdump = FALSE;
   pfin = NULL;
   j = 0;
   rotate = 0;
   resolution = 0;
   fileno = 99;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
	 bp++;
	 switch (*bp)
	 {
	 case 's':
	    simhfmt = TRUE;
            /* Fall through */

         case 'a':
            altchars = TRUE;
	    break;

	 case 'f':
	    i++;
	    if (i >= argc) goto USAGE;
	    fileno = atoi (argv[i]);
	    break;

	 case 'p':
	    i++;
	    if (i >= argc) goto USAGE;
	    resolution = atoi (argv[i]);
	    break;

	 case 'r':
	    rotate = 1;
	    break;

	 case 'v':
	    if (*(bp+1) == 'd')
	       verdump = TRUE;
	    verbose = TRUE;
	    break;

	 default:
	    goto USAGE;
         }
      }
      else if (j == 0)
      {
         pfin = argv[i];
	 j++;
      }
      else
      {
      USAGE:
         fprintf (stderr,
	    "usage: plotctss [-a][-f N][-p N][-r][-s] plot.tap \n");
	 exit (1);
      }
   }

   if (pfin == NULL)
      goto USAGE;

   /* 
    * Open the Plot tape.
    */

   if ((fin = fopen (pfin, "rb")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", pfin, strerror (errno));
      exit (1);
   }

   plots (resolution, rotate, fileno);

   done = FALSE;
   status = 0;
   while (!done)
   {
      char *ep;
      char filetype[2][8];

      if ((reclen = readrecord (fin, record, pfin)) <= 0)
      {
         done = TRUE;
	 plot (0.0, 0.0, 999);
	 break;
      }

      bp = record;
      ep = bp + reclen;

      bp = bcdword (bp, filetype[0]);
      bp = bcdword (bp, filetype[1]);
      if (verbose)
         fprintf (stderr, "type = '%s' '%s'\n", filetype[0], filetype[1]);

      if (!strcmp (filetype[0], "(PLOT") && !strcmp (filetype[1], "TAPE)"))
      {
	 bp += 6;
	 for (i = 0; (i < 33) && (bp < ep); i++)
	 {
	    t_uint64 tags;

	    bp = binword (bp, &tags);
	    if (verbose)
	       fprintf (stderr, " tags = %012llo\n", tags);

	    for (j = 0; (j < 6) && (bp < ep); j++)
	    {
	       t_uint64 x, y;
	       int p;

	       p = (tags & 003);
	       if (tags & 004) p = -p;
	       tags >>= 6;

	       bp = binword (bp, &x);
	       bp = binword (bp, &y);
	       if (verbose)
		  fprintf (stderr,
		           "  plot: x = %012llo, y = %012llo, p = %d\n",
			   x, y, p);

	       plot (cvtfloat (x), cvtfloat (y), p);
	    }
	 }
      }
      else
      {
         fprintf (stderr, "File %s is not a plot tape.\n", pfin);
	 exit (1);
      }

   }

   fclose (fin);
   return status;
}
