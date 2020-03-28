#ifndef __TIMACC_H__
#define __TIMACC_H__

/***********************************************************************
*
* timacc.h - TIMACC definitions. Taken from TEMPB.INC.
*
* Changes:
*      07/18/13   DGP   Original
*
***********************************************************************/

#define MAXUSERS 100
#define MAXREPORT 1000

#define UFDFILE    "ufd.data"
#define QUOTAFILE  "quota.data"
#define TIMUSDFILE "timusd.data"
#define UACCNTFILE "uaccnt.data"
#define REPORTFILE "report.data"

#define NAMELEN       6
#define NUMLEN        6
#define COMMENTLEN    66
#define DATETIMELEN   12

#define SHIFTTIME     360

/* Definition for UACCNT TIMACC */

/* rcode bits in spread octal word */

#define RCOMBT 00001          /* If user may use common files */ 
#define RCALBT 00002          /* If user may make privileged file calls */
#define RPROBT 00004          /* If user may modify 'P' file of other user */
#define RPRVBT 00010          /* If user may read 'V' file of other user */
#define RPATBT 00020          /* If user may patch the supervisor */
#define RKLDBT 00040          /* If user may use the esl display (kludge) */
#define R636BT 00100          /* If user may use the 636 tape calls */
#define RDSKBT 00200          /* If user may not use disk loaded commands */
#define RTSSBT 00400          /* If user may not alter file directory */
#define RSYSBT 01000          /* If user may alter subsystem status */
#define ROPRBT 02000          /* If user may use operator functions */

typedef struct 
{
   char uname[NAMELEN];		/* User name */
   char progno[NUMLEN];		/* Programmer number */
   char group[NUMLEN];		/* Party group */
   char stanby[NUMLEN];		/* Stanby user */
   char ufd[NAMELEN];		/* File directory */
   char unit[NUMLEN];		/* Unit group */
   char rcode[NUMLEN];		/* Restriction Code (privs) */
   char flags[NUMLEN];		/* Indicator flags */
   char password[NAMELEN];	/* Password */
   char filler1[30];
   char drumquota[NUMLEN];	/* Drum Quota */
   char diskquota[NUMLEN];	/* Disk Quota */
   char tapequota[NUMLEN];	/* Tape Quota */
   char s1time[NUMLEN];		/* Shift 1 time alloted */
   char s2time[NUMLEN];		/* Shift 2 time alloted */
   char s3time[NUMLEN];		/* Shift 3 time alloted */
   char s4time[NUMLEN];		/* Shift 4 time alloted */
   char s5time[NUMLEN];		/* Shift 5 time alloted (FIB) */
   char trailer[2];
} UACCNT_t;

/* Definition for TIMUSD TIMACC */

typedef struct 
{
   char probno[NAMELEN];	/* Problem number */
   char progno[NUMLEN];		/* Programmer number */
   char uname[NAMELEN];		/* User name */
   char comment[COMMENTLEN];	/* Comments */
   char lastlogout[DATETIMELEN]; /* Date/Time last logout */
   char unit[NUMLEN];		/* Last Unit */
   char filler1[6];
   char s1time[NUMLEN];		/* Shift 1 time used */
   char s2time[NUMLEN];		/* Shift 2 time used */
   char s3time[NUMLEN];		/* Shift 3 time used */
   char s4time[NUMLEN];		/* Shift 4 time used */
   char s5time[NUMLEN];		/* Shift 5 time used (FIB) */
   char contime[NUMLEN];	/* Console time used */
   char trailer[2];
} TIMUSD_t;

/* Definition for REPORT TIMACC */

/* flags decribe the action taking place, the format of the flags
 * is a spread octal word 'CNNFFF' as defined below. 
 * 
 * C - command flag
 * 0 for login
 * 1 for logout
 * 2 for auto logout
 * 4 otherwise
 *
 * NN - notime flags (on logout)
 * 00 for system comedown
 * 01 for exceeded shift allottment
 * 02 for user inactive too long
 * 04 for user hung up
 * 10 for user exceeded esttim of FIB
 * 20 for user logged out by load leveler
 * 
 * FFF - error flags detected by login (or logout)
 * 001 for system full
 * 002 for no time allotted on present shift
 * 003 for allotted time exceeded on present shift
 * 004 for out of funds
 * 005 for past expiration date
 * 006 for alread logged in
 * 007 for has FIB job running
 * 010 for not in uaccnt (includes incorrect password)
 * 011 illegal unitid
 * 012 system or machine error, error in accounting files (incl. logout)
 */

typedef struct 
{
   char probno[NAMELEN];	/* Problem number */
   char progno[NUMLEN];		/* Programmer number */
   char unit[NUMLEN];		/* Unit ID */
   char datetime[DATETIMELEN];	/* Date / Time */
   char flags[NUMLEN];		/* Flags */
   char s1time[NUMLEN];		/* Shift 1 time */
   char s2time[NUMLEN];		/* Shift 2 time */
   char s3time[NUMLEN];		/* Shift 3 time */
   char s4time[NUMLEN];		/* Shift 4 time */
   char s5time[NUMLEN];		/* Shift 5 time (FIB) */
   char contime[NUMLEN];	/* Console time */
   char uname[NAMELEN];		/* User name */
   char trailer[6];
} REPORT_t;

#endif
