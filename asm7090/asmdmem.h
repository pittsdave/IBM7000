/***************************************************************************
*
* asmdmem.h - Asm Debug memory header.
*
* Changes:
*      07/02/97   DGP   Original
*
***************************************************************************/

#ifndef __FUNCTION__
#define __FUNCTION__ __FILE__
#endif

#define TRACE_FUNC_ENTRY   1
#define TRACE_FUNC_EXIT    0

#define TRACE_NOBUMP       0
#define TRACE_BUMP         1
#define TRACE_DEBUMP       2


#define TRACE_IMPORT_NONE         0
#define TRACE_IMPORT_CRIT        15
#define TRACE_IMPORT_OPERATIONAL 15
#define TRACE_IMPORT_IMPRT       13
#define TRACE_IMPORT_SGNF        11
#define TRACE_IMPORT_NOTBL        9
#define TRACE_IMPORT_TRIV         7

/*
**  Debug levels are defined as 15-x so that an event will be
**  traced only if the importance level exceeds the debug-level
*/

#define TRACE_DEBUG_NONE     0	/* 15 , exceeded by nothing  */
#define TRACE_DEBUG_OPER     1	/* 14 , exceeded by CRIT/OPER */
#define TRACE_DEBUG_LIGHT    3	/* 12 , exceeded by IMPRT    */
#define TRACE_DEBUG_MED      5	/* 10 , exceeded by SGNF     */
#define TRACE_DEBUG_HEAVY    7	/*  8 , exceeded by NOTBL    */
#define TRACE_DEBUG_ALL      9	/*  6 , exceeded by TRIV     */

#define DBGMEMARRAYSIZE  8000
#define MEMOBJLIM        8000
#define MEMCONSUMELIM    16000000

#define MAXFUNCNAMESIZE	 80

#define EYECATCHER "MEMDEBUG"


typedef struct
{
   char eyecatcher[8];
   int  begin_sentinal;
   int  MemoryLength;
   int  MemoryId;
   int  LineNum;
   int  Pid;
   int  Tid;
   char FuncName[MAXFUNCNAMESIZE];
}
DBGMEMWRAPPER;

typedef struct
{
   DBGMEMWRAPPER memwrapper;
   char usermemory[1];
}
DBGMEMALLOCOBJ;

extern int DebugLevel;
extern int MemDebug;
extern int MemObjLimit;
extern int MemAloLimit;

extern void     dbg_memdump (void);
extern void     dbg_memcheck (void);
extern void     debug_prompt (char * promptmsg);
extern void     func_trace (int trace_lvl,
			    int bump,
			    char * funcname,
			    int ent);
extern void     debug_trace (int level,
			     char * text,
			     char * callfunc);
extern void    *dbg_memget (int len,
			    char * funcname,
			    int linenum);
extern void     dbg_memrel (void * pmem);


/*
** Macro to handle entry into 'C' functions .
** Macro to handle completion of the function.
*/

#ifdef DEBUGMALLOCS
#define DBG_FUNC_ENTRY(TraceImportance)  \
   func_trace(TraceImportance,TRACE_BUMP,__FUNCTION__,TRACE_FUNC_ENTRY);
#else
#define DBG_FUNC_ENTRY(TraceImportance)
#endif


/*
** Macro to exit from 'C' functions  with void type.
** Macro to handle completion of the function.
*/

#ifdef DEBUGMALLOCS
#define DBG_VFUNC_EXIT(TraceImportance)  \
   func_trace(TraceImportance,TRACE_DEBUMP,__FUNCTION__,TRACE_FUNC_EXIT); \
   return ;
#else
#define DBG_VFUNC_EXIT(TraceImportance)  \
   return ;
#endif


/*
** Macro to exit from 'C' functions  with non-void type.
** Macro to handle completion of the function.
*/

#ifdef DEBUGMALLOCS
#define DBG_FUNC_EXIT(TraceImportance,RetVal)  \
   func_trace(TraceImportance,TRACE_DEBUMP,__FUNCTION__,TRACE_FUNC_EXIT); \
   return(RetVal) ;
#else
#define DBG_FUNC_EXIT(TraceImportance,RetVal) \
   return(RetVal) ;
#endif


/*
** Macro to call debug functions for memory allocation / release.
** Macro to handle completion of the function.
*/

#ifdef DEBUGMALLOCS
#define DBG_TRACE(IMPLVL,LOGDATA,FUNCNAME) \
           debug_trace(IMPLVL,LOGDATA,FUNCNAME) ;
#define DBG_MEMGET(MemLen)  dbg_memget(MemLen,__FUNCTION__,__LINE__)
#define DBG_MEMREL(MemPtr)  dbg_memrel(MemPtr)
#define DBG_MEMDUMP         dbg_memdump
#define DBG_MEMCHECK        dbg_memcheck
#else /* !DEBUGMALLOCS */
#define DBG_TRACE(IMPLVL,LOGDATA,FUNCNAME)
#define DBG_MEMGET(MemLen)  malloc(MemLen)
#define DBG_MEMREL(MemPtr)  free(MemPtr)
#define DBG_MEMDUMP         dbg_memdump
#define DBG_MEMCHECK        dbg_memcheck
#endif /* DEBUGMALLOCS */

