/***************************************************************************
*
* ASMDMEM - Asm Debug Memory services.
*
* Changes:
*      07/01/97   DGP   Original.
*
***************************************************************************/


#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

#include "asmdmem.h"

#define ALLOCMSGLEN 256
#define OBJCONTLEN 20

#ifdef DEBUGMALLOCS

static char indent_string[40] = "";
static char *pfuncmsg = NULL;
static int nest_level = 0;

static FILE *fhErrorLog = NULL;
static FILE *fhTrace = NULL;

/*
** Global variables
*/

char  ErrMsg[400];
int DebugLevel = TRACE_DEBUG_OPER;	/* runtime debug level can be
					** chgd externally */
int MemDebug = 1;
int MemObjLimit = MEMOBJLIM;
int MemAloLimit = MEMCONSUMELIM;

int nbr_mallocs = 0;
int nbr_frees = 0;

int size_mallocs = 0;
int size_frees = 0;

int MemObjId = 0;
int msg_ctr = 0;

/*
** array of ptrs to memory alloc objs
*/

DBGMEMALLOCOBJ *memobjs[DBGMEMARRAYSIZE];

#endif

static void
dbg_hexdump (void *ptr, int size, int offset)
{
#ifdef DEBUGMALLOCS
   int jjj;
   int iii;
   char *tp;
   char *cp;
   char PathName[BUFSIZ];

   sprintf (PathName,
	    "dbgtrace%05d%05d.dat",
	    getpid(),
	    0);

   fhTrace = fopen (PathName,
		    "a");

   for (iii = 0, tp = (char *)(ptr), cp = (char *)(ptr); iii < (size); )
   {
      fprintf (fhTrace, "%08X  ", iii+offset);
      for (jjj = 0; jjj < 8; jjj++)
      {
	 if (cp < ((char *)(ptr)+(size)))
	 {
	    fprintf (fhTrace, "%02.2X", *cp++ & 0xFF);
	    if (cp < ((char *)(ptr)+(size)))
	    {
	       fprintf (fhTrace, "%02.2X ", *cp++ & 0xFF);
	    }
	    else
	    {
	       fprintf (fhTrace, "   ");
	    }
	 }
	 else
	 {
	    fprintf (fhTrace, "     ");
	 }
	 iii += 2;
      }
      fprintf (fhTrace, "   ");
      for (jjj = 0; jjj < 8; jjj++)
      {
	 if (tp < ((char *)(ptr)+(size)))
	 {
	    if (isprint(*tp))
	       fprintf (fhTrace, "%c", *tp);
	    else
	       fprintf (fhTrace, ".");
	    tp++;
	    if (tp < ((char *)(ptr)+(size)))
	    {
	       if (isprint(*tp))
		  fprintf (fhTrace, "%c ", *tp);
	       else
		  fprintf (fhTrace, ". ");
	       tp++;
	    }
	    else
	    {
	       fprintf (fhTrace, "  ");
	    }
	 }
	 else
	 {
	    fprintf (fhTrace, "   ");
	 }
      }
      fprintf (fhTrace, "\n");
   }

   fclose (fhTrace);
#endif
}

/***************************************************************************
*
* dbg_memcheck - Checks any unfreed blocks of memory for eycatcher corruption.
*
*   Args:
*      None.
*
*   Returns:
*      None.
*
***************************************************************************/

void
dbg_memcheck (void)
{
#ifdef DEBUGMALLOCS
   void  *memptr;
   DBGMEMALLOCOBJ *dbgmemobj;
   int   dbgvalue;
   int   i;
   int   pid, tid;
   int   real_len;
   int   memlen;
   char  AllocMsg[ALLOCMSGLEN + 1];

   DBG_FUNC_ENTRY (TRACE_IMPORT_TRIV)

   pid = getpid();
   tid = 0;
   for (i = 0; i < MEMOBJLIM; i++)
   {
      if (memobjs[i] != NULL)
      {
	 dbgmemobj = memobjs[i];
	 if (pid == dbgmemobj->memwrapper.Pid &&
	     tid == dbgmemobj->memwrapper.Tid)
	 {
	    memlen = dbgmemobj->memwrapper.MemoryLength;
	    if (memcmp (dbgmemobj->memwrapper.eyecatcher, EYECATCHER, 8))
	    {
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "HEADER EYECATCHER DAMAGED",
			  "dbg_memcheck")
	       sprintf (AllocMsg,
			"MemoryObj @(%p), size(%d), by %s:%d",
			dbgmemobj,
			memlen,
			dbgmemobj->memwrapper.FuncName,
			dbgmemobj->memwrapper.LineNum);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  AllocMsg,
			  "dbg_memcheck")

	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Header Dump",
			  "dbg_memcheck")
	       dbg_hexdump (dbgmemobj, sizeof (DBGMEMWRAPPER), 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "User Data Dump",
			  "dbg_memcheck")
	       dbg_hexdump (dbgmemobj->usermemory, memlen, 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Trailer Dump",
			  "dbg_memcheck")
	       dbg_hexdump (dbgmemobj->usermemory+memlen, 8, 0);
	    }
	    if (memcmp (dbgmemobj->usermemory+memlen, EYECATCHER, 8))
	    {
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "TRAILER EYECATCHER DAMAGED",
			  "dbg_memcheck")
	       sprintf (AllocMsg,
			"MemoryObj @(%p), size(%d), by %s:%d",
			dbgmemobj,
			memlen,
			dbgmemobj->memwrapper.FuncName,
			dbgmemobj->memwrapper.LineNum);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  AllocMsg,
			  "dbg_memcheck")

	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Header Dump",
			  "dbg_memcheck")
	       dbg_hexdump (dbgmemobj, sizeof (DBGMEMWRAPPER), 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "User Data Dump",
			  "dbg_memcheck")
	       dbg_hexdump (dbgmemobj->usermemory, memlen, 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Trailer Dump",
			  "dbg_memcheck")
	       dbg_hexdump (dbgmemobj->usermemory+memlen, 8, 0);
	    }
	 }
      }

   } /* endfor */

#endif
}

/***************************************************************************
*
* dbg_memdump - Dumps any unfreed blocks of memory.
*
*   Args:
*      None.
*
*   Returns:
*      None.
*
***************************************************************************/

void
dbg_memdump (void)
{
#ifdef DEBUGMALLOCS
   void  *memptr;
   DBGMEMALLOCOBJ *dbgmemobj;
   int   dbgvalue;
   int   i;
   int   pid, tid;
   int   real_len;
   int   memlen;
   char  AllocMsg[ALLOCMSGLEN + 1];
   char  ObjCont[OBJCONTLEN + 1];

   DBG_FUNC_ENTRY (TRACE_IMPORT_TRIV)
   DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
	      "Following is a list of allocated memory objects.",
	      "dbg_memdump")

   sprintf (AllocMsg,
	    "mallocs = %d, frees = %d, size_mallocs = %d, size_frees = %d",
	    nbr_mallocs, nbr_frees, size_mallocs, size_frees);
   DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
	      AllocMsg,
	      "dbg_memdump")

   pid = getpid();
   tid = 0;
   for (i = 0; i < MEMOBJLIM; i++)
   {
      if (memobjs[i] != NULL)
      {
	 dbgmemobj = memobjs[i];
	 if (pid == dbgmemobj->memwrapper.Pid &&
	     tid == dbgmemobj->memwrapper.Tid)
	 {
	    memlen = dbgmemobj->memwrapper.MemoryLength,
	    memset (ObjCont,
		    '\0',
		    OBJCONTLEN + 1);
	    memcpy ((char *) & ObjCont[0],
		    (char *) & dbgmemobj->usermemory[0],
		    OBJCONTLEN);
	    sprintf (AllocMsg,
		     "MemoryObj @(%p), size(%d), by %s:%d",
		     dbgmemobj,
		     memlen,
		     dbgmemobj->memwrapper.FuncName,
		     dbgmemobj->memwrapper.LineNum);
#if 0
	    if (isprint (ObjCont[0]))
	    {
	       sprintf (&AllocMsg[strlen(AllocMsg)],
			", usermemory(%*.*s).",
			OBJCONTLEN,
			OBJCONTLEN,
			(char *) & ObjCont[0]);
	    }
#endif
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       AllocMsg,
		       "dbg_memdump")
	    if (memcmp (dbgmemobj->memwrapper.eyecatcher, EYECATCHER, 8))
	    {
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "HEADER EYECATCHER DAMAGED",
			  "dbg_memdump")

	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Header Dump",
			  "dbg_memdump")
	       dbg_hexdump (dbgmemobj, sizeof (DBGMEMWRAPPER), 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "User Data Dump",
			  "dbg_memdump")
	       dbg_hexdump (dbgmemobj->usermemory, memlen, 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Trailer Dump",
			  "dbg_memdump")
	       dbg_hexdump (dbgmemobj->usermemory+memlen, 8, 0);
	    }
	    if (memcmp (dbgmemobj->usermemory+memlen, EYECATCHER, 8))
	    {
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "TRAILER EYECATCHER DAMAGED",
			  "dbg_memdump")

	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Header Dump",
			  "dbg_memdump")
	       dbg_hexdump (dbgmemobj, sizeof (DBGMEMWRAPPER), 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "User Data Dump",
			  "dbg_memdump")
	       dbg_hexdump (dbgmemobj->usermemory, memlen, 0);
	       DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
			  "Trailer Dump",
			  "dbg_memdump")
	       dbg_hexdump (dbgmemobj->usermemory+memlen, 8, 0);
	    }
	 }
      }

   } /* endfor */

#endif
}

/***************************************************************************
*
* debug_trace - Cuts trace entries, optionally display to stdout.
*
*   Args:
*      ImportanceLevel - Importance of this entry.
*      test - Pointer to text for this entry.
*      callfunc - The function that calling.
*
*   Returns:
*      None.
*
***************************************************************************/

void
debug_trace (int ImportanceLevel,
	     char * text,
	     char * callfunc)
{
#ifdef DEBUGMALLOCS
   int size;
   int i;
   char PathName[BUFSIZ];

   /* printf("debug_trace:%s",text); */
   /*-----------------------------------------------*/
   /* importance level must exceed 15-(debug level) */
   /*-----------------------------------------------*/

   if (ImportanceLevel <= (15 - DebugLevel))
   {
      return;
   }
   sprintf (PathName,
	    "dbgtrace%05d%05d.dat",
	    getpid(),
	    0);

   fhTrace = fopen (PathName,
		    "a");

   size = 25 + strlen (text) + strlen (indent_string);
   if (pfuncmsg == NULL)
   {
      pfuncmsg = malloc (size);
   }
   else
   {
      if (size > strlen (pfuncmsg))
      {
	 free (pfuncmsg);
	 pfuncmsg = malloc (size);
      }
   }

   sprintf (pfuncmsg,
	    "%-15.15s,(%d)%s%s\n",
	    callfunc,
	    nest_level,
	    indent_string,
	    text);

   size = strlen (pfuncmsg);

   fputs (pfuncmsg,
	  fhTrace);
   fclose (fhTrace);

   if (size > 75)
   {
      printf ("%-15.75s\n",
	      pfuncmsg);
   }
   else
   {
      printf ("%s",
	      pfuncmsg);
   }

   return;
#endif
}


/***************************************************************************
*
* debug_prompt
*
*   Args:
*      text - Prompt text.
*
*   Returns:
*      None.
*
***************************************************************************/

void
debug_prompt (char * text)
{
#ifdef DEBUGMALLOCS
   int  i;
   char prompt_text[100];

   sprintf (prompt_text,
	    "Prompt(Hit Enter)-%.80s",
	    text);

   debug_trace (TRACE_IMPORT_CRIT,
		prompt_text,
		"");
   i = getchar ();
   return;
#endif
}

/***************************************************************************
*
* func_trace function entry/exit trace
*
*   Args:
*      Importance Level - controls whether this func gets traces depending on
*                         current system debug level.
*      Bump - Indent/Deindent in trace
*      Funcname - of calling function
*      Ent - is this function entry or exit
*
*   Returns:
*      None.
*
***************************************************************************/

void
func_trace (int ImportanceLevel,
	    int bump,
	    char * funcname,
	    int ent)
{
#ifdef DEBUGMALLOCS
   int size;
   int i;

   /*
   ** No function tracing if TRACE_DEBUG_NONE
   */
   
   if (DebugLevel == TRACE_DEBUG_NONE)
   {
      return;
   }

   /*
   ** importance level must exceed 15-(debug level)
   */
   
   if (ImportanceLevel <= (15 - DebugLevel))
   {
      return;
   }

   if (ent == TRACE_FUNC_ENTRY)
   {
      if (bump == TRACE_BUMP)
      {
	 nest_level++;
	 if (nest_level <= 20)
	 {
	    strcat (indent_string,
		    "  ");
	 }
      }
      debug_trace (TRACE_IMPORT_CRIT,
		   "Function Entry",
		   funcname);
   }
   else if (ent == TRACE_FUNC_EXIT)
   {
      debug_trace (TRACE_IMPORT_CRIT,
		   "Function Exit",
		   funcname);
      if (bump == TRACE_DEBUMP)
      {
	 if (nest_level > 0)
	 {
	    nest_level--;
	 }
	 if (nest_level <= 20)
	 {
	    *(indent_string + (nest_level * 2)) = 0x00;
	 }
      }
   }
   return;
#endif
}
/***************************************************************************
*
* dbg_memget - common memory acquistion - storage wrappers, sentinels, etc
*
*   Args:
*      memlen - Length of memory to get.
*      funcname - name of calling function.
*      linenum - line in calling function.
*
*   Returns:
*      !NULL = Address of allocated memory.
*      =NULL = Error, No memory available.
*
***************************************************************************/

void *
dbg_memget (int memlen,
	    char * funcname,
	    int linenum)
{
#ifdef DEBUGMALLOCS
   void *memptr;
   DBGMEMALLOCOBJ *dbgmemobj;
   int dbgvalue;
   int i;
   int real_len;
   char AllocMsg[ALLOCMSGLEN + 1];

   DBG_FUNC_ENTRY (TRACE_IMPORT_TRIV)

   if (MemDebug)
   {

      if (nbr_mallocs == 0)
      {
	 for (i = 0; i < DBGMEMARRAYSIZE; i++)
	 {
	    memobjs[i] = NULL;
	 }
      }

      real_len = memlen + sizeof (DBGMEMWRAPPER) + 8;

      dbgmemobj = malloc (real_len);

      nbr_mallocs++;
      size_mallocs += memlen;

      memptr = &dbgmemobj->usermemory[0];

      memcpy (dbgmemobj->memwrapper.eyecatcher, EYECATCHER, 8);
      dbgmemobj->memwrapper.MemoryLength = memlen;
      dbgmemobj->memwrapper.MemoryId = MemObjId++;
      dbgmemobj->memwrapper.LineNum = linenum;
      dbgmemobj->memwrapper.Pid = getpid();
      dbgmemobj->memwrapper.Tid = 0;
      strcpy (dbgmemobj->memwrapper.FuncName,
	      funcname);
      memcpy (dbgmemobj->usermemory+memlen, EYECATCHER, 8);

      for (i = 0; i < DBGMEMARRAYSIZE; i++)
      {
	 if (memobjs[i] == NULL)
	 {
	    memobjs[i] = dbgmemobj;
	    break;
	 }
      }

      real_len = size_mallocs - size_frees;

      if ((i > MemObjLimit) || (real_len > MemAloLimit))
      {
	 DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		    "We have detected excessive memory usage.",
		    "dbg_memget")
	    dbg_memdump ();
	 DBG_FUNC_EXIT (TRACE_IMPORT_TRIV,
			NULL)

      }
   }
   else
   {
      memptr = malloc (memlen);
   }

   if (DebugLevel >= TRACE_DEBUG_ALL)
   {
      sprintf (AllocMsg,
	       "%d requested bytes allocated @ %p",
	       memlen,
	       memptr);
      DBG_TRACE (TRACE_IMPORT_TRIV,
		 AllocMsg,
		 "dbg_memget")
   }

   DBG_FUNC_EXIT (TRACE_IMPORT_TRIV,
		  memptr)
#else
   return (NULL);
#endif
}

/***************************************************************************
*
* dbg_memrel - common memory release - storage wrappers, sentinels, etc
*
*   Args:
*      memrel - address of memory to be released.
*
*   Returns:
*      None.
*
***************************************************************************/

void
dbg_memrel (void *memptr)
{
#ifdef DEBUGMALLOCS
   DBGMEMALLOCOBJ *dbgmemobj;
   int i;
   char DeallocMsg[60];

   if (MemDebug)
   {

      dbgmemobj = (DBGMEMALLOCOBJ *) ((char *) memptr -
				      sizeof (DBGMEMWRAPPER));

      for (i = 0; i < DBGMEMARRAYSIZE; i++)
      {
	 if (memobjs[i] == dbgmemobj)
	 {
	    memobjs[i] = NULL;
	    break;
	 }
      }

      if (i >= DBGMEMARRAYSIZE)
      {
	 sprintf (DeallocMsg,
		  "memory @ %p not in dbgmemarray",
		  memptr);
	 DBG_TRACE (TRACE_IMPORT_TRIV,
		    DeallocMsg,
		    "dbg_memrel")
      }
      else
      {
	 int memlen;
	 int real_len;
	 char  AllocMsg[ALLOCMSGLEN + 1];

	 memlen = dbgmemobj->memwrapper.MemoryLength;
	 if (memcmp (dbgmemobj->memwrapper.eyecatcher, EYECATCHER, 8))
	 {
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "HEADER EYECATCHER DAMAGED",
		       "dbg_memrel")
	    sprintf (AllocMsg,
		     "MemoryObj @(%p), size(%d), by %s:%d",
		     dbgmemobj,
		     memlen,
		     dbgmemobj->memwrapper.FuncName,
		     dbgmemobj->memwrapper.LineNum);
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       AllocMsg,
		       "dbg_memrel")

	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "Header Dump",
		       "dbg_memrel")
	    dbg_hexdump (dbgmemobj, sizeof (DBGMEMWRAPPER), 0);
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "User Data Dump",
		       "dbg_memrel")
	    dbg_hexdump (dbgmemobj->usermemory, memlen, 0);
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "Trailer Dump",
		       "dbg_memrel")
	    dbg_hexdump (dbgmemobj->usermemory+memlen, 8, 0);
	 }
	 if (memcmp (dbgmemobj->usermemory+memlen, EYECATCHER, 8))
	 {
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "TRAILER EYECATCHER DAMAGED",
		       "dbg_memrel")
	    sprintf (AllocMsg,
		     "MemoryObj @(%p), size(%d), by %s:%d",
		     dbgmemobj,
		     memlen,
		     dbgmemobj->memwrapper.FuncName,
		     dbgmemobj->memwrapper.LineNum);
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       AllocMsg,
		       "dbg_memrel")

	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "Header Dump",
		       "dbg_memrel")
	    dbg_hexdump (dbgmemobj, sizeof (DBGMEMWRAPPER), 0);
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "User Data Dump",
		       "dbg_memrel")
	    dbg_hexdump (dbgmemobj->usermemory, memlen, 0);
	    DBG_TRACE (TRACE_IMPORT_OPERATIONAL,
		       "Trailer Dump",
		       "dbg_memrel")
	    dbg_hexdump (dbgmemobj->usermemory+memlen, 8, 0);
	 }
	 size_frees += memlen;
	 free (dbgmemobj);
	 nbr_frees++;
      }
   }
   else
   {
      free (memptr);
   }

   if (DebugLevel >= TRACE_DEBUG_ALL)
   {
      sprintf (DeallocMsg,
	       "memory @ %p freed",
	       memptr);
      DBG_TRACE (TRACE_IMPORT_TRIV,
		 DeallocMsg,
		 "dbg_memrel")

   }

   DBG_VFUNC_EXIT (TRACE_IMPORT_TRIV)
#endif
}
