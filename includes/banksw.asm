; Provided under the CC0 license. See the included LICENSE.txt for details.

; every bank has this stuff at the same place
; this code can switch to/from any bank at any entry point
; and can preserve register values
; note: lines not starting with a space are not placed in all banks
;
; line below tells the compiler how long this is - do not remove
;size=48  (actual size for 64kSC bankswitching with bank encoding)

begin_bscode
          ldx #$ff
          ifconst FASTFETCH ; using DPC+
          stx FASTFETCH
          endif 
          txs
          if bankswitch == 64
          lda #(>(start-1) & $0F)
          ifconst current_bank
          ora #(current_bank << 4)
          else
          ora #$F0
          endif
          else
          lda #>(start-1)
          endif
          pha
          lda #<(start-1)
          pha
BS_return
          pha
          txa
          pha
          tsx

          if bankswitch != 64
          lda 4,x ; get high byte of return address

          rol
          rol
          rol
          rol
          and #bs_mask ;1 3 or 7 for F8/F6/F4
          tax
          inx
          else
          lda 4,x ; get encoded high byte of return address from stack
          tay ; save encoded byte for restoration
          and #$F0 ; extract bank number from high nibble
          lsr ; shift right once
          lsr ; shift right twice
          lsr ; shift right three times
          lsr ; shift right four times - bank now in low nibble
          pha ; save bank number temporarily
          tya ; get saved encoded byte
          and #$0F ; mask low nibble with original address info
          ora #$F0 ; restore to $Fx format
          sta 4,x ; store restored address back to stack (X still has stack pointer)
          pla ; restore bank number
          tax ; bank number (0-F) now in X
          inx ; convert to 1-based index (bank 0 -> 1, bank 1 -> 2, etc.)
          endif
BS_jsr
          lda bankswitch_hotspot-1,x
          pla
          tax
          pla
          rts
          if ((. & $1FFF) > ((bankswitch_hotspot & $1FFF) - 1))
          echo "WARNING: size parameter in banksw.asm too small - the program probably will not work."
          echo "Change to ", [. - start_bank1]d, " and try again."
          endif
