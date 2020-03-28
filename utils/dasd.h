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
#define DASDOVERHEAD 16
#define MAXTRACKLEN 6020

#define FMT_DATA	0		/* Data */
#define FMT_HDR		1		/* Header */
#define FMT_HA2		2		/* Home address 2 */
#define FMT_END		3		/* End of track */

typedef struct
{
   char *model;
   int cyls;
   int modules;
   int access;
   int heads;
   int byttrk;
} dasd_types;


dasd_types dasd[MAXDASDMODEL] =
{
   { "1301-1",  250, 1, 1,  41,  2840 },
   { "1301-2",  250, 2, 1,  41,  2840 },
   { "1302-1",  250, 1, 2,  41,  5902 },
   { "1302-2",  250, 2, 2,  41,  5902 },
   { "7320",      1, 1, 1, 401,  2836 },
   { "7289",      6, 1, 1,  16, 12288 },
};

#endif /* __DASD_H__ */

