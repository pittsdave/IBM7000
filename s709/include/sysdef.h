/***********************************************************************
*
* sysdef.h - IBM 7090 emulator system definitions.
*
* Changes:
*   01/28/05   DGP  Original.
*	
***********************************************************************/

#define ERRVIEWLINENUM	50
#define ERRVIEWLINELEN	80

#define PRTVIEWLINENUM	100
#define PRTVIEWLINELEN	80

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if defined(WIN32) && !defined(MINGW)

#define EXPMASK		0377000000000I64

#define DFRACMASK	00777777777777777777I64
#define DNRMMASK	00400000000000000000I64
#define DCRYMASK	01000000000000000000I64

#else

#define EXPMASK		0377000000000ULL

#define DFRACMASK	00777777777777777777ULL
#define DNRMMASK	00400000000000000000ULL
#define DCRYMASK	01000000000000000000ULL

#endif

#define EXPSHIFT 	27
#define FRACSHIFT	27
#define FRACMASK	0000777777777
#define CRYMASK		0001000000000
#define NRMMASK		0000400000000
#define HIADD		0001000000000


#define int8            char
#define int16           short
#define int32           int
typedef int             t_stat;                         /* status */
typedef int             t_bool;                         /* boolean */
typedef unsigned int8   uint8;
typedef unsigned int16  uint16;
typedef unsigned int32  uint32, t_addr;                 /* address */

#if defined (WIN32)                                     /* Windows */
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
 
/*
** Define things specific to the system environment
*/

#if defined(VAXVMS)
#if defined(TGVTCP)
#define ERRNO		socket_errno
#define CLOSE		socket_close
#define READ		socket_read
#define WRITE		socket_write
#define PERROR		socket_perror
#endif /* TGVTCP */
#if defined(UCXTCP)
/*
** Select uses bit masks of file descriptors in longs.
** These macros manipulate such bit fields (the filesystem macros use
** chars). FD_SETSIZE may be defined by the user, but the default here
** should be >= NOFILE (param.h).
*/
#ifndef FD_SETSIZE
#define FD_SETSIZE      256
#endif
#ifndef NBBY
#define NBBY    8
#endif

#define NFDBITS (sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define howmany(x, y)   (((x)+((y)-1))/(y))
#endif

#if defined(VMSV5) || defined(VMSV6)
typedef long    fd_mask;

typedef struct fd_set
{
   fd_mask         fds_bits[howmany (FD_SETSIZE, NFDBITS)];
}
fd_set;
#endif /* VMSV5 || VMSV6 */

#define  FD_SET(n, p)   ((p)->fds_bits[((unsigned)n)/NFDBITS] \
                        |= (1 << (((unsigned)n) % NFDBITS)))
#define  FD_CLR(n, p)   ((p)->fds_bits[((unsigned)n)/NFDBITS] \
                        &= ~(1 << (((unsigned)n) % NFDBITS)))
#define  FD_ISSET(n, p) ((p)->fds_bits[((unsigned)n)/NFDBITS] \
                        & (1 << (((unsigned)n) % NFDBITS)))
#define FD_ZERO(p) memset((char *)(p), '\0', sizeof(*(p)))

#define APPTCPBLOCKMODE	0
#endif /* UCXTCP */
#endif /* VAXVMS */

#if defined(OS2)
#define ERRNO		sock_errno()
#define CLOSE		soclose
#define PERROR		psock_errno
#endif /* OS2 */

#if defined(WIN32) 
#include <winsock.h>

#define CLOSE		closesocket
#define IOCTL(a,b,c,d)	ioctlsocket((a),(b),(unsigned long *)(c))
#define ERRNO		WSAGetLastError()
#define PERROR(m)	fprintf (stderr, "%s: errno = %d\n", (m), ERRNO);
typedef unsigned long	caddr_t;
#ifndef ETIMEDOUT
#define ETIMEDOUT	WSAETIMEDOUT
#endif
#ifndef EADDRINUSE
#define EADDRINUSE	WSAEADDRINUSE
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED	WSAECONNREFUSED
#endif
#ifndef EINPROGRESS
#define EINPROGRESS	WSAEWOULDBLOCK
#endif
#ifndef ENOTCONN
#define ENOTCONN	WSAENOTCONN
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED	WSAECONNREFUSED
#endif
#ifndef ENOBUFS
#define ENOBUFS	        WSAENOBUFS
#endif
#endif /* WIN32  */

#if defined(OS390)
#define PERROR		tcperror
#define IOCTL(a,b,c,d)	ioctl((a),(b),(char *)(c))
#endif /* OS390 */

#if defined(HPUXV9)
#define SELECT(a,b,c,d,e) select((a),(int *)(b),(int *)(c),(int *)(d),(e))
#endif /* HPUX */

#if defined(RISCOS)
extern int errno;
#endif /* RISCOS */

/*
** Define things left undefined above
*/

#ifndef ERRNO
#define ERRNO		errno
#endif
#ifndef CLOSE
#define CLOSE		close
#endif
#ifndef READ
#define READ		read
#endif
#ifndef WRITE
#define WRITE		write
#endif
#ifndef PERROR
#define PERROR		perror
#endif
#ifndef SELECT
#define SELECT		select
#endif
#ifndef IOCTL
#define IOCTL		ioctl
#endif
