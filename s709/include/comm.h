#ifndef __COMM_H__
#define __COMM_H__
/***********************************************************************
*
* comm.h - COMM definitions.
*
* Changes:
*   03/11/10   DGP   Original
*   03/25/11   DGP   Added CTRL-U and CTRL-BS support.
*
***********************************************************************/
 
#define MAXCOMMMODEL 9

#define COMM_CTL	0
#define COMM_READ	1
#define COMM_WRITE	2

#define COM_VALID	02000
#define COM_COMPLETE    01000

#define COM_DIALUP	02001
#define COM_ENDID	02002
#define COM_INTR	02003
#define COM_QUIT	02004
#define COM_HANGUP	02005

#define COM_EOM		03777

#define COM_KSR35       1
#define COM_1050        2
#define COM_TELEX       3
#define COM_TWX         4
#define COM_KSR33       5
#define COM_ASR35       6
#define COM_KSR37       7
#define COM_2741        8
#define COM_ESLSCOPE    9

#define COM_CTRLC	0x03
#define COM_BELL        0x07
#define COM_BACKSPACE   0x08
#define COM_DC2         0x12
#define COM_DC4         0x14
#define COM_CTRLU	0x15
#define COM_ESC         0x1B
#define COM_CTRLBS	0x1C
#define COM_DELETE      0x7F

typedef struct
{
   char *model;
   int width;
   int parity;
   int type;
   int support;
} comm_types;

extern int commopen (char *s, int dev);
extern int commgo (int ch, uint32 cmd);
extern int commdone (int ch);
extern int commread (int ch);
extern int commwrite (int ch);

#endif /* __COMM_H__ */
