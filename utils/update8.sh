#
# Update System tape ASYS8 images.
#
if [ "$1" = "editor" ];then
   echo "Updating EDITOR"
   obj2bin ../sources/EDITOR/editor.obj asys8_009001.bin
   if [ "$2" = "check" ];then
      echo "Compare EDITOR"
      bincmp asys8_009001.bin Asys8_009001.bin
   fi
elif [ "$1" = "restart" ];then
   echo "Updating RESTART"
   obj2bin ../sources/RESTART/ior00.obj asys8_007001.bin
   if [ "$2" = "check" ];then
      echo "Compare RESTART"
      bincmp asys8_007001.bin Asys8_007001.bin
   fi
else
   echo "Invalid update target " $1
   echo "usage: $0 editor|restart [check]"
fi
