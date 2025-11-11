; Provided under the CC0 license. See the included LICENSE.txt for details.

 processor 6502
 include "vcs.h"
 include "macro.h"
 ; NOTE: Do not include 2600basic.h here; multisprite.h provides its own TIA alias map.
 ; Including both headers causes DASM EQU redefinition failures.
 include "multisprite.h"
 include "superchip.h"
 include "2600basic_variable_redefs.h"
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
     ORG $1000
     RORG $1000
  endif
 else
   ORG $F000
 endif
 repeat 256
 .byte $ff
 repend
