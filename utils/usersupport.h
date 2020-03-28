#ifndef __USERSUPPORT_H__
#define __USERSUPPORT_H__

/***********************************************************************
*
* usersupport.h - CTSS users support routines.
*
* Changes:
*   07/18/13   DGP   Original.
*   
***********************************************************************/

extern void upcase (char *bp);
extern char *getalphanum (char *to, char *from, int len);
extern int getnum (char *from, int len);
extern int getoctnum (char *from, int len);
extern char *convertdate (char *todate, char *fromdate);
extern void printuaccnt (UACCNT_t *puaccnt);
extern void printtimusd (TIMUSD_t *ptimusd);
extern void printreport (REPORT_t *preport);

#endif
