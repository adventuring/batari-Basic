; Provided under the CC0 license. See the included LICENSE.txt for details.

 processor 6502
 include "vcs.h"
 include "macro.h"
; multisprite.h and superchip.h definitions are provided by
; Source/Common/MultiSpriteSuperChip.s
 include "2600basic_variable_redefs.h"

; Bank boundary definitions for 64K SuperChip ROM (16 banks x 4K each)
; Each bank has 32 bytes reserved for bankswitching code at the end
BANK1_END = $0FC0
BANK2_END = $1FC0
BANK3_END = $2FC0
BANK4_END = $3FC0
BANK5_END = $4FC0
BANK6_END = $5FC0
BANK7_END = $6FC0
BANK8_END = $7FC0
BANK9_END = $8FC0
BANK10_END = $9FC0
BANK11_END = $AFC0
BANK12_END = $BFC0
BANK13_END = $CFC0
BANK14_END = $DFC0
BANK15_END = $EFC0
BANK16_END = $FFC0

 ifconst bankswitch
  if bankswitch == 8
     ORG $1000
     RORG $D000
  endif
  if bankswitch == 16
     ORG $1000
     RORG $9000
  endif
  if bankswitch == 32
     ORG $1000
     RORG $1000
  endif
  if bankswitch == 64
     ORG $0000
     RORG $F000
  endif
 else
   ORG $F000
 endif
 repeat 256
 .byte $ff
 repend
