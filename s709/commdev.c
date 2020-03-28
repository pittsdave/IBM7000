/***********************************************************************
*
* commdev.c - Comm device support for the IBM 7090 simulator.
*
* Changes:
*   03/11/10   DGP   Original.
*   08/24/10   DGP   Added KSR37/KSR33 support.
*   03/25/11   DGP   Added CTRL-U and CTRL-BS support.
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <errno.h>

#if defined (WIN32)
#define __TTYROUTINES 0
#include <conio.h>
#include <windows.h>
#include <signal.h>
#include <process.h>
#include <winsock.h>
#define IAC 255
#define WILL 251
#define TELOPT_SGA 3
#define TELOPT_ECHO 1
#define strcasecmp strcmp
#endif

#if defined(UNIX)
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <pthread.h>
#endif

#define EXTERN extern

#include "sysdef.h"
#include "regs.h"
#include "chan7607.h"
#include "chan7909.h"
#include "io.h"
#include "screen.h"
#include "comm.h"

#define DEPTH 1
#define MAXIDLECNT 5

extern char errview[ERRVIEWLINENUM][ERRVIEWLINELEN+2];
extern comm_types comm_data[MAXCOMMMODEL];

static int ungetchar = -1;

static int chrcnt = 0;
static int chrndx = 0;
static uint16 lineid;
static int16 seqnum = 03777;
static int16 linechr[COMM_RINGLEN];
static uint8 linebuf[COMM_RINGLEN];

/***********************************************************************
* Nwrite - Network write.
***********************************************************************/

static int
Nwrite (int fd, uint8 *buf, int count)
{
#ifdef DEBUGCOMMTASKa
   fprintf (stderr, "Nwrite: fd = %d, count = %d, buf = '%s'\n",
	    fd, count, buf);
#endif
   while (count > 0)
   {
      int wcnt = 0;

      if (fd < 3)
      {
	 wcnt = write (fd, buf, count);
      }
      else
      {
	 wcnt = send (fd, buf, count, 0);
      }
      if (wcnt < 0)
      {
#ifdef DEBUGCOMMTASK
         PERROR ("Nwrite");
#endif
         return (-1);
      }
      buf += wcnt;
      count -= wcnt;
   }
   return (0);
}

/***********************************************************************
* Nread - Network read.
***********************************************************************/

static int
Nread (int fd, uint8 *buf, int count)
{
   int cnt;

   if (ungetchar > 0)
   {
      cnt = ungetchar;
      ungetchar = -1;
   }
   else
   {
      cnt = recv (fd, buf, count, 0);
#ifdef DEBUGCOMMTASK
      if (cnt < 0) PERROR ("Nread");
#endif
   }
   return (cnt);
}

/***********************************************************************
* commdoiac - IAC processor. We just eat them.
***********************************************************************/

static void
commdoiac (int fd)
{
   uint8 buf[2];

#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commdoiac: entered\n");
#endif
   if (Nread (fd, buf, 1) == 1)
   {
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "   c1 = %02X\n", buf[0]);
#endif
      if (Nread (fd, buf, 1) == 1)
      {
#ifdef DEBUGCOMMTASK
	 fprintf (stderr, "   c2 = %02X\n", buf[0]);
#endif
      }
   }
}

/***********************************************************************
* putring - Put char in the line ring buffer.
***********************************************************************/

static void
putring (COMM_t *line, uint16 ch)
{
   line->ring[line->tail++] = ch;
   if (line->tail == COMM_RINGLEN)
      line->tail = 0;
}

/***********************************************************************
* getring - Get a char from the line ring buffer.
***********************************************************************/

static uint16
getring (COMM_t *line)
{
   uint16 ch;

   ch = line->ring[line->head++];
   if (line->head == COMM_RINGLEN)
      line->head = 0;
   return ch;
}

/***********************************************************************
* senddialup - Send DIALUP and id string
***********************************************************************/

static int 
senddialup (COMM_t *line, int ch)
{
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "senddialup: sms = %o, flags = %o\n",
      channel[ch].csms, channel[ch].cflags);
#endif
   if (!channel[ch].csms && (channel[ch].cflags & CHAN_COMMUP))
   {
      putring (line, COM_DIALUP);
      putring (line, line->model);
      putring (line, 0);
      putring (line, 0);
      putring (line, 0);
      putring (line, COM_ENDID);
      channel[ch].cind |= CIND_ATTN1;
      line->complete = TRUE;
      if (!(channel[ch].csms & SMS_INHIBAT1))
	 channel[ch].cflags |= CHAN_INTRPEND;
      return TRUE;
   }
   return FALSE;
}

/***********************************************************************
* sendhangup - Send HANGUP
***********************************************************************/

static int 
sendhangup (COMM_t *line, int ch)
{
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "sendhangup: sms = %o, flags = %o\n",
      channel[ch].csms, channel[ch].cflags);
#endif
   if (!channel[ch].csms && (channel[ch].cflags & CHAN_COMMUP))
   {
      putring (line, COM_HANGUP);
      channel[ch].cind |= CIND_ATTN1;
      line->complete = TRUE;
      if (!(channel[ch].csms & SMS_INHIBAT1))
	 channel[ch].cflags |= CHAN_INTRPEND;
      return TRUE;
   }
   return FALSE;
}

/***********************************************************************
* commwork - COMM worker task
***********************************************************************/

static void
commwork (void *vline)
{
   COMM_t *line;
   int fd;
   int ch;
   int maxfd;
   int low_latency = 1;
   int sent_dialup;
   struct timeval tv;
   fd_set readfds;
   uint8 msg[82];
   uint8 buf[2];
   uint8 will[] = { IAC, WILL, '%', 'c', 0 };

   line = (COMM_t *)vline;
   fd = line->fd;
   ch = line->ch;
   channel[ch].cmodstart++;

#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commwork: starting: ch = %c, fd = %d, who = %s\n",
	    ch+'A', fd, line->who);
#endif

   /*
   ** Turn off piggybacking for low latency
   */

   if (setsockopt (fd, IPPROTO_TCP, TCP_NODELAY,
                   (char *) &low_latency, sizeof(low_latency)) < 0)
   {
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "commwork: setsockopt(), fd = %d, who = %s\n",
               fd, line->who);
#endif
      goto WORK_EXIT;
   }

   /*
   ** Tell the telnet client we're doing the work
   */

   sprintf ((char *)msg, (char *)will, TELOPT_SGA);
   Nwrite (fd, msg, 3);
   sprintf ((char *)msg, (char *)will, TELOPT_ECHO);
   Nwrite (fd, msg, 3);

   /*
   ** Display the introduction banner
   */

   sprintf ((char *)msg, "s709 %s COMM tty%d (%s) from %s\r\n",
   	    VERSION, line->tty, comm_data[line->model-1].model, line->who);
   Nwrite (fd, msg, strlen((char *)msg));

   sent_dialup = senddialup (line, ch);

   while (TRUE)
   {

      /*
      ** If told to hangup, disconnect
      */

      if (line->hangup)
         break;

      /*
      ** Await user input or timeout
      */

      FD_ZERO (&readfds);
      FD_SET (fd, &readfds);

      tv.tv_sec = 0;
      tv.tv_usec = 10000;
      if ((maxfd = select (fd+1, &readfds, NULL, NULL, &tv)) < 0)
      {
	 if (ERRNO != EINTR && ERRNO != EAGAIN)
	 {
	    PERROR ("commwork: select failed");
	    break;
	 }
      }

      /*
      ** If input data, process it
      */

      if (maxfd > 0)
      {
#ifdef DEBUGCOMMTASK
	 fprintf (stderr, "Select: fd = %d, maxfd = %d\n", fd, maxfd);
#endif
	 if (!sent_dialup)
	 {
	    sent_dialup = senddialup (line, ch);
	 }

         if (Nread (fd, buf, 1) == 1)
	 {
	    if (buf[0] == IAC)
	    {
	       commdoiac (fd);
	    }
	    else
	    {
#ifdef DEBUGCOMMTASK
	       fprintf (stderr, "Complete select: inchar = %02X(%c)\n",
			buf[0] & 0xFF, isprint(buf[0]) ? buf[0] : '.');
	       fprintf (stderr, "   cind = %o, sms = %o, echo = %d\n",
			channel[ch].cind, channel[ch].csms, !line->noecho);
#endif
	       if (!channel[ch].csms && (channel[ch].cflags & CHAN_COMMUP))
	       {
		  switch (buf[0])
		  {
		  case 000:
		     break;

		  case COM_CTRLC:
		     putring (line, COM_INTR);
		     channel[ch].cind |= CIND_ATTN1;
		     line->complete = TRUE;
		     if (!(channel[ch].csms & SMS_INHIBAT1))
			channel[ch].cflags |= CHAN_INTRPEND;
		     break;

		  case COM_CTRLBS:
		     putring (line, COM_QUIT);
		     channel[ch].cind |= CIND_ATTN1;
		     line->complete = TRUE;
		     if (!(channel[ch].csms & SMS_INHIBAT1))
			channel[ch].cflags |= CHAN_INTRPEND;
		     break;

		  case COM_CTRLU:
		     buf[0] = COM_BACKSPACE;
		     buf[1] = ' ';
		     buf[2] = COM_BACKSPACE;
		     while (line->tail != line->head)
		     {
			Nwrite (fd, buf, 3);
			if (line->tail > 0)
			   line->tail--;
			else
			   line->tail = COMM_RINGLEN;
		     }
		     break;

		  case COM_BACKSPACE:
		  case COM_DELETE:
		     if (line->tail != line->head)
		     {
			buf[0] = COM_BACKSPACE;
			buf[1] = ' ';
			buf[2] = COM_BACKSPACE;
			Nwrite (fd, buf, 3);
			if (line->tail > 0)
			   line->tail--;
			else
			   line->tail = COMM_RINGLEN;
		     }
		     else
		     {
			buf[0] = COM_BELL;
			Nwrite (fd, buf, 1);
		     }
		     break;

		  case '\r':
		     buf[1] = '\n';
		     if (line->model == COM_KSR37)
			putring (line, buf[1]);
		     putring (line, buf[0]);
		     if (!line->noecho)
			Nwrite (fd, buf, 2);
		     channel[ch].cind |= CIND_ATTN1;
		     line->complete = TRUE;
		     if (!(channel[ch].csms & SMS_INHIBAT1))
			channel[ch].cflags |= CHAN_INTRPEND;
		     break;

		  default:
		     if (!line->noecho)
			Nwrite (fd, buf, 1);
		     if (islower (buf[0]) && (line->model != COM_KSR37))
			buf[0] = toupper (buf[0]);
		     putring (line, buf[0]);
		     break;
		  }
	       }
	    }
	 }
	 else break;
      }

      /*
      ** If idle, check for queued completions.
      */

      else if (maxfd == 0 && line->idlecnt >= 0)
      {
         line->idlecnt++;
	 if ((line->idlecnt > MAXIDLECNT) && line->complcount &&
	      !(channel[ch].csms & SMS_INHIBAT1))
	 {
	    line->idlecnt = -1;
	    while (line->complcount)
	    {
	       int ccnt;

	       ccnt = (line->complcount > 037) ? 037 : line->complcount;
	       putring (line, COM_VALID | COM_COMPLETE | ccnt);
	       line->complcount -= ccnt;
	    }
	    channel[ch].cind |= CIND_ATTN1;
	    channel[ch].cflags |= CHAN_INTRPEND;
	    line->complete = TRUE;
	 }
      }
   }

WORK_EXIT:
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commwork: terminate: fd = %d, who = %s\n",
	    fd, line->who);
#endif

   if (!line->hangup)
      sendhangup (line, ch);

   CLOSE (fd);
   channel[ch].cmodstart--;
   line->fd = 0;
   line->hangup = FALSE;
   line->noecho = FALSE;
   line->inescape = FALSE;
   line->idlecnt = -1;
   strcpy (line->who, "LISTEN");
#if defined(UNIX)
    pthread_exit (0);
#endif /* UNIX */
}

/***********************************************************************
* commstart - Start connection task
***********************************************************************/

void
commstart (void *vdev)
{
   COMM_t *line;
   int dev;
   int ch;
   int sfd;
   int fd;
   int i;
   int fromlen;
   struct sockaddr_in peer;
   struct sockaddr_in frominet;
   uint8 msg[82];
#if defined(UNIX)
   pthread_t thread_handle;
#endif

   dev = (int)((long)vdev & 0xFFFFFFFF);
   ch = (dev - COMMOFFSET) / 10;
   sfd = sysio[dev].iocyls;

#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commstart: starting: dev = %d, ch = %c\n", dev, 'A' + ch);
#endif

   for (i = 0; i < sysio[dev].iomodend; i++)
   {
      line = &channel[ch].devices.comlines[i];
      if (i < FIRSTTTY)
      {
	 line->model = COM_2741;
	 strcpy (line->who, "OFFLINE");
      }
      else
      {
	 line->model = sysio[dev].iomodel;
	 strcpy (line->who, "LISTEN");
      }
   }
   channel[ch].cmodstart = sysio[dev].iomodend << 16;

   line = &channel[ch].devices.comlines[35];
   line->fd = 1;
   line->ch = ch;
   line->tty = 35 - sysio[dev].iomodstart;

SERVER_LOOP:

   /*
   ** Accept user connection
   */

   fromlen = sizeof(struct sockaddr_in);
   if ((fd = accept (sfd, (struct sockaddr *)&frominet, (void *)&fromlen)) < 0)
   {
      if (ERRNO == EAGAIN || ERRNO == EINTR)
      {
	 goto SERVER_LOOP;
      }
      PERROR ("commstart: accept");
      return;
   }
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commstart: accepting: fd = %d\n", fd);
#endif

   i = sizeof(peer);

   if (getpeername (fd, (struct sockaddr *)&peer, (void *)&i) < 0)
   {
      PERROR ("commstart: getpeername");
      CLOSE (fd);
      goto SERVER_LOOP;
   }

#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commstart: from %s\n", inet_ntoa (peer.sin_addr));
#endif
   
   /*
   ** Locate a line to use.
   */
   line = NULL;
   for (i = sysio[dev].iomodstart; i < sysio[dev].iomodend; i++)
   {
      if (channel[ch].devices.comlines[i].fd == 0)
      {
	 line = &channel[ch].devices.comlines[i];
         line->fd = fd;
	 line->ch = ch;
	 line->hangup = FALSE;
	 line->noecho = FALSE;
	 line->inescape = FALSE;
	 line->complete = FALSE;
	 line->idlecnt = -1;
	 line->tty = i - sysio[dev].iomodstart;
	 break;
      }
   }
   if (line == NULL)
   {
      sprintf ((char *)msg, "All lines are in use, terminating.\n");
      Nwrite (fd, msg, strlen((char *)msg));
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "commstart: All lines are in use.\n");
#endif
      CLOSE (fd);
      goto SERVER_LOOP;
   }

   strcpy (line->who, inet_ntoa (peer.sin_addr));

   /*
   ** Process connection by giving to the worker.
   */

#if defined(UNIX)
   if (pthread_create (&thread_handle, NULL, (void *(*)(void *)) commwork,
		       (void *)line) < 0)
#endif
#if defined(WIN32)
   if (_beginthread (commwork, 0, (void *)line) < 0)
#endif
   {
      sprintf (errview[0], "commstart: thread create failed: %s",
	       strerror (ERRNO));
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "%s\n", errview[0]);
#endif
   }

   goto SERVER_LOOP;
}

/***********************************************************************
* commdone - Process comm end of record
***********************************************************************/

int
commdone (int ch)
{
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commdone: ch = %c, chrcnt = %d\n", ch + 'A', chrcnt);
#endif

   /*
   ** Write, send out.
   */
   if (!channel[ch].devices.comlines[lineid & 077].hangup &&
       (channel[ch].csel == WRITE_SEL) && chrcnt)
   {
      Nwrite (channel[ch].devices.comlines[lineid & 077].fd, linebuf, chrcnt);
   }
   return 0;
}

/***********************************************************************
* commgo - Process comm CTL command
***********************************************************************/

int
commgo (int ch, uint32 cmd)
{
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commgo: ch = %c, cmd = %o\n", ch + 'A', cmd);
#endif

   /*
   ** Read, gather the characters
   */
   if (cmd == COMM_READ)
   {
      int i;
      int gotchars = FALSE;

      chrcnt = 0;
      for (i = 0; i < MAXCOMM; i++)
      {
	 COMM_t *line = &channel[ch].devices.comlines[i];

	 /*
	 ** Return any completions.
	 */
	 while (line->complcount)
	 {
	    int ccnt;

	    if (!gotchars)
	    {
	       seqnum = (seqnum + 1) & 03777;
	       linechr[chrcnt++] = seqnum;
	       gotchars = TRUE;
	    }

	    line->idlecnt = -1;
	    ccnt = (line->complcount > 037) ? 037 : line->complcount;
	    linechr[chrcnt++] = COM_VALID | i;
	    linechr[chrcnt++] = COM_VALID | COM_COMPLETE | ccnt;
	    line->complcount -= ccnt;
	 }

         if (line->complete && (line->head != line->tail))
	 {
	    int indialup;

	    line->complete = FALSE;
	    if (!gotchars)
	    {
	       seqnum = (seqnum + 1) & 03777;
	       linechr[chrcnt++] = seqnum;
	       gotchars = TRUE;
	    }

	    indialup = FALSE;
	    while (line->head != line->tail)
	    {
	       uint16 dchr;

	       dchr = getring (line);
	       linechr[chrcnt++] = COM_VALID | i;
	       if (dchr > 0x7F)
	       {
	          linechr[chrcnt++] = dchr;
		  if (dchr == COM_DIALUP)
		     indialup = TRUE;
		  else if (dchr == COM_ENDID)
		     indialup = FALSE;
	       }
	       else
	       {
		  if (indialup)
		  {
		     linechr[chrcnt++] = dchr;
		  }
		  else
		  {
		     /*
		     ** KSR37 and ESLSCOPE mode wants EVEN parity.
		     */
		     if (comm_data[line->model-1].parity)
		     {
		        int jj, js = 0;
			uint16 xj = dchr;

			for (jj = 0; jj < 8; jj++)
			{
			   if (xj & 1) js++;
			   xj >>= 1;
			}
			if ((js & 1) == 1)
			   dchr |= 0200;
#ifdef DEBUGCOMMTASK
			fprintf (stderr, "%s PARITY: dchr = %03o, js = %d\n",
			         js & 1 ? "ODD" : "EVEN", dchr, js);
#endif
		     }
		     linechr[chrcnt++] = ~dchr & 0377;
		  }
	       }
	    }
	 }
      }
      if (gotchars)
      {
	 linechr[chrcnt++] = COM_EOM;
	 chrndx = 0;
      }
   }

   /*
   ** Write, set up.
   */
   else if (cmd == COMM_WRITE)
   {
      chrndx = 0;
      chrcnt = 0;
      channel[ch].cdcmd = 0;
   }
   return 0;
}

/***********************************************************************
* commread - Read char from a connection for the COMM
***********************************************************************/

int
commread (int ch)
{
#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commread: ch = %c, chrcnt = %d\n", ch + 'A', chrcnt);
#endif

#ifdef USE64
   channel[ch].cdr = 0;
#else
   channel[ch].cdrh = 0;
   channel[ch].cdrl = 0;
#endif

   if (chrcnt > 0)
   {
      int i;

      for (i = 0; i < 3; i++)
      {
         if (chrcnt <= 0)
	    linechr[chrndx] = COM_EOM;
#ifdef DEBUGCOMMTASK
         fprintf (stderr, "   chr = %o(0x%X) %o(0x%X)'%c'\n",
		  linechr[chrndx], linechr[chrndx],
		  ~linechr[chrndx] & 0377, ~linechr[chrndx] & 0377,
		  isprint (~linechr[chrndx] & 0177)
		        ? (~linechr[chrndx] & 0177) : '.');
#endif
#ifdef USE64
	 channel[ch].cdr = (channel[ch].cdr & 077777777) << 12;
	 channel[ch].cdr |= linechr[chrndx] & 07777;
#else
	 channel[ch].cdrh = 0;
	 if (channel[ch].cdrl &                  000040000000)
	    channel[ch].cdrh |= SIGN;
	 channel[ch].cdrh |= (channel[ch].cdrl & 000034000000) >> 20;
	 channel[ch].cdrl =  (channel[ch].cdrl & 000003777777) << 12;
	 channel[ch].cdrl |= linechr[chrndx] & 07777;
#endif
         chrndx++;
	 chrcnt--;
      }
   }

#ifdef DEBUGCOMMTASK
#ifdef USE64
      fprintf (stderr, "   cdr = %012llo\n", channel[ch].cdr & DATAMASK);
#else
      fprintf (stderr, "   cdr = %02o%010lo\n",
	       ((channel[ch].cdrh & SIGN) >> 2) |
	       ((channel[ch].cdrh & HMSK) << 2) |
	       (uint16)(channel[ch].cdrl >> 30),
	       (unsigned long)channel[ch].cdrl & 07777777777);
#endif
#endif
   return 0;
}

/***********************************************************************
* commgetc - get 6bit char from channel
***********************************************************************/

static uint8
commgetc (int ch)
{
   uint8 dchr;

   if (chrndx == 0)
   {
#ifdef USE64
      dchr = (channel[ch].cdr >> 30) & 077;
      channel[ch].cdr &= 0007777777777;
#else
      dchr = ((channel[ch].cdrh & SIGN) >> 2) |
	     ((channel[ch].cdrh & HMSK) << 2) |
	     ((channel[ch].cdrl >> 30) & 03);
      channel[ch].cdrh = 0;
      channel[ch].cdrl &= 0007777777777;
#endif
   }
   else
   {
#ifdef USE64
      dchr = ((channel[ch].cdr >> 24) & 077);
      channel[ch].cdr <<= 6;
      channel[ch].cdr &= 0007777777777;
#else
      dchr = ((channel[ch].cdrl >> 24) & 077);
      channel[ch].cdrl <<= 6;
      channel[ch].cdrl &= 0007777777777;
#endif
   }
   chrndx++;
   if (chrndx == 6) chrndx = 0;
   return dchr;
}

/***********************************************************************
* commwrite - Write char from a connection for the COMM
***********************************************************************/

int
commwrite (int ch)
{
   COMM_t *line;
   int i, j;
   int atend;
   static int ctlcmd = FALSE;
   uint16 dchr;
   uint16 count;

#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commwrite: ch = %c, start = %d\n",
   	    ch + 'A', channel[ch].cdcmd);
#ifdef USE64
      fprintf (stderr, "   cdr = %012llo\n", channel[ch].cdr & DATAMASK);
#else
      fprintf (stderr, "   cdr = %02o%010lo\n",
	       ((channel[ch].cdrh & SIGN) >> 2) |
	       ((channel[ch].cdrh & HMSK) << 2) |
	       (uint16)(channel[ch].cdrl >> 30),
	       (unsigned long)channel[ch].cdrl & 07777777777);
#endif
#endif

   /*
   ** First time in we get the line and count fields.
   */
   j = 0;
   if (!channel[ch].cdcmd)
   {
      lineid = 0;
      for (i = 0; i < 2; i++)
         lineid = (lineid << 6) | commgetc (ch);
      count = 0;
      for (i = 0; i < 2; i++)
         count = (count << 6) | commgetc (ch);
      if (lineid == 07777 && count == 07777)
      {
	 dchr = (commgetc (ch) << 6) | commgetc (ch);
	 channel[ch].cflags |= CHAN_COMMUP;
         return (0);
      }
      channel[ch].cdcmd = 1;
      channel[ch].clcmd = 0;
      channel[ch].cunit = lineid & 0777;
      ctlcmd = FALSE;
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "   line = %d, count = %d\n", lineid & 077, count);
#endif
      line = &channel[ch].devices.comlines[lineid & 077];
      if (count)
      {
	 line->complcount += count;
	 line->idlecnt = 0;
      }
      if (lineid & 02000)
      {
         channel[ch].clcmd = 1;
      }
      j = 4;
   }

   line = &channel[ch].devices.comlines[lineid & 077];
   dchr = 0;
   atend = FALSE;
   for (i = j; i < 6; i += 2)
   {
      dchr = ((uint16)commgetc (ch) << 6) | (uint16)commgetc (ch);
      if (dchr == 03770)
      {
	 line->hangup = TRUE;
	 continue;
      }
      if (dchr == 03777)
      {
         ctlcmd = TRUE;
	 continue;
      }
      if (dchr == 07777)
      {
         atend = TRUE;
      }
#ifdef DEBUGCOMMTASK
      fprintf (stderr,
      "   dchr = %o(0x%X) %o(0x%X)'%c', chrcnt = %d, atend = %d, ctlcmd = %d\n",
	       dchr, dchr, (~dchr >> 1) & 0x7F, (~dchr >> 1) & 0x7F,
	       isprint ((~dchr >> 1) & 0x7F) ? (~dchr >> 1) & 0x7F : '.',
	       chrcnt, atend, ctlcmd);
#endif
      if (line->inescape)
      {
	 dchr = (~dchr >> 1) & 0x7F;
	 switch (dchr)
	 {
	 case 0x3A:
	    line->noecho = TRUE;
	    break;
	 case 0x3B:
	    line->noecho = FALSE;
	    break;
	 default:
	    break;
	 }
	 line->esccnt--;
	 if (!line->esccnt) line->inescape = FALSE;
      }
      else if (!ctlcmd && !atend && dchr)
      {
	 dchr = (~dchr >> 1) & 0x7F;
	 if (dchr == COM_DC2)
	 {
	    line->noecho = TRUE;
	 }
	 else if (dchr == COM_DC4)
	 {
	    line->noecho = FALSE;
	 }
	 else if (dchr == COM_ESC)
	 {
	    line->inescape = TRUE;
	    line->esccnt = 2;
	 }
	 else if (!line->noecho)
	 {
	    linebuf[chrcnt++] = dchr;
	    if ((line->model != COM_KSR33) && (line->model != COM_TWX))
	    {
	       if ((dchr & 0177) == '\r')
	       {
		  linebuf[chrcnt++] = '\n';
	       }
	       else if ((dchr & 0177) == '\n')
	       {
		  linebuf[chrcnt++] = '\r';
	       }
	    }
	    linebuf[chrcnt] = 0;
	 }
      }
      ctlcmd = FALSE;
   }
   return 0;
}

/***********************************************************************
* commopen - Open a connection for the COMM
***********************************************************************/

int
commopen (char *bp, int dev)
{
   char *cp;
   int sfd;
   short port;
   struct sockaddr_in sinme;
#if defined(WIN32)
   WSADATA nt_data;
#endif /* WIN32 */
#if defined(UNIX)
   pthread_t thread_handle;
#endif


#ifdef DEBUGCOMMTASK
   fprintf (stderr, "commopen: bp = %s, dev = %d\n", bp, dev);
#endif

   memset ((char *)&sinme, '\0', sizeof(sinme));

#if defined(WIN32)
   if (WSAStartup (MAKEWORD (1, 1), &nt_data) != 0)
   {
      sprintf (errview[0], "commopen: WSAStartup failed: %s\n",
	       strerror (ERRNO));
      sprintf (errview[1], "socket: %s\n", bp);
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "%s", errview[0]);
      fprintf (stderr, "%s", errview[1]);
#endif
      return (-1);
   }
#endif /* WIN32 */

   sysio[dev].iomodel = COM_KSR37;
   if ((cp = strchr (bp, ':')) != NULL)
   {
      int i;

      *cp++ = '\0';
      for (i = 0; i < MAXCOMMMODEL; i++)
      {
         if (!strcasecmp (cp, comm_data[i].model))
	 {
	    if (!comm_data[i].support)
	    {
	       sprintf (errview[0],
		  "commopen: Unsupported terminal model: %s\n", cp);
	       return (-1);
	    }
	    sysio[dev].iomodel = comm_data[i].type;
	    break;
	 }
      }
      if (i == MAXCOMMMODEL)
      {
	 sprintf (errview[0], "commopen: Unknown terminal model: %s\n", cp);
	 return (-1);
      }
   }

   port = atoi(bp);
   sinme.sin_port = htons (port);
   sinme.sin_family = AF_INET;

   if ((sfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
   {
      sprintf (errview[0], "commopen: socket failed: %s\n",
	       strerror (ERRNO));
      sprintf (errview[1], "socket: %s\n", bp);
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "%s", errview[0]);
      fprintf (stderr, "%s", errview[1]);
#endif
      return (-1);
   }

   if (bind (sfd, (struct sockaddr *)&sinme, sizeof(sinme)) < 0)
   {
      sprintf (errview[0], "commopen: bind failed: %s\n",
	       strerror (ERRNO));
      sprintf (errview[1], "socket: %s\n", bp);
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "%s", errview[0]);
      fprintf (stderr, "%s", errview[1]);
#endif
      return (-1);
   }

   if (listen (sfd, DEPTH) < 0)
   {
      sprintf (errview[0], "commopen: listen failed: %s\n",
	       strerror (ERRNO));
      sprintf (errview[1], "socket: %s\n", bp);
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "%s", errview[0]);
      fprintf (stderr, "%s", errview[1]);
#endif
      return (-1);
   }

   sysio[dev].iofd = IO_COMMFD;
   sysio[dev].iocyls = sfd;
   sprintf (sysio[dev].iostr, "PORT: %d", port);

#if defined(UNIX)
   if (pthread_create (&thread_handle, NULL, (void *(*)(void *)) commstart,
		       (void *)((long)dev)) < 0)
#endif
#if defined(WIN32)
   if (_beginthread (commstart, 0, (void *)dev) < 0)
#endif
   {
      sprintf (errview[0], "commopen: thread create failed: %s\n",
	       strerror (ERRNO));
#ifdef DEBUGCOMMTASK
      fprintf (stderr, "%s", errview[0]);
#endif
      return (-1);
   }

   return (0);
}
