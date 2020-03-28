/************************************************************************
*
* lnkloader - Loads objects from asm7090 for linking.
*
* Changes:
*   05/21/03   DGP   Original.
*   12/28/04   DGP   New object tags.
*   02/14/05   DGP   Revamped operation to allow stacked objects and new
*                    link map listing format.
*   06/07/05   DGP   Added ABSORG support.
*   12/15/05   RMS   MINGW changes.
*   12/07/09   DGP   Fix address/decrement relocations.
*   03/24/10   DGP   Added dope vector code.
*   03/30/10   DGP   Added cross reference and wide listing.
*   06/10/10   DGP   Added COMMON support.
*   06/10/10   DGP   Added "MOVIE)" support.
*   08/10/10   DGP   Fixed COMMON.
*   09/15/10   DGP   Fixed COMMON DECR relo.
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

#include "lnkdef.h"

extern FILE *lstfd;

extern int listmode;
extern int pc;
extern int errcount;
extern int absentry;
extern int relentry;
extern int modcount;
extern int modnumber;
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


/***********************************************************************
* chkxref - If XREF mode, add reference.
***********************************************************************/

void
chkxref (SymNode *s, int refaddr)
{
   if (genxref)
   {
      XrefNode *newref;

      if ((newref =
	  (XrefNode *) malloc (sizeof (XrefNode))) == NULL)
      {
	 fprintf (stderr,
		  "lnk7090: Unable to allocate memory\n");
	 exit (ABORT);
      }
      memset (newref, 0, sizeof (XrefNode));
      newref->next = NULL;
      newref->modnum = memory[refaddr].modnum;
      newref->value = refaddr;
      if (s->xref_head == NULL)
	 s->xref_head = newref;
      else
	 (s->xref_tail)->next = newref;
      s->xref_tail = newref;
   }
}


/***********************************************************************
* lnkloader - Load modules into memory.
***********************************************************************/

int
lnkloader (FILE *fd, int loadpt, int pass, char *file)
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

#ifdef DEBUGLOADER
   printf ("lnkloader: loadpt = %5.5o, pass = %d, file = %s\n",
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
#ifdef DEBUGLOADER
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


#ifdef DEBUGLOADER
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
#ifdef DEBUGLOADER
            printf ("IDT = %s\n", item);
#endif
	    strcpy (module, item);
	    modlength = (int)ldata;
	    if (modlength & 1) modlength += 1;
	    if (pass == 1)
	    {
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
#ifdef DEBUGLOADER
	       printf ("   COMMON ORG: common = %o, commonorg = %o, val = %o, "
	               "curraddr = %o, commontop = %o\n",
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
#ifdef DEBUGLOADER
	       printf ("   COMMON ADDR: common = %o, commonorg = %o, val = %o, "
	       	       "addtmp = %llo\n",
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
	    if (ctsscommon && ((int)((ldata & ~DECRMASK) >> 18) >= modcommon))
	    {
	       dectmp = (commonorg + (((ldata & ~DECRMASK) >> 18) - common)
	       		& ~ADDRMASK) << 18;
#ifdef DEBUGLOADER
	       printf ("   COMMON DECR: common = %o, commonorg = %o, val = %o, "
	       	       "dectmp = %llo\n",
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
#ifdef DEBUGLOADER
	       printf ("   COMMON ADDR: common = %o, commonorg = %o, val = %o, "
	       	       "addtmp = %llo\n",
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
	    if (ctsscommon && ((int)((ldata & ~DECRMASK) >> 18) >= modcommon))
	    {
	       dectmp = (commonorg + (((ldata & ~DECRMASK) >> 18) - common)
	       		& ~ADDRMASK) << 18;
#ifdef DEBUGLOADER
	       printf ("   COMMON DECR: common = %o, commonorg = %o, val = %o, "
	       	       "dectmp = %llo\n",
		       common, commonorg, (int)((ldata & ~DECRMASK) >> 18),
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
#ifdef DEBUGLOADER
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
		  s->modnum = modnumber+1;
		  s->value = (int)((ldata + loadpt) & ~ADDRMASK);
#ifdef DEBUGLOADER
		  printf ("lnk7090: Symbol %s undefined, value = %05o\n",
		  	  item, s->value);
#endif
	          status = -1;
	       }
	       else
	       {
		  int refaddr = (int)((ldata & ~ADDRMASK) + loadpt);

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
#ifdef DEBUGLOADER
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
	       if (!(s = symlookup (item, module, FALSE)))
	       {
		  if (s = symlookup (item, module, TRUE))
		  {
		     s->value = (int)((ldata & ~ADDRMASK) + loadpt);
		     s->global = TRUE;
		     s->relocatable = TRUE;
		  }
	       }
	       else
	       {
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
#ifdef DEBUGLOADER
               printf ("   FAPCOMMON: modcommon = %o, common = %o, "
	       	       "commonorg = %o, commontop = %o\n",
		       modcommon, common, commonorg, commontop);
#endif
	    }
	    break;

	 default: ; /* we ignore any other tags */
	 }
	 op += 12;
      }
      pc = curraddr;
      if (setcommonorg)
         pc += commontop - common + 2;
      if (pc & 1) pc += 1;
   }
   return (status);
}
