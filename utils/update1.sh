#
# Update System tape ASYS1.BIN images.
#
#
# IBSYS/SYSDMP nucleus
#
if [ "$1" = "ibsys" ];then
   echo "Updating IBSYS"
   obj2bin      ../sources/IBSYS/ibsys.obj asys1_001002.bin
   echo "Updating SYSDMP"
   obj2bin -s 1 ../sources/IBSYS/ibsys.obj asys1_001003.bin
   if [ "$2" = "check" ];then
      echo "Compare IBSYS"
      bincmp asys1_001002.bin Asys1_001002.bin
      echo "Compare SYSDMP"
      bincmp asys1_001003.bin Asys1_001003.bin
   fi
#
# COBOL compiler
#
elif [ "$1" = "cobol" ];then
   echo "Updating COBOL compiler"
   obj2bin ../sources/IBCBC/cz00.obj asys1_004001.bin
   obj2bin ../sources/IBCBC/c000.obj asys1_004002.bin
   obj2bin ../sources/IBCBC/c100.obj asys1_004003.bin
   obj2bin ../sources/IBCBC/c200.obj asys1_004004.bin
   obj2bin ../sources/IBCBC/c300.obj asys1_004005.bin
   obj2bin ../sources/IBCBC/c400.obj asys1_004006.bin
   obj2bin ../sources/IBCBC/c500.obj asys1_004007.bin
   obj2bin ../sources/IBCBC/c600.obj asys1_004008.bin
   obj2bin ../sources/IBCBC/c700.obj asys1_004009.bin
   obj2bin ../sources/IBCBC/c800.obj asys1_004010.bin
   obj2bin ../sources/IBCBC/c900.obj asys1_004011.bin
   if [ "$2" = "check" ];then
      echo "Compare IBCBCZ"
      bincmp asys1_004001.bin Asys1_004001.bin
      echo "Compare IBCBC0"
      bincmp asys1_004002.bin Asys1_004002.bin
      echo "Compare IBCBC1"
      bincmp asys1_004003.bin Asys1_004003.bin
      echo "Compare IBCBC2"
      bincmp asys1_004004.bin Asys1_004004.bin
      echo "Compare IBCBC3"
      bincmp asys1_004005.bin Asys1_004005.bin
      echo "Compare IBCBC4"
      bincmp asys1_004006.bin Asys1_004006.bin
      echo "Compare IBCBC5"
      bincmp asys1_004007.bin Asys1_004007.bin
      echo "Compare IBCBC6"
      bincmp asys1_004008.bin Asys1_004008.bin
      echo "Compare IBCBC7"
      bincmp asys1_004009.bin Asys1_004009.bin
      echo "Compare IBCBC8"
      bincmp asys1_004010.bin Asys1_004010.bin
      echo "Compare IBCBC9"
      bincmp asys1_004011.bin Asys1_004011.bin
   fi
#
# MAP assembler
#
elif [ "$1" = "map" ];then
   echo "Updating MAP assembler"
   obj2bin ../sources/IBMAP/ibmapj.obj asys1_005001.bin
   obj2bin ../sources/IBMAP/ibmapk.obj asys1_005002.bin
   if [ "$2" = "check" ];then
      echo "Compare IBMAPJ"
      bincmp asys1_005001.bin Asys1_005001.bin
      echo "Compare IBMAPK"
      bincmp asys1_005002.bin Asys1_005002.bin
   fi
#
# FORTRAN II compiler
#
elif [ "$1" = "fortII" ];then
   echo "Updating FORTRAN II compiler"
   obj2bin      ../sources/FORTRAN/ibsfap.obj asys1_006001.bin
   obj2bin      ../sources/FORTRAN/9F00.obj asys1_007001.bin
   obj2bin      ../sources/FORTRAN/9F01.obj asys1_007002.bin
   obj2bin      ../sources/FORTRAN/9F02.obj asys1_007003.bin
   obj2bin      ../sources/FORTRAN/9F03.obj asys1_007004.bin
   obj2bin      ../sources/FORTRAN/9F04.obj asys1_007005.bin
   obj2bin -s 1 ../sources/FORTRAN/9F04.obj asys1_007006.bin
   obj2bin      ../sources/FORTRAN/9F06.obj asys1_007007.bin
   obj2bin      ../sources/FORTRAN/9F07.obj asys1_007008.bin
   obj2bin -s 1 ../sources/FORTRAN/9F07.obj asys1_007009.bin
   obj2bin -s 2 ../sources/FORTRAN/9F07.obj asys1_007010.bin
   obj2bin      ../sources/FORTRAN/9F10.obj asys1_007011.bin
   obj2bin      ../sources/FORTRAN/9F11.obj asys1_007012.bin
   obj2bin      ../sources/FORTRAN/9F12.obj asys1_007013.bin
   obj2bin      ../sources/FORTRAN/9F13.obj asys1_008001.bin
   obj2bin -s 1 ../sources/FORTRAN/9F13.obj asys1_008002.bin
   obj2bin -s 2 ../sources/FORTRAN/9F13.obj asys1_008003.bin
   obj2bin -s 3 ../sources/FORTRAN/9F13.obj asys1_008004.bin
   obj2bin -s 4 ../sources/FORTRAN/9F13.obj asys1_008005.bin
   obj2bin      ../sources/FORTRAN/9F18.obj asys1_008006.bin
   obj2bin -s 1 ../sources/FORTRAN/9F18.obj asys1_008007.bin
   obj2bin -s 2 ../sources/FORTRAN/9F18.obj asys1_008008.bin
   obj2bin -s 3 ../sources/FORTRAN/9F18.obj asys1_008009.bin
   obj2bin      ../sources/FORTRAN/9F22.obj asys1_008010.bin
   obj2bin      ../sources/FORTRAN/9F23.obj asys1_008011.bin
   obj2bin -s 1 ../sources/FORTRAN/9F23.obj asys1_008012.bin
   obj2bin -s 2 ../sources/FORTRAN/9F23.obj asys1_008013.bin
   obj2bin -s 3 ../sources/FORTRAN/9F23.obj asys1_008014.bin
   obj2bin -s 4 ../sources/FORTRAN/9F23.obj asys1_008015.bin
   obj2bin -s 5 ../sources/FORTRAN/9F23.obj asys1_008016.bin
   obj2bin -s 6 ../sources/FORTRAN/9F23.obj asys1_008017.bin
   obj2bin      ../sources/FORTRAN/9F30.obj asys1_008018.bin
   obj2bin -s 1 ../sources/FORTRAN/9F30.obj asys1_008019.bin
   obj2bin      ../sources/FORTRAN/9F32.obj asys1_008020.bin
   obj2bin -s 1 ../sources/FORTRAN/9F32.obj asys1_008021.bin
   obj2bin -s 2 ../sources/FORTRAN/9F32.obj asys1_008022.bin
   if [ "$2" = "check" ];then
      echo "Compare IBSFAP"
      bincmp asys1_006001.bin Asys1_006001.bin
      echo "Compare FORTRAN"
      bincmp asys1_007001.bin Asys1_007001.bin
      echo "Compare 9F01"
      bincmp asys1_007002.bin Asys1_007002.bin
      echo "Compare 9F02"
      bincmp asys1_007003.bin Asys1_007003.bin
      echo "Compare 9F03"
      bincmp asys1_007004.bin Asys1_007004.bin
      echo "Compare 9F04"
      bincmp asys1_007005.bin Asys1_007005.bin
      echo "Compare 9F05"
      bincmp asys1_007006.bin Asys1_007006.bin
      echo "Compare 9F06"
      bincmp asys1_007007.bin Asys1_007007.bin
      echo "Compare 9F07"
      bincmp asys1_007008.bin Asys1_007008.bin
      echo "Compare 9F08"
      bincmp asys1_007009.bin Asys1_007009.bin
      echo "Compare 9F09"
      bincmp asys1_007010.bin Asys1_007010.bin
      echo "Compare 9F10"
      bincmp asys1_007011.bin Asys1_007011.bin
      echo "Compare 9F11"
      bincmp asys1_007012.bin Asys1_007012.bin
      echo "Compare 9F12"
      bincmp asys1_007013.bin Asys1_007013.bin
      echo "Compare 9F13"
      bincmp asys1_008001.bin Asys1_008001.bin
      echo "Compare 9F14"
      bincmp asys1_008002.bin Asys1_008002.bin
      echo "Compare 9F15"
      bincmp asys1_008003.bin Asys1_008003.bin
      echo "Compare 9F16"
      bincmp asys1_008004.bin Asys1_008004.bin
      echo "Compare 9F17"
      bincmp asys1_008005.bin Asys1_008005.bin
      echo "Compare 9F18"
      bincmp asys1_008006.bin Asys1_008006.bin
      echo "Compare 9F19"
      bincmp asys1_008007.bin Asys1_008007.bin
      echo "Compare 9F20"
      bincmp asys1_008008.bin Asys1_008008.bin
      echo "Compare 9F21"
      bincmp asys1_008009.bin Asys1_008009.bin
      echo "Compare 9F22"
      bincmp asys1_008010.bin Asys1_008010.bin
      echo "Compare 9F23"
      bincmp asys1_008011.bin Asys1_008011.bin
      echo "Compare 9F24"
      bincmp asys1_008012.bin Asys1_008012.bin
      echo "Compare 9F25"
      bincmp asys1_008013.bin Asys1_008013.bin
      echo "Compare 9F26"
      bincmp asys1_008014.bin Asys1_008014.bin
      echo "Compare 9F27"
      bincmp asys1_008015.bin Asys1_008015.bin
      echo "Compare 9F28"
      bincmp asys1_008016.bin Asys1_008016.bin
      echo "Compare 9F29"
      bincmp asys1_008017.bin Asys1_008017.bin
      echo "Compare 9F30"
      bincmp asys1_008018.bin Asys1_008018.bin
      echo "Compare 9F31"
      bincmp asys1_008019.bin Asys1_008019.bin
      echo "Compare 9F32"
      bincmp asys1_008020.bin Asys1_008020.bin
      echo "Compare 9F33"
      bincmp asys1_008021.bin Asys1_008021.bin
      echo "Compare 9F34"
      bincmp asys1_008022.bin Asys1_008022.bin
   fi
#
# IBJOB monitor
#
elif [ "$1" = "ibjob" ];then
   echo "Updating IBJOB monitor"
   obj2bin ../sources/IBJOB/ibjob.obj asys1_002001.bin
   echo "Updating JDUMP module"
   obj2bin ../sources/IBJOB/jdump.obj asys1_002002.bin
   echo "Updating DMPREC module"
   obj2bin ../sources/IBJOB/dmprec.obj asys1_002003.bin
   echo "Updating IOCSB module"
   obj2bin ../sources/IBJOB/iocsb.obj asys1_002004.bin
   echo "Updating IBJOBB module"
   obj2bin ../sources/IBJOB/ibjobb.obj asys1_002005.bin
   echo "Updating IBJOBC module"
   obj2bin -s 1 ../sources/IBJOB/ibjobb.obj asys1_002006.bin
   if [ "$2" = "check" ];then
      echo "Compare IBJOB"
      bincmp asys1_002001.bin Asys1_002001.bin
      echo "Compare JDUMP"
      bincmp asys1_002002.bin Asys1_002002.bin
      echo "Compare DMPREC"
      bincmp asys1_002003.bin Asys1_002003.bin
      echo "Compare IOCSB"
      bincmp asys1_002004.bin Asys1_002004.bin
      echo "Compare IBJOBB"
      bincmp asys1_002005.bin Asys1_002005.bin
      echo "Compare IBJOBC"
      bincmp asys1_002006.bin Asys1_002006.bin
   fi
#
# IOCS
#
elif [ "$1" = "iocs" ];then
   echo "Updating IOCS module"
   obj2bin ../sources/IOCS/iocs.obj asys1_012001.bin
   if [ "$2" = "check" ];then
      echo "Compare IOCS"
      bincmp asys1_012001.bin Asys1_012001.bin
   fi
#
# None of the above.
#
else
   echo "Invalid update target " $1
   echo "usage: $0 ibsys|cobol|map|fortII|ibjob|iocs [check]"
fi
