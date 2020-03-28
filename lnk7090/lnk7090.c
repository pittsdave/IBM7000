/************************************************************************
*
* lnk7090 - Links objects from asm7090
*
* Changes:
*   05/21/03   DGP   Original.
*   12/28/04   DGP   Changed to new object tags and new memory layout.
*   02/14/05   DGP   Revamped operation to allow stacked objects and new
*                    link map listing format.
*   03/29/10   DGP   Added module numbers to listing.
*   03/30/10   DGP   Added cross reference and wide listing.
*   06/10/10   DGP   Added COMMON support.
*   06/10/10   DGP   Added "MOVIE)" support.
*   06/14/10   DGP   Added library support.
*   08/24/10   DGP   Fixed COMMON support.
*   11/02/10   DGP   More COMMON changes.
*   12/24/10   DGP   Correct page count to eject new page.
*	
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>

#include "lnkdef.h"
#include "nativebcd.h"
#include "tobcd.h"

FILE *lstfd = NULL;

int pc; /* the linker pc */
int listmode = FALSE;
int absolute = FALSE;
int partiallink = FALSE;
int undefs = FALSE;
int muldefs = FALSE;
int genxref = FALSE;
int widelist = FALSE;
int movieref = FALSE;
int ctsscommon = FALSE;
int symbolcount = 0;
int modcount = 0;
int modnumber = 0;
int errcount = 0;
int muldefcount = 0;
int undefcount = 0;
int warncount = 0;
int pgmlength = 0;
int absentry = -1;
int relentry = -1;
int pass = 0;
int common = -1;
int commonorg = 0;
int commonmod = -1;
int commontop = 077777;

char errline[120];
char deckname[MAXSYMLEN+2];
char inbuf[MAXLINE];

SymNode *symbols[MAXSYMBOLS];
Module modules[MAXMODULES];
Memory memory[MEMSIZE];
struct tm *timeblk;

static int linecnt = MAXLINE;
static int pagenum = 0;
static char datebuf[48];
static char subtitle[132];

#ifdef DEBUGDUMP
/***********************************************************************
* octdump - Octal dump routine.
***********************************************************************/

static void
octdump (FILE *file, int size)
{
   int jjj;
   int iii;
   int pc;
   int dmpmax;

   dmpmax = 8;

   for (iii = 0; iii < (size);)
   {
      fprintf (file, "%05o  ", iii);

      for (jjj = 0; jjj < dmpmax; jjj++)
      {
         fprintf (file, "%12.12llo ", memory[iii].word);
	 iii++;
      }
      fprintf ((file), "\n");
   }
}
#endif

/***********************************************************************
* printheader - Print header on listing.
***********************************************************************/

void
printheader (FILE *lstfd)
{
   if (linecnt >= LINESPAGE)
   {
      linecnt = 4;
      if (pagenum) fputc ('\f', lstfd);
      if (widelist)
	 fprintf (lstfd, H2FORMAT, VERSION, deckname, datebuf, ++pagenum);
      else
	 fprintf (lstfd, H1FORMAT, VERSION, deckname, datebuf, ++pagenum);
      fprintf (lstfd, "%s\n", subtitle);
   }
}

/***********************************************************************
* printsymbols - Print the symbol table.
***********************************************************************/

static void
printsymbols (FILE *lstfd)
{
   int i, j, k;
   int symppage;
   char type;
   char type1;

   j = 0;

   linecnt = MAXLINE;
   if (genxref)
   {
      strcpy (subtitle, "SYMBOL  VALUE  NUM                  REFERENCES\n");
      if (widelist)
	 symppage = 9;
      else
	 symppage = 5;
   }
   else
   {
      subtitle[0] = 0;
      if (widelist)
	 symppage = 6;
      else
	 symppage = 4;
      for (i = 0; i < symppage; i++)
	 strcat (subtitle, "SYMBOL  VALUE  NUM  ");
      strcat (subtitle, "\n");
   }

   for (i = 0; i < symbolcount; i++)
   {
      printheader (lstfd);

      if (partiallink)
      {
	 if (symbols[i]->global) type = 'G';
	 else if (symbols[i]->external) type = 'E';
      }
      else if (symbols[i]->relocatable) type = 'R';
      else type = ' ';

      if (symbols[i]->muldef)
      {
	 muldefs = TRUE;
	 warncount++;
	 muldefcount++;
	 type1 = 'M';
      }
      else if (symbols[i]->undef)
      {
	 if (!partiallink) errcount++;
	 undefcount++;
	 undefs = TRUE;
	 type1 = 'U';
      }
      else type1 = ' ';

      fprintf (lstfd, SYMFORMAT,
	       symbols[i]->symbol,
	       symbols[i]->value,
	       type, type1, symbols[i]->modnum);
      if (genxref)
      {
	 XrefNode *ref = symbols[i]->xref_head;

         k = 0;
	 while (ref)
	 {
	    fprintf (lstfd, "%05o %3d  ", ref->value, ref->modnum);
	    if (++k >= symppage)
	    {
	       fprintf (lstfd, "\n");
	       linecnt++;
	       printheader (lstfd);
	       fprintf (lstfd, "                    ");
	       k = 0;
	    }
	    ref = ref->next;
	 }
	 fprintf (lstfd, "\n");
	 linecnt++;
      }
      else
      {
	 j++;
	 if (j == symppage)
	 {
	    fprintf (lstfd, "\n");
	    linecnt++;
	    j = 0;
	 }
      }
   }
   fprintf (lstfd, "\n");
}

/***********************************************************************
* fillmovie - Fill the movie symbol table.
***********************************************************************/

static void
fillmovie (void)
{
   SymNode *s;
   t_uint64 ldata;
   int i;

   if ((s = symlookup (MOVIE, "SYSTEM", FALSE)) == NULL)
   {
      if (s = symlookup (MOVIE, "SYSTEM", TRUE))
      {
	 s->value = pc;
	 s->global = TRUE;
	 s->undef = FALSE;
	 s->relocatable = TRUE;
      }
   }
   else
   {
      int refaddr;
      s->undef = FALSE;
      refaddr = s->value;
      s->value = pc;
      while (refaddr)
      {
	 int k;
	 char kt;

	 /* TODO: Must handle DECR & BOTH */
	 k = (int)(memory[refaddr].word & ~ADDRMASK);
	 kt = memory[refaddr].tag;
	 memory[refaddr].word =
	       (memory[refaddr].word & ADDRMASK) | s->value;
	 if (memory[refaddr].tag == RELDECR_TAG)
	 {
	    FILE *fd;
	    errcount++;
	    if (listmode)
	       fd = lstfd;
	    else
	       fd = stderr;
	    fprintf (fd,
	       "RELDECR: sym = %s, refaddr = %o, module = %d\n",
		     s->symbol, refaddr, modnumber+1);
	 }
	 if (memory[refaddr].tag == RELBOTH_TAG)
	 {
	    FILE *fd;
	    errcount++;
	    if (listmode)
	       fd = lstfd;
	    else
	       fd = stderr;
	    fprintf (fd,
	       "RELBOTH: sym = %s, refaddr = %o, module = %d\n",
		     s->symbol, refaddr, modnumber+1);
	 }
	 memory[refaddr].tag = RELADDR_TAG;
	 chkxref (s, refaddr);
	 refaddr = k;
      }
   }

   ldata = ((t_uint64)symbolcount << 18) | pc;
   memory[pc].word = ldata;
   memory[pc].tag = RELADDR_TAG;
   memory[pc].reladdr = TRUE;
   pc++;

   for (i = 0; i < symbolcount; i++)
   {
      int j;
      char sym[8];

      s = symbols[i];
      ldata = 0;
      sprintf (sym, "%-6.6s", s->symbol);
      for (j = 0; j < 6; j++)
      {
         ldata = (ldata << 6) | (t_uint64)tobcd[sym[i]];
      }
      memory[pc].word = ldata;
      memory[pc].tag = ABSDATA_TAG;
      pc++;

      ldata = s->value;
      memory[pc].word = ldata;
      memory[pc].tag = RELADDR_TAG;
      memory[pc].reladdr = TRUE;
      pc++;
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
   char *infile[MAXFILES];
   char *libdirs[MAXFILES];
   char *outfile = NULL;
   char *lstfile = NULL;
   char *bp;
   int i;
   int libdircnt = 0;
   int incnt = 0;
   int status = 0;
   time_t curtime;
  
#ifdef DEBUG
   printf ("lnk7090: Entered:\n");
   printf ("args:\n");
   for (i = 1; i < argc; i++)
   {
      printf ("   arg[%2d] = %s\n", i, argv[i]);
   }
#endif

   /*
   ** Clear files.
   */

   for (i = 0; i < MAXFILES; i++)
   {
      infile[i] = NULL;
      libdirs[i] = NULL;
   }

   /*
   ** Clear symboltable
   */

   for (i = 0; i < MAXSYMBOLS; i++)
   {
      symbols[i] = NULL;
   }

   /*
   ** Scan off the the args.
   */

   deckname[0] = '\0';

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];

      if (*bp == '-')
      {
	 bp++;
	 while (*bp) switch (*bp)
         {
	 case 'C': /* Relocate CTSS program Common */
	 case 'c': /* Relocate CTSS program Common */
	    ctsscommon = TRUE;
	    bp++;
	    break;

	 case 'd': /* Deckname */
	    i++;
	    if (i >= argc) goto USAGE;
	    if (strlen (argv[i]) > MAXSYMLEN) argv[i][MAXSYMLEN] = '\0';
	    strcpy (deckname, argv[i]);
	    bp++;
	    break;

	 case 'L':
	    libdirs[libdircnt++] = bp+1;
	    while (*bp) bp++;
	    break;

	 case 'm': /* List link map */
	    i++;
	    if (i >= argc) goto USAGE;
	    lstfile = argv[i];
	    listmode = TRUE;
	    bp++;
	    break;

         case 'o': /* Link output */
            i++;
	    if (i >= argc) goto USAGE;
            outfile = argv[i];
	    bp++;
            break;

	 case 'w': /* Widelist */
	    widelist = TRUE;
	    bp++;
	    break;

	 case 'x': /* Xref */
	    genxref = TRUE;
	    bp++;
	    break;

         default:
      USAGE:
	    fprintf (stderr, "lnk7090 - version %s\n", VERSION);
	    fprintf (stderr,
		  "usage: lnk7090 [-options] -o outfile.obj infile1.obj...\n");
	    fprintf (stderr, " options:\n");
	    fprintf (stderr, "    -C         - Relocate CTSS Program Common\n");
	    fprintf (stderr, "    -d deck    - Deckname for output\n");
	    fprintf (stderr, "    -Llibdir   - Library directory\n");
	    fprintf (stderr, "    -m lstfile - Generate link map listing\n");
	    fprintf (stderr, "    -o outfile - Linked object file name\n");
	    fprintf (stderr, "    -w         - Generate wide listing\n");
	    fprintf (stderr, "    -x         - Generate cross reference\n");
	    return (ABORT);
         }
      }

      else
      {
	 if (incnt >= MAXFILES)
	 {
	    fprintf (stderr,
	    	     "lnk7090: Maximum input files exceeded: maximum = %d\n",
	             MAXFILES);
	    exit (1);
	 }
         infile[incnt++] = argv[i];
      }

   }

   if (!infile[0] && !outfile) goto USAGE;

#ifdef DEBUG
   printf (" outfile = %s\n", outfile);
   for (i = 0; i < incnt; i++)
      printf (" infile[%d] = %s\n", i, infile[i]);
   for (i = 0; i < libdircnt; i++)
      printf (" libdirs[%d] = %s\n", i, libdirs[i]);
   if (listmode)
      printf (" lstfile = %s\n", lstfile);
#endif

   /*
   ** Open the files.
   */

   if ((outfd = fopen (outfile, "w")) == NULL)
   {
      perror ("lnk7090: Can't open output file");
      exit (1);
   }
   if (listmode) {
      if ((lstfd = fopen (lstfile, "w")) == NULL)
      {
	 perror ("lnk7090: Can't open listing file");
	 exit (1);
      }

   }

   /*
   ** Get current date/time.
   */

   curtime = time(NULL);
   timeblk = localtime(&curtime);
   strcpy (datebuf, ctime(&curtime));
   *strchr (datebuf, '\n') = '\0';

   /*
   ** Clear memory.
   */

   for (i = 0; i < MEMSIZE; i++)
   {
      memset (&memory[i], '\0', sizeof (Memory));
   }

   /*
   ** Load the objects for pass 1.
   */

   pass = 1;
   modcount = 0;
   modnumber = 0;
   for (i = 0; i < incnt; i++)
   {
      if ((infd = fopen (infile[i], "r")) == NULL)
      {
	 sprintf (inbuf, "lnk7090: Can't open input file %s", infile[i]);
	 perror (inbuf);
	 exit (1);
      }

      status = lnkloader (infd, pc, pass, infile[i]);

      fclose (infd);
   }

   /*
   ** Clear memory.
   */

   for (i = 0; i < MEMSIZE; i++)
   {
      memset (&memory[i], '\0', sizeof (Memory));
   }

   /*
   ** reLoad the objects for pass 2.
   */

   pc = 0;
   modnumber = 0;
   commonorg = 0;
   pass = 2;
   for (i = 0; i < incnt; i++)
   {
      if ((infd = fopen (infile[i], "r")) == NULL)
      {
	 sprintf (inbuf, "lnk7090: Can't open input file %s", infile[i]);
	 perror (inbuf);
	 exit (1);
      }

      status = lnkloader (infd, pc, pass, infile[i]);

      fclose (infd);
   }

   /*
   ** If libraries, search them.
   */

   for (i = 0; i < libdircnt; i++)
   {
      DIR *dirfd;

      if ((dirfd = opendir (libdirs[i])) != NULL)
      {
#ifdef DEBUG
	 printf (" Library directory: %s\n", libdirs[i]);
#endif
	 status = lnklibrary (dirfd, pc, 3, libdirs[i]);
	 closedir (dirfd);
      }
      else
      {
	 sprintf (inbuf, "lnk7090: Can't open library: %s", libdirs[i]);
	 perror (inbuf);
	 exit (1);
      }
   }

   /*
   ** If "MOVIE)" referenced, then fill it in.
   */

   if (movieref)
   {
      fillmovie();
   }
#ifdef DEBUGDUMP
   octdump (stderr, pc);
#endif

   if (listmode)
   {
      /*
      ** Print the modules
      */

      strcpy (subtitle,
	 "MODULE NUM  ORIGIN  LENGTH    DATE      TIME     CREATOR   FILE\n");
      for (i = 0; i < modcount; i++)
      {
	 printheader (lstfd);
         fprintf (lstfd, MODFORMAT, modules[i].name, i+1, modules[i].origin,
		  modules[i].length, modules[i].date, modules[i].time,
		  modules[i].creator, modules[i].objfile);
	 linecnt ++;
	 if (ctsscommon && i == commonmod && common > 0)
	 {
	    fprintf (lstfd, MODFORMAT, "COMMON", i+1, commonorg,
		     commontop - common, "", "", "", "");
	    linecnt ++;
	 }
      }

      /*
      ** Print the symbol table
      */

      printsymbols (lstfd);

      strcpy (subtitle, "\n");
      printheader (lstfd);

      fprintf (lstfd, "\nDeckname: %s\n", deckname);
      fprintf (lstfd, "%5.5o Length of program\n", pc);
      if (relentry >= 0)
	 fprintf (lstfd, "%5.5o Program entry\n", relentry);
      if (ctsscommon && common > 0)
	 fprintf (lstfd, "%5.5o(%5.5o:%5.5o) Program COMMON\n",
		  common, commonorg, commontop - common);
      fputc ('\n', lstfd);
      if (undefs)
	 fprintf (lstfd, "%d Unresolved References - U\n", undefcount);
      if (muldefs)
	 fprintf (lstfd, "%d Multiple Definintions - M\n", muldefcount);
      fprintf (lstfd, "\n%d errors, %d warnings\n", errcount, warncount);
   }

   if (errcount || warncount)
   {
      fprintf (stderr, "lnk7090: %d errors, %d warnings\n",
      	       errcount, warncount);
      if (errcount)
	 status = ABORT;
   }

   /*
   ** Punch out linked object
   */

   if (!errcount)
   {
      lnkpunch (outfd);
   }

   /*
   ** Close the files
   */

   fclose (outfd);
   if (errcount)
      unlink (outfile);
   if (listmode)
      fclose (lstfd);

   return (status == 0 ? NORMAL : ABORT);
}
