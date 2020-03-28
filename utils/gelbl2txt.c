/***********************************************************************
*
* gelbl2txt.c - GE Label BCD to Text converter.
*
* Changes:
*   03/23/05   DGP   Original.
*   12/15/05   RMS   MINGW changes.
*   
***********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#include "sysdef.h"
#include "nativebcd.h"


#define BCDEOF 0217
#define WORDPERRECORD 433

#if defined(WIN32) && !defined(MINGW)
#define GELBL 0272560600600I64
#define GEEOF 0606025462660I64
#define GEEOT 0602546436360I64

#define WORDCOUNTMASK 0000000777777I64
#define WORDMARKMASK  0777777000000I64
#else
#define GELBL 0272560600600ULL
#define GEEOF 0606025462660ULL
#define GEEOT 0602546436360ULL

#define WORDCOUNTMASK 0000000777777ULL
#define WORDMARKMASK  0777777000000ULL
#endif

/***********************************************************************
* skiptoeof - Skip to EOF.
***********************************************************************/

static void
skiptoeof (FILE *fd)
{
   int ch;

   while ((ch = fgetc (fd)) != EOF)
      if (ch == BCDEOF)
         break;

}

/***********************************************************************
* readword - read a word.
***********************************************************************/

static t_uint64
readword (FILE *fd)
{
   t_uint64 word;
   int i;
   int ch;

   word = 0;

   for (i = 0; i < 6; i++)
   {
      ch = fgetc (fd);
      if (ch == EOF) return -1;
      if (ch == BCDEOF) return -1;
      word = word << 6 | (t_uint64)(ch & 077);
   }
#ifdef DEBUG
   printf ("readword: word = %12.12llo\n", word);
#endif

   return word;
}

/***********************************************************************
* outputbcd - Output word as a text.
***********************************************************************/

static void
outputbcd (FILE *fd, t_uint64 word)
{
   int i;

   for (i = 0; i < 6; i++)
   {
      fputc (tonative[(uint8)((word >> 30) & 077)], fd);
      word <<= 6;
   }
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   t_uint64 word;
   FILE *infd;
   FILE *outfd;
   char *bp;
   char *infile;
   char *outfile;
   int i;
   int alldone;
   int done;
   int list;
   int filecnt, blkcnt, reccnt;

   /*
   ** Process command line arguments
   */

   infile = NULL;
   outfile = NULL;
   filecnt = 0;
   blkcnt = 0;
   reccnt = 0;
   list = FALSE;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
         for (bp++; *bp; bp++) switch (*bp)
	 {
	 case 'l': /* List labels */
	    list = TRUE;
	    break;

	 default:
	    goto usage;
	 }
      }
      else if (infile == NULL)
         infile = argv[i];
      else if (outfile == NULL)
         outfile = argv[i];
      else
      {
      usage:
         fprintf (stderr, "usage: gelbl2txt [options] infile outfile\n");
	 fprintf (stderr, " options:\n");
	 fprintf (stderr, "     -l      - List labels\n");
	 exit (1);
      }
   }

   if (infile == NULL) goto usage;
   if (!list && outfile == NULL) goto usage;

   /*
   ** Open the files.
   */

   if ((infd = fopen (infile, "rb")) == NULL)
   {
      fprintf (stderr, "gelbl2txt: input open failed: %s",
	       strerror (errno));
      fprintf (stderr, "filename: %s", infile);
      exit (1);
   }

   if (!list)
   {
      if ((outfd = fopen (outfile, "w")) == NULL)
      {
	 fprintf (stderr, "gelbl2txt: output open failed: %s",
		  strerror (errno));
	 fprintf (stderr, "filename: %s", outfile);
	 fclose (infd);
	 exit (1);
      }
   }

   alldone = FALSE;
   while (!alldone)
   {
      /*
      ** Get Initial word, should be GE label
      */

      word  = readword (infd);
      if (word == GELBL)
      {
	 int recwrd;
	 int wrdcnt;
	 int inblk;

	 if (list)
	 {
	    outputbcd (stdout, word);
	    done = FALSE;
	    while (!done)
	    {
	       word = readword (infd);
	       if (word == -1)
	          done = TRUE;
	       else
		  outputbcd (stdout, word);
	    }
	    fputc ('\n', stdout);
	 }
	 else
	    skiptoeof (infd);

	 filecnt++;
	 done = FALSE;
	 inblk = FALSE;
	 while (!done)
	 {
	    word = readword (infd);
	    if (word == -1) break;
	    blkcnt++;

	    for (recwrd = 0; recwrd < WORDPERRECORD-1; recwrd++)
	    {
	       word = readword (infd);
	       if (word == -1)
	       {
		  done = TRUE;
		  break;
	       }
	       else
	       {
		  if (inblk)
		  {
		     if (!list)
			outputbcd (outfd, word);
		     wrdcnt--;
		     if (wrdcnt == 0)
		     {
			inblk = FALSE;
			if (!list)
			   fputc ('\n', outfd);
		     }
		  }
		  else if (word & WORDMARKMASK)
		  {
		     reccnt++;
		     wrdcnt = (int)(word & WORDCOUNTMASK);
		     if (wrdcnt)
		     {
			inblk = TRUE;
		     }
		     else
		     {
			if (!list)
			   fputc ('\n', outfd);
		     }
#ifdef DEBUG
		     printf ("gelbl2txt: Start block %d words\n", wrdcnt);
#endif
		  }
	       }
	    }
	 }

      }
      else if (word == GEEOF)
      {
         skiptoeof (infd);
	 if (!list)
	    fputc ('\f', outfd);
      }
      else if (word == GEEOT)
      {
         skiptoeof (infd);
	 alldone = TRUE;
	 printf ("gelbl2txt: Processed %d files, %d blocks, %d records\n",
	       filecnt, blkcnt, reccnt);
      }
      else
      {
	 fprintf (stderr, "gelbl2txt: Not a GE 600 label\n");
	 alldone = TRUE;
      }
   }

   fclose (infd);
   if (!list)
      fclose (outfd);

   return (0);
}
