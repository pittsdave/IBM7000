/***********************************************************************
*
* asm7090 - Assembler for the IBM 7090 computer.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/13/03   DGP   Added XREF support.
*   12/20/04   DGP   Added MAP support and 2.0 additions.
*   02/03/05   DGP   Added JOBSYM mode.
*   02/08/05   DGP   Added FAP support and multiple assembly.
*   03/08/05   DGP   Added CPU model.
*   03/25/05   DGP   Added 704 model and instructions.
*   06/07/05   DGP   Added CTSS insts and RMT and TAPENO psops.
*   08/19/05   SCM   Added MSYS ifdefs.
*   12/15/05   RMS   MINGW changes.
*   07/30/07   DGP   Change mktemp to mkstemp.
*   09/30/09   DGP   Added FAP Linkage Director.
*   04/30/10   DGP   Added sequence checking option.
*   05/10/10   DGP   Added opcore ACORE/BCORE support.
*   06/07/10   DGP   Added punch symbol table option.
*   06/15/10   DGP   Added XBUILD sym def for CTSS.
*   11/01/10   DGP   Added CTSS Common mode.
*   03/30/11   DGP   Allow multiple include directories.
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

int absolute;			/* In absolute section */
int absmod;			/* In absolute module */
int genxref;			/* Generate cross reference listing */
int widemode;			/* Generate wide listing */
int sequcheck;			/* Check sequence numbers */
int lastsequ;			/* Last sequence number */
int monsym;			/* Include IBSYS Symbols (MONSYM) */
int jobsym;			/* Include IBJOB Symbols (JOBSYM) */
int addext;			/* Add extern for undef'd symbols (!absolute) */
int addrel;			/* ADDREL mode */
int termparn;			/* Parenthesis are terminals (NO()) */
int rboolexpr;			/* RBOOL expression */
int lboolexpr;			/* LBOOL expression */
int fapmode;			/* FAP assembly mode */
int exprtype;			/* Expression type */
int pc;				/* the assembler pc */
int symbolcount;		/* Number of symbols in symbol table */
int entsymbolcount;		/* Number of symbols in entry symbol table */
int opdefcount;			/* Number of user defined opcode in opdef */
int begincount;			/* Number of BEGINs */
int inpass;			/* Which pass are we in */
int errcount;			/* Number of errors in assembly */
int warncount;			/* Number of warnings in assembly */
int errnum;			/* Index into errline array */
int warnnum;			/* Index into warnline array */
int linenum;			/* Source line number */
int p1errcnt;			/* Number of pass 0/1 errors */
int p1erralloc;			/* Number of pass 0/1 errors allocated */
int pgmlength;			/* Length of program */
int radix;			/* Number scanner radix */
int qualindex;			/* QUALifier table index */
int litorigin;			/* Literal pool origin */
int litpool;			/* Literal pool size */
int headcount;			/* Number of entries in headtable */
int cpumodel;			/* CPU model (709, 7090 7094) */
int filenum;			/* INSERT file index */
int fileseq;			/* INSERT file sequence */
int genlnkdir;			/* FAP generate linkage director */
int nolnkdir;			/* FAP DO NOT generate linkage director */
int bcore;			/* BCORE assembly */
int punchsymtable;		/* Punch symbol table */
int ctsscommon;			/* Relocate CTSS Common mode */
int commonctr;			/* Common counter */
int commonused;			/* Common used */
int incldircnt;			/* Include dir count */

char errline[10][120];		/* Pass 2 error lines for current statment */
char warnline[10][120];		/* Pass 2 warning lines for current statment */
char inbuf[MAXLINE];		/* The input buffer for the scanners */
char lbl[MAXLBLLEN+2];		/* The object label */
char deckname[MAXLBLLEN+2];	/* The assembly deckname */
char ttlbuf[TTLSIZE+2];		/* The assembly TTL buffer */
char titlebuf[TTLSIZE+2];	/* The assembly TITLE buffer */
char qualtable[MAXQUALS][MAXSYMLEN+2]; /* The QUALifier table */
char headtable[MAXHEADS];	/* HEAD table */
char datebuf[48];		/* Date/Time buffer for listing */
char *incldir[MAXINCLUDES];	/* -I include directory */
char lnkdirsym[MAXSYMLEN+2];	/* Linkage director symbol */

SymNode *addextsym;		/* Added symbol for externals */
SymNode *symbols[MAXSYMBOLS];	/* The Symbol table */
SymNode *entsymbols[MAXSYMBOLS];/* The Entry Symbol table */
SymNode *freesymbols;		/* Reusable symbols nodes */
XrefNode *freexrefs;		/* Reusable xref nodes */
OpDefCode *freeops;		/* Reusable opdef nodes */
OpDefCode *opdefcode[MAXDEFOPS];/* The user defined opcode table */
ErrTable p1error[MAXERRORS];	/* The pass 0/1 error table */
BeginTable begintable[MAXBEGINS];/* The USE/BEGIN table */
INSERTfile files[MAXASMFILES];	/* INSERT file descriptors */
RMTSeq *rmthead;		/* Pointer to start of remote sequences */
RMTSeq *rmttail;		/* Pointer to end of remote sequences */
time_t curtime;			/* Assembly time */
struct tm *timeblk;		/* Assembly time */
char *pscanbuf;			/* Pointer for tokenizers */

static int verbose;		/* Verbose mode */
static int gblabsmod;		/* In absolute module */
static int gblmonsym;		/* Include IBSYS Symbols (MONSYM) */
static int gbljobsym;		/* Include IBJOB Symbols (JOBSYM) */
static int filecnt;		/* File process count */
static int reccnt;		/* File record count */

#include "asmsymbols.h"

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
      fprintf (stderr, "asm7090: Detab %s file \n", type);
   reccnt = 0;

   /* set initial tab stops */

   settab (tabs, 4, tabstops);

   detabit (tabs, ifd, ofd);

   if (verbose)
      fprintf (stderr, "asm7090: File contains %d records\n", reccnt);
   return (0);

} /* detab */

/***********************************************************************
* definemonsyms - Define MONSYM symbols
***********************************************************************/

void
definemonsyms (int fapselect)
{
   SysDefs *ss;
   int i;
   int oldline;

#ifdef DEBUGMONSYM
   fprintf (stderr,
	 "definemonsyms: fapmode = %d, absmod = %d, fapselect = %d\n",
	    fapmode, absmod, fapselect);
#endif
   if (!fapmode && !absmod) return;

   if (fapmode)
   {
      if (fapselect == 1)
      {
	 if (ffapdefsset) return;
	 ss = ffapmondefs;
	 ffapdefsset = TRUE;
      }
      else
      {
	 if (ifapdefsset) return;
	 ss = ifapmondefs;
	 ifapdefsset = TRUE;
      }
   }
   else
   {
      if (mondefsset) return;
      ss = mapmondefs;
      mondefsset = TRUE;
   }

   oldline = linenum;
   linenum = 0;
   for (i = 0; ss[i].val >= 0; i++)
   {
      SymNode *s;

      s = symlookup (ss[i].name, ss[i].qual, TRUE, TRUE);

      if (s)
      {
	 s->value = ss[i].val;
	 s->flags = 0;
      }
   }
   linenum = oldline;
}

/***********************************************************************
* definejobsyms - Define JOBSYM symbols
***********************************************************************/

void
definejobsyms (void)
{
   int i;
   int oldline;

   if (!absmod) return;

   /*
   ** JOBSYM gets MONSYM, too
   */

   definemonsyms(0);

   if (!jobdefsset)
   {
      oldline = linenum;
      linenum = 0;
      jobdefsset = TRUE;
      for (i = 0; jobdefs[i].val >= 0; i++)
      {
	 SymNode *s;

	 if ((s = symlookup (jobdefs[i].name, jobdefs[i].qual, TRUE, TRUE))
	       != NULL)
	 {
	    s->value = jobdefs[i].val;
	    s->flags = 0;
	 }
      }
      linenum = oldline;
   }
}

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
   char *bp;
   int i;
   int done;
   int listmode;
   int status = 0;
   int tmpdes0, tmpdes1;
   char tname0[MAXFILENAMELEN+2];
   char tname1[MAXFILENAMELEN+2];
  
#ifdef DEBUG
   fprintf (stderr, "asm7090: Entered:\n");
   fprintf (stderr, "args:\n");
   for (i = 1; i < argc; i++)
   {
      fprintf (stderr, "   arg[%2d] = %s\n", i, argv[i]);
   }
#endif

   freesymbols = NULL;
   freexrefs = NULL;
   freeops = NULL;

   termparn = TRUE;

   gblabsmod = FALSE;
   gbljobsym = FALSE;
   gblmonsym = FALSE;
   verbose = FALSE;

   genxref = FALSE;
   fapmode = FALSE;
   listmode = FALSE;
   widemode = FALSE;
   sequcheck = FALSE;
   punchsymtable = FALSE;
   ctsscommon = FALSE;

   filecnt = 0;
   p1erralloc = 0;
   incldircnt = 0;
   cpumodel = 7090;

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
	 case 'a': /* Absolute assembly */
	    gblabsmod = TRUE;
	    break;

	 case 'C': /* Relocate CTSS Common mode */
	    ctsscommon = TRUE;
	    break;

	 case 'c': /* CPU model */
	    i++;
	    if (i >= argc) goto USAGE;
	    if (!strcmp (argv[i], "CTSS") || !strcmp (argv[i], "ctss"))
	    {
	       /* For CTSS we add model 7096 and run in FAP mode */
	       cpumodel = 7096;
	       fapmode = TRUE;
	       termparn = FALSE;
	    }
	    else
	    {
	       cpumodel = atoi(argv[i]);
	       switch (cpumodel)
	       {
	       case 704:
	       case 709:
	       case 7090:
	       case 7094:
		  break;
	       default:
		  goto USAGE;
	       }
	    }
	    break;

	 case 'd': /* Deckname */
	    i++;
	    if (i >= argc) goto USAGE;
	    if (strlen (argv[i]) > MAXSYMLEN) argv[i][MAXSYMLEN] = '\0';
	    strcpy (deckname, argv[i]);
	    break;

	 case 'f': /* FAP mode */
	    fapmode = TRUE;
	    termparn = FALSE;
	    break;

	 case 'I': /* Include directory */
	    i++;
	    if (i >= argc) goto USAGE;
	    incldir[incldircnt++] = argv[i];
	    break;

	 case 'j': /* JOBSYM */
	    gbljobsym = TRUE;
	    break;

	 case 'l': /* Listing file */
	    i++;
	    if (i >= argc) goto USAGE;
	    lstfile = argv[i];
	    listmode = TRUE;
	    break;

	 case 'm': /* MONSYM */
	    gblmonsym = TRUE;
	    break;

         case 'o': /* Output file */
            i++;
	    if (i >= argc) goto USAGE;
            outfile = argv[i];
            break;

	 case 'p': /* Parens OK == ()OK */
	    termparn = FALSE;
	    break;

	 case 'r': /* ADDREL mode */
	    gblabsmod = FALSE;
	    break;

	 case 's': /* punch symbol table */
	    punchsymtable = TRUE;
	    break;

	 case 'S': /* Sequence check */
	    sequcheck = TRUE;
	    break;

	 case 't': /* Title */
	    i++;
	    if (i >= argc) goto USAGE;
	    if (strlen (argv[i]) > WIDETITLELEN) argv[i][WIDETITLELEN] = '\0';
	    strcpy (titlebuf, argv[i]);
	    break;

	 case 'V': /* Verbose mode */
	    verbose = TRUE;
	    break;

	 case 'w': /* Wide mode */
	    widemode = TRUE;
	    break;

         case 'x': /* Generate Xref */
	    genxref = TRUE;
	    break;

         default:
      USAGE:
	    fprintf (stderr, "asm7090 - version %s\n", VERSION);
	    fprintf (stderr,
		 "usage: asm7090 [-options] -o file.obj file.asm\n");
            fprintf (stderr, " options:\n");
	    fprintf (stderr, "    -a         - Absolute assembly\n");
	    fprintf (stderr, "    -C         - Relocate CTSS Program Common\n");
	    fprintf (stderr,
		 "    -c model   - CPU model (704, 709, 7090, 7094, CTSS)\n");
	    fprintf (stderr, "    -d deck    - Deckname for output\n");
	    fprintf (stderr, "    -f         - FAP assembly mode\n");
	    fprintf (stderr, "    -I incldir - Include directory for INSERT\n");
	    fprintf (stderr, "    -j         - JOBSYM mode\n");
	    fprintf (stderr, "    -l lstfile - Generate listing to lstfile\n");
	    fprintf (stderr, "    -m         - MONSYM mode\n");
	    fprintf (stderr, "    -o outfile - Object file name\n");
	    fprintf (stderr, "    -p         - ParensOK == ()OK\n");
	    fprintf (stderr, "    -r         - ADDREL mode\n");
	    fprintf (stderr, "    -s         - Punch symbol table\n");
	    fprintf (stderr, "    -t title   - Title for listing\n");
	    fprintf (stderr, "    -w         - Generate wide listing\n");
	    fprintf (stderr, "    -x         - Generate cross reference\n");
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

   if (!infile || !outfile) goto USAGE;

   if (!strcmp (infile, outfile))
   {
      fprintf (stderr, "asm7090: Identical source and object file names\n");
      exit (1);
   }
   if (listmode)
   {
      if (!strcmp (outfile, lstfile))
      {
	 fprintf (stderr, "asm7090: Identical object and listing file names\n");
	 exit (1);
      }
      if (!strcmp (infile, lstfile))
      {
	 fprintf (stderr, "asm7090: Identical source and listing file names\n");
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
      perror ("asm7090: Can't open input file");
      exit (1);
   }

   if ((outfd = fopen (outfile, "w")) == NULL)
   {
      perror ("asm7090: Can't open output file");
      exit (1);
   }

   if (listmode) {
      if ((lstfd = fopen (lstfile, "w")) == NULL)
      {
	 perror ("asm7090: Can't open listing file");
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
      perror ("asm7090: Can't mkstemp");
      return (ABORT);
   }

#ifdef DEBUGTMPFILE
   fprintf (stderr, "asm7090: tmp0 = %s\n", tname0);
#endif

   if ((tmpfd0 = fdopen (tmpdes0, "w+")) == NULL)
   {
      perror ("asm7090: Can't open temporary file");
      exit (1);
   }

   /*
   ** Get current date/time.
   */

   curtime = time(NULL);
   timeblk = localtime(&curtime);
   strcpy (datebuf, ctime(&curtime));
   *strchr (datebuf, '\n') = '\0';

#ifdef DEBUGPARN
   fprintf (stderr, "termparnI = %s\n", termparn ? "TRUE" : "FALSE");
#endif

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
      perror ("asm7090: Can't rewind input temp file");
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

      for (i = 0; i < MAXSYMBOLS; i++)
      {
	 symbols[i] = NULL;
      }

      for (i = 0; i < MAXSYMBOLS; i++)
      {
	 entsymbols[i] = NULL;
      }

      for (i = 0; i < MAXDEFOPS; i++)
      {
	 opdefcode[i] = NULL;
      }

      addextsym = NULL;
      rmthead = NULL;
      rmttail = NULL;

      rboolexpr = FALSE;
      lboolexpr = FALSE;
      mondefsset = FALSE;
      jobdefsset = FALSE;
      ifapdefsset = FALSE;
      ffapdefsset = FALSE;
      genlnkdir = FALSE;
      nolnkdir = FALSE;
      bcore = FALSE;
      commonused = FALSE;

      exprtype = ADDREXPR;

      absmod = gblabsmod;
      monsym = gblmonsym;
      jobsym = gbljobsym;
      if (absmod)
      {
	 addext = FALSE;
	 addrel = FALSE;
	 absolute = TRUE;
      }
      else
      {
	 addext = TRUE;
	 addrel = TRUE;
	 absolute = FALSE;
      }

      pc = 0;
      symbolcount = 0;
      entsymbolcount = 0;
      opdefcount = 0;
      headcount = 0;
      errcount = 0;
      warncount = 0;
      errnum = 0;
      linenum = 0;
      p1errcnt = 0;
      pgmlength = 0;
      radix = 10;
      qualindex = 0;
      litorigin = 0;
      litpool = 0;
      filenum = 0;
      fileseq = 0;

      commonctr = FAPCOMMONSTART;

      ttlbuf[0] = '\0';
      lbl[0] = '\0';
      qualtable[0][0] = '\0';

      begincount = 1;
      strcpy (begintable[0].symbol, "  ");
      begintable[0].chain = 0;
      begintable[0].bvalue = 0;
      begintable[0].value = 0;
      pscanbuf = inbuf;

      if (verbose)
	 fprintf (stderr, "\nasm7090: Processing file %d\n", filecnt+1);

      /*
      ** If MONSYM mode, define symbols
      */

      if (monsym)
	 definemonsyms (0);

      /*
      ** If JOBSYM mode, define symbols
      */

      if (jobsym)
	 definejobsyms ();

      /*
      ** If CTSS mode, define XBUILD
      */

      if (cpumodel == 7096)
      {
	 SymNode *s;

	 if (s = symlookup ("XBUILD", "", TRUE, TRUE))
	 {
	    s->value = 1;
	    s->flags = SETVAR;
	 }
      }

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
	 perror ("asm7090: Can't mkstemp");
	 return (ABORT);
      }

#ifdef DEBUGTMPFILE
   fprintf (stderr, "asm7090: tmp1 = %s\n", tname1);
#endif

      if ((tmpfd1 = fdopen (tmpdes1, "w+b")) == NULL)
      {
	 perror ("asm7090: Can't open temporary file");
	 exit (1);
      }

      /*
      ** Call pass 0 to preprocess macros, etc.
      */

      inpass = 000;
      if (verbose)
	 fprintf (stderr, "asm7090: Calling pass %03o\n", inpass);

      status = asmpass0 (tmpfd0, tmpfd1);
#ifdef DEBUGPARN
      fprintf (stderr, "termparn0 = %s\n", termparn ? "TRUE" : "FALSE");
#endif
      if (status)
      {
	 if (verbose)
	    fprintf  (stderr, "asm7090: File %d: EOF\n",
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

      inpass = 010;
      if (verbose)
	 fprintf (stderr, "asm7090: Calling pass %03o\n", inpass);

      status = asmpass1 (tmpfd1, TRUE);
#ifdef DEBUGPARN
      fprintf (stderr, "termparn1 = %s\n", termparn ? "TRUE" : "FALSE");
#endif

      /* 
      ** Call pass 1 again to scan the source and assign storage.
      ** We have to do this again because MAP allows forward decls of
      ** variables that affect storage. Also, USE/BEGIN and COMMON.
      */

      if (!fapmode)
      {
	 inpass = 011;
	 if (verbose)
	    fprintf (stderr, "asm7090: Calling pass %03o\n", inpass);

	 status = asmpass1 (tmpfd1, FALSE);
	 status = asmpass1 (tmpfd1, FALSE);

	 inpass = 012;
	 if (verbose)
	    fprintf (stderr, "asm7090: Calling pass %03o\n", inpass);

	 status = asmpass1 (tmpfd1, FALSE);
      }

      /*
      ** Call pass 2 to generate object and optional listing
      */

      inpass = 020;
      if (verbose)
	 fprintf (stderr, "asm7090: Calling pass %03o\n", inpass);

      status = asmpass2 (tmpfd1, outfd, listmode, lstfd);
#ifdef DEBUGPARN
      fprintf (stderr, "termparn2 = %s\n", termparn ? "TRUE" : "FALSE");
#endif

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

      for (i = 0; i < entsymbolcount; i++)
      {
	 if (entsymbols[i])
	 {
	    XrefNode *xr, *nxr;

	    xr = symbols[i]->xref_head;
	    while (xr)
	    {
	       nxr = xr->next;
	       freexref (xr);
	       xr = nxr;
	    }
	    freesym (entsymbols[i]);
	 }
      }

      for (i = 0; i < opdefcount; i++)
      {
	 if (opdefcode[i])
	    freeopd (opdefcode[i]);
      }

      while (rmthead)
      {
         RMTSeq *lclnext;

	 lclnext = rmthead->next;
	 DBG_MEMREL (rmthead);
	 rmthead = lclnext;
      }

      /*
      ** Print assembly error count.
      */

   PRINT_ERROR:
      if (listmode)
      {
	 fprintf (lstfd, "\n%d Warnings in assembly\n", warncount);
	 fprintf (lstfd, "%d Errors in assembly\n", errcount);
      }
      if (errcount || warncount || verbose)
      {
	 fprintf (stderr,
		  "asm7090: File %d: %d Errors, %d Warnings in assembly\n",
		  filecnt, errcount, warncount);
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
