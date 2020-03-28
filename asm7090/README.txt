Version: 2.2.29
Date: 2013/05/07


I. Introduction

asm7090 is a cross assembler for the IBM 7090 and 7094 computer. It is MAP and
FAP compatible and supports absolute and relocatable assembly. Macros and
conditional assembly are supported. The floating and binary point conversion
for the DEC pseudo op was corrected and supports double precision.  Also,
supported is the FAP mode of assembly and supports multiple assemblies in a
single files as FAP does.  Supports the 7090 macros for 7094 instructions and
CPU model checks.

If IBSYS control cards are at the beginning of assembly source the TITLE and
IBMAP fields are parsed and used to set the appropriate options in the assembly
of the module. The EXECUTE IBSFAP control card is parsed to allow support of
FAP assemblies. The control cards are listed in the listing on a seperate page.

Also supported are the CTSS instructions and pseudo operation codes.


II. Build asm7090

Lunix/Unix:

$ make

The make attempts to figure out which system to make for using uname.

WinBlows:

$ nmake nt


III. To run asm7090

$ asm7090 [-options] -o file.obj file.asm

Where options are:

   -a           - Absolute assembly "ABSMOD"
   -C           - Relocate CTSS Program Common
   -c CPU       - CPU model (704, 709, 7090, 7094, CTSS)
   -d deck      - Deckname for output
   -f           - FAP mode assembly
   -I incldir   - Include directory for INSERT, CTSS mode.
   -j           - IBJOB symbol mode "JOBSYM"
   -l listfile  - Generate listing to listfile
   -m           - Nucleus symbol mode "MONSYM"
   -o outfile   - Object file name
   -p           - Parens in symbols OK "()OK"
   -r           - ADDREL mode
   -t title     - Title for listing
   -w           - Generate wide listing
   -x           - Generate cross reference


IV. Supported Pseudo Ops

The standard MAP/FAP pseudo ops are:

   ABS    - ABSolute object card format, FAP.
   BCD    - Encode BCD strings.
   BCI    - Encode BCD strings.
   BEGIN  - Specifies a BEGINning location counter, MAP.
   BES    - Block Ended by Symbol (reserve storage).
   BOOL   - BOOLean variable declaration.
   BSS    - Block Started by Symbol (reserve storage).
   CALL   - Generate IBSYS CALL prolog.
   COMMON - COMMON storage.
   CONTRL - Currently ignored.
   COUNT  - Card COUNT, FAP ignored.
   DEC    - Generate data in DECimal.
   DETAIL - Provide DETAIL in listing.
   DUP    - DUPlicate source lines.
   END    - END of assembler input and MACRO in FAP.
   ENDM   - END Macro definition, MAP.
   ENDQ   - END Qualified section, MAP.
   ENTRY  - Defined ENTRY (global).
   EJECT  - Start next listing line on a new page.
   EQU	  - EQUate symbols to values.
   ETC    - ETC specifies statement continuation.
   EVEN   - EVEN location counter. Ignored.
   EXTERN - Define EXTERNal reference.
   FILE   - Currently ignored.
   FUL    - FULl object card format, ignored.
   GOTO   - GOTO line in source file, MAP.
   HEAD   - Section HEADs, FAP.
   HED    - Section HEADs, FAP.
   IFF    - IF False conditional.
   IFT    - IF True conditional, MAP.
   INDEX  - Generates INDEX to code, Table of Contents.
   IRP    - Indefinite RePeat.
   KEEP   - Currently ignored.
   LABEL  - Currently ignored.
   LBL    - LaBeL object.
   LBOOL  - Left BOOLean variable declaration.
   LDIR   - Linkage Director linkage, MAP.
   LIST   - Enable listing generation.
   LIT    - Add data to literal pool, MAP.
   LITORG - Currently ignored, MAP.
   LOC    - Set LOCation counter.
   LORG   - Literal pool origin, MAP.
   MACRO  - Specify MACRO.
   MAX    - MAXimum value of arguments.
   MIN    - MINimum value of arguments.
   NOCRS  - Currently ignored.
   NULL   - NULL defines any label to the location counter.
   OCT    - Generate data in OCTal.
   OPD    - Operation definition.
   OPSYN  - Operation Synonym.
   OPVFD  - Operation variable field definition.
   ORG    - Program ORiGin.
   ORGCRS - Currently ignored.
   PCC    - Print Control Cards.
   PCG    - Print Control Group.
   PMC    - Print Macro Cards.
   PUNCH  - Enable object generation, MAP.
   PURGE  - Currently ignored.
   QUAL   - QUALifed section beginning, MAP.
   RBOOL  - Right BOOLean variable declaration.
   REM    - Program REMark.
   RETURN - Generate IBSYS RETURN epilog, MAP.
   RMT    - ReMoTe sequences, FAP.
   SAVE   - Save CPU context, MAP.
   SAVEN  - Save CPU context, no Linkage Director, MAP.
   SET    - SET assembly control variable.
   SPACE  - SPACE listing by the specified number of lines.
   SST    - System Symbol Table, FAP.
   SYN	  - SYNonym equates symbols to values.
   TAPENO - TAPE Number, FAP.
   TCD    - Transfer Control.
   TITLE  - Turn off DETAIL in listing.
   TTL    - Sub TiTLe of the assembly, placed in listing header.
   UNLIST - Disable listing generation.
   UNPNCH - Disable object generation, MAP.
   USE    - USE location counter specified in variable field, MAP.
   VFD    - Variable Field Data definition.


The CTSS pseudo ops are:

   12BIT  - 12 bit character encoding.
   ACORE  - Specify an A CORE module.
   BCORE  - Specify an B CORE module.
   COMBES - COMmon BES.
   COMBSS - COMmon BSS.
   INSERT - Insert source code (Include).
   LINK   - LINKage director ON/OFF.
   LSTNG  - Toggle Online listing, ignored.
   MACRON - Specify MACRO.
   NOLNK  - NO LiNKage director.
   PAR    - Plus ARgument


Files included with the INSERT pseudo op have their names folded to lower 
case and a ".inc" suffix is appended. For example:

   INSERT MACRO

Includes the file:

   [incldir/]macro.inc

If the include directory is not specified the file is read from the current
working directory. If the file is not found then the ".fap" suffix is appended
and the file open it tried again.


The following Pseudo Ops are implemented as Type A instructions and allow the
initialization of the various fields, decrement, tag and address. For
use by the program:

   FIVE  - Plus Five
   FOR   - Plus Four
   FOUR  - Plus Four
   FVE   - Plus Five
   MON   - Minus One
   MTH   - Minus Three
   MTW   - Minus Two
   MZE   - Minus Zero
   ONE   - Plus One
   PON   - Plus One
   PTH   - Plus Three
   PTW   - Plus Two
   PZE   - Plus Zero
   SEVEN - Plus Seven
   SIX   - Plus Six
   SVN   - Plus Seven
   THREE - Plus Three
   TWO   - Plus Two
   ZERO  - Plus Zero


V. Symbol Table Flags

The symbol table is listed with tags after the value field that specify the
symbols usage. The tags are:

   blank - Absolute symbol
   B     - Boolean symbol
   E     - External symbol
   G     - Global symbol
   R     - Relocatable symbol
   S     - SET variable symbol
   T     - TAPENO symbol


VI. Known problems/issues

1. The overloaded use of the asterisk, '*', as the PC, MULTIPLICATION, the
   relational AND and the NULL symbols can sometimes cause problems. May
   need to change the parser from SLR(1) to LALR(2).

2. FAP mode externals are currently generated, by asm7090, using a back chain.
   Native FAP generates a transfer vector at the beginning of the module. 

