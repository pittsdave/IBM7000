/***********************************************************************
*
* sysdef.h - System defs.
*
* Changes:
*   01/20/05   DGP   Original.
*	
***********************************************************************/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*
** Misc constants
*/

#define IBSYSSYM '$'				/* IBSYS control card */

#define MEMSIZE 32768				/* Size of memory */
#define MAXREC 32768				/* Max record length */
#define MAXRECSIZE 196610			/* The size of a tape record */

#define EXPSHIFT  27
#define OPSHIFT   24
#define DECRSHIFT 18
#define FLAGSHIFT 18
#define TAGSHIFT  15

#define int8            char
#define int16           short
#define int32           int
typedef int             t_stat;                         /* status */
typedef int             t_bool;                         /* boolean */
typedef unsigned int8   uint8;
typedef unsigned int16  uint16;
typedef unsigned int32  uint32, t_addr;                 /* address */

#if defined(WIN32)                                      /* Windows */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (VMS)                /* Alpha VMS */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (__unix__)           /* Alpha UNIX */
#define t_int64 long
#else                                                   /* default GCC */
#define t_int64 long long
#endif                                                  /* end OS's */
typedef unsigned t_int64        t_uint64, t_value;      /* value */
typedef t_int64                 t_svalue;               /* signed value */
 
