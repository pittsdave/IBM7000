/************************************************************************
*
* lnklibrary - Loads library objects from asm7090 for linking.
*
* Changes:
*   06/14/10   DGP   Original. Hacked from lnkloader.c
*   06/23/10   DGP   Fixed ref chain when link is zero.
*   08/10/10   DGP   Fixed COMMON.
*   11/02/10   DGP   More COMMON changes.
*   12/23/10   DGP   Make COMMON origin address even on definition.
*	
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include "lnkdef.h"

extern FILE *lstfd;

extern int listmode;
extern int pc;
extern int errcount;
extern int absentry;
extern int relentry;
extern int modcount;
extern int genxref;
extern int ctsscommon;
extern int common;
extern int commonorg;
extern int commonmod;
extern int commontop;
extern int movieref;
extern int errno;

extern char deckname[MAXSYMLEN+2];

extern Module modules[MAXMODULES];
extern Memory memory[MEMSIZE];

static int modnumber;

/************************************************************************
* checkdef - Check if ENTRY is needed.
************************************************************************/

static int
checkdef (char *item, int relo, int flags)
{
   SymNode *s;

#ifdef DEBUGLIBRARYCHECK
   printf (", R-ENTRY = %s", item);
#endif

   if ((s = symlookup (item, "", FALSE)) != NULL)
   {
      if (s->undef)
      {
#ifdef DEBUGLIBRARYCHECK
	 printf (", value = %05o", s->value);
#endif
         return (1);
      }
   }
   return (0);
}

/***********************************************************************
* libloader - Load library modules into memory.
***********************************************************************/

static int
libloader (FILE *fd, int loadpt, int pass, char *file)
{
   int status = 0;
   int modcommon = 0177777;
   int modlength = 0;
   int reclen;
   int curraddr;
   int setcommonorg;
   int offset;
   int addrflag, decrflag;
   char module[MAXSYMLEN+2];
   char inbuf[82];

#ifdef DEBUGLIBRARYLOADER
   printf ("libloader: loadpt = %5.5o, pass = %d, file = %s\n",
	    loadpt, pass, file);
#endif

   curraddr = loadpt;
   setcommonorg = FALSE;

   while (fgets (inbuf, sizeof(inbuf), fd))
   {
      SymNode *s;
      char *op = inbuf;
      int i;

      reclen = strlen (inbuf);
      if (*op == IBSYSSYM)
      {
	 if (pass == 1)
	 {
	    if (reclen > 60)
	    {
	       strncpy (modules[modcount].date, &inbuf[TIMEOFFSET], 8);
	       modules[modcount].date[9] = '\0';
	       strncpy (modules[modcount].time, &inbuf[DATEOFFSET], 8);
	       modules[modcount].time[9] = '\0';
	       strncpy (modules[modcount].creator, &inbuf[CREATOROFFSET], 8);
	       modules[modcount].creator[9] = '\0';
	    }
	    else
	    {
	       strncpy (modules[modcount].date, "        ", 8);
	       strncpy (modules[modcount].time, "        ", 8);
	       strncpy (modules[modcount].creator, "        ", 8);
	    }
	    modcount++;
	 }
	 modnumber++;
	 loadpt = curraddr;
	 continue;
      }

      for (i = 0; i < CHARSPERREC; i += WORDTAGLEN)
      {
	 t_uint64 ldata;
	 t_uint64 dectmp, addtmp;
	 char otag;
	 char item[16];

	 otag = *op++;
	 if (otag == ' ') break;
	 strncpy (item, op, 12);
	 item[12] = '\0';
#ifdef DEBUGLIBRARYLOADER
	 printf ("   loadpt = %5.5o, curraddr = %5.5o\n",
		  loadpt, curraddr);
	 printf ("   otag = %c, item = %s\n", otag, item);
#endif
	 switch (otag)
	 {
	 case IDT_TAG:
	 case ABSEXTRN_TAG:
	 case RELEXTRN_TAG:
	 case ABSGLOBAL_TAG:
	 case RELGLOBAL_TAG:
	 case ABSXFERVEC_TAG:
	 case RELXFERVEC_TAG:
	    {
	       char *bp = item;

#if defined(WIN32)
	       sscanf (&item[6], "%I64o", &ldata);
#else
	       sscanf (&item[6], "%llo", &ldata);
#endif
	       item[MAXSYMLEN] = '\0';
	       for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
	    }
	    break;

	 default:
#if defined(WIN32)
	    sscanf (item, "%I64o", &ldata);
#else
	    sscanf (item, "%llo", &ldata);
#endif
	 }


#ifdef DEBUGLIBRARYLOADER
#if defined(WIN32)
	 printf ("      ldata = %12.12I64o\n", ldata);
#else
	 printf ("      ldata = %12.12llo\n", ldata);
#endif
#endif
         ldata = ldata & WORDMASK;

         switch (otag)
	 {
	 case IDT_TAG:
#ifdef DEBUGLIBRARYLOADER
            printf ("IDT = %s\n", item);
#endif
	    strcpy (module, item);
	    modlength = (int)ldata;
	    if (modlength & 1) modlength += 1;
	    if (pass == 1)
	    {
#ifdef DEBUGLIBRARYLOADER
	       printf ("   add module: modcount = %d, name = %s, file = %s\n",
			modcount, item, file);
#endif
	       strcpy (modules[modcount].objfile, file);
	       strcpy (modules[modcount].name, item);
	       modules[modcount].length = modlength;
	       modules[modcount].origin = loadpt;
	       modules[modcount].creator[0] = '\0';
	       modules[modcount].date[0] = '\0';
	       modules[modcount].time[0] = '\0';
	    }
	    if (deckname[0] == '\0')
	    {
	       strcpy (deckname, item);
	    }
	    break;

	 case BSS_TAG:
	    memory[curraddr].word = ldata;
	    memory[curraddr].tag = otag;
	    curraddr += (int)(ldata & ~ADDRMASK);
	    break;

	 case RELORG_TAG:
	    if (ctsscommon && ((int)(ldata & ~ADDRMASK) >= modcommon))
	    {
	       curraddr = commonorg + ((int)(ldata & ~ADDRMASK) - common);
#ifdef DEBUGLIBRARYLOADER
	       printf ("   LIB COMMON ORG: common = %o, commonorg = %o, "
	               "val = %o, curraddr = %o, commontop = %o\n",
		       common, commonorg, (int)(ldata & ~ADDRMASK),
		       curraddr, commontop);
#endif
	    }
	    else
	    {
	       offset = loadpt;
	       if (!ctsscommon && ((int)(ldata & ~ADDRMASK) >= modcommon))
	          offset = 0;
	       ldata = ldata + offset;
	       curraddr = (int)(ldata & ~ADDRMASK);
	    }
	    break;

	 case ABSDATA_TAG:
	    memory[curraddr].word = ldata;
	    memory[curraddr].tag = otag;
	    memory[curraddr].modnum = modnumber+1;
	    curraddr++;
	    break;

	 case RELADDR_TAG:
	    addrflag = TRUE;
	    if (ctsscommon && ((int)(ldata & ~ADDRMASK) >= modcommon))
	    {
	       addtmp = commonorg + ((ldata & ~ADDRMASK) - common)
			& ~ADDRMASK;
#ifdef DEBUGLIBRARYLOADER
	       printf ("   LIB COMMON ADDR: common = %o, commonorg = %o, "
	       	       "val = %o, addtmp = %llo\n",
		       common, commonorg, (int)(ldata & ~ADDRMASK), addtmp);
#endif
	    }
	    else
	    {
	       offset = loadpt;
	       if (!ctsscommon && ((int)(ldata & ~ADDRMASK) >= modcommon)) {
		  otag = ABSDATA_TAG;
	          offset = 0;
		  addrflag = FALSE;
	       }
	       addtmp = ((ldata & ~ADDRMASK) + offset) & ~ADDRMASK;
	    }
	    memory[curraddr].word = (ldata & ADDRMASK ) | addtmp;
	    memory[curraddr].tag = otag;
	    memory[curraddr].reladdr = addrflag;
	    memory[curraddr].modnum = modnumber+1;
	    curraddr++;
	    break;

	 case RELDECR_TAG:
	    decrflag = TRUE;
	    if (ctsscommon && (((int)(ldata & ~DECRMASK) >> 18) >= modcommon))
	    {
	       dectmp = (commonorg + (((ldata & ~DECRMASK) >> 18) - common)
	       		& ~ADDRMASK) << 18;
#ifdef DEBUGLIBRARYLOADER
	       printf ("   LIB COMMON DECR: common = %o, commonorg = %o, "
	       	       "val = %o, dectmp = %llo\n",
		       common, commonorg, (int)((ldata & ~DECRMASK) >> 18),
		       (int)(dectmp >> 18));
#endif
	    }
	    else
	    {
	       offset = loadpt;
	       if (!ctsscommon &&
	           (((int)(ldata & ~DECRMASK) >> 18) >= modcommon)) {
		  otag = ABSDATA_TAG;
	          offset = 0;
		  decrflag = FALSE;
	       }
	       dectmp = ((((ldata & ~DECRMASK) >> 18) + offset)
	       		& ~ADDRMASK) << 18;
	    }
	    memory[curraddr].word = (ldata & DECRMASK) | dectmp;
	    memory[curraddr].tag = otag;
	    memory[curraddr].reldecr = decrflag;
	    memory[curraddr].modnum = modnumber+1;
	    curraddr++;
	    break;

	 case RELBOTH_TAG:
	    addrflag = TRUE;
	    decrflag = TRUE;
	    if (ctsscommon && ((int)(ldata & ~ADDRMASK) >= modcommon))
	    {
	       addtmp = commonorg + ((ldata & ~ADDRMASK) - common)
			& ~ADDRMASK;
#ifdef DEBUGLIBRARYLOADER
	       printf ("   LIB COMMON ADDR: common = %o, commonorg = %o, "
	       	       "val = %o, addtmp = %llo\n",
		       common, commonorg, (int)(ldata & ~ADDRMASK), addtmp);
#endif
	    }
	    else
	    {
	       offset = loadpt;
	       if (!ctsscommon && ((int)(ldata & ~ADDRMASK) >= modcommon)) {
		  otag = RELDECR_TAG;
	          offset = 0;
		  addrflag = FALSE;
	       }
	       addtmp = ((ldata & ~ADDRMASK) + offset) & ~ADDRMASK;
	    }
	    if (ctsscommon && (((int)(ldata & ~DECRMASK) >> 18) >= modcommon))
	    {
	       dectmp = (commonorg + (((ldata & ~DECRMASK) >> 18) - common)
	       		& ~ADDRMASK) << 18;
#ifdef DEBUGLIBRARYLOADER
	       printf ("   LIB COMMON DECR: common = %o, commonorg = %o, "
	       	       "val = %o, dectmp = %llo\n",
		       modcommon, commonorg, (int)((ldata & ~DECRMASK) >> 18),
		       (int)(dectmp >> 18));
#endif
	    }
	    else
	    {
	       offset = loadpt;
	       if (!ctsscommon &&
	           (((int)(ldata & ~DECRMASK) >> 18) >= modcommon)) {
		  otag = addrflag ? RELADDR_TAG : ABSDATA_TAG;
	          offset = 0;
		  decrflag = FALSE;
	       }
	       dectmp = ((((ldata & ~DECRMASK) >> 18) + offset)
	       		& ~ADDRMASK) << 18;
	    }
	    memory[curraddr].word = (ldata & BOTHMASK) | addtmp | dectmp;
	    memory[curraddr].tag = otag;
	    memory[curraddr].reldecr = decrflag;
	    memory[curraddr].reladdr = addrflag;
	    memory[curraddr].relboth = addrflag & decrflag;
	    memory[curraddr].modnum = modnumber+1;
	    curraddr++;
	    break;

	 case ABSORG_TAG:
	 case ABSGLOBAL_TAG:
	 case ABSENTRY_TAG:
	    if (pass == 2)
	    {
	       errcount++;
	       if (listmode)
	       {
		  printheader (lstfd);
		  fprintf (lstfd,
		     "Linking of absolute modules not supported\n");
	       }
	       else
	       {
		  fprintf (stderr, 
			"lnk7090: Linking of absolute modules not supported\n");
	       }
#ifdef DEBUGLIBRARYLOADER
	       printf (
		     "lnk7090: Linking of absolute modules not supported\n");
#endif
	    }
	    status = -1;
	    break;

	 case RELENTRY_TAG:
	    relentry = (int)((ldata & ~ADDRMASK) + loadpt);
	    break;

	 case RELEXTRN_TAG:
	    if (pass == 1)
	    {
	       if (!strcmp (item, MOVIE))
	          movieref = TRUE;
	    }
	    else /* pass == 2 */
	    {
	       if (!(s = symlookup (item, module, FALSE)))
	       {
	          s = symlookup (item, module, TRUE);
		  s->relocatable = TRUE;
		  s->undef = TRUE;
		  s->value = (int)((ldata + loadpt) & ~ADDRMASK);
		  s->modnum = modnumber+1;
#ifdef DEBUGLIBRARYLOADER
		  printf ("lnk7090: Symbol %s undefined, value = %05o\n",
		  	  item, s->value);
#endif
	          status = -1;
	       }
	       else
	       {
		  int refaddr = (int)((ldata + loadpt) & ~ADDRMASK);

		  if (!s->undef)
		  {
		     if (!refaddr)
		     {
			memory[refaddr].word =
			      (memory[refaddr].word & ADDRMASK) | s->value;
			memory[refaddr].tag = RELADDR_TAG;
			chkxref (s, refaddr);
		     }
		     else while (refaddr)
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
		  else
		  {
#ifdef DEBUGLIBRARYLOADER
		     printf ("lnk7090: Symbol %s undefined, refaddr = %05o\n",
			     item, refaddr);
#endif
		     while (refaddr)
		     {
			int k;

			k = (int)(memory[refaddr].word & ~ADDRMASK);
			if (k == 0)
			{
			   memory[refaddr].tag = RELADDR_TAG;
			   memory[refaddr].word =
				 (memory[refaddr].word & ADDRMASK) | s->value;
			   s->value = (int)((ldata + loadpt) & ~ADDRMASK);
			}
			refaddr = k;
		     }
		  }
	       }
	    }
	    break;

	 case RELGLOBAL_TAG:
	    if (pass == 1)
	    {
#ifdef DEBUGLIBRARY
	       printf ("      R-ENTRY = %s\n", item);
#endif
	       if ((s = symlookup (item, module, FALSE)) == NULL)
	       {
		  s = symlookup (item, module, TRUE);
		  s->value = (int)((ldata & ~ADDRMASK) + loadpt);
		  s->global = TRUE;
		  s->undef = FALSE;
		  s->relocatable = TRUE;
		  s->modnum = modnumber + 1;
#ifdef DEBUGLIBRARY
		  printf ("      def: value = %05o", s->value);
#endif
	       }
	       else
	       {
		  if (s->undef)
		  {
		     int refaddr = s->value;

		     s->value = (int)((ldata & ~ADDRMASK) + loadpt);
		     s->modnum = modnumber + 1;
#ifdef DEBUGLIBRARY
		     printf ("      fill: value = %05o, refaddr = %05o\n",
			      s->value, refaddr);
#endif
		     if (!refaddr)
		     {
			memory[refaddr].word =
			      (memory[refaddr].word & ADDRMASK) | s->value;
			memory[refaddr].tag = RELADDR_TAG;
			chkxref (s, refaddr);
		     }
		     else while (refaddr)
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
			if (!k && kt == RELADDR_TAG)
			{
			   memory[refaddr].word =
				 (memory[refaddr].word & ADDRMASK) | s->value;
			   memory[refaddr].tag = RELADDR_TAG;
			   chkxref (s, refaddr);
			}
#ifdef DEBUGLIBRARY
			printf ("      refaddr = %05o\n", refaddr);
#endif
		     }
		     s->relocatable = TRUE;
		     s->global = TRUE;
		     s->undef = FALSE;
		  }
		  else
		     s->muldef = TRUE;
	       }
	    }
	    break;

	 case RELXFERVEC_TAG:
	    ldata = ldata + loadpt;
	 case ABSXFERVEC_TAG:
	    if (pass == 2)
	    {
	       if (!(s = symlookup (item, module, FALSE)))
	       {
	          s = symlookup (item, module, TRUE);
		  memory[ldata].word = TRAINST | s->value;
	       }
	    }
	    break;

	 case FAPCOMMON_TAG:
	    modcommon = (int)(ldata & ~ADDRMASK);
	    if (modcommon & 1) modcommon -= 1;
	    if (ctsscommon)
	    {
	       if (modcommon == FAPCOMMONSTART)
		  break;
	       if ((common < 0) || (modcommon < common))
		  common = modcommon;
	       if (!commonorg)
	       {
		  setcommonorg = TRUE;
		  commonorg = modlength;
		  if (commonmod < 0)
		     commonmod = modcount;
	       }
#ifdef DEBUGLIBRARYLOADER
               printf ("   LIB FAPCOMMON: modcommon = %o, common = %o, "
	       	       "commonorg = %o, commontop = %o\n",
		       modcommon, common, commonorg, commontop);
#endif
	    }
	    break;

	 default: ; /* we ignore any other tags */
	 }
	 op += 12;
      }
   }
   if (setcommonorg)
      curraddr += commontop - common + 1;
   if (curraddr & 1) curraddr += 1;
   return (curraddr);
}

/************************************************************************
* libcheck - Check if file is referenced.
************************************************************************/

static int
libcheck (FILE *fd, char *name)
{
   int status;
   int binarymode = FALSE;
   int reclen;
   int relo;
   int done;
   int i;
   int wordlen = WORDTAGLEN-1;
   unsigned char inbuf[82];

#ifdef DEBUGLIBRARYCHECK
   printf ("libcheck: module = %s\n", name);
#endif

   status = 0;

   done = FALSE;
   while (!done)
   {
      char *bp;
      unsigned char *op = inbuf;

      if (fgets ((char *)inbuf, 82, fd) == NULL)
      {
	 done = TRUE;
	 break;
      }
      reclen = strlen ((char *)inbuf);

#ifdef DEBUGLIBRARYCHECK1
      printf ("reclen = %d\n", reclen);
#endif
      if (*op == IBSYSSYM)
      {
	 done = TRUE;
	 break;
      }

      for (i = 0; i < CHARSPERREC; i += WORDTAGLEN)
      {
	 t_uint64 ldata;
	 t_uint64 dectmp, addtmp;
	 char otag;
	 char item[16];

	 otag = *op++;
	 if (otag == ' ') break;
	 strncpy (item, op, 12);
	 item[12] = '\0';
	 bp = item;


	 relo = FALSE;
	 switch (otag)
	 {
	 case RELGLOBAL_TAG:
	    relo = TRUE;
	 case ABSGLOBAL_TAG:
	    item[6] = '\0';
	    for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
#ifdef DEBUGLIBRARYCHECK
	    printf ("   otag = %c, item = %s",
		    otag, item);
#endif
	    status += checkdef (item, relo, 0);
#ifdef DEBUGLIBRARYCHECK
            fputc ('\n', stdout);
#endif
	    break;

	 default: ;
	 }

	 op += 12;
      }
   }

   return (status);

}

/************************************************************************
* lnklibrary - Object library loader for lnk990.
************************************************************************/

int
lnklibrary (DIR *dirfd, int loadpt, int pass, char *lib)
{
   int more;
   int pcstart;
   int curraddr;
   int curmodnum;
   struct dirent *entdir;

   curraddr = loadpt;
#ifdef DEBUGLIBRARY
   printf ("lnklibrary: loadpt = %05o, lib = %s\n", loadpt, lib);
#endif

   /*
   ** Process the entire library directory
   */

   more = TRUE;
   while (more)
   {
      more = FALSE;
      while ((entdir = readdir (dirfd)) != NULL)
      {
	 FILE *modfd;
	 char *cp;
	 char fullname[256];
	 char modname[256];

	 strcpy (modname, entdir->d_name);
	 if (!strcmp (modname, ".") || !strcmp (modname, ".."))
	    continue;
	 cp = strrchr (modname, '.');
	 if (cp == NULL)
	    continue;
	 else
	 {
	    if (strcmp (cp+1, "obj"))
	       continue;
	 }

	 /*
	 ** Got an object file, process it.
	 */

	 sprintf (fullname, "%s/%s", lib, modname);
	 if ((modfd = fopen (fullname, "r")) == NULL)
	 {
	    sprintf (modname, "lnk7090: lnklibrary: %s", fullname);
	    perror (modname);
	    continue;
	 }
#ifdef DEBUGLIBRARY
	 printf ("   module = %s\n", fullname);
#endif

	 pcstart = curraddr;

	 /*
	 ** Check if current module has ENTRYs that are needed for an
	 ** unresolved reference.
	 */

	 if (libcheck (modfd, fullname))
	 {
	    /*
	    ** Yes, load module.
	    */

	    more = TRUE;
	    curmodnum = modnumber = modcount;
	    fseek (modfd, 0, SEEK_SET);
	    libloader (modfd, pcstart, 1, fullname);
	    fseek (modfd, 0, SEEK_SET);
	    modnumber = curmodnum;
	    curraddr = libloader (modfd, pcstart, 2, fullname);
	 }
	 fclose (modfd);
      }
      rewinddir (dirfd);
   }

   pc = curraddr;
   return (0);
}
