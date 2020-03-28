/***********************************************************************
*
* bcd2txt.c - IBM 7090 BCD to Text converter.
*
* Changes:
*   ??/??/??   PRP   Original.
*   01/28/05   DGP   Changed to use IBSYS standard characters as default.
*   03/18/05   DGP   Changed 072 usage, in altbcd it is a '?'.
*   05/31/06   DGP   Added simh support and blocking.
*   10/24/06   DGP   Remove length mask in tapereadint.
*   09/12/07   DGP   Correct Carriage control.
*   08/19/11   DGP   Changed Carriage control processing.
*   12/02/11   DGP   Process until double EOF. Fix end of block processing.
*   12/13/11   DGP   Fixed when block size not default size.
*   
***********************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "sysdef.h"
#include "nativebcd.h"
#include "prsf2.h"

#define MAXREC 32768

char fin[300], fon[300];

static int altchars;
static int reclen;
static int blklen;
static int argblklen;
static int chrblk;
static int chrrec;
static int simhfmt;
static int blank;
static int termc;
static int chkc;
static int printer;
static int fileflag;
static int eofhit;
static int verbose;

static uint8 block[MAXREC];
static uint8 record[MAXREC];

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
   if (feof (fd)) return (EOF);
   return (r);
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
* readblock - Read block from input.
***********************************************************************/

static int
readblock (FILE *fd)
{
   int done;
   int c;

   if (simhfmt)
   {
      if ((blklen = tapereadint (fd)) < 0)
         return -1;
      if (blklen == 0) return EOF;
      if (fread (block, 1, blklen, fd) != blklen)
         return -1;
      if (tapereadint (fd) != blklen)
         return -1;
      block[0] |= 0200;
   }
   else
   {
      if ((c = fgetc (fd)) == EOF)
         return -1;
      if (c & 0200)
      {
         blklen = 0;
	 if (c == 0217) return EOF;
	 done = FALSE;
	 while (!done)
	 {
	    block[blklen++] = c;
	    if ((c = fgetc (fd)) == EOF)
	       return -1;
	    if (c & 0200) done = TRUE;
	 } 
	 ungetc (c, fd);
	 if (verbose)
	    printf ("readblock: blklen = %d\n", blklen);
      }
      else
         return -1;
   }

   if (!printer && (blklen != argblklen))
      reclen = blklen;

   chrblk = 0;
   chrrec = 0;
   return 0;
}

/***********************************************************************
* outputrec - Output accumulated record.
***********************************************************************/

static uint8 *
outputrec (FILE *fd, uint8 *p, uint8 c)
{
   if (verbose)
      printf ("outputrec: c = 0%03o, chrblk = %d, p = %p, record = %p\n",
              c, chrblk, p, record);
   if (p != record)
   {
      while (p != record && p[-1] == ' ')
	 p--;

      if (p != record)
      {
	 fwrite (record, 1, p - record, fd);
	 fputc ('\n', fd);
	 if (verbose)
	 {
	    fwrite (record, 1, p - record, stdout);
	    fputc ('\n', stdout);
	 }
      }
      else if (!fileflag)
      {
	 fputc ('\n', fd);
	 if (verbose)
	    fputc ('\n', stdout);
      }

      fileflag = FALSE;
      p = record;
   }

   if (printer && (c != 0377))
   {
      uint8 ch;

      if ((c == termc) && (chrblk < blklen))
      {
	 c = block[chrblk++];
	 chrrec++;
      }

      ch = unmapchar (c);
      if (verbose)
         printf ("   carriage c = 0%03o, ch = '%c'(0x%02x)\n",
	         c, ch, ch);
      switch (ch)
      {
      case '-':
	 fputc ('\n', fd);
      case '0':
	 fputc ('\n', fd);
	 break;
      case '1':
	 fputc ('\f', fd);
	 break;
      case ' ':
	 break;
      default: 
	 fputc (ch, fd);
	 break;
      }
   }

   chrrec = 0;
   return p;
}

/***********************************************************************
* main - Main procedure.
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fi, *fo;
   uint8 *p;
   char *optarg;
   int optind;
   int done;
   uint8 c;

   blank = FALSE;
   altchars = FALSE;
   printer = FALSE;
   simhfmt = FALSE;
   verbose = FALSE;
   termc = 072;
   chkc = 0132;

   for (optind = 1, optarg = argv[optind];
       (optind < argc) && (*optarg == '-');
       optind++, optarg = argv[optind])
   {
      ++optarg;
      while (*optarg)
      {
         switch (*optarg++)
	 {

         case 's':
	    simhfmt = TRUE;
            /* Fall through */

         case 'a':
            altchars = TRUE;
	    termc = 0132;
	    chkc = 072;
            break;

         case 'b':
	    blank = TRUE;
            break;

         case 'p':
	    printer = TRUE;
            break;

         case 'v':
	    verbose = TRUE;
            break;

         default:
            fprintf (stderr,
	       "Usage: bcd2txt [-option] infile [outfile] [reclen [blklen]]\n");
            fprintf (stderr, "  -a     Use Alternate character set\n");
            fprintf (stderr, "  -p     Convert printer controls\n");
            fprintf (stderr, "  -s     Use simh format\n");
            exit (1);
         }
      }
   }

   argblklen = 84;
   reclen = 80;

   parsefiles (argc - (optind-1), &argv[optind-1], "bcd", "txt",
	       &reclen, &argblklen);

   blklen = argblklen;

   if ((fi = fopen (fin, "rb")) == NULL)
   {
      perror (fin);
      exit (1);
   }
   if ((fo = fopen (fon, "w")) == NULL)
   {
      perror (fon);
      exit (1);
   }

   p = record;
   fileflag = TRUE;
   done = FALSE;
   eofhit = FALSE;
   while (!done)
   {
      int skiprem;

      if (readblock (fi) < 0)
      {
	 if (eofhit)
	 {
	    done = TRUE;
	 }
	 eofhit = TRUE;
	 continue;
      }

      skiprem = FALSE;
      eofhit = FALSE;
      while (chrblk < blklen)
      {
	 c = block[chrblk++];
	 chrrec++;

	 if (c & 0200 || (!printer && (p - record == reclen)) || c == termc)
	 {
	    if (!printer && ((blklen - (chrblk-1)) < reclen))
	    {
	       chrblk = blklen;
	       skiprem = TRUE;
	    }

	    p = outputrec (fo, p, c);
	    if (printer) c = termc;
	 }

	 if (!skiprem && c != termc)
	 {
	    if (c == 0217)
	    {
	       fputc ('\n', fo);
	       if (verbose)
		  fputc ('\n', stdout);
	       fileflag = TRUE;
	    }
	    else if (c != chkc)
	    {
	       if (blank && (c & 077) == 020)
		  *p++ = ' ';
	       else 
		  *p++ = unmapchar (c);
	    }
	 }
      }
   }

   p = outputrec (fo, p, 0377);

   fclose (fi);
   fclose (fo);

   return (0);
}
