/***********************************************************************
*
* regs.h - IBM 7090 emulator Register and Channel definitions.
*
* Changes:
*   ??/??/??   PRP   Original. Version 1.0.0
*   01/20/05   DGP   Changes for correct channel operation.
*   01/28/05   DGP   Revamped channel and tape controls.
*   05/19/05   DGP   Added DASD devices.
*   01/03/06   DGP   Added alternate BCD support.
*   02/29/08   DGP   Changed to use bit mask flags.
*   03/11/10   DGP   Added CHAN_IOCX flag.
*   03/11/10   DGP   Added COMM support.c
*   03/22/11   DGP   Added CPU_PROTMODE and CPU_RELOMODE support.
*	
***********************************************************************/

#define VERSION		"2.4.0"

#define NUMCHAN		4	/* Maximum current channels on a system */
#define MAXCHAN		8	/* Maximum channels on a system */
#define MAXTAPE		10	/* Maximum tapes on a channel */
#define MAXDASD		10	/* Maximum DASD devices on a channel */
#define MAXCOMM		36	/* Maximum COMM lines on a channel */
#define FIRSTTTY	4	/* First TTY line on a channel */

#define NEXTLIGHTS	300000 /*83333*/

#define MEMLIMIT	32767
#define MAXMEM		MEMLIMIT*2

#define MAXTRACKLEN	12300	/* Maximum track length */
#define MAXRECLEN	196610	/* Maximum tape record size */

#define STD_TRAP	000
#define CLOCK_TRAP      006
#define FP_TRAP		010
#define CHAN_TRAP	012
#define PROT_TRAP	032

#define BDATA_TRAP	020000
#define BINST_TRAP	040000

#define CLOCK		005

#define SPILL_MQ	0001
#define SPILL_AC	0002
#define SPILL_OV	0004
#define SPILL_SP	0010
#define SPILL_DV	0020
#define SPILL_ODD	0040

#define CPU_STOP	0	/* Stop the CPU */
#define CPU_RUN		1	/* Normal running state */
#define CPU_EXEC	2	/* Execute input instruction ER */
#define CPU_CYCLE	3	/* Single cycle CPU */

#define CHAN_IDLE	0
#define CHAN_RUN	1
#define CHAN_LOAD	2
#define CHAN_LOADDATA	3
#define CHAN_WAIT	4
#define CHAN_END        6
#define CHAN_UNSTACK    7

#define NOT_SEL		0
#define CMD_SEL		1
#define READ_SEL	2
#define WRITE_SEL	3
#define BSR_SEL 	4
#define BSF_SEL 	5
#define WEF_SEL 	6
#define REW_SEL		7
#define RUN_SEL		8
#define SDN_SEL		9
#define SNS_SEL		10
#define CHAN_SEL        11
#define WAIT_SEL        12

#define TAPEIDLE	0
#define TAPEREAD	1
#define TAPEWRITE	2

#define DASDIDLE	0
#define DASDREAD	1
#define DASDWRITE	2

#define TAGSHIFT	15
#define DECRSHIFT	18
#define OPSHIFT		24
#define PRESHIFT	33


#ifdef USE64

#if defined(WIN32) && !defined(MINGW)

#define ACSIGN		002000000000000I64
#define Q		001000000000000I64
#define P		000400000000000I64
#define BIT1		000200000000000I64
#define BIT2		000100000000000I64
#define MAGMASK		000377777777777I64
#define DATAMASK	000777777777777I64

#define SIGN		000400000000000I64
#define PREMASK		000700000000000I64
#define OPMASK		000377700000000I64
#define DECRMASK	000077777000000I64
#define FLAGMASK	000000060000000I64
#define TAGMASK		000000000700000I64
#define IBITMASK	000000000400000I64
#define NBITMASK	000000000200000I64
#define ADDRMASK	000000000077777I64
#define BCORE		000000000100000I64

#define CVRMASK		000000377000000I64
#define VOPMASK		000000077000000I64
#define ENBMASK		000000377000377I64

#define CHARHMSK	000770000000000I64
#define CHARLMSK	000000000000077I64
#define CHAR5MSK	000007777777777I64

#else

#define ACSIGN		002000000000000ULL
#define Q		001000000000000ULL
#define P		000400000000000ULL
#define BIT1		000200000000000ULL
#define BIT2		000100000000000ULL
#define MAGMASK		000377777777777ULL
#define DATAMASK	000777777777777ULL

#define SIGN		000400000000000ULL
#define PREMASK		000700000000000ULL
#define OPMASK		000377700000000ULL
#define DECRMASK	000077777000000ULL
#define FLAGMASK	000000060000000ULL
#define TAGMASK		000000000700000ULL
#define IBITMASK	000000000400000ULL
#define NBITMASK	000000000200000ULL
#define ADDRMASK	000000000077777ULL
#define BCORE		000000000100000ULL

#define CVRMASK		000000377000000ULL
#define VOPMASK		000000077000000ULL
#define ENBMASK		000000377000377ULL

#define CHARHMSK	000770000000000ULL
#define CHARLMSK	000000000000077ULL
#define CHAR5MSK	000007777777777ULL

#endif

EXTERN t_uint64 mem[MAXMEM+2];

EXTERN t_uint64 sr; 
EXTERN t_uint64 sr2;
EXTERN t_uint64 ac;
EXTERN t_uint64 mq;
EXTERN t_uint64 si;
EXTERN t_uint64 ky;
EXTERN t_uint64 adder;
EXTERN t_uint64 ilr;
EXTERN t_uint64 dsr;

#define ACCESS(a) access1(a)
#define ACCESS2(a) access2(a)

#define LOAD(a) load(a,1)
#define LOADA(a) load(a,0)
#define STORE(a,v) store(a,v,1)
#define STOREA(a,v) store(a,v,0)

#else /* !USE64 */

#define SIGN		0200
#define QCARRY		0040
#define Q		0020
#define P		0010
#define BIT1		0004
#define BIT2		0002
#define HMSK		0007

#define OPMASK		037700000000
#define DECRMASK	037777000000
#define FLAGMASK	000060000000
#define TAGMASK		000000700000
#define IBITMASK	000000400000
#define NBITMASK	000000200000
#define ADDRMASK	000000077777
#define BCORE		000000100000

#define CVRMASK		000377000000
#define VOPMASK		000077000000
#define ENBMASK		000377000377

#define CHARLMSK	000000000077
#define BIT4		020000000000

EXTERN uint8  memh[MAXMEM+2];
EXTERN uint32 meml[MAXMEM+2];

EXTERN uint8 srh; EXTERN uint32 srl;
EXTERN uint8 sr2h; EXTERN uint32 sr2l;
EXTERN uint8 ach; EXTERN uint32 acl;
EXTERN uint8 mqh; EXTERN uint32 mql;
EXTERN uint8 sih; EXTERN uint32 sil;
EXTERN uint8 kyh; EXTERN uint32 kyl;
EXTERN uint8 adderh; EXTERN uint32 adderl;
EXTERN uint8 ilrh; EXTERN uint32 ilrl;
EXTERN uint8 dsrh; EXTERN uint32 dsrl;


#define ACCESS(a) access1(a)
#define ACCESS2(a) access2(a)

#define LOAD(a) load(a,1)
#define LOADA(a) load(a,0)
#define STORE(a,h,l) store(a,h,l,1)
#define STOREA(a,h,l) store(a,h,l,0)

#endif /* !USE64 */

EXTERN uint8 ssw;
EXTERN uint8 sl;
EXTERN uint16 ic;
EXTERN uint16 xr[8];
EXTERN uint32 dsra;

#define CPU_AUTO	0000001
#define CPU_PROGSTOP	0000002
#define CPU_TRAPMODE	0000004
#define CPU_MULTITAG	0000010
#define CPU_ACOFLOW	0000020
#define CPU_MQOFLOW	0000040
#define CPU_DIVCHK	0000100
#define CPU_IOCHK	0000200
#define CPU_MACHCHK	0000400
#define CPU_FPTRAP	0001000
#define CPU_CHTRAP	0002000
#define CPU_RWSEL	0004000
#define CPU_PROTTRAP	0010000
#define CPU_PROTMODE	0020000
#define CPU_RELOMODE	0040000
#define CPU_USERMODE	0100000

EXTERN uint32 cpuflags;
EXTERN uint8 run;
EXTERN uint8 atbreak;
EXTERN uint8 altbcd;
EXTERN uint8 chan_in_op;

EXTERN uint16 op;
EXTERN uint8 tag;
EXTERN uint8 flag;
EXTERN uint16 iaddr;
EXTERN uint16 idecr;
EXTERN uint16 y;
EXTERN uint8 shcnt;
EXTERN uint16 spill;

EXTERN int32 cpumode;
EXTERN int32 windowlen;
EXTERN uint32 bcoredata;
EXTERN uint32 bcoreinst;
EXTERN uint32 inst_count;
EXTERN uint32 cycle_count;
EXTERN uint32 next_steal;
EXTERN uint8 single_cycle;
EXTERN uint32 next_lights;
EXTERN uint32 prog_reloc;
EXTERN uint32 prog_base;
EXTERN uint32 prog_limit;
EXTERN uint32 trap_enb;
EXTERN int trap_inhibit;
EXTERN int numchan;

EXTERN char txtcpumode[32];


#define CYCLE() \
	cycle_count++; \
	if (cycle_count >= next_steal) steal_cycle();
#define DCYCLE() \
	if (cycle_count >= next_steal-1) steal_cycle(); \
	else { CYCLE() }


typedef struct 
{
   uint32 curloc;		/* Current location on tape */
   uint32 reclen;		/* Record length */
   uint32 flags;		/* flags */
   uint8  state;		/* Tape state: 0 idle, 1 read, 2 write */
   uint8  buf[MAXRECLEN];	/* Tape buffer */
} Tape_t;

#define DASD_INFORMAT	00000001
#define DASD_CTSSDRUM	00000002

typedef struct 
{
   uint32 curloc;		/* Current location on device */
   uint32 reclen;		/* Record length */
   uint32 cmd;			/* Disk command */
   uint32 access;		/* access field */
   uint32 module;		/* module field */
   uint32 unit;			/* Device unit */
   uint32 track;		/* track field */
   uint32 record;		/* record field */
   uint32 ha2id;		/* ha2 id field */
   uint32 flags;		/* flags */
   uint16 fcyl;			/* format cyl */
   uint16 fmod;			/* format module */
   uint8  state;		/* state: 0 idle, 1 read, 2 write */
   uint8  fbuf[MAXTRACKLEN/4];  /* format buffer */
   uint8  dbuf[MAXTRACKLEN];	/* data buffer */
} DASD_t;

#define COMM_RINGLEN 1024

typedef struct
{
   int fd;			/* Socket for this line */
   int ch;			/* Reverse map to the channel */
   int model;			/* Terminal model */
   int tty;			/* Our ttyNN number */
   int hangup;			/* Told to hang up */
   int noecho;			/* Told not to echo */
   int inescape;		/* In escape sequence */
   int esccnt;			/* Escape sequence count */
   int complcount;		/* Completion count */
   int idlecnt;			/* Line idle count */
   int complete;		/* Line input complete */
   int head, tail;		/* Ring buffer pointers */
   char who[80];		/* Who's connected/state */
   uint16 ring[COMM_RINGLEN];	/* Terminal ring buffer */
} COMM_t;

#define CHAN_7607	0
#define CHAN_7909	1
#define CHAN_7750	2

#define CHAN_CHECK	00000001
#define CHAN_EOF	00000002
#define CHAN_BOT	00000004
#define CHAN_EOT	00000010
#define CHAN_EOR	00000020
#define CHAN_TRAPPEND	00000040
#define CHAN_INTRPEND	00000100
#define CHAN_LOADPEND	00000200
#define CHAN_CTSSDRUM	00000400
#define CHAN_EIGHTBIT	00001000
#define CHAN_INWAIT	00002000
#define CHAN_PRINTCLK	00004000
#define CHAN_DXFER	00010000
#define CHAN_NDXFER	00020000
#define CHAN_IOCX	00040000
#define CHAN_SDXFER	00100000
#define CHAN_SNDXFER	00200000
#define CHAN_TSCAN	01000000 
#define CHAN_COMMUP	02000000 

#define CIND_IOCHK    01000
#define CIND_SEQCHK   00400
#define CIND_UNEND    00200
#define CIND_ATTN1    00100
#define CIND_ATTN2    00040
#define CIND_ADPCHK   00020
#define CIND_READ     00010
#define CIND_WRITE    00004
#define CIND_READS    00002
#define CIND_WRITES   00001
#define CIND_ERROR    (CIND_IOCHK | CIND_SEQCHK | CIND_UNEND | CIND_ADPCHK)
#define CIND_INTMASK  (CIND_ERROR | CIND_ATTN1 | CIND_ATTN2)

#define SMS_INHIBINT  001000
#define SMS_ENCONINT  000100
#define SMS_READBACK  000040
#define SMS_BCDMODE   000020
#define SMS_INHIBUE   000010
#define SMS_INHIBAT1  000004
#define SMS_INHIBAT2  000002
#define SMS_SELECT2   000001
#define SMS_MASK      000177

typedef struct 
{
   uint8  ctype;        /* Channel type, 0=7607, 1=7909, 2=7750 */
   uint8  cop;		/* Channel Operation */
   uint16 car;		/* Channel Address Register */
   uint16 cwr;		/* Channel Word Count */
   uint16 clr;		/* Channel Memory address */
#ifdef USE64
   t_uint64 cdr;	/* Channel Data Register */
   t_uint64 casr;	/* Channel Assembly Register */
   t_uint64 csns[3];	/* Sense words */
#else
   uint8  cdrh;		/* Channel Data Register */
   uint32 cdrl;		/* Channel Data Register */
   uint8  casrh;	/* Channel Assembly Register */
   uint32 casrl;	/* Channel Assembly Register */
   uint8  csnsh[3];	/* Sense words */
   uint32 csnsl[3];	/* Sense words */
#endif
   uint8  csdcmd;	/* Stacked data xfer command */
   uint16 csdunit;	/* Stacked data xfer unit */
   uint8  csndcmd;	/* Stacked non-data xfer command */
   uint16 csndunit;	/* Stacked non-data xfer unit */
   uint32 ccore;	/* Which core bank command */
   uint32 dcore;	/* Which core bank data */
   uint32 ccyc;		/* Channel Cycle */
   uint32 cmodstart;	/* Channel module start */
   uint32 cflags;	/* Channel flags */
   uint16 cunit;	/* Channel Unit */
   uint16 caccess;	/* Channel Access */
   uint16 cind;		/* Channel Status Indicator bits */
   uint16 csms;		/* Channel SMS bits */
   uint16 cloady;	/* Channel Load pending address */
   uint8  csel;		/* Channel Select Type */
   uint8  cccr;		/* Channel Control Counter Register */
   uint8  cact;		/* Channel Active */
   uint8  cdcmd;	/* Channel device command */
   uint8  clcmd;	/* Channel last device command */
   uint8  spracode;	/* Channel SPRA code */
   union {
      Tape_t tapes[MAXTAPE]; /* Tapes on this channel */
      DASD_t dasd[MAXDASD];  /* DASD Disks/Drums on this channel */
      COMM_t comlines[FIRSTTTY+MAXCOMM];  /* COMM lines */
   } devices;
} Channel_t;

EXTERN Channel_t channel[MAXCHAN];

