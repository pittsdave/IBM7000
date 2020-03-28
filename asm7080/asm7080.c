/***********************************************************************
*
* asm7080 - Assembler for the IBM 7080 computer.
*
* Changes:
*   02/08/07   DGP   Original.
*   07/30/07   DGP   Change mktemp to mkstemp.
*	
***********************************************************************/

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "asmdef.h"
#include "asmdmem.h"

int pc;				/* the assembler pc */
int curpcindex;			/* Current PC counter index */
int genxref;			/* Generate cross reference listing */
int widemode;			/* Generate wide listing */
int stdlist;			/* Generate standard listing */
int exprtype;			/* Expression type */
int symbolcount;		/* Number of symbols in symbol table */
int inpass;			/* Which pass are we in */
int errcount;			/* Number of errors in assembly */
int errnum;			/* Index into errline array */
int linenum;			/* Source line number */
int p1errcnt;			/* Number of pass 0/1 errors */
int p1erralloc;			/* Number of pass 0/1 errors allocated */
int pgmlength;			/* Length of program */
int radix;			/* Number scanner radix */
int litorigin;			/* Literal pool origin */
int litpoolsize;		/* Literal pool size */
int cpumodel;			/* CPU model (705, 7053, 7080) */
int filenum;			/* INSERT file index */
int noloader;			/* Don't punch loader option flag */
int bootobject;			/* "boot format" object */

int pccounter[MAXPCCOUNTERS];	/* PC counters */
char errline[10][120];		/* Pass 2 error lines for current statment */
char inbuf[MAXLINE];		/* The input buffer for the scanners */
char deckname[MAXDECKNAMELEN+2];/* The assembly deckname */
char ttlbuf[TTLSIZE+2];		/* The assembly TTL buffer */
char titlebuf[TTLSIZE+2];	/* The assembly TITLE buffer */
char datebuf[48];		/* Date/Time buffer for listing */

SymNode *symbols[MAXSYMBOLS];	/* The Symbol table */
SymNode *freesymbols;		/* Reusable symbols nodes */
XrefNode *freexrefs;		/* Reusable xref nodes */
ErrTable p1error[MAXERRORS];	/* The pass 0/1 error table */
time_t curtime;			/* Assembly time */
struct tm *timeblk;		/* Assembly time */
char *pscanbuf;			/* Pointer for tokenizers */

static int verbose;		/* Verbose mode */
static int filecnt;		/* File process count */
static int reccnt;		/* File record count */

/***********************************************************************
* tabpos - Return TRUE if col is a tab stop.
***********************************************************************/

static int
tabpos (int col, int tabs[]) 
{
   if (col < BUFSIZ)
      return (tabs[col]);

    return (TRUE); 

} /* tabpos */

/***********************************************************************
* detabit - Convert tabs to equivalent number of blanks.
***********************************************************************/

static void
detabit (int tabs[], FILE *ifd, FILE *ofd)
{
   int c, col;
 
   col = 0;
   while ((c = fgetc (ifd)) != EOF)
   {

      switch (c)
      {

         case '\t' :
            while (TRUE)
	    { 
               fputc (' ', ofd);
               if (tabpos (++col, tabs) == TRUE) 
                  break ;
            } 
            break;

         case '\n' :
	    while (col < 80)
	    {
	       fputc (' ', ofd);
	       col++;
	    }
            fputc (c, ofd);
            col = 0;
	    reccnt++;
            break;

         default: 
            fputc (c, ofd);
            col++;

      } /* switch */

   } /* while */

} /* detabit */

/***********************************************************************
* alldig - Check if all digits.
***********************************************************************/

static int
alldig (char *digs)
{
   while (*digs)
   {
      if (!isdigit (*digs))
      {
         return (FALSE);
      }
      digs++;
   }

   return (TRUE);

} /* alldig */

/***********************************************************************
* settab - Set initial tab stops.
***********************************************************************/

static void
settab (int tabs[], int argc, char *argv[])
{
   int m, p, i, j, l;
   char *bp;
  
   p = 0;

   for (i = 0; i < BUFSIZ; i++)
       tabs[i] = FALSE;

   for (j = 1; j < argc; j++)
   {

      bp = argv[j];

      if (*bp == '+')
         bp++;

      if (alldig (bp))
      {

         l = atoi (bp) ;

         if (l < 0 || l >= BUFSIZ)
             continue;

         if (*argv[j] != '+')
         { 
            p = l;
            tabs[p] = TRUE;
         } 

         else
         { 
            if (p == 0) 
               p = l;
            for (m = p; m < BUFSIZ; m += l) 
            {
               tabs[m] = TRUE;
            }
         }

      }

   } 

   if (p == 0)
   {
      for (i = 8; i < BUFSIZ; i += 8) 
      {
         tabs[i] = TRUE;
      }
   }

} /* settab */

/***********************************************************************
* detab - Detab source.
***********************************************************************/

int
detab (FILE *ifd, FILE *ofd, char *type) 
{
   int tabs[BUFSIZ];
   char *tabstops[] = { "as", "7", "15", "+8", "\0" };
  
#ifdef DEBUG
   fprintf (stderr, "detab: Entered: infile = %s, outfile = %s\n",
	    infile, outfile);
#endif

   if (verbose)
      fprintf (stderr, "asm7080: Detab %s file \n", type);
   reccnt = 0;

   /* set initial tab stops */

   settab (tabs, 4, tabstops);

   detabit (tabs, ifd, ofd);

   if (verbose)
      fprintf (stderr, "asm7080: File contains %d records\n", reccnt);
   return (0);

} /* detab */

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *infd = NULL;
   FILE *outfd = NULL;
   FILE *lstfd = NULL;
   FILE *tmpfd0 = NULL;
   FILE *tmpfd1 = NULL;
   char *infile = NULL;
   char *outfile = NULL;
   char *lstfile = NULL;
   char *cp, *bp;
   int i;
   int done;
   int listmode;
   int status = 0;
   int tmpdes0, tmpdes1;
   char tname0[MAXFILENAMELEN+2];
   char tname1[MAXFILENAMELEN+2];
  
#ifdef DEBUG
   fprintf (stderr, "asm7080: Entered:\n");
   fprintf (stderr, "args:\n");
   for (i = 1; i < argc; i++)
   {
      fprintf (stderr, "   arg[%2d] = %s\n", i, argv[i]);
   }
#endif

   freesymbols = NULL;
   freexrefs = NULL;

   verbose = FALSE;
   noloader = FALSE;
   bootobject = FALSE;

   genxref = FALSE;
   listmode = FALSE;
   widemode = FALSE;
   stdlist = FALSE;

   filecnt = 0;
   p1erralloc = 0;
   cpumodel = 7080;

   deckname[0] = '\0';
   titlebuf[0] = '\0';

   /*
   ** Scan off arguments.
   */

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];

      if (*bp == '-')
      {
	 for (bp++; *bp; bp++) switch (*bp)
         {
         case 'b': /* boot object flag */
	    bootobject = TRUE;
	    noloader = TRUE;
	    break;

	 case 'c': /* CPU model */
	    i++;
	    if ((cp = strchr (argv[i], '-')) != NULL)
	    {
	       *cp++ = '\0';
	       cpumodel = atoi (argv[i]);
	       if (!strcmp (cp, "III"))
		  cpumodel = cpumodel * 10 + 3;
	    }
	    else
	       cpumodel = atoi(argv[i]);
	    switch (cpumodel)
	    {
	    case 705:
	    case 7053:
	    case 7080:
	       break;
	    default:
	       goto USAGE;
	    }
	    break;

	 case 'd': /* Deckname */
	    i++;
	    if (strlen (argv[i]) > MAXSYMLEN) argv[i][MAXSYMLEN] = '\0';
	    strcpy (deckname, argv[i]);
	    break;

	 case 'l': /* Listing file */
	    i++;
	    lstfile = argv[i];
	    listmode = TRUE;
	    break;

         case 'n': /* No loader flag */
	    noloader = TRUE;
	    break;

         case 'o': /* Output file */
            i++;
            outfile = argv[i];
            break;

	 case 't': /* Title */
	    i++;
	    if (strlen (argv[i]) > WIDETITLELEN) argv[i][WIDETITLELEN] = '\0';
	    strcpy (titlebuf, argv[i]);
	    break;

	 case 'V': /* Verbose mode */
	    verbose = TRUE;
	    break;

	 case 's': /* Standard list format */
	    stdlist = TRUE;
	    /* fall through */

	 case 'w': /* Wide mode */
	    widemode = TRUE;
	    break;

         case 'x': /* Generate Xref */
	    genxref = TRUE;
	    break;

         default:
      USAGE:
	    fprintf (stderr, "asm7080 - version %s\n", VERSION);
	    fprintf (stderr,
		 "usage: asm7080 [-options] -o file.obj file.asm\n");
            fprintf (stderr, " options:\n");
	    fprintf (stderr, "    -b           - Boot loader format\n");
	    fprintf (stderr,
		 "    -c model     - CPU model (705, 705-I, 705-III, 7080)\n");
	    fprintf (stderr, "    -d deck      - Deckname for output\n");
	    fprintf (stderr,
		 "    -l listfile  - Generate listing to listfile\n");
	    fprintf (stderr, "    -n           - No loader records\n");
	    fprintf (stderr, "    -o outfile   - Object file name\n");
	    fprintf (stderr, "    -s           - Generate standard listing\n");
	    fprintf (stderr, "    -t title     - Title for listing\n");
	    fprintf (stderr, "    -w           - Generate wide listing\n");
	    fprintf (stderr, "    -x           - Generate cross reference\n");
	    return (ABORT);
         }
      }

      else
      {
         if (infile) goto USAGE;
         infile = argv[i];
      }

   }
   if (!widemode) titlebuf[NARROWTITLELEN] = '\0';

   if (!infile && !outfile) goto USAGE;

   if (!strcmp (infile, outfile))
   {
      fprintf (stderr, "asm7080: Identical source and object file names\n");
      exit (1);
   }
   if (listmode)
   {
      if (!strcmp (outfile, lstfile))
      {
	 fprintf (stderr, "asm7080: Identical object and listing file names\n");
	 exit (1);
      }
      if (!strcmp (infile, lstfile))
      {
	 fprintf (stderr, "asm7080: Identical source and listing file names\n");
	 exit (1);
      }
   }

#ifdef DEBUG
   fprintf (stderr, " infile = %s\n", infile);
   fprintf (stderr, " outfile = %s\n", outfile);
   if (listmode)
      fprintf (stderr, " lstfile = %s\n", lstfile);
#endif

   /*
   ** Open the files.
   */

   if ((infd = fopen (infile, "r")) == NULL)
   {
      perror ("asm7080: Can't open input file");
      exit (1);
   }

   if ((outfd = fopen (outfile, "w")) == NULL)
   {
      perror ("asm7080: Can't open output file");
      exit (1);
   }

   if (listmode) {
      if ((lstfd = fopen (lstfile, "w")) == NULL)
      {
	 perror ("asm7080: Can't open listing file");
	 exit (1);
      }
   }

   /*
   ** Create a temp files for intermediate use.
   */

#if defined(WIN32)
   sprintf (tname0, "a0%s", TEMPSPEC);
#else
   sprintf (tname0, "/tmp/a0%s", TEMPSPEC);
#endif
   if ((tmpdes0 = mkstemp (tname0)) < 0)
   {
      perror ("asm7080: Can't mkstemp");
      return (ABORT);
   }

#ifdef DEBUGTMPFILE
   fprintf (stderr, "asm7080: tmp0 = %s\n", tname0);
#endif

   if ((tmpfd0 = fdopen (tmpdes0, "w+")) == NULL)
   {
      perror ("asm7080: Can't open temporary file");
      exit (1);
   }

   /*
   ** Get current date/time.
   */

   curtime = time(NULL);
   timeblk = localtime(&curtime);
   strcpy (datebuf, ctime(&curtime));
   *strchr (datebuf, '\n') = '\0';

   /*
   ** Detab the source, so we know where we are on a line.
   */

   if (verbose) fprintf (stderr, "\n");
   status = detab (infd, tmpfd0, "source");

   /*
   ** Rewind the input.
   */

   if (fseek (tmpfd0, 0, SEEK_SET) < 0)
   {
      perror ("asm7080: Can't rewind input temp file");
      exit (1);
   }

   /*
   ** Allow multiple assemblies in source
   */

   done = FALSE;
   while (!done)
   {

      /*
      ** Initalize various tables
      */

      for (i = 0; i < MAXPCCOUNTERS; i++)
	 pccounter[i] = DEFAULTPCLOC;

      for (i = 0; i < MAXSYMBOLS; i++)
	 symbols[i] = NULL;

      exprtype = ADDREXPR;

      pc = DEFAULTPCLOC;
      curpcindex = 0;

      symbolcount = 0;
      errcount = 0;
      errnum = 0;
      linenum = 0;
      p1errcnt = 0;
      pgmlength = 0;
      radix = 10;
      litorigin = 0;
      litpoolsize = 0;
      filenum = 0;

      ttlbuf[0] = '\0';

      pscanbuf = inbuf;

      if (verbose)
	 fprintf (stderr, "\nasm7080: Processing file %d\n", filecnt+1);

      /*
      ** Open the intermediate temp file
      */

#if defined(WIN32)
      sprintf (tname1, "a1%s", TEMPSPEC);
#else
      sprintf (tname1, "/tmp/a1%s", TEMPSPEC);
#endif
      if ((tmpdes1 = mkstemp (tname1)) < 0)
      {
	 perror ("asm7080: Can't mkstemp");
	 return (ABORT);
      }

#ifdef DEBUGTMPFILE
      fprintf (stderr, "asm7080: tmp1 = %s\n", tname1);
#endif

      if ((tmpfd1 = fdopen (tmpdes1, "w+b")) == NULL)
      {
	 perror ("asm7080: Can't open temporary file");
	 exit (1);
      }

      /*
      ** Call pass 0 to preprocess macros, etc.
      */

      inpass = 000;
      if (verbose)
	 fprintf (stderr, "asm7080: Calling pass %03o\n", inpass);

      status = asmpass0 (tmpfd0, tmpfd1);
      if (status)
      {
	 if (verbose)
	    fprintf  (stderr, "asm7080: File %d: EOF\n",
		     filecnt+1);
	 if (tmpfd1)
	 {
	    fclose (tmpfd1);
	    unlink (tname1);
	 }
	 tmpfd1 = NULL;
         done = TRUE;
	 if (status == 2)
	 {
	    if (listmode)
	       fprintf (lstfd, "\nNo END record\n");
	    filecnt++;
	    goto PRINT_ERROR;
	 }
	 else
	 {
	    status = 0;
	 }
	 break;
      }
      if (filecnt++ > 0 && listmode)
         fputc ('\f', lstfd);

      /* 
      ** Call pass 1 to scan the source and get labels.
      */

      curpcindex = 0;
      inpass = 010;
      if (verbose)
	 fprintf (stderr, "asm7080: Calling pass %03o\n", inpass);

      status = asmpass1 (tmpfd1, TRUE);

      /*
      ** Call pass 2 to generate object and optional listing
      */

      curpcindex = 0;
      inpass = 020;
      if (verbose)
	 fprintf (stderr, "asm7080: Calling pass %03o\n", inpass);

      status = asmpass2 (tmpfd1, outfd, listmode, lstfd);

      /*
      ** Close the intermediate file
      */

      if (tmpfd1)
      {
	 fclose (tmpfd1);
	 unlink (tname1);
      }
      tmpfd1 = NULL;

      /*
      ** Place table entries on free lists for use again.
      */

      for (i = 0; i < symbolcount; i++)
      {
	 if (symbols[i])
	 {
	    XrefNode *xr, *nxr;

	    xr = symbols[i]->xref_head;
	    while (xr)
	    {
	       nxr = xr->next;
	       freexref (xr);
	       xr = nxr;
	    }
	    freesym (symbols[i]);
	 }
      }

      /*
      ** Print assembly error count.
      */

   PRINT_ERROR:
      if (listmode)
      {
	 fprintf (lstfd, "\n%d Errors in assembly\n", errcount);
      }
      if (errcount || verbose)
      {
	 fprintf (stderr, "asm7080: File %d: %d Errors in assembly\n",
		  filecnt, errcount);
      }

      titlebuf[0] = '\0';
      deckname[0] = '\0';
      DBG_MEMCHECK();
   }

   /*
   ** Close the files
   */


   if (infd)
      fclose (infd);
   if (outfd)
      fclose (outfd);
   if (tmpfd0)
   {
      fclose (tmpfd0);
      unlink (tname0);
   }
   if (listmode && lstfd)
      fclose (lstfd);

   if (status != 0)
      unlink (outfile);

#ifdef DEBUGMALLOCSDUMP
   DBG_MEMDUMP();
#endif

   return (status == 0 ? NORMAL : ABORT);
}
