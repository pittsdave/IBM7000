/***********************************************************************
* Scan tables
*     This module defines the tables used by the scanner. Two tables are
*     defined:
*
*     chartable - the character translation table used to find the
*                 translate value of the input character
*     scntbl - the finite state machine upon which the scanner operates.
*
*     scntbl is an array of pointers (indexed by state number) to state
*     transition lists. Each entry of the state list contains a field for
*     the comparand character, the next state and a scan action which
*     applies when the comparand character matches the translated input
*     character. Each state transition list ends with a default entry
*     which is used when the current input character does not match any
*     characters in the list. Also in this module are equates (character
*     translate equates) and macros (scan state element) to generate the
*     two scan tables.
*
***********************************************************************/
/*
** Character translate equates
*/
#define EOFS    0       /* End_of_file */
#define EOLN    1       /* End_of_line */
#define SPACE   2
#define EE      3	/* 'E' */
#define DIGIT   4       /* Numeric digits [8-9] */
#define OPERAT  5       /* Operators  <,>,[,],{,},^,= */
#define PCSYM   7       /* '*' */
#define ALPHA   8       /* Alphabetic */
#define SPECIAL 9       /* Special characters */
#define GRAPHIC SPACE   /* Graphics characters */
#define PLUS    10	/* '+' */
#define MINUS   11	/* '-' */
#define DOLLAR  12	/* '$' */
#define SLASH   13	/* '/' */
#define PERIOD  14      /* '.' period */
#define RELOP   15
#define STROP   16
#define COMMA   17	/* ',' comma */
#define LPARN   18	/* '(' */
#define RPARN   19	/* ')' */
#define BB      20	/* 'B' */
#define ODIGIT  21	/* Octal digit [0-7] */
#define DEFAULT 31      /* Scan action list default indicator */

static char chartable[] = {
#if defined(USS) || defined(OS390)
        EOLN,   EOFS,   GRAPHIC,GRAPHIC,  /*            >00->03 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >04->07 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >08->0B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >0C->0F */

        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >10->13 */
        GRAPHIC,EOLN   ,GRAPHIC,GRAPHIC,  /*            >14->17 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >18->1B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >1C->1F */

        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >20->23 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >24->27 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >28->2B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >2C->2F */

        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >30->33 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >34->37 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >38->3B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >3C->3F */

        SPACE,  GRAPHIC,GRAPHIC,GRAPHIC,  /* SP         >40->43 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >44->47 */
        GRAPHIC,GRAPHIC,SPECIAL,PERIOD,   /*       .    >48->4B */
        RELOP,  LPARN,  PLUS,   SPECIAL,  /* < ( + |    >4C->4F */

        RELOP,  GRAPHIC,GRAPHIC,GRAPHIC,  /* &          >50->53 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >54->57 */
        GRAPHIC,GRAPHIC,SPECIAL,DOLLAR,   /*     ! $    >58->5B */
        PCSYM,  RPARN,  SPECIAL,SPECIAL,  /* * ) ; ^    >5C->5F */

        MINUS,  SLASH,  GRAPHIC,GRAPHIC,  /* - /        >60->63 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >64->67 */
        GRAPHIC,GRAPHIC,GRAPHIC,COMMA,    /*       ,    >68->6B */
        SPECIAL,ALPHA,  RELOP,  SPECIAL,  /* % _ > ?    >6C->6F */

        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >70->73 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >74->77 */
        GRAPHIC,SPECIAL,SPECIAL,RELOP,    /*   ` : #    >78->7B */
        SPECIAL,STROP  ,RELOP,  SPECIAL,  /* @ ' = "    >7C->7F */

        GRAPHIC,ALPHA,  BB,     ALPHA,    /*   a b c    >80->83 */
        ALPHA,  EE,     ALPHA,  ALPHA,    /* d e f g    >84->87 */
        ALPHA,  ALPHA,  GRAPHIC,GRAPHIC,  /* h i        >88->8B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >8C->8F */

        GRAPHIC,ALPHA,  ALPHA,  ALPHA,    /*   j k l    >90->93 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* m n o p    >94->97 */
        ALPHA,  ALPHA,  GRAPHIC,GRAPHIC,  /* q r        >98->9B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >9C->9F */

        GRAPHIC,SPECIAL,ALPHA,  ALPHA,    /*   ~ s t    >A0->A3 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* u v w x    >A4->A7 */
        ALPHA,  ALPHA,  GRAPHIC,GRAPHIC,  /* y z        >A8->AB */
        GRAPHIC,OPERAT, GRAPHIC,GRAPHIC,  /*   [        >AC->AF */

        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >B0->B3 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >B4->B7 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >B8->BB */
        GRAPHIC,OPERAT, GRAPHIC,GRAPHIC,  /*   ]        >BC->BF */

        OPERAT, ALPHA,  BB,     ALPHA,    /* { A B C    >C0->C3 */
        ALPHA,  EE,     ALPHA,  ALPHA,    /* D E F G    >C4->C7 */
        ALPHA,  ALPHA,  GRAPHIC,GRAPHIC,  /* H I        >C8->CB */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >CC->CF */

        OPERAT, ALPHA,  ALPHA,  ALPHA,    /* } J K L    >D0->D3 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* M N O P    >D4->D7 */
        ALPHA,  ALPHA,  GRAPHIC,GRAPHIC,  /* Q R        >D8->DB */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >DC->DF */

        SPECIAL,GRAPHIC,ALPHA,  ALPHA,    /* \   S T    >E0->E3 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* U V W X    >E4->E7 */
        ALPHA,  ALPHA,  GRAPHIC,GRAPHIC,  /* Y Z        >E8->EB */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >EC->EF */

        ODIGIT, ODIGIT, ODIGIT, ODIGIT,   /* 0 1 2 3    >F0->F3 */
        ODIGIT, ODIGIT, ODIGIT, ODIGIT,   /* 4 5 6 7    >F4->F7 */
        DIGIT,  DIGIT,  GRAPHIC,GRAPHIC,  /* 8 9        >F8->FB */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC   /*            >FC->FF */
#else
        EOLN,   EOFS,   GRAPHIC,GRAPHIC,  /*            >00->03 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >04->07 */
        GRAPHIC,GRAPHIC,EOLN   ,GRAPHIC,  /*            >08->0B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >0C->0F */

        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >10->13 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >14->17 */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >18->1B */
        GRAPHIC,GRAPHIC,GRAPHIC,GRAPHIC,  /*            >1C->1F */

        SPACE,  SPECIAL,SPECIAL,RELOP,    /* SP ! " #   >20->23 */
        DOLLAR, SPECIAL,RELOP,  STROP,    /* $ % & '    >24->27 */
        LPARN,  RPARN,  PCSYM,  PLUS,     /* ( ) * +    >28->2B */
        COMMA,  MINUS,  PERIOD, SLASH,    /* , - . /    >2C->2F */

        ODIGIT, ODIGIT, ODIGIT, ODIGIT,   /* 0 1 2 3    >30->33 */
        ODIGIT, ODIGIT, ODIGIT, ODIGIT,   /* 4 5 6 7    >34->37 */
        DIGIT,  DIGIT,  SPECIAL,SPECIAL,  /* 8 9 : ;    >38->3B */
        RELOP,  RELOP,  RELOP,  SPECIAL,  /* < = > ?    >3C->3F */

        SPECIAL,ALPHA,  BB,     ALPHA,    /* @ A B C    >40->43 */
        ALPHA,  EE,     ALPHA,  ALPHA,    /* D E F G    >44->47 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* H I J K    >48->4B */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* L M N O    >4C->4F */

        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* P Q R S    >50->53 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* T U V W    >54->57 */
        ALPHA,  ALPHA,  ALPHA,  OPERAT,   /* X Y Z [    >58->5B */
        SPECIAL,OPERAT, SPECIAL,ALPHA,    /* \ ] ^ _    >5C->5F */

        SPECIAL,ALPHA,  BB,     ALPHA,    /* ` a b c    >60->63 */
        ALPHA,  EE,     ALPHA,  ALPHA,    /* d e f g    >64->67 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* h i j k    >68->6B */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* l m n o    >6C->6F */

        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* p q r s    >70->73 */
        ALPHA,  ALPHA,  ALPHA,  ALPHA,    /* t u v w    >74->77 */
        ALPHA,  ALPHA,  ALPHA,  OPERAT,   /* x y z {    >78->7B */
        SPECIAL,OPERAT, SPECIAL,EOLN      /* | } ~ EOL  >7C->7F */
#endif
};

/***********************************************************************
*
* Macro for generating scanner state list element
*
*           F E D C B A 9 8 7 6 5 4 3 2 1 0
*          +---------+-----------+---------+
*          |  char   |next state | action  |
*          +---------+-----------+---------+
*
* CR = character
* NS = next state
* AC = action
*
***********************************************************************/

#define ENTRY(cr,ns,ac) (short)((cr<<6|ns)<<5|ac)

/*
** State 1 - Address expression start state
*/
#define ADDREXPRSTART 1
static short int S1[] = {
        ENTRY(ALPHA,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(DOLLAR,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,4,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DIGIT,4,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(PCSYM,5,EAT_ACTION),
        ENTRY(EOLN,6,BUILD_ACTION),
        ENTRY(COMMA,6,BUILD_ACTION),
        ENTRY(SPACE,6,BUILD_ACTION),
        ENTRY(DEFAULT,0,EAT_ACTION|BUILD_ACTION)
};

/*
** State 2 - recognize address symbol
*/
static short int S2[] = {
        ENTRY(ALPHA,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(DOLLAR,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 3 - recognize address qualified symbol
*/
static short int S3[] = {
        ENTRY(ALPHA,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 4 - recognize address number
*/
static short int S4[] = {
        ENTRY(DIGIT,4,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,4,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(EE,2,EAT_ACTION|MOVE_ACTION),
	ENTRY(BB,2,EAT_ACTION|MOVE_ACTION),
	ENTRY(ALPHA,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,2,EAT_ACTION|MOVE_ACTION),
        ENTRY(DOLLAR,3,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 5 - recognize PC
*/
static short int S5[] = {
	ENTRY(PCSYM,23,EAT_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 6 - recognize statement terminator
*/
static short int S6[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 7 - Boolean expression start state
*/
#define BOOLEXPRSTART 7
static short int S7[] = {
        ENTRY(ALPHA,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,9,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(EOLN,10,BUILD_ACTION),
        ENTRY(COMMA,6,BUILD_ACTION),
        ENTRY(SPACE,6,BUILD_ACTION),
        ENTRY(PCSYM,25,EAT_ACTION),
        ENTRY(DEFAULT,0,EAT_ACTION|BUILD_ACTION)
};

/*
** State 8 - recognize boolean symbol
*/
static short int S8[] = {
        ENTRY(ALPHA,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 9 - recognize boolean number
*/
static short int S9[] = {
        ENTRY(ODIGIT,9,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DIGIT,8,EAT_ACTION|MOVE_ACTION),
	ENTRY(ALPHA,8,EAT_ACTION|MOVE_ACTION),
	ENTRY(BB,8,EAT_ACTION|MOVE_ACTION),
	ENTRY(EE,8,EAT_ACTION|MOVE_ACTION),
	ENTRY(PERIOD,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,8,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 10 - Data expression start state
*/
#define DATAEXPRSTART 10
static short int S10[] = {
        ENTRY(ALPHA,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,13,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,13,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(PERIOD,14,EAT_ACTION|MOVE_ACTION),
        ENTRY(DOLLAR,15,EAT_ACTION|MOVE_ACTION),
        ENTRY(PCSYM,5,EAT_ACTION),
        ENTRY(EOLN,6,BUILD_ACTION),
        ENTRY(COMMA,6,BUILD_ACTION),
        ENTRY(SPACE,6,BUILD_ACTION),
        ENTRY(DEFAULT,0,EAT_ACTION|BUILD_ACTION)
};

/*
** State 11 - recognize data symbol
*/
static short int S11[] = {
        ENTRY(ALPHA,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(DOLLAR,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 12 - recognize data qualified symbol
*/
static short int S12[] = {
        ENTRY(ALPHA,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(BB,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(DIGIT,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(ODIGIT,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(LPARN,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(RPARN,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 13 - recognize data number
*/
static short int S13[] = {
        ENTRY(DIGIT,13,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,13,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(BB,19,EAT_ACTION|MOVE_ACTION),
	ENTRY(EE,15,EAT_ACTION|MOVE_ACTION),
	ENTRY(PERIOD,14,EAT_ACTION|MOVE_ACTION),
	ENTRY(ALPHA,11,EAT_ACTION|MOVE_ACTION),
        ENTRY(DOLLAR,12,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 14 - recognize fraction
*/
static short int S14[] = {
        ENTRY(DIGIT,14,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,14,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(BB,19,EAT_ACTION|MOVE_ACTION),
        ENTRY(EE,15,EAT_ACTION|MOVE_ACTION),
        ENTRY(PERIOD,0,ERROR_ACTION),
        ENTRY(DEFAULT,18,BUILD_ACTION)
};

/*
** State 15 - recognize floating point exponent sign
*/
static short int S15[] = {
	ENTRY(PLUS,16,EAT_ACTION),
	ENTRY(MINUS,16,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(EE,15,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(DIGIT,17,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,17,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)
};

/*
** State 16 - process floating point exponent sign
*/
static short int S16[] = {
	ENTRY(DIGIT,17,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,17,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)
};

/*
** State 17 - recognize floating point exponent
*/
static short int S17[] = {
        ENTRY(DIGIT,17,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,17,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(BB,28,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,18,BUILD_ACTION)
};

/*
** State 18 - build floating point number
*/
static short int S18[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 19 - recognize binary point exponent sign
*/
static short int S19[] = {
	ENTRY(PLUS,20,EAT_ACTION),
	ENTRY(MINUS,20,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(BB,19,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(EE,32,EAT_ACTION|MOVE_ACTION),
	ENTRY(DIGIT,21,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,21,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,22,BUILD_ACTION)
};

/*
** State 20 - process binary point exponent sign
*/
static short int S20[] = {
	ENTRY(DIGIT,21,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,21,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)
};

/*
** State 21 - recognize binary point exponent
*/
static short int S21[] = {
        ENTRY(DIGIT,21,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,21,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(EE,32,EAT_ACTION|MOVE_ACTION),
        ENTRY(DEFAULT,22,BUILD_ACTION)
};

/*
** State 22 - build binary point number
*/
static short int S22[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 23 - recognize null symbol "**"
*/
static short int S23[] = {
	ENTRY(EOLN,0,BUILD_ACTION),
	ENTRY(SPACE,0,BUILD_ACTION),
	ENTRY(COMMA,0,BUILD_ACTION),
	ENTRY(GRAPHIC,0,BUILD_ACTION),
	ENTRY(PLUS,0,BUILD_ACTION),
	ENTRY(LPARN,0,BUILD_ACTION),
	ENTRY(OPERAT,0,BUILD_ACTION),
        ENTRY(DEFAULT,24,BACK_ACTION|BUILD_ACTION)
};

/*
** State 24 - recognize PC
*/
static short int S24[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 25 - recognize Boolean AND '*'
*/
static short int S25[] = {
	ENTRY(PCSYM,26,EAT_ACTION),
        ENTRY(DEFAULT,0,BUILD_ACTION)
};

/*
** State 26 - recognize null symbol "**"
*/
static short int S26[] = {
	ENTRY(SPACE,0,BUILD_ACTION),
	ENTRY(COMMA,0,BUILD_ACTION),
	ENTRY(GRAPHIC,0,BUILD_ACTION),
	ENTRY(PLUS,0,BUILD_ACTION),
        ENTRY(DEFAULT,27,BACK_ACTION|BUILD_ACTION)
};

/*
** State 27 - recognize AND
*/
static short int S27[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 28 - recognize nnEnnBnn format
*/
static short int S28[] = {
	ENTRY(PLUS,29,EAT_ACTION|BUILD_ACTION),
	ENTRY(MINUS,29,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(BB,28,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(DIGIT,30,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,30,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)

};

/*
** State 29 - process binary point exponent sign
*/
static short int S29[] = {
	ENTRY(DIGIT,30,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,30,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)
};

/*
** State 30 - recognize binary point exponent
*/
static short int S30[] = {
        ENTRY(DIGIT,30,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,30,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,31,BUILD_ACTION)
};

/*
** State 31 - build nnEnnBnn binary point number
*/
static short int S31[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};

/*
** State 32 - recognize nnBnnEnn format
*/
static short int S32[] = {
	ENTRY(PLUS,33,EAT_ACTION|BUILD_ACTION),
	ENTRY(MINUS,33,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(EE,32,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(DIGIT,34,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,34,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)
};

/*
** State 33 - process binary point exponent sign
*/
static short int S33[] = {
	ENTRY(DIGIT,34,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
	ENTRY(ODIGIT,34,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,0,ERROR_ACTION)
};

/*
** State 34 - recognize binary point exponent
*/
static short int S34[] = {
        ENTRY(DIGIT,34,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(ODIGIT,34,EAT_ACTION|MOVE_ACTION|BUILD_ACTION),
        ENTRY(DEFAULT,35,BUILD_ACTION)
};

/*
** State 35 - build nnBnnEnn binary point number
*/
static short int S35[] = {
        ENTRY(DEFAULT,0,NULL_ACTION)
};


static short int *scantable[] = {
         S1,  S2,  S3,  S4,  S5,  S6,  S7,  S8,  S9, S10,
	S11, S12, S13, S14, S15, S16, S17, S18, S19, S20,
	S21, S22, S23, S24, S25, S26, S27, S28, S29, S30,
	S31, S32, S33, S34, S35
};

