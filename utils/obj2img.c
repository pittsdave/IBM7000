/***********************************************************************
*
* ob2img - Convert asm7090 assembler output to image tape format.
*
* Changes:
*   05/12/10   DGP   Original. Hacked from obj2bin.
*   08/24/10   DGP   Added Line Marked eXtended (-x) file mode
*   10/15/10   DGP   Correct to use even parity on BCD data.
*   10/15/10   DGP   Added -A alternate BCD support.
*   10/18/10   DGP   Added -S simh format.
*   11/09/10   DGP   Added Odd parity to BSS writes.
*   12/27/10   DGP   Fixed -x (12 bit) mode.
*   04/01/11   DGP   Fixed BSS EOF processing.
*   11/15/11   DGP   Added absimage processing.
*   12/07/11   DGP   Added additional SYSENTRY points.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <ctype.h>
#if !defined(WIN32)
#include <unistd.h>
#endif

#include "sysdef.h"
#include "objdef.h"

static t_uint64 memory[MEMSIZE];

static int verbose;
static int countformat;
static int bit12mode;
static int altbcd;
static int simhfmt;
static int absimage;

static uint8 word[12];
static uint8 ctlbits;

#include "parity.h"
#include "tobcd.h"

#define BLOCKSIZE 432
#define SYSENTRY  024

#if defined(WIN32) && !defined(MINGW)
#define TTR  0002100000000I64
#else
#define TTR  0002100000000ULL
#endif

#ifdef DEBUGDUMP
#include "octdump.h"
#endif

/***********************************************************************
* tapewriteint - Write an integer.
***********************************************************************/

static void
tapewriteint (FILE *fd, int v)
{
   fputc (v & 0xFF, fd);
   fputc ((v >> 8) & 0xFF, fd);
   fputc ((v >> 16) & 0xFF, fd);
   fputc ((v >> 24) & 0xFF, fd);
}

/***********************************************************************
* writeword - Write a word.
***********************************************************************/

static void
writeword (t_uint64 wrd, FILE *fd)
{
   int i;

   for (i = 0; i < 6; i++)
   {
      word[i] = oddpar[(uint8)((wrd >> 30) & 077)];
      if (!simhfmt && (i == 0)) word[i] |= ctlbits;
      else ctlbits = 0;
      wrd <<= 6;
   }
   fwrite (word, 1, 6, fd);
}

/***********************************************************************
* writeblock - Write a block of words.
***********************************************************************/

static void
writeblock (int start, int wrdcnt, FILE *fd)
{
   int i;

   if (verbose)
   {
      printf ("obj2img: writeblock: start = %05o, wrdcnt = %o\n",
	      start, wrdcnt);
   }

   /*
   ** Write the block
   */

   if (simhfmt) tapewriteint (fd, wrdcnt*6);
   for (i = 0; i < wrdcnt; i++)
   {
      writeword (memory[start+i], fd);
   }
   if (simhfmt) tapewriteint (fd, wrdcnt*6);

}

/***********************************************************************
* mapchar - Map char to BCD and even parity.
***********************************************************************/

static int
mapchar (int ch)
{
   return evenpar[altbcd ? toaltbcd[ch] : tobcd[ch]];
}

/***********************************************************************
* writetext - Write text data.
***********************************************************************/

static void
writetext (char *inbuf, int reclen, FILE *fd)
{
   int i;
   int wdlen;
   int modulo;
   char rec[512];

   if (verbose)
      printf ("obj2img: writetext: inbuf = %s\n", inbuf);

   if (countformat)
      reclen = strlen (inbuf);
   wdlen = reclen;
   if (bit12mode)
   {
      modulo = 3;
   }
   else
   {
      modulo = 6;
   }
   wdlen /= modulo;
   if (reclen % modulo != 0) wdlen++;
   if (wdlen < 3)
      wdlen = 3;
   if (verbose)
      printf ("   reclen = %d, wdlen = %d\n", reclen, wdlen);
   wdlen *= modulo;
   if (simhfmt) tapewriteint (fd, wdlen);

   /* Copy data to record */
   reclen = wdlen * (bit12mode ? 2 : 1);
   for (i = 0; *inbuf && i < reclen; i++, inbuf++)
   {
      if (bit12mode)
      {
	 uint16 xch = to12bit[*inbuf];
	 char cch;
	 cch = (xch >> 6) & 077;
	 if (altbcd && (cch & 020)) cch = cch ^ 040;
	 rec[i] = evenpar[cch];
	 i++;
	 cch = xch & 077;
	 if (altbcd && (cch & 020)) cch = cch ^ 040;
	 rec[i] = evenpar[cch];
      }
      else
      {
	 rec[i] = mapchar (*inbuf);
      }
   }

   /* Blank remainder */
   for (; i < reclen; i++)
   {
      if (bit12mode)
      {
	 rec[i] = evenpar[0];
	 i++;
	 rec[i] = evenpar[057];
      }
      else
      {
	 rec[i] = countformat ? evenpar[057] : mapchar (' ');
      }
   }

   rec[0] |= 0200;

   fwrite (rec, 1, reclen, fd);
   if (simhfmt) tapewriteint (fd, wdlen);
}

/***********************************************************************
* writefname - Write the file name and extension.
***********************************************************************/

static void
writefname (char *pfname, char *pfext, char mode, int reclen, char *attach,
	    FILE *fd)
{
   int i;
   char rec[84];
   char fname[10];
   char fext[10];

   if (verbose)
   {
      printf ("obj2img: writefname: fname = %s, fext = %s, mode = %c\n",
	      pfname, pfext, mode);
   }

   /* Justify names */
   sprintf (fname, "%6.6s", pfname);
   sprintf (fext, "%6.6s", pfext);

   /* Copy names to record */
   for (i = 0; i < 6; i++)
   {
      if (islower (fname[i])) fname[i] = toupper (fname[i]);
      rec[i] = mapchar (fname[i]);
      if (islower (fext[i])) fname[i] = toupper (fext[i]);
      rec[i+6] = mapchar (fext[i]);
   }

   /* Blank remainder */
   for (i = 12; i < 84; i++)
   {
      rec[i] = mapchar (' ');
   }

   /* Fill in mode */
   rec[17] = mapchar (mode);

   /* Fill in reclen if not image mode */
   if (mode != 'I')
   {
      unsigned int k;

      k = (unsigned int)reclen / 6;
      for (i = 0; i < 6; i++)
      {
	 char cch;

	 cch = k & 077;
	 if (altbcd && (cch & 020)) cch = cch ^ 040;
         rec[(6-i)+17] = evenpar[cch];
	 k >>= 6;
      }
   }

   /* Fill in attach info */
   for (i = 0; i < 12; i++)
   {
      rec[i+24] = mapchar (attach[i]);
   }

   if (simhfmt) tapewriteint (fd, 84);
   else         rec[0] |= 0200;
   fwrite (rec, 1, 84, fd);
   if (simhfmt) tapewriteint (fd, 84);
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char *argv[])
{
   FILE *infd;
   FILE *outfd;
   char *bp;
   char *infile;
   char *outfile;
   int loadaddr;
   int curraddr;
   int entaddr;
   int wrdcnt;
   int usefname;
   int image;
   int reclen;
   int ufdfile;
   int linkfile;
   int quotafile;
   int bssfile;
   int common;
   int i, j;
   int idx;
   int maxaddr;
   int processsyms;
   char inbuf[512];
   char attach[32];
   char fname[10];
   char fext[10];

   /*
   ** Process command line arguments
   */

   verbose = FALSE;
   usefname = FALSE;
   ufdfile = FALSE;
   linkfile = FALSE;
   quotafile = FALSE;
   bssfile = FALSE;
   processsyms = FALSE;
   countformat = FALSE;
   bit12mode = FALSE;
   altbcd = FALSE;
   simhfmt = FALSE;
   absimage = FALSE;
   image = TRUE;

   infile = NULL;
   outfile = NULL;

   maxaddr = 0;
   common = 0;
   loadaddr = 0100;
   curraddr = 0100;
   reclen = 84;

   entaddr = -1;

   strcpy (fext, "TSSDC.");
   strcpy (attach, " M1416CMFL02");
   fname[0] = '\0';

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];
      if (*bp == '-')
      {
         for (bp++; *bp; bp++) switch (*bp)
	 {
	 case 'S': /* Simh format */
	    simhfmt = TRUE;
	    /* Fall through */

	 case 'A': /* Alternate BCD */
	    altbcd = TRUE;
	    break;

	 case 'a': /* Attach to user */
	    i++;
	    if (i >= argc) goto usage;
	    strcpy (attach, argv[i]);
	    if (strlen (argv[i]) != 12)
	    {
	       fprintf (stderr,
	       		"obj2img: %s: Attach name length not equal 12.\n",
			argv[i]);
	       exit (1);
	    }
	    break;

	 case 'b': /* BSS file */
	    bssfile = TRUE;
	    image = FALSE;
	    reclen = 168;
	    break;

	 case 'c': /* Process text in 6-bit COUNT format */
	    countformat = TRUE;
	    break;

	 case 'e': /* file Extension */
	    i++;
	    if (i >= argc) goto usage;
	    if (strlen (argv[i]) > 6)
	    {
	       fprintf (stderr, "obj2img: %s: File extension too long.\n",
	       		argv[i]);
	       exit (1);
	    }
	    strcpy (fext, argv[i]);
	    break;

	 case 'f': /* File name */
	    i++;
	    if (i >= argc) goto usage;
	    strcpy (fname, argv[i]);
	    if (strlen (argv[i]) > 6)
	    {
	       fprintf (stderr, "obj2img: %s: File name too long.\n", argv[i]);
	       exit (1);
	    }
	    usefname = TRUE;
	    break;

	 case 'I': /* absolute Image mode */
	    absimage = TRUE;
	    break;

	 case 'l': /* LINK file */
	    linkfile = TRUE;
	    image = FALSE;
	    break;

	 case 'L': /* Load address, for relocatable images */
	    i++;
	    if (i >= argc) goto usage;
	    curraddr = loadaddr = atoi (argv[i]);
	    break;

	 case 'o': /* Output file */
	    i++;
	    if (i >= argc) goto usage;
	    outfile = argv[i];
	    break;

	 case 'q': /* Quota file */
	    quotafile = TRUE;
	    image = FALSE;
	    break;

	 case 'r': /* Record length */
	    i++;
	    if (i >= argc) goto usage;
	    reclen = atoi (argv[i]);
	    break;

	 case 's': /* Process Symbols file */
	    image = FALSE;
	    processsyms = TRUE;
	    strcpy (fname, "COM000");
	    strcpy (fext, "SYMTB");
	    break;

	 case 't': /* Text file */
	    image = FALSE;
	    break;

	 case 'u': /* UFD file */
	    ufdfile = TRUE;
	    image = FALSE;
	    break;

	 case 'v': /* Verbose */
	    verbose = TRUE;
	    break;

	 case 'x': /* Process text in 12-bit eXtended COUNT format */
	    countformat = TRUE;
	    bit12mode = TRUE;
	    reclen = 168;
	    break;

	 default:
	 usage:
	     fprintf (stderr,
		"usage: obj2img [options] -o outfile infile [infile...]\n");
	     fprintf (stderr,
		"  -A           - Use Alternate character set\n");
	     fprintf (stderr,
	   "  -a \"PROBPROG\" - Attach to user, default = \" M1416CMFL02\"\n");
	     fprintf (stderr,
		"  -b           - BSS mode file\n");
	     fprintf (stderr,
		"  -c           - LINE Marked Count file\n");
	     fprintf (stderr,
		"  -e EXT       - Extension, default = TSSDC.\n");
	     fprintf (stderr,
		"  -f FILE      - File name, default = module IDT\n");
	     fprintf (stderr,
		"  -I           - Absolute Image mode\n");
	     fprintf (stderr,
		"  -l           - LINK file\n");
	     fprintf (stderr,
		"  -L addr      - Load address\n");
	     fprintf (stderr,
		"  -o outfile   - Output file\n");
	     fprintf (stderr,
		"  -q           - QUOTA file\n");
	     fprintf (stderr,
		"  -r LEN       - Text mode record length, default = 84\n");
	     fprintf (stderr,
		"  -S           - Use simh format\n");
	     fprintf (stderr,
		"  -s           - Symbol table file\n");
	     fprintf (stderr,
		"  -t           - Text file\n");
	     fprintf (stderr,
		"  -u           - UFD file\n");
	     fprintf (stderr,
		"  -x           - LINE Marked eXtended Count file\n");
	     exit (1);
	 }
      }
      else
         break;
   }

   if (outfile == NULL) goto usage;
   if (i >= argc) goto usage;
   idx = i;

   /*
   ** Open the output file.
   */

   if ((outfd = fopen (outfile, "wb")) == NULL)
   {
      fprintf (stderr, "obj2img: output open failed: %s\n",
	       strerror (errno));
      fprintf (stderr, "filename: %s\n", outfile);
      exit (1);
   }

   if (image)
   {
      memset (&memory, '\0', sizeof (memory));

      for (; idx < argc; idx++)
      {

	 /*
	 ** Open the input files.
	 */

	 infile = argv[idx];
	 if (verbose)
	 {
	    printf ("obj2img: Process %s\n", infile);
	 }
	 if ((infd = fopen (infile, "r")) == NULL)
	 {
	    fprintf (stderr, "obj2img: input open failed: %s\n",
		     strerror (errno));
	    fprintf (stderr, "filename: %s\n", infile);
	    exit (1);
	 }

	 /*
	 ** Load "memory" with the program.
	 */

	 memory[SYSENTRY] = TTR | loadaddr;
	 memory[SYSENTRY+1] = TTR | loadaddr;
	 memory[SYSENTRY+2] = TTR | loadaddr;
	 while (fgets (inbuf, sizeof (inbuf), infd))
	 {
	    char *op = inbuf;

	    if (*op == IBSYSSYM) break; /* $EOF */

	    for (i = 0; i < 5; i++)
	    {
	       char otag;
	       char item[16];
	       t_uint64 lldata, dectmp;

	       otag = *op++;
	       if (otag == ' ') break;
	       lldata = 0;
	       strncpy (item, op, 12);
	       item[12] = '\0';
#if defined(WIN32)
	       sscanf (item, "%I64o", &lldata);
#else
	       sscanf (item, "%llo", &lldata);
#endif

#ifdef DEBUG
	       printf ("loadaddr = %05o, curraddr = %05o\n",
			loadaddr, curraddr);
	       printf ("   otag = %c, item = %s\n", otag, item);
#if defined(WIN32)
	       printf ("   lldata = %12.12I64o\n", lldata);
#else
	       printf ("   lldata = %12.12llo\n", lldata);
#endif
#endif

	       switch (otag)
	       {
	       case IDT_TAG:
		  if (!usefname)
		  {
		     strncpy (fname, op, 6);
		     fname[6] = '\0';
		     for (j = 5; j > 0; j--)
		     {
			if (fname[j] == ' ') fname[j] = '\0';
			else break;
		     }
		     if (fname[0] == ' ')
		     {
			fprintf (stderr, "obj2img: No file name for image\n");
			exit (1);
		     }
		  }
		  break;

	       case ABSORG_TAG:
		  curraddr = (int)(lldata & ADDRMASK);
		  if (verbose)
		  {
		     printf ("obj2img: ABSORG = %05o\n", curraddr);
		  }
		  break;

	       case RELORG_TAG:
		  curraddr = (int)((lldata + loadaddr) & ADDRMASK);
		  if (verbose)
		  {
		     printf ("obj2img: RELORG = %05o\n", curraddr);
		  }
		  break;

	       case BSS_TAG:
		  curraddr += (int)(lldata & ADDRMASK);
		  curraddr &= ADDRMASK;
		  if (verbose)
		  {
		     printf ("obj2img: BSS = %05o\n", curraddr);
		  }
		  break;

	       case ABSDATA_TAG:
		  memory[curraddr] = lldata;
		  curraddr++;
		  break;

	       case RELADDR_TAG:
		  memory[curraddr] = lldata + loadaddr;
		  curraddr++;
		  break;

	       case RELBOTH_TAG:
		  lldata += loadaddr;
	       case RELDECR_TAG:
		  dectmp = ((lldata & DECRMASK) >> DECRSHIFT) + loadaddr;
		  lldata &= ~DECRMASK;
		  lldata |= (dectmp << DECRSHIFT); 
		  memory[curraddr] = lldata;
		  curraddr++;
		  break;

	       case ABSXFER_TAG:
		  goto LOADED;
	       case ABSENTRY_TAG:
		  entaddr = (int)(lldata & ADDRMASK);
		  if (verbose)
		  {
		     printf ("obj2img: ABSENTRY = %05o\n", entaddr);
		  }
		  break;

	       case RELXFER_TAG:
		  goto LOADED;
	       case RELENTRY_TAG:
		  entaddr = (int)(lldata & ADDRMASK) + loadaddr;
		  if (verbose)
		  {
		     printf ("obj2img: RELENTRY = %05o\n", entaddr);
		  }
		  break;

	       case FAPCOMMON_TAG:
		  common = (int)(lldata & ADDRMASK);
		  break;

	       default: ;
	       }
	       op += 12;
	       if (curraddr > maxaddr)
		  maxaddr = curraddr;
	    }
	 }
	 fclose (infd);
      }

      LOADED:

      if (verbose)
      {
         printf ("obj2img: maxaddr = %o, entaddr = %o, ABSimage = %s\n",
	 	 maxaddr, entaddr, absimage ? "TRUE" : "FALSE");
	 if (common)
	    printf ("   common = %o\n", common);
      }
      writefname (fname, fext, 'I', 0, attach, outfd);

      if (!absimage && (entaddr > 0))
      {
	 memory[SYSENTRY] = TTR | entaddr;
	 memory[SYSENTRY+1] = TTR | entaddr;
	 memory[SYSENTRY+2] = TTR | entaddr;
      }
#ifdef DEBUGDUMP
      octdump (stderr, &memory[loadaddr], maxaddr, 0);
#endif
      if (common)
         wrdcnt = FAPCOMMONSTART;
      else
	 wrdcnt = maxaddr;

      loadaddr = SYSENTRY;
      while (wrdcnt)
      {
	 int blksize;

	 ctlbits = 0200;
	 blksize = wrdcnt >= BLOCKSIZE ? BLOCKSIZE : wrdcnt;
	 writeblock (loadaddr, blksize, outfd);
	 wrdcnt -= blksize;
	 loadaddr += blksize;
      }
   }

   /*
   ** Check if processing a Symbol table file.
   */

   else if (processsyms)
   {
      char *outp;

      int symrec;
      char outbuf[86];

      infile = argv[idx];
      if ((infd = fopen (infile, "r")) == NULL)
      {
	 fprintf (stderr, "obj2img: input open failed: %s\n",
		  strerror (errno));
	 fprintf (stderr, "filename: %s\n", infile);
	 exit (1);
      }

      writefname (fname, fext, 'T', reclen, attach, outfd);

      symrec = 0;
      outp = outbuf;
      *outp = '\0';

      while (fgets (inbuf, sizeof (inbuf), infd))
      {
	 char *op = inbuf;

	 if (*op == IBSYSSYM) break; /* $EOF */

	 for (i = 0; i < 5; i++)
	 {
	    char relflag;
	    char otag;
	    char item[16];

	    otag = *op++;
	    if (otag == ' ') break;
	    strncpy (item, op, 12);
	    item[12] = '\0';
	    relflag = '0';

	    switch (otag)
	    {
	    case RELGLOBAL_TAG:
	    case RELSYM_TAG:
	       relflag = '1';
	    case ABSGLOBAL_TAG:
	    case ABSSYM_TAG:
	    case COMMON_TAG:

	       if (verbose)
	       {
	          printf ("obj2img: COMMON item = %s\n", item);
	       }
	       strncpy (outp, &item[6], 6);
	       outp += 6;
	       strncpy (outp, "   ", 3);
	       *(outp+1) = relflag;
	       outp += 3;
	       strncpy (outp, item, 6);
	       outp += 6;
	       strncpy (outp, "   ", 3);
	       outp += 3;
	       *outp = '\0';
	       symrec++;
	    default: ;
	    }
	    op += 12;
	    if (symrec >= 4)
	    {
	       writetext (outbuf, reclen, outfd);
	       symrec = 0;
	       outp = outbuf;
	       *outp = '\0';
	    }
	 }
      }
      if (symrec)
      {
	 writetext (outbuf, reclen, outfd);
      }
      fclose (infd);
   }

   /*
   ** Process BSS mode file.
   */

   else if (bssfile)
   {
      int ch;
      uint8 datbuf[MAXRECSIZE];

      infile = argv[idx];
      if ((infd = fopen (infile, "rb")) == NULL)
      {
	 fprintf (stderr, "obj2img: input open failed: %s\n",
		  strerror (errno));
	 fprintf (stderr, "filename: %s\n", infile);
	 exit (1);
      }

      writefname (fname, fext, 'B', reclen, attach, outfd);

      while ((ch = fgetc (infd)) != EOF)
      {
	 int j;

	 if (ch == 0217)
	    break;
	 ungetc (ch, infd);
	 if (simhfmt) tapewriteint (outfd, reclen);
	 for (j = 0; j < reclen; j++)
	 {
	    datbuf[j] = fgetc (infd);
	    if (datbuf[j] == 0217)
	    {
	       break;
	    }
	    datbuf[j] = oddpar[datbuf[j] & 077];
	    if (j == 0) datbuf[j] |= 0200;
	    fputc (datbuf[j], outfd);
	 }
	 if (simhfmt) tapewriteint (outfd, reclen);
      }
      fclose (infd);
   }

   /*
   ** If Text, LINK, QUOTA or UFD mode process here.
   */

   else
   {
      char mode;

      if (ufdfile)          mode = 'U';
      else if (linkfile)    mode = 'L';
      else if (quotafile)   mode = 'Q';
      else if (countformat) mode = 'C';
      else                  mode = 'T';

      infile = argv[idx];
      if ((infd = fopen (infile, "r")) == NULL)
      {
	 fprintf (stderr, "obj2img: input open failed: %s\n",
		  strerror (errno));
	 fprintf (stderr, "filename: %s\n", infile);
	 exit (1);
      }

      writefname (fname, fext, mode, reclen, attach, outfd);

      while (fgets (inbuf, sizeof (inbuf), infd))
      {
	 char *p = strchr (inbuf, '\n');
	 if (*p) *p = '\0';
	 writetext (inbuf, reclen, outfd);
      }
      fclose (infd);
   }

   if (simhfmt) tapewriteint (outfd, 0);
   else         fputc (0217, outfd);
   fclose (outfd);

   return (0);
}
