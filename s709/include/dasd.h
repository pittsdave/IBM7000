#ifndef __DASD_H__
#define __DASD_H__
/***********************************************************************
*
* dasd.h - DASD definitions.
*
* Changes:
*      06/03/05   DGP   Original
*      03/17/06   DGP   Added access field and changed cyls on 1302.
*
***********************************************************************/
 
#define MAXDASDMODEL 6
#define CETRACK 10000

typedef struct
{
   char *model;
   int cyls;
   int modules;
   int access;
   int heads;
   int byttrk;
   int overhead;
} dasd_types;


/*
** Format codes
*/

#define FMT_DATA	0		/* Data */
#define FMT_HDR		1		/* Header */
#define FMT_HA2		2		/* Home address 2 */
#define FMT_END		3		/* End of track */

/*
** DASD commands
*/

#define DNOP 0
#define DREL 4
#define DEBM 8
#define DSBM 9
#define DSEK 80
#define DVSR 82
#define DWRF 83
#define DVTN 84
#define DVCY 85
#define DWRC 86
#define DSAI 87
#define DVTA 88
#define DVHA 89

#endif /* __DASD_H__ */
