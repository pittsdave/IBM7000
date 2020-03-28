/***********************************************************************
*
* bss2obj.c - CTSS BSS file to object converter.
*
* Changes:
*   06/08/10   DGP   Original.
*   08/10/10   DGP   Correct COMMON relocation processing.
*   
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sysdef.h"
#include "objdef.h"
#include "nativebcd.h"
#include "prsf2.h"

char fin[300], fon[300];

static int reclen;
static int simhfmt;
static int verbose;
static int wrdcnt;
static int objnum;
static size_t blklen;

static uint8 block[MAXREC];
static t_uint64 binblk[28];
static char objrec[82];

static char progname[8];

static time_t curtime;			/* Conversion time */
static struct tm *timeblk;		/* Conversion time */

static t_uint64 relobits;
static int reloctr;

#if defined(WIN32) && !defined(MINGW)
#define PROG_CARD 0400000000000I64
#define DATA_CARD 0200000000000I64
#define TTR       0002100000000I64
#define RELOBIT   0400000000000I64
#define RELOMASK  0777777777777I64
#else
#define PROG_CARD 0400000000000ULL
#define DATA_CARD 0200000000000ULL
#define TTR       0002100000000ULL
#define RELOBIT   0400000000000ULL
#define RELOMASK  0777777777777ULL
#endif

#define VERSION "1.0.1"

#include "octdump.h"

/***********************************************************************
* punchrecord - Punch object.
***********************************************************************/

static int
punchrecord (FILE *fd, char *objbuf)
{
   strcat (objrec, objbuf);
   if (++wrdcnt >= 5)
   {
      fprintf (fd, "%-72.72s%-4.4s%4.4d\n", objrec, progname, ++objnum);
      wrdcnt = 0;
      objrec[0] = '\0';
      if (objnum > 9999) objnum = 0;
   }
   return 0;
}

/***********************************************************************
* puncheof - Punch EOF object.
***********************************************************************/

static int
puncheof (FILE *fd)
{
   char temp[80];

   if (wrdcnt)
   {
      fprintf (fd, "%-72.72s%-4.4s%4.4d\n", objrec, progname, ++objnum);
      wrdcnt = 0;
      objrec[0] = '\0';
      if (objnum > 9999) objnum = 0;
   }
   sprintf (temp, "%-8.8s  %02d/%02d/%02d  %02d:%02d:%02d    BSS2OBJ %s",
	    progname,
	    timeblk->tm_mon+1, timeblk->tm_mday, timeblk->tm_year - 100,
	    timeblk->tm_hour, timeblk->tm_min, timeblk->tm_sec,
	    VERSION);
   fprintf (fd, "$EOF   %-65.65s%-4.4s%4.4d\n", temp, progname, ++objnum);
   return 0;
}

/***********************************************************************
* tapereadint - Read an integer.
***********************************************************************/

static size_t
tapereadint (FILE *fd)
{
   size_t r;

   r = fgetc (fd);
   r = r | (fgetc (fd) << 8);
   r = r | (fgetc (fd) << 16);
   r = r | (fgetc (fd) << 24);
   if (feof (fd)) return (EOF);
   return (r);
}

/***********************************************************************
* readblock - Read block from input.
***********************************************************************/

static int
readblock (FILE *fd)
{
   int done;
   int i, j, k;
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
      }
      else
         return -1;
   }

   /*
   ** Convert block to binary words.
   */

   for (k = 0, i = 0; i < (int)blklen; i += 6, k++)
   {
      t_uint64 wrd = 0;

      for (j = 0; j < 6; j++)
      {
         wrd = (wrd << 6) | ((t_uint64)block[i+j] & 077);
      }
      binblk[k] = wrd;
   }

#ifdef DEBUG
   if (verbose)
   {
      printf ("BIN blk:\n");
      j = 0;
      for (i = 0; i < k; i++)
      {
         printf ("%12.12llo ", binblk[i]);
	 if (++j >= 3)
	 {
	    fputc ('\n', stdout);
	    j = 0;
	 }
      }
      if (j)
	 fputc ('\n', stdout);
   }
#endif

   return 0;
}

/***********************************************************************
* getname - get a name field.
***********************************************************************/

static void
getname (char *name, int objndx)
{
   int i;

   for (i = 0; i < 6; i++)
      name[i] = tonative[block[objndx*6+i] & 077];
   name[6] = '\0';
}

/***********************************************************************
* adjrelo - Adjust relobits.
***********************************************************************/

static void
adjrelo (int cnt)
{
   reloctr -= cnt;
   relobits <<= cnt;
   relobits &= RELOMASK;
   if (reloctr <= 0)
   {
      if (verbose)
      {
	 printf ("  reload relobits: ctr = %d\n", reloctr);
      }
      relobits = binblk[3];
      if (reloctr < 0)
         relobits <<= -reloctr;
      reloctr = 36;
   }
}

/***********************************************************************
* main - Main procedure.
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fi, *fo;
   char *optarg;
   int optind;
   int done;
   int firstprog;
   int i;
   int recnum;
   int objndx;
   int tvcnt;
   int tvndx;
   int common;

   simhfmt = FALSE;
   verbose = FALSE;
   recnum = 0;
   common = 0;

   curtime = time(NULL);
   timeblk = localtime(&curtime);

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
            break;

         case 'v':
	    verbose = TRUE;
            break;

         default:
            fprintf (stderr,
	       "Usage: bss2obj [-option] infile [outfile]\n");
            fprintf (stderr, "  -s     Use simh format\n");
            exit (1);
         }
      }
   }

   blklen = 168;
   reclen = 168;

   parsefiles (argc - (optind-1), &argv[optind-1], "bss", "obj",
	       &reclen, (int *)&blklen);

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

   tvcnt = 0;
   tvndx = 0;
   wrdcnt = 0;
   objnum = 0;
   objrec[0] = '\0';

   firstprog = TRUE;
   done = FALSE;

   while (!done)
   {
      if (readblock (fi) < 0)
      {
         done = TRUE;
      }

      else
      {
	 char objbuf[32];
	 char name[8];

         if (verbose)
	 {
	    printf ("BSS record %d:\n", ++recnum);
	    octdump (stdout, block, reclen, 0);
	 }

	 /*
	 ** Process Program card
	 */

	 if (binblk[0] & PROG_CARD)
	 {
	    int length;
	    int wc = (int)((binblk[0] & 077000000) >> 18);

	    if (firstprog)
	    {
	       firstprog = FALSE;
	       length = (int)binblk[2] & 077777;
	       tvcnt = (int)((binblk[2] >> 18) & 077777);

	       if (verbose )
	       {
		  printf ("Program: length = %o, tv = 0%o(%d)\n",
			   length, tvcnt, tvcnt);
	       }

	       objndx = 4;
	       if (!binblk[objndx])
	       {
		  int entry;

		  strcpy (progname, "(MAIN)");
		  common = (int)binblk[3] & 077777;
		  entry = (int)binblk[5] & 077777;

		  if (verbose)
		  {
		     printf ("  name = %s, entry = %o, common = %o\n",
			     progname, entry, common);
		  }

		  sprintf (objbuf, "%c%6.6s %5.5o", IDT_TAG, progname, length);
		  punchrecord (fo, objbuf);

		  sprintf (objbuf, "%c%12.12o", RELENTRY_TAG, entry);
		  punchrecord (fo, objbuf);

		  objndx += 2;
		  wc -= 4;
	       }
	       else
	       {
		  getname (name, objndx);
		  common = (int)binblk[3] & 077777;
		  strcpy (progname, name);
		  sprintf (objbuf, "%c%6.6s %5.5o", IDT_TAG, name, length);
		  punchrecord (fo, objbuf);
		  if (verbose)
		     printf ("  name = %s, common = %o\n", name, common);
	       }
	       if (common)
	       {
		  sprintf (objbuf, "%c%12.12o", FAPCOMMON_TAG, common);
		  punchrecord (fo, objbuf);
	       }
	    }
	    else
	    {
	       objndx = 2;
	    }

	    while (wc && binblk[objndx])
	    {
	       getname (name, objndx);
	       length = (int)binblk[objndx + 1] & 077777;
	       sprintf (objbuf, "%c%6.6s %5.5o", RELGLOBAL_TAG, name, length);
	       if (verbose)
		  printf ("  entry: name = %s, addr = %o\n", name, length);
	       punchrecord (fo, objbuf);
	       objndx += 2;
	       wc -= 2;
	    }
	 }

	 /*
	 ** Process DATA cards
	 */

	 else
	 {
	    int wc = (int)((binblk[0] & 077000000) >> 18);

	    if (common && ((binblk[0] & 077777) >= common))
	    {
	       if (verbose)
	       {
	          printf ("  COMMON INIT: loc = %llo\n", binblk[0] & 077777);
	       }
	       sprintf (objbuf, "%c%12.12llo", RELORG_TAG, binblk[0] & 077777);
	       punchrecord (fo, objbuf);
	       for (i = 0; i < wc; i++)
	       {
		  t_uint64 wrd;

		  wrd = binblk[i+2];
		  if (verbose)
		  {
		     printf ("  wrd = %012llo\n", wrd);
		  }
		  sprintf (objbuf, "%c%12.12llo", ABSDATA_TAG, wrd);
		  punchrecord (fo, objbuf);
	       }
	    }
	    else
	    {
	       sprintf (objbuf, "%c%12.12llo", RELORG_TAG, binblk[0] & 077777);
	       punchrecord (fo, objbuf);

	       reloctr = 36;
	       relobits = binblk[2];

	       for (i = 0; i < wc; i++)
	       {
		  t_uint64 wrd;

		  wrd = binblk[i+4];

		  /*
		  ** Scan off transfer vectors
		  */

		  if (tvcnt)
		  {
		     getname (name, i+4);
		     wrd = TTR | 0;

		     sprintf (objbuf, "%c%12.12llo", ABSDATA_TAG, wrd);
		     punchrecord (fo, objbuf);
		     sprintf (objbuf, "%c%6.6s %5.5o",
			      RELEXTRN_TAG, name, tvndx);
		     punchrecord (fo, objbuf);

		     if (verbose)
		     {
			printf ("  external: name = %s, addr = %o\n",
				name, tvndx); 
			printf ("  octr = %2d, bits = %012llo, wrd = %012llo\n",
				reloctr, relobits & RELOMASK, wrd);
		     }
		     tvndx++;
		     tvcnt--;
		     adjrelo (2);
		  }

		  /*
		  ** Instructions
		  */

		  else
		  {
		     int relotag;

		     relotag = ABSDATA_TAG;

		     if (verbose)
		     {
			printf ("  octr = %2d, bits = %012llo, wrd = %012llo\n",
				reloctr, relobits & RELOMASK, wrd);
		     }

		     if (relobits & RELOBIT)
		     {
			if (common && (((wrd >> 18) & 077777) < common))
			{
			   if (verbose)
			      printf ("  RELO DECR\n");
			   relotag = RELDECR_TAG;
			}
			else
			{
			   if (verbose)
			      printf ("  COMMON DECR\n");
			   relotag = RELDECR_TAG;
			}
			adjrelo (2);
		     }
		     else
		     {
			adjrelo (1);
		     }

		     if (relobits & RELOBIT)
		     {
			if (common && ((wrd & 077777) < common))
			{
			   if (verbose)
			      printf ("  RELO ADDR\n");
			   if (relotag == RELDECR_TAG)
			      relotag = RELBOTH_TAG;
			   else
			      relotag = RELADDR_TAG;
			}
			else
			{
			   if (verbose)
			      printf ("  COMMON ADDR\n");
			   if (relotag == RELDECR_TAG)
			      relotag = RELBOTH_TAG;
			   else
			      relotag = RELADDR_TAG;
			}
			adjrelo (2);
		     }
		     else
		     {
			adjrelo (1);
		     }

		     sprintf (objbuf, "%c%12.12llo", relotag, wrd);
		     punchrecord (fo, objbuf);
		  }
	       }
	    }
	 }
      }
   }
   puncheof (fo);

   fclose (fi);
   fclose (fo);

   return (0);
}
