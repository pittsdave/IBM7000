/***********************************************************************
*
* io.h - IBM 7090 emulator I/O definitions.
*
* Changes:
*   ??/??/??   PP    Original.
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   05/18/05   DGP   Added DASD (Disk/Drum) channels.
*   02/29/08   DGP   Changed to use bit mask flags.
*   03/11/10   DGP   Added COMM support.
*   03/18/15   DGP   Added real tape support.
*	
***********************************************************************/

/*
 * I/O device numbering:
 *	0	Card reader (711 Channel A)
 *	1	Card punch  (721 Channel A)
 *	2	Printer     (716 Channel A)
 *	3-12	Tapes channel A
 *	13-22	Tapes channel B
 *	...and so on
 *      103-112 DASD channel A
 *      113-122 DASD channel B
 *	...and so on
 *      200-20N COMM Channel A
 *      210-21N COMM Channel B
 *	...and so on
 */

#define TAPEOFFSET   3
#define DASDOFFSET   103
#define COMMOFFSET   200
#define DASDOVERHEAD 16

#define IODEV (COMMOFFSET+10*MAXCHAN) //(DASDCHAN*100+10*MAXCHAN+TAPEOFFSET)
#define IO_MAXNAME 73

#define IO_COMMFD (FILE*)1

#define IO_READ  1
#define IO_WRITE 2
#define IO_RDWRT 3

#define TAPESIZE 16000000

#define IO_TAPE		000001
#define IO_DASD		000002
#define IO_DISK		000004
#define IO_ALTBCD	000010
#define IO_SIMH		000020
#define IO_CTSSDRUM	000040
#define IO_PRINTCLK	000100
#define IO_COMM		000200
#define IO_ATEOF        000400
#define IO_REALDEV      001000

typedef struct
{
   FILE *iofd;
   int  iochn;			/* Channel to which we belong */
   int  ioflags;		/* Device flags */
   int  iomodel;		/* DASD model, 0 = 1301-1, 1 = 1301-2, ... */
   				/* COMM model, 1 = KSR-35, 2 = 1050 ... */
   int  iochntype;		/* Channel type, 2 = 7750, 1 = 7909, 0 = 7607 */
   int  iocyls;			/* DASD cylinders */
   int  ioaccess;		/* DASD access */
   int  iomodules;		/* DASD modules */
   int  ioheads;		/* DASD heads */
   int  iobyttrk;		/* DASD bytes/track */
   				/* TAPE last read/write record length */
   int  iomodstart;		/* DASD module start */
   				/* COMM first line */
   int  iomodend;		/* DASD module end */
   				/* COMM last line */
   int  iooverhead;		/* DASD track overhead */
   long iopos;			/* Current position on device */
   char iorw;			/* Device read/write flag */
   char iostr[IO_MAXNAME];
} IO_t;

EXTERN IO_t sysio[IODEV];

/* #define IODEBUG */

char *devstr (int );
int mount(char *);
int dismount(char *);
void listmount();
int readrec(int , uint8 *, int);
void bsr(int);
void bsf(int);
void writerec(int, uint8 *, int);
void dorewind(int);
void unload(int);
void wef(int);
void bincard(uint8 *, uint16 *);
void cardbin(uint16 *, uint8 *);
void cardbcd(uint16 *, uint8 *, int);
void logstr(char *, int);
void ioinit();
void iofin();
int isdrum(int);
