#!/usr/bin/env bash

[ -f ~/.profile ] && source ~/.profile

#z26="/Path/To/z26"
#Stella="/Path/To/Stella"

PATH="$bB/source:$PATH"
which preprocess || { echo "Building bAtariBasic..." ;  cd $bB/source ; make ; echo ; }

echo -n "Starting build of " ; echo -n $( basename "$1" ) ...

preprocess < "$1" | 2600basic -i "$bB" > bB.asm

[ "$?" -ne "0" ] && { echo "Compilation failed."; exit; }

if [ "$2" = "-O" ]
  then
   postprocess -i "$bB" | optimize > "$1.asm"
  else
   postprocess -i "$bB" > "$1.asm"
fi
dasm "$1.asm" -I"$bB/includes" -f3 -o"$1.bin" | sed '/Label mismatch/d' \
| sed '/shakescreen/d;/rand16/d;/debugscore/d;/pfscore/d;/noscore/d;/vblank_bB_code/d;/PFcolorandheight/d;/pfrowheight/d;/pfres/d;/PFmaskvalue/d;/overscan_time/d;/vblank_time/d;/no_blank_lines/d;/superchip/d;/ROM2k/d;/NO_ILLEGAL_OPCODES/d;/minikernel/d;/debugcycles/d;/mincycles/d;/legacy/d;/PFcolors/d;/playercolors/d;/player1colors/d;/backgroundchange/d;/readpaddle/d;/multisprite/d;/PFheights/d;/bankswitch/d;/Unresolved Symbols/d' \
| sed '2,/-->/!{ /-->/,/-->/d; }' \
| sed 's/--> 0./Possible duplicate label: /'

[ "$?" -ne "0" ] && { echo "Assembly failed."; exit; }

echo "Build complete."

[ -f "$z26" ] && "$z26" "$1.bin"
[ -f "$Stella" ] && "$Stella" "$1.bin"

exit
