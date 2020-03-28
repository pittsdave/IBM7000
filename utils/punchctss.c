/***********************************************************************
*
* punchctss.c - CTSS punch tape processor.
*
* The DSKEDT program generates two tapes for the RQUEST punch operations.
* The punchid tape contains the punch mode and file name for each processed
* file. The punchdata tape contains EOF separated files for each ID.
*
* Changes:
*   12/05/11   DGP   Original.
*   
***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "sysdef.h"
#include "nativebcd.h"

#define BPUNCH 1
#define DPUNCH 2
#define SPUNCH 3

static char *pfid, *pfdat;
static char *pfout;

static int altchars;
static int reclen;
static int simhfmt;
static int verbose;
static int verdump;

static uint8 idrecord[32767];
static uint8 datrecord[32767];

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
	    printf ("EOF: %s\n", name);
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
	    printf ("EOF: %s\n", name);
         return EOF;
      }
      if (c & 0200)
      {
	 if (c == 0217)
	 {
	    if (verdump)
	       printf ("EOF: %s\n", name);
	    return EOF;
	 }
	 done = FALSE;
	 while (!done)
	 {
	    record[reclen++] = c;
	    if ((c = fgetc (fd)) == EOF)
	    {
	       if (verdump)
		  printf ("EOF: %s\n", name);
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
      printf ("RECORD: %s: reclen = %d(0%o)\n",
	      name, reclen, reclen);
      octdump (stdout, record, reclen, 0);
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
* getword - Map BCD word (6 chars) to local charset.
***********************************************************************/

static void
getword (uint8 *ibuf, char *wbuf)
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
}

/***********************************************************************
* openfile - Open the file.
***********************************************************************/

static FILE *
openfile (int icmd, uint8 *ibuf)
{
   FILE *fd;
   char *bp;
   char name1[8], name2[8];
   char fname[1024];

   fd = NULL;

   getword (&ibuf[30], name1);
   getword (&ibuf[36], name2);
   if (verbose)
      printf ("\"%6s %6s\"", name1, name2);

   if (name1[0] == '.') name1[0] = '_';
   for (bp = name1; *bp; bp++)
      if (isupper (*bp)) *bp = tolower (*bp);
   if (name2[0] == '.') name2[0] = '_';
   for (bp = name2; *bp; bp++)
      if (isupper (*bp)) *bp = tolower (*bp);

   if (pfout != NULL)
      sprintf (fname, "%s/%s.%s", pfout, name1, name2);
   else
      sprintf (fname, "%s.%s", name1, name2);
   if (verbose)
      printf (" into %s\n", fname);

   if ((fd = fopen (fname, icmd == DPUNCH ? "w" : "wb")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", fname, strerror (errno));
   }
   return fd;
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *fid, *fdat;
   FILE *fout;
   char *bp;
   int status;
   int i, j;
   int done;
   int fileno;

   /*
    * Process the arguments.
    */

   simhfmt = FALSE;
   altchars = FALSE;
   verbose = FALSE;
   verdump = FALSE;
   pfid = NULL;
   pfdat = NULL;
   pfout = NULL;
   j = 0;

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
         pfid = argv[i];
	 j++;
      }
      else if (j == 1)
      {
         pfdat = argv[i];
	 j++;
      }
      else if (j == 2)
      {
         pfout = argv[i];
	 j++;
      }
      else
      {
      USAGE:
         fprintf (stderr,
	    "usage: punchctss [-a][-s] punchid.tap punchdata.tap [outdir]\n");
	 exit (1);
      }
   }

   if ((pfid == NULL) || (pfdat == NULL))
      goto USAGE;

   /* 
    * Open the input tapes.
    */

   if ((fid = fopen (pfid, "rb")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", pfid, strerror (errno));
      exit (1);
   }
   if ((fdat = fopen (pfdat, "rb")) == NULL)
   {
      fprintf (stderr, "Can't open: %s: %s\n", pfdat, strerror (errno));
      exit (1);
   }

   /*
    * Skip first ID record.
    */

   if (readrecord (fid, idrecord, pfid) <= 0)
      return 1;

   /*
    * Process all files.
    */

   done = FALSE;
   status = 0;
   fileno = 0;
   while (!done)
   {
      int icommand;
      char command[8];

      if (readrecord (fid, idrecord, pfid) < 0)
      {
         done = TRUE;
	 break;
      }

      /* Skip initial FLIP records. */

      for (i = 0; i < 6; i++)
      {
         if (readrecord (fdat, datrecord, pfdat) <= 0)
	 {
	    status = 1;
	    continue;
	 }
      }

      getword (idrecord, command);
      icommand = 0;
      if (!strcmp (command, "BPUNCH"))
         icommand = BPUNCH;
      else if (!strcmp (command, "DPUNCH"))
         icommand = DPUNCH;
      else if (!strcmp (command, "7PUNCH"))
         icommand = SPUNCH;
      if (verbose)
         printf ("Processing: File %3d: %-6.6s ", ++fileno, command);

      if ((fout = openfile (icommand, idrecord)) == NULL)
         continue;
      
      while ((reclen = readrecord (fdat, datrecord, pfdat)) > 0)
      {
         if (icommand == DPUNCH)
	 {
	    for (i = 0; i < reclen; i++)
	       datrecord[i] = unmapchar (datrecord[i]);
	    for (i = reclen-1; i; i--)
	       if (datrecord[i] == ' ') datrecord[i] = '\0';
	       else break;
	    fprintf (fout, "%s\n", (char *)datrecord);
	 }
	 else
	 {
	    fwrite (datrecord, 1, reclen, fout);
	 }
      }
      fclose (fout);
   }

   if (verbose)
      printf ("Processed %d Files.\n", fileno);

   fclose (fid);
   fclose (fdat);
   return status;
}
