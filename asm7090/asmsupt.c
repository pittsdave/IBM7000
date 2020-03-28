/***********************************************************************
*
* asmsupt.c - Support routines for the IBM 7090 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/13/03   DGP   Added XREF support.
*   08/20/03   DGP   Changed to call the SLR(1) parser.
*   12/20/04   DGP   Added Qualifiers and condexpr.
*   12/27/04   DGP   Added symlocate.
*   01/04/05   DGP   Added entsymlookup().
*   02/09/05   DGP   Unified error reporting using logerror.
*   03/16/05   DGP   Change operation of symparse.
*   04/26/05   DGP   Use pscanbuf in place of inbuf.
*   06/09/05   DGP   Added opencopy/closecopy for INSERT.
*   06/10/05   DGP   Added RMT support.
*   12/15/05   RMS   MINGW changes.
*   10/31/06   DGP   Change "/" expression to return zero. ie: PXD ,/
*   07/30/07   DGP   Change mktemp to mkstemp.
*   05/19/09   DGP   If RMT line is a duplicate, don't add to list.
*   10/16/09   DGP   Pad Hollerith literal with blanks.
*   04/30/10   DGP   Added sequence checking option.
*   06/17/10   DGP   Changed tokscan to allow tokens beginning with '='.
*   11/01/10   DGP   Added CTSS Common mode.
*   12/15/10   DGP   Added line number to temporary file.
*                    Added file sequence for errors.
*   12/16/10   DGP   Added a unified source read function readsource().
*   12/16/10   DGP   Added a unified source write function writesource().
*   12/17/10   DGP   Added boolean/relocatable error.
*   03/25/11   DGP   Allow (we ignore) "$" on INSERT filenames.
*   03/30/11   DGP   Allow multiple include directories.
*   11/11/11   DGP   Allow INSERT files to have "inc" or "fap" extension.
*   11/16/11   DGP   In tokscan return qualifer with token.
*   06/19/12   DGP   Fix scantok bug.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include "asmdef.h"
#include "asmdmem.h"
#include "errors.h"

extern int pc;			/* the assembler pc */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int absolute;		/* In absolute section */
extern int radix;		/* Number scanner radix */
extern int linenum;		/* Source line number */
extern int errcount;		/* Number of errors in assembly */
extern int warncount;		/* Number of warnings in assembly */
extern int errnum;		/* Number of pass 2 errors for current line */
extern int warnnum;		/* Index into warnline array */
extern int lastsequ;		/* Last sequence number */
extern int exprtype;		/* Expression type */
extern int p1errcnt;		/* Number of pass 0/1 errors */
extern int p1erralloc;		/* Number of pass 0/1 errors allocated */
extern int pgmlength;		/* Length of program */
extern int absmod;		/* In absolute module */
extern int monsym;		/* Include IBSYS Symbols (MONSYM) */
extern int jobsym;		/* Include IBJOB Symbols (JOBSYM) */
extern int termparn;		/* Parenthesis are terminals (NO()) */
extern int genxref;		/* Generate cross reference listing */
extern int addext;		/* Add extern for undef'd symbols (!absolute) */
extern int addrel;		/* ADDREL mode */
extern int inpass;		/* Which pass are we in */
extern int qualindex;		/* QUALifier table index */
extern int begincount;		/* Number of BEGINs */
extern int litorigin;		/* Literal pool origin */
extern int litpool;		/* Literal pool size */
extern int entsymbolcount;	/* Number of ENTRY symbols */
extern int widemode;		/* Generate wide listing */
extern int rboolexpr;		/* RBOOL expression */
extern int lboolexpr;		/* LBOOL expression */
extern int fapmode;		/* FAP assembly mode */
extern int headcount;		/* Number of entries in headtable */
extern int filenum;		/* INSERT file index */
extern int fileseq;		/* INSERT file sequence */
extern int ctsscommon;		/* CTSS Common */
extern int incldircnt;		/* Include directory count */

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */
extern char qualtable[MAXQUALS][MAXSYMLEN+2]; /* The QUALifier table */
extern char errline[10][120];	/* Pass 2 error lines for current statment */
extern char warnline[10][120];	/* Pass 2 warning lines for current statment */
extern char headtable[MAXHEADS];/* HEAD table */
extern char *incldir[MAXINCLUDES]; /* -I include directories */

extern SymNode *addextsym;	/* Added symbol for externals */
extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern SymNode *entsymbols[MAXSYMBOLS];/* The Entry Symbol table */
extern SymNode *freesymbols;	/* Reusable symbols nodes */
extern XrefNode *freexrefs;	/* Reusable xref nodes */
extern RMTSeq *rmthead;		/* Pointer to start of remote sequences */
extern RMTSeq *rmttail;		/* Pointer to end of remote sequences */
extern ErrTable p1error[MAXERRORS];/* The pass 0/1 error table */
extern INSERTfile files[MAXASMFILES];/* INSERT file descriptors */
extern char *pscanbuf;		/* Pointer for tokenizers */

static char errtmp[120];	/* Error print string */
#ifdef DEBUGSYM
static int ssdebug;
#endif

/***********************************************************************
* readsource - Read source mode, line number and text from the temp file.
***********************************************************************/

int
readsource (FILE *fd, int *inmode, int *linenum, int *filenum, char *inbuf)
{
#ifdef WIN32
   int i;
   char temp[32];
#endif
#if defined(DEBUGP1RDR) || defined(DEBUGP2RDR)
   int flag = FALSE;
#endif

#ifdef WIN32
   /*
   ** Stupid WinBLOWS can't handle inter mixed binary/ascii reads....
   ** Even though we fdopen in binary mode.
   */

   for (i = 0; i < 8; i++)
   {
      if ((temp[i] = fgetc(fd)) == EOF)
         return (-1);
   }
   temp[8] = 0;
   sscanf (temp, "%x", inmode);
   for (i = 0; i < 8; i++)
   {
      if ((temp[i] = fgetc(fd)) == EOF)
         return (-1);
   }
   temp[8] = 0;
   sscanf (temp, "%d", linenum);
#else
   if (fread(inmode, 1, 4, fd) != 4)
      return (-1);
   if (fread(linenum, 1, 4, fd) != 4)
      return (-1);
#endif

   if (fgets(inbuf, MAXLINE, fd) == NULL)
      return (-1);

   *filenum = (*inmode & NESTMASK) >> NESTSHFT;

#ifdef DEBUGP1RDR
   if (inpass == 010) flag = TRUE;
#endif
#ifdef DEBUGP2RDR
   if (inpass == 020) flag = TRUE;
#endif
#if defined(DEBUGP1RDR) || defined(DEBUGP2RDR)
   if (flag)
   {
      fprintf (stderr, "linenum = %d, inmode = %06x, filenum = %d\n",
	      *linenum, *inmode, *filenum);
      fprintf (stderr, "inbuf = %s", inbuf);
   }
#endif
   return (0);
}

/***********************************************************************
* writesource - Write source to the temp file or seq.
***********************************************************************/

void
writesource (FILE *fd, int rmtflg, int mode, int num, char *buf, char *loc)
{
   if (rmtflg)
   {
      rmtseqadd (mode, buf);
   }
   else
   {
#ifdef WIN32
      char temp[16];
#endif

      mode |= ((filenum ? fileseq : 0) << NESTSHFT) & NESTMASK;

#ifdef WIN32
      /*
      ** Stupid WinBLOWS can't handle inter mixed binary/ascii reads....
      ** Even though we fdopen in binary mode.
      */

      sprintf (temp, "%08x", mode);
      fwrite (temp, 1, 8, fd);
      sprintf (temp, "%08d", num);
      fwrite (temp, 1, 8, fd);
#else
      fwrite (&mode, 1, 4, fd);
      fwrite (&num, 1, 4, fd);
#endif

      fputs (buf, fd);
#ifdef DEBUGP0OUT
      fprintf (stderr, "%s(%06X) %d:%s", loc, mode, num, buf);
#endif
   }
}

#ifdef WIN32
/***********************************************************************
* mkstemp - Brain dead support for WinBlows.
***********************************************************************/

int mkstemp (char *template)
{
   int fd;
   char tmpnam[1024];

   strcpy (tmpnam, mktemp(template));
   fd = open (tmpnam, O_CREAT | O_TRUNC | O_RDWR, 0666);
   strcpy (template, tmpnam);
   return (fd);
}
#endif

/***********************************************************************
* opencopy - Open a copy INSERT
***********************************************************************/

FILE *
opencopy (char *bp, FILE *tmpfd, char *etcbuf)
{
   FILE *infd;
   char *cp;
   static int tmpndx = 3;
   int tmpdes;
   int curfile;
   int i, found;
   int fapext;
   char tname[1024];

   while (isspace (*bp)) bp++;
   cp = bp;
   if (*cp == '$') cp++;
   while (!isspace (*bp))
   {
      if (isupper(*bp)) *bp = tolower(*bp);
      bp++;
   }
   *bp = '\0';

   found = FALSE;
   fapext = FALSE;
   i = 0;
   while (!found)
   {
      if (incldircnt && incldir[i])
      {
	 sprintf (tname, "%s/%s.%s", incldir[i], cp, fapext ? "fap" : "inc");
      }
      else
      {
	 sprintf (tname, "%s.%s", cp, fapext ? "fap" : "inc");
      }
#ifdef DEBUGP0RDR
      fprintf (stderr, "opencopy: linenum = %d\n", linenum);
      fprintf (stderr, "   insert = %s\n", tname);
      fprintf (stderr, "   etcbuf = %s", etcbuf);
#endif

      if ((infd = fopen (tname, "r")) == NULL)
      {
	 if (!fapext)
	 {
	    fapext = TRUE;
	 }
	 else
	 {
	    fapext = FALSE;
	    i++;
	    if (i == incldircnt)
	    {
	       char tmp[1024];

	       sprintf (tmp, "asm7090: Can't open input file for INSERT: %s",
			tname);
	       perror (tmp);
	       exit (1);
	    }
	 }
      }
      else
         found = TRUE;
   }

   curfile = filenum;
   files[curfile].fd = tmpfd;
   files[curfile].linenum = linenum;
   files[curfile].lastseq = lastsequ;
#if defined(WIN32)
   sprintf (files[curfile].tmpname, "a%d%s", tmpndx++, TEMPSPEC);
#else
   sprintf (files[curfile].tmpname, "/tmp/a%d%s", tmpndx++, TEMPSPEC);
#endif
   strcpy (files[curfile].etcline, etcbuf);
   etcbuf[0] = '\0';
   filenum++;
   fileseq++;

   if (filenum >= MAXASMFILES)
   {
      fprintf (stderr, "asm7090: Too many INSERT files\n");
      exit (1);
   }
   linenum = 0;
   lastsequ = 0;
   if ((tmpdes = mkstemp (files[curfile].tmpname)) < 0)
   {
      perror ("asm7090: Can't mkstemp for INSERT");
      exit (1);
   }

   if ((tmpfd = fdopen (tmpdes, "w+")) == NULL)
   {
      perror ("asm7090: Can't open temporary file for INSERT");
      exit (1);
   }

   detab (infd, tmpfd, "insert");
   fclose (infd);
   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm7090: Can't rewind temp file");
      exit (1);
   }
   return (tmpfd);
}

/***********************************************************************
* closecopy - Close a copy INSERT
***********************************************************************/

FILE *
closecopy (FILE *tmpfd, char *etcbuf)
{
   if (filenum)
   {
      filenum--;
      fclose (tmpfd);
      unlink (files[filenum].tmpname);
      linenum = files[filenum].linenum;
      lastsequ = files[filenum].lastseq;
      strcpy (etcbuf, files[filenum].etcline);
#ifdef DEBUGP0RDR
      fprintf (stderr, "closecopy: linenum = %d\n", linenum);
      fprintf (stderr, "   etcbuf = %s", etcbuf);
#endif
      return (files[filenum].fd);
   }
   return (NULL);
}

/***********************************************************************
* rmtseqadd - Add a line to the remote sequence pool.
***********************************************************************/

void
rmtseqadd (int mode, char *line)
{
   RMTSeq *new;

#if 0
   new = rmthead;
   while (new)
   {
      if (!strcmp(new->line, line))
          return;
       new = new->next;
   }
#endif

   if ((new = (RMTSeq *)DBG_MEMGET (sizeof (RMTSeq))) == NULL)
   {
      fprintf (stderr, "asm7090: Unable to allocate memory\n");
      exit (ABORT);
   }

   memset (new, '\0', sizeof (RMTSeq));

   new->linemode = mode | RMTSEQ;
   strcpy (new->line, line);

   if (!rmthead) rmthead = new;
   if (rmttail) rmttail->next = new;
   rmttail = new;

}

/***********************************************************************
* rmtseqget - Get remote sequence pool line for processing.
***********************************************************************/

char *
rmtseqget (int *mode, char *line)
{
   if (rmthead)
   {
      RMTSeq *lclptr;

      rmthead->linemode |= RMTSEQ;
      lclptr = rmthead;
      rmthead = rmthead->next;
      strcpy (line, lclptr->line);
      *mode = lclptr->linemode;
      DBG_MEMREL (lclptr);
      return (line);
   }

   rmttail = NULL;
   return (NULL);
}

/***********************************************************************
* chkerror - Check to see if the Pass 1 error has been recorded.
***********************************************************************/

static int
chkerror (int line)
{
   int i;

   for (i = 0; i < p1errcnt; i++)
      if (p1error[i].errorline == line)
         return (TRUE);
   return (FALSE);
}

/***********************************************************************
* logwarning - Log a warning for printing.
***********************************************************************/

void
logwarning (char *warnmsg)
{
#ifdef DEBUGERROR
   fprintf (stderr, "logwarning: linenum = %d, warncount = %d, inpass = %o\n",
	   linenum, warncount, inpass);
   fprintf (stderr, "   p1errcnt = %d, p1erralloc = %d, warnnum = %d\n",
	   p1errcnt, p1erralloc, warnnum);
   fprintf (stderr, "   warnmsg = %s\n", warnmsg);
#endif

   if (!chkerror (linenum))
   {
      warncount++;
      if (inpass == 020)
      {
	 strcpy (warnline[warnnum++], warnmsg);
      }
   }
}

/***********************************************************************
* logerror - Log an error for printing.
***********************************************************************/

void
logerror (char *errmsg)
{
#ifdef DEBUGERROR
   fprintf (stderr, "logerror: linenum = %d, errcount = %d, inpass = %o\n",
	   linenum, errcount, inpass);
   fprintf (stderr, "   p1errcnt = %d, p1erralloc = %d, errnum = %d\n",
	   p1errcnt, p1erralloc, errnum);
   fprintf (stderr, "   errmsg = %s\n", errmsg);
#endif

   if (!chkerror (linenum))
   {
      errcount++;
      if (inpass == 020)
      {
	 strcpy (errline[errnum++], errmsg);
      }
      else
      {
	 if (p1errcnt >= p1erralloc)
	 {
	    p1error[p1errcnt].errortext = (char *)DBG_MEMGET (120);
	    p1erralloc++;
	 }
	 p1error[p1errcnt].errorline = linenum;
	 p1error[p1errcnt].errorfile = (filenum ? fileseq : 0);
	 strcpy (p1error[p1errcnt].errortext, errmsg);
	 p1errcnt++;
      }
   }
}

/***********************************************************************
* Parse_Error - Print parsing error
***********************************************************************/

void
Parse_Error (int cause, int state, int defer)
{
   char errorstring[256];

#ifdef DEBUGPARSEERROR
   fprintf (stderr, 
   "Parse_Error: pass = %o, defer = %s, cause = %d, state = %d, linenum = %d\n",
	 inpass, defer ? "TRUE " : "FALSE", cause, state, linenum);
#endif
   if (defer) return;

   strcpy (errorstring, "Unknown error");
   if (cause == PARSE_ERROR)
   {
      switch (state)
      {
      /* Include generated parser errors */
#include "asm7090.err"
      default: ;
      }
   }
   else if (cause == SCAN_ERROR)
   {
      switch (state)
      {
      case INVOCTNUM:
	 strcpy (errorstring, "Invalid octal number");
	 break;
      case INVEXPONENT:
	 strcpy (errorstring, "Exponent out of range");
	 break;
      case INVSYMBOL:
	 strcpy (errorstring, "Invalid symbol");
	 break;
      default: ;
      }
   }
   else if (cause == INTERP_ERROR)
   {
      strcpy (errorstring, "Divide by zero");
   }
   else if (cause == EXPR_ERROR)
   {
      if (inpass != 020) return;
      strcpy (errorstring, "Illegal use of boolean");
   }

   sprintf (errtmp, "%s in expression", errorstring);
   logerror (errtmp);

} /* Parse_Error */

/***********************************************************************
* tokscan - Scan a token.
***********************************************************************/

char *
tokscan (char *buf, char **token, int *tokentype, int *tokenvalue, char *term,
         int usequaltable)
{
   int index;
   toktyp tt;
   tokval val;
   static char t[80];
   static char q[80];
   char c;

#ifdef DEBUGTOK
   fprintf (stderr, "tokscan: Entered: buf = %s", buf);
#endif

   /*
   ** Skip leading blanks
   */

   while (*buf && isspace(*buf))
   {
#ifdef DEBUGTOK
      fprintf (stderr, "   *buf = %c(%X)\n", *buf, *buf);
      fprintf (stderr, "   buf = %p, pscanbuf = %p\n", buf, pscanbuf);
#endif

      /*
      ** We stop scanning on a newline or when we're passed the operands.
      */

      if (*buf == '\n' || (buf - pscanbuf >= NOOPERAND))
      {
	 t[0] = '\0';
	 *token = t;
	 *term = '\n';
	 *tokentype = EOS;
         return (buf);
      }
      buf++;
   }

   /*
   ** Check for "strange" opcodes.
   */

   if ((buf-pscanbuf) >= OPCSTART && (buf-pscanbuf) < VARSTART)
   {
#ifdef DEBUGTOKNULLOP
      fprintf (stderr, "tokscan: buf = %s", buf);
#endif
      if (!strncmp (buf, "...", 3) ||
          !strncmp (buf, "***", 3))
      {
#ifdef DEBUGTOKNULLOP
	 fprintf (stderr, "  null opcode ... found\n");
#endif
	 strcpy (t, "PZE");
	 *tokentype = SYM;
	 *tokenvalue = 0;
	 *token = t;
	 index = 3;
	 *term = *(buf+index);
	 return (buf+index);
      }
      else if (*buf == '=')
      {
	 char *bp = buf;
	 while (*buf && !isspace(*buf)) buf++;
	 index = buf - bp;
	 strncpy (t, bp, index);
	 t[index] = '\0';
	 *tokentype = SYM;
	 *tokenvalue = 0;
	 *token = t;
	 *term = *(bp+index);
	 return (bp+index);
      }
   }

   /*
   ** If nil operand, return type
   */

   if (!strncmp (buf, "--", 2))
   {
      strcpy (t, "--");
      index = 2;
      if (radix == 8)
	 *tokentype = OCTNUM;
      else
	 *tokentype = SYM;
      *tokenvalue = 0;
      *token = t;
      *term = *(buf+index);
      return (buf+index);
   }

   /*
   ** Call the scanner to get the token
   */

   index = 0;
#ifdef DEBUGTOK
   fprintf (stderr, "  calling scanner: buf = %s\n", buf);
#endif
   tt = Scanner (buf, &index, &val, t, q, &c, inpass == 020 ? FALSE : TRUE,
                 usequaltable); 
#ifdef DEBUGTOK
   fprintf (stderr, "  token = %s, tokentype = %d, term = %02x, val = %d\n",
	    t, tt, c, val);
   fprintf (stderr, " index = %d, *(buf+index) = %02X\n",  index, *(buf+index));
#endif

   /*
   ** Return proper token and index
   */

   if (tt == DECNUM || tt == OCTNUM)
      c = *(buf+index);
   if (c == ',') index++;
   if (q[0])
   {
      if (q[0] == '0')
      {
         strcpy (q, "$");
      }
      else
      {
         strcat (q, "$");
      }
      strcat (q, t);
      *token = q;
   }
   else
   {
      *token = t;
   }
   *tokentype = tt;
   *tokenvalue = val;
   *term = c;
   return (buf+index);
}

/***********************************************************************
* freexref - Link an xref entry on free chain.
***********************************************************************/

void
freexref (XrefNode *xr)
{
#ifdef DEBUGMALLOCS
   DBG_MEMREL (xr);
#else
   xr->next = freexrefs;
   freexrefs = xr;
#endif
}

/***********************************************************************
* freesym - Link a symbol entry on free chain.
***********************************************************************/

void
freesym (SymNode *sr)
{
#ifdef DEBUGMALLOCS
   DBG_MEMREL (sr);
#else
   sr->next = freesymbols;
   freesymbols = sr;
#endif
}

/***********************************************************************
* symdelete - Delete a symbol from the symbol table.
***********************************************************************/

void
symdelete (SymNode *sym)
{
   int i;

   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i] == sym)
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
	 for (; i < symbolcount; i++)
	 {
	    symbols[i] = symbols[i+1];
	 }
	 symbolcount --;
         return;
      }
   }
}

/***********************************************************************
* symlookup - Lookup a symbol in the symbol table.
***********************************************************************/

SymNode *
symlookup (char *sym, char *qual, int add, int xref)
{
   SymNode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;
   char tblsym[80];
   char lksym[80];

#ifdef DEBUGSYM
#ifdef DEBUGSYMVAL
   ssdebug = !strcmp (sym, "LPFLAG");
#else
   ssdebug = TRUE;
#endif
   if (ssdebug)
   {
      fprintf (stderr, "symlookup: Entered: sym = '%s', qual = '%s'\n",
	    sym, qual);
      fprintf (stderr,
	    "   add = %s, xref = %s, inpass = %03o, pc = %05o, line = %d\n",
	    add ? "TRUE" : "FALSE",
	    xref ? "TRUE" : "FALSE",
	    inpass, pc, linenum);
   }
#endif

   /*
   ** Empty symbol table.
   */

   if (symbolcount == 0)
   {
      if (!add) return (NULL);

#ifdef DEBUGSYM
      if (ssdebug)
	 fprintf (stderr, "   add symbol at top\n");
#endif
      if (freesymbols)
      {
         symbols[0] = freesymbols;
	 freesymbols = symbols[0]->next;
      }
      else if ((symbols[0] = (SymNode *)DBG_MEMGET (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm7090: Unable to allocate memory\n");
	 exit (ABORT);
      }
      memset (symbols[0], '\0', sizeof (SymNode));
      strcpy (symbols[0]->symbol, sym);
      strcpy (symbols[0]->qualifier, qual);
      symbols[0]->value = pc;
      symbols[0]->line = linenum;
#ifdef DEBUGXREF
      fprintf (stderr, "symlookup: sym = %s defline = %d\n", sym, linenum);
#endif
      if (absolute || absmod)
	 symbols[0]->flags &= ~RELOCATABLE;
      else
	 symbols[0]->flags |= RELOCATABLE;
      if (inpass == 000)
	 symbols[0]->flags |= P0SYM;
      symbolcount++;
      return (symbols[0]);
   }

   /*
   ** Locate using binary search
   */

   lo = 0;
   up = symbolcount;
   last = -1;
   sprintf (lksym, "%s$%s", sym, qual);
   
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGSYMSEARCH
      fprintf (stderr, "   mid = %d, last = %d\n", mid, last);
#endif
      if (symbolcount == 1)
	 done = TRUE;
      else if (last == mid) break;

      sprintf (tblsym, "%s$%s", symbols[mid]->symbol, symbols[mid]->qualifier);
#ifdef DEBUGSYMSEARCH
      fprintf (stderr, "   tblsym = '%s', lksym = '%s'\n", tblsym, lksym);
#endif
      r = strcmp (tblsym, lksym);

      /*
      ** We have a hit
      */

      if (r == 0)
      {
	 SymNode *symp = symbols[mid];

	 if (add)
	 {
	    if (!(symbols[mid]->flags & P0SYM))
	       return (NULL); /* must be a duplicate */

#ifdef DEBUGSYM
	    if (ssdebug)
	       fprintf (stderr, "   update symbol\n");
#endif
	    symbols[mid]->flags &= ~P0SYM;
	    symbols[mid]->value = pc;
	    symbols[mid]->line = linenum;
	    if (absolute || absmod)
	       symbols[mid]->flags &= ~RELOCATABLE;
	    else
	       symbols[mid]->flags |= RELOCATABLE;
	 }

#ifdef DEBUGSYM
	 if (ssdebug)
	    fprintf (stderr, "   found symbol, value = %05o\n",
		  symbols[mid]->value);
#endif

	 /*
	 ** If xref mode, chain a line reference
	 */

	 if (genxref && xref)
	 {
	    XrefNode *new;

	    if (freexrefs)
	    {
	       new = freexrefs;
	       freexrefs = new->next;
	    }
	    else if ((new = (XrefNode *)DBG_MEMGET (sizeof (XrefNode))) == NULL)
	    {
	       fprintf (stderr, "asm7090: Unable to allocate memory\n");
	       exit (ABORT);
	    }
#ifdef DEBUGXREF
	    fprintf (stderr, "symlookup: sym = %s refline = %d\n",
		     sym, linenum);
#endif
	    memset (new, '\0', sizeof(XrefNode));
	    new->next = NULL;
	    new->line = linenum;
	    if (symp->xref_head == NULL)
	    {
	       symp->xref_head = new;
	    }
	    else
	    {
	       (symp->xref_tail)->next = new;
	    }
	    symp->xref_tail = new;
	 }
         return (symbols[mid]);
      }

      /*
      ** Otherwise, get next symbol
      */

      else if (r < 0)
      {
         lo = mid;
      }
      else 
      {
         up = mid;
      }
      last = mid;
   }

   /*
   ** Not found, check to add
   */

   if (add)
   {
      SymNode *new;

#ifdef DEBUGSYM
      if (ssdebug)
	 fprintf (stderr, "   add new symbol\n");
#endif
      /*
      ** Add to table, allocate storage and initialize symbol
      */

      if (symbolcount+1 > MAXSYMBOLS)
      {
         fprintf (stderr, "asm7090: Symbol table exceeded\n");
	 exit (ABORT);
      }

      if (freesymbols)
      {
         new = freesymbols;
	 freesymbols = new->next;
      }
      else if ((new = (SymNode *)DBG_MEMGET (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm7090: Unable to allocate memory\n");
	 exit (ABORT);
      }

      memset (new, '\0', sizeof (SymNode));
      strcpy (new->symbol, sym);
      strcpy (new->qualifier, qual);
#ifdef DEBUGXREF
      fprintf (stderr, "symlookup: sym = %s defline = %d\n", sym, linenum);
#endif
      new->value = pc;
      new->line = linenum;
      if (absolute || absmod)
	 new->flags &= ~RELOCATABLE;
      else
	 new->flags |= RELOCATABLE;
      if (inpass == 000)
	 new->flags |= P0SYM;

      /*
      ** Insert pointer in sort order.
      */

      for (lo = 0; lo < symbolcount; lo++)
      {
	 sprintf (tblsym, "%s$%s", symbols[lo]->symbol, symbols[lo]->qualifier);

         if (strcmp (tblsym, lksym) > 0)
	 {
	    for (up = symbolcount + 1; up > lo; up--)
	    {
	       symbols[up] = symbols[up-1];
	    }
	    symbols[lo] = new;
	    symbolcount++;
	    return (symbols[lo]);
	 }
      }
      symbols[symbolcount] = new;
      ret = symbols[symbolcount];
      symbolcount++;
   }
   return (ret);
}

/***********************************************************************
* entsymlookup - Lookup an extry symbol in the symbol table.
***********************************************************************/

SymNode *
entsymlookup (char *sym, char *qual, int add, int xref)
{
   SymNode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;
   char tblsym[80];
   char lksym[80];

#ifdef DEBUGSYM
#ifdef DEBUGSYMVAL
   ssdebug = !strcmp (sym, "LPFLAG");
#else
   ssdebug = TRUE;
#endif
   if (ssdebug)
   {
      fprintf (stderr, "entsymlookup: Entered: sym = '%s', qual = '%s'\n",
	       sym, qual);
      fprintf (stderr, "   add = %s, xref = %s, inpass = %03o, linenum = %d\n",
	       add ? "TRUE" : "FALSE",
	       xref ? "TRUE" : "FALSE",
	       inpass, linenum);
   }
#endif

   /*
   ** Empty symbol table.
   */

   if (entsymbolcount == 0)
   {
      if (!add) return (NULL);

#ifdef DEBUGSYM
      if (ssdebug)
	 fprintf (stderr, "add symbol at top\n");
#endif
      if (freesymbols)
      {
         entsymbols[0] = freesymbols;
	 freesymbols = entsymbols[0]->next;
      }
      else if ((entsymbols[0] =
		     (SymNode *)DBG_MEMGET (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm7090: Unable to allocate memory\n");
	 exit (ABORT);
      }
      memset (entsymbols[0], '\0', sizeof (SymNode));
      strcpy (entsymbols[0]->symbol, sym);
      strcpy (entsymbols[0]->qualifier, qual);
      entsymbols[0]->value = pc;
      entsymbols[0]->line = linenum;
#ifdef DEBUGXREF
      fprintf (stderr, "entsymlookup: sym = %s defline = %d\n", sym, linenum);
#endif
      if (absolute || absmod)
	 entsymbols[0]->flags &= ~RELOCATABLE;
      else
	 entsymbols[0]->flags |= RELOCATABLE;
      entsymbolcount++;
      return (entsymbols[0]);
   }

   /*
   ** Locate using binary search
   */

   lo = 0;
   up = entsymbolcount;
   last = -1;
   sprintf (lksym, "%s$%s", sym, qual);
   
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGSYMSEARCH
      fprintf (stderr, " mid = %d, last = %d\n", mid, last);
#endif
      if (entsymbolcount == 1)
	 done = TRUE;
      else if (last == mid) break;

      sprintf (tblsym, "%s$%s",
	       entsymbols[mid]->symbol, entsymbols[mid]->qualifier);
#ifdef DEBUGSYMSEARCH
      fprintf (stderr, "   tblsym = '%s', lksym = '%s'\n", tblsym, lksym);
#endif
      r = strcmp (tblsym, lksym);

      /* 
      ** We have a hit
      */

      if (r == 0)
      {
	 SymNode *symp = entsymbols[mid];

	 if (add)
	    return (NULL); /* must be a duplicate */

	 /*
	 ** If xref mode, chain a line reference
	 */

	 if (genxref && xref)
	 {
	    XrefNode *new;

	    if (freexrefs)
	    {
	       new = freexrefs;
	       freexrefs = new->next;
	    }
	    else if ((new = (XrefNode *)DBG_MEMGET (sizeof (XrefNode))) == NULL)
	    {
	       fprintf (stderr, "asm7090: Unable to allocate memory\n");
	       exit (ABORT);
	    }
#ifdef DEBUGXREF
	    fprintf (stderr, "entsymlookup: sym = %s refline = %d\n",
		  sym, linenum);
#endif
	    memset (new, '\0', sizeof(XrefNode));
	    new->next = NULL;
	    new->line = linenum;
	    if (symp->xref_head == NULL)
	    {
	       symp->xref_head = new;
	    }
	    else
	    {
	       (symp->xref_tail)->next = new;
	    }
	    symp->xref_tail = new;
	 }
         return (entsymbols[mid]);
      }

      /*
      ** Otherwise, get next symbol
      */

      else if (r < 0)
      {
         lo = mid;
      }
      else 
      {
         up = mid;
      }
      last = mid;
   }

   /*
   ** Not found, check to add
   */

   if (add)
   {
      SymNode *new;

#ifdef DEBUGSYM
      if (ssdebug)
	 fprintf (stderr, "add new symbol\n");
#endif
      /*
      ** Add to table, allocate storage and initialize symbol
      */

      if (entsymbolcount+1 > MAXSYMBOLS)
      {
         fprintf (stderr, "asm7090: Entry symbol table exceeded\n");
	 exit (ABORT);
      }

      if (freesymbols)
      {
         new = freesymbols;
	 freesymbols = new->next;
      }
      else if ((new = (SymNode *)DBG_MEMGET (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm7090: Unable to allocate memory\n");
	 exit (ABORT);
      }

      memset (new, '\0', sizeof (SymNode));
      strcpy (new->symbol, sym);
      strcpy (new->qualifier, qual);
#ifdef DEBUGXREF
      fprintf (stderr, "entsymlookup: sym = %s defline = %d\n", sym, linenum);
#endif
      new->value = pc;
      new->line = linenum;
      if (absolute || absmod)
	 new->flags &= ~RELOCATABLE;
      else
	 new->flags |= RELOCATABLE;

      /*
      ** Insert pointer in sort order.
      */

      for (lo = 0; lo < entsymbolcount; lo++)
      {
	 sprintf (tblsym, "%s$%s",
		  entsymbols[lo]->symbol, entsymbols[lo]->qualifier);

         if (strcmp (tblsym, lksym) > 0)
	 {
	    for (up = entsymbolcount + 1; up > lo; up--)
	    {
	       entsymbols[up] = entsymbols[up-1];
	    }
	    entsymbols[lo] = new;
	    entsymbolcount++;
	    return (entsymbols[lo]);
	 }
      }
      entsymbols[entsymbolcount] = new;
      ret = entsymbols[entsymbolcount];
      entsymbolcount++;
   }
   return (ret);
}

/***********************************************************************
* symlocate - Locate first occurance of a symbol in the symbol table.
***********************************************************************/

SymNode *
symlocate (char *sym, int *count)
{
   int mid;
   int lo;
   int up;
   int r;

#ifdef DEBUGSYMLOC
   fprintf (stderr, "symlocate: Entered: sym = '%s'\n", sym);
#endif

   /*
   ** Empty symbol table.
   */

   *count = 0;
   if (symbolcount == 0)
      return (NULL);

   /*
   ** Locate using linear search as binary search will miss without quals.
   */

   lo = 0;
   up = symbolcount;
   
   for (mid = 0; mid < up; mid++)
   {
      r = strcmp (symbols[mid]->symbol, sym);
      if (r == 0)
      {
#ifdef DEBUGSYMLOC
	 fprintf (stderr, "   mid = %d, qual = %s\n",
		  mid, symbols[mid]->qualifier);
#endif
	 /*
	 ** Check if other symbols with different qualifiers above and below.
	 */

	 lo = mid;
	 while (!strcmp (symbols[mid]->symbol, symbols[lo-1]->symbol) &&
	        strcmp (symbols[mid]->qualifier, symbols[lo-1]->qualifier))
	 {
#ifdef DEBUGSYMLOC
	    fprintf (stderr, "   lo = %d, qual = %s\n",
		     lo-1, symbols[lo-1]->qualifier);
#endif
	    lo--;
	 }
	 up = mid;
	 while (!strcmp (symbols[mid]->symbol, symbols[up-1]->symbol) &&
	        strcmp (symbols[mid]->qualifier, symbols[up-1]->qualifier))
	 {
#ifdef DEBUGSYMLOC
	    fprintf (stderr, "   up = %d, qual = %s\n",
		     up-1, symbols[up-1]->qualifier);
#endif
	    up++;
	 }

	 /*
	 ** Number of instances of the same symbols with different quals.
	 */

	 *count = up - lo + 1;
#ifdef DEBUGSYMLOC
	 fprintf (stderr, "   lo = %d, up = %d, count = %d\n",
		  lo, up, *count);
#endif
         return (symbols[mid]);
      }
   }

   /*
   ** Not found
   */

   return (NULL);
}

/***********************************************************************
* symparse - Process symbol references from parser.
***********************************************************************/

tokval 
symparse (char *sym, char *qual, int *relo, int *boolsym, int defer, int pcinc)
{
   SymNode *s;
   tokval value;
   int relocatable;
   int qndx;
   int xref;
   int count;
   char temp2[MAXSYMLEN+2];

   s = NULL;
   addextsym = NULL;
   *boolsym = FALSE;
   relocatable = *relo;

   xref = (((inpass & 070) == 010) && defer == FALSE) ? FALSE : TRUE;

#ifdef DEBUGSYMPARSE
   fprintf (stderr, "symparse: Entered: sym = '%s', qual = '%s'\n", sym, qual);
   fprintf (stderr, "   exprtype = %03o, defer = %s, xref = %s\n",
	    exprtype,
	    defer ? "TRUE" : "FALSE",
	    xref ? "TRUE" : "FALSE");
#endif

   /*
   ** With FAP check symbol with head.
   */

   if (fapmode)
   {
      if (qual[0])
      {
	 if (qual[0] == '0')
	    s = symlookup (sym, "", FALSE, xref);
	 else
	    s = symlookup (sym, qual, FALSE, xref);
      }
      else if (strlen(sym) == MAXSYMLEN)
      {
	 temp2[0] = sym[0];
	 temp2[1] = '\0';
	 s = symlookup (&sym[1], temp2, FALSE, xref);
      }
      else if (headcount)
      {
	 sprintf (temp2, "%c", headtable[0]);
	 s = symlookup (sym, temp2, FALSE, xref);
      }
      if (!s)
	 s = symlookup (sym, "", FALSE, xref);
   }

   /*
   ** With MAP check symbol with qualifers
   */

   else
   {
      s = symlookup (sym, qual, FALSE, xref);
      if (!s) 
      {

	 /*
	 ** Not found, check if in QUAL scope
	 */

	 if (qualindex)
	 {
	    for (qndx = qualindex; qndx >= 0; qndx--)
	    {
	       strcpy (temp2, qualtable[qndx]);
	       s = symlookup (sym, temp2, FALSE, xref);
	       if (s) break;
	    }
	 }

	 /*
	 ** If not found, check if Monitor Symbol
	 */

	 if (!s)
	 {
	    if (monsym || jobsym)
	    {
	       s = symlookup (sym, "S.S", FALSE, xref);
	    }

	 }

	 /*
	 ** If not found, check if symbol exists without a qual
	 */

	 if (!s)
	 {
	    if (inpass > 010 && (s = symlocate (sym, &count)))
	    {
	       /* 
	       ** If only one occurance, we found it
	       */
	       if (count != 1) s = NULL;
	    }
	 }
      }
   }


   /*
   ** Not found
   */

   if (!s)
   {
      if (!defer)
      {
	 /*
	 ** If in add external mode (relocatable) add as an external
	 */

	 if (addext)
	 {
	    if (fapmode)
	    {
	       if (qual[0] == '0') qual[0] = '\0';
	    }
	    s = symlookup (sym, qual, TRUE, xref);

	    if (s)
	    {
	       s->value = 0;
	       if (absmod || absolute)
		  s->flags &= ~RELOCATABLE;
	       else
		  s->flags |= RELOCATABLE;
	       s->flags |= EXTERNAL;
	       addextsym = s;
	       goto SYM_FOUND;
	    }
	 }
	 
	 /*
	 ** Otherwise, it is an error
	 */

	 sprintf (errtmp, "Undefined symbol: %s", sym);
	 logerror (errtmp);
      }
      value = 0;
   }

   /*
   ** We found it.
   */

   else
   {

   SYM_FOUND:
#ifdef DEBUGSYMPARSE
      fprintf (stderr, "    SYM_FOUND:\n");
#endif
      /*
      ** If in Boolean expression handle appropriately
      */

      if ((exprtype & EXPRTYPEMASK) == BOOLEXPR)
      {
#if 0
	 /*
	 ** Only BOOL data allowed in a boolean expression
	 */

	 if (!(s->flags & BOOL))
	 {
	    sprintf (errtmp, "Invalid symbol for BOOL expression: %s", sym);
	    logerror (errtmp);
	    value = 0;
	 }

	 /*
	 ** Bool value, return value and characteristics
	 */

	 else
	 {
#endif
#ifdef DEBUGBOOLSYM
            fprintf (stderr, 
	       "symparse: BOOL sym = %s, val = %o, lrbool = %s, rbool = %s\n",
		  sym, s->value,
		  s->flags & LRBOOL ? "TRUE" : "FALSE",
		  s->flags & RBOOL ? "TRUE" : "FALSE");
#endif
	    value = s->value;
	    if (s->flags & BOOLBITS)
	       *boolsym = TRUE;
	    if (s->flags & LRBOOL)
	    {
	       if (s->flags & RBOOL) rboolexpr = TRUE;
	       else                  lboolexpr = TRUE;
	    }
#if 0
	 }
#endif
      }

      /*
      ** Address expression
      */

      else
      {
#if 0
	 if (s->flags & BOOL)
	 {
	    sprintf (errtmp, "Invalid symbol for BOOL expression: %s", sym);
	    logerror (errtmp);
	    value = 0;
	 }
#endif
	 if (s->flags & BOOLBITS)
	    *boolsym = TRUE;

	 /* Mark REF as being used */
	 if ((s->flags & EXTERNAL) && s->value == 0)
	 {
	    if (absolute)
	       s->flags &= ~RELOCATABLE;
	    else
	       s->flags |= RELOCATABLE;
	 }
	 if (s->flags & RELOCATABLE) relocatable++;
	 else if (ctsscommon && (s->flags & COMMON)) relocatable++;
	 value = s->value;
	 if (s->flags & EXTERNAL)
	 {
	    /* Don't relocate since it's extern tail */
	    if (value == 0) relocatable = 100;
	    /* back link reference chain */
	    s->value = pc + pcinc; 
	 }
      }
   }

   *relo = relocatable;
   return (value);
}

/***********************************************************************
* exprscan - Scan an expression and return its value.
***********************************************************************/

char *
exprscan (char *bp, int *val, char *tterm, int *relo, int entsize, int defer,
	  int pcinc)
{
   char *cp;
   int spaces = FALSE;
   int index = 0;
   int lrelo = FALSE;
   tokval lval = 0;

#ifdef DEBUGEXPR
   fprintf (stderr, "exprscan: entered: bp = %s\n", bp);
#endif
   cp = bp;

   /*
   ** Skip leading blanks
   */

   while (*bp && isspace(*bp))
   {
      spaces = TRUE;
      bp++;
   }

   /*
   ** If no operands, return EOL
   */

   if (spaces && (bp - pscanbuf >= NOOPERAND))
   {
      bp = cp;
      *tterm = '\n';
      *val = lval;
      *relo = lrelo;
      return (bp);
   }

#ifdef DEBUGEXPR
   fprintf (stderr, "   bp = %s\n", bp);
#endif

   /*
   ** If nil operand, return type
   */

   if (!strncmp (bp, "--", 2))
   {
      index = 2;
      *tterm = *(bp+index);
      *val = lval;
      *relo = lrelo;
#ifdef DEBUGEXPR
      fprintf (stderr, "   null op: term = %c\n", *tterm);
#endif
      if (*tterm == ',') index++;
      return (bp+index);
   }

   /*
   ** If literal then scan it off and put in symbol table
   */

   if (*bp == LITERALSYM)
   {
      SymNode *s;
      char *cp;

      cp = bp+1;
      if (*cp == 'H')
      {
         while (*cp && *cp >= ' ' && ((cp-bp) < 8)) cp++;
	 if (((cp-bp) < 8) || *cp < ' ')
	    while ((cp-bp) < 8)
	    {
	       *cp++ = ' ';
	    }
      }
      else if (*cp == 'V')
      {
	 while (*cp && !isspace(*cp)) cp++;
      }
      else
      {
	 while (*cp && *cp != ',' && !isspace(*cp)) cp++;
      }
      *tterm = *cp;
      index = cp - bp;
      *cp = '\0';

#ifdef DEBUGLIT
      fprintf (stderr,
	       "exprscan: pc = %o, litpool = %d, litorigin = %o, inpass = %o\n",
	       pc, litpool, litorigin, inpass);
#endif
      if (!(s = symlookup (bp, qualtable[qualindex], FALSE, TRUE)))
      {
	 if (!(s = symlookup (bp, qualtable[qualindex], TRUE, TRUE)))
	 {
	    sprintf (errtmp, "Unable to add Literal: %s", bp);
	    logerror (errtmp);
	    lval = 0;
	 }
	 if ((inpass & 070) == 010)
	 {
	    s->value = pc;
	    litpool++;
	 }
	 else
	 {
	    if (litorigin)
	       s->value = litorigin++;
	    else
	       s->value = pgmlength++;
	 }
      }
      *cp = *tterm;
      lval = s->value;
      if (s->flags & RELOCATABLE) lrelo = TRUE;
   }

   /*
   ** Call the parser to scan the expression
   */

   else if (strlen(bp) && *bp != ',')
   {
      if (*bp == '-')
      {
         if (*(bp+1) == ',' || isspace(*(bp+1)))
	 {
	    index = 1;
	    *tterm = *(bp+index);
	    *val = lval;
	    *relo = lrelo;
	    if (*tterm == ',') index++;
	    return (bp+index);
	 }
	 if (*(bp+1) == '1' && (*(bp+2) == ',' || isspace(*(bp+2))))
	 {
	    lval = (exprtype & BOOLVALUE) ? 0777777 : 077777 ;
	    index = 2;
	    *tterm = *(bp+index);
	    *val = lval;
	    *relo = lrelo;
	    if (*tterm == ',') index++;
	    return (bp+index);
	 }
      }
      else if (*bp == '/' && (*(bp+1) == ',' || isspace (*(bp+1))))
      {
	 lval = -1;
	 index = 1;
	 *tterm = *(bp+index);
	 *val = lval;
	 *relo = lrelo;
	 if (*tterm == ',') index++;
	 return (bp+index);
      }
      lval = Parser (bp, &lrelo, &index, entsize, defer, pcinc);
   }
#ifdef DEBUGEXPR
   fprintf (stderr, " index = %d, *(bp+index) = %02X\n",
	 index, *(bp+index));
   fprintf (stderr, " lrelo = %d, lval = %d\n", lrelo, lval);
#endif
   
   /*
   ** Return proper value and index
   */

   *tterm = *(bp+index);
   if (*tterm == ',') index++;
   *val = lval;
   *relo = lrelo;
   return (bp+index);
}


/***********************************************************************
* condexpr - Scan a conditional assembly expression and return its value.
***********************************************************************/

char *
condexpr (char *bp, int *val, char *term)
{
   char *lbp;
   char *rbp;
   int r;
   int rel;
   int ret;

#ifdef DEBUGCONDEXPR
   fprintf (stderr, "condexpr: linenum = %d: bp = %s", linenum, bp);
#endif

   /*
   ** Skip leading blanks 
   */

   while (*bp && isspace(*bp)) bp++;
   lbp = bp;

   /*
   ** Locate terminal space 
   */

   while (*bp && !isspace(*bp)) bp++;
   *term = *bp;
   *bp = '\0';

   /*
   ** Locate added condition, if any
   */

   if ((rbp = strchr (lbp, ',')) != NULL)
   {
      *term = *rbp;
      *rbp++ = '\0';
      bp = rbp;
   }

   /*
   ** If there is an equal sign, we have an expression. evaluate
   */

   if ((rbp = strchr (lbp, '=')) != NULL)
   {
      *rbp++ = '\0';

      /*
      ** check if GT operation
      */

      if (*rbp == '+')
      {
         rel = GT;
	 rbp++;
      }

      /*
      ** check if LT operation
      */

      else if (*rbp == '-')
      {
         rel = LT;
	 rbp++;
      }

      /* 
      ** Must be strict equality
      */

      else
      {
         rel = EQ;
      }
#ifdef DEBUGCONDEXPR
      fprintf (stderr, "   lbp = %s, rel = %d, rbp = %s\n", lbp, rel, rbp);
#endif

      /* 
      ** Check if string compare
      */

      if (*lbp == '/')
      {
	 char *cp;
	 char lch[MAXSYMLEN+2];
	 char rch[MAXSYMLEN+2];

	 /*
	 ** Yes, isolate compare tokens
	 */

         if (*rbp != '/')
	 {
	    *term = *rbp;
	    *val = -1;
	    return (bp);
	 }
	 lbp++, rbp++;
	 cp = lch;
	 *cp = '\0';

	 while (*lbp && *lbp != '/')
	 {
	    *cp++ = *lbp++;
	    *cp = '\0';
	 }
	 cp = rch;
	 *cp = '\0';

	 while (*rbp && *rbp != '/')
	 {
	    *cp++ = *rbp++;
	    *cp = '\0';
	 }
#ifdef DEBUGCONDEXPR
	 fprintf (stderr, "   lch = '%s', rch = '%s'\n", lch, rch);
#endif
	 /*
	 ** Compare string tokens
	 */

	 r = strcmp (lch, rch);
      }

      /*
      ** Not a string, do arithmetic compare
      */

      else
      {
	 int index = 0;
	 int lrelo = 0;
	 tokval lval;
	 tokval rval;

	 /*
	 ** Use the parser to return expression values
	 */

	 if (*lbp)
	    lval = Parser (lbp, &lrelo, &index, 1, FALSE, 0);
	 else
	    lval = 0;
	 if (*rbp)
	    rval = Parser (rbp, &lrelo, &index, 1, FALSE, 0);
	 else
	    rval = 0;
#ifdef DEBUGCONDEXPR
	 fprintf (stderr, "   lval = %d, rval = %d\n", lval, rval);
#endif

	 /*
	 ** Set compare values
	 */

	 if (lval > rval) r = 1;
	 else if (lval < rval) r = -1;
	 else r = 0;
      }

      /*
      ** Set return relation based on requested compare
      */

      if (rel == EQ && r == 0) ret = TRUE;
      else if (rel == GT && r > 0) ret = TRUE;
      else if (rel == LT && r < 0) ret = TRUE;
      else ret = FALSE;
#ifdef DEBUGCONDEXPR
      fprintf (stderr, "   val = %d, ret = %s\n", r, ret ? "TRUE" : "FALSE");
      fprintf (stderr, "   term = %c(%x), bp = %s\n", *term, *term, bp);
#endif
      *val = ret;
   }
   return (bp);
}
