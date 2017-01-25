; every bank has this stuff at the same place
; this code can switch to/from any bank at any entry point
; and can preserve register values
; note: lines not starting with a space are not placed in all banks
;
; line below tells the compiler how long this is - do not remove
;size=32
begin_bscode
 ldx #$ff
 txs
 lda #>(start-1)
 pha
 lda #<(start-1)
 pha
BS_return
 pha
 txa
 pha
 tsx
 lda 4,x ; get high byte of return address
 rol   
 rol
 rol
 rol
 and #bs_mask ;1 3 or 7 for F8/F6/F4
 tax
 inx
BS_jsr
 lda bankswitch_hotspot-1,x
 pla
 tax
 pla
 rts
 if ((* & $1FFF) > ((bankswitch_hotspot & $1FFF) - 1))
   echo "WARNING: size parameter in banksw.asm too small - the program probably will not work."
   echo "Change to",[(*-begin_bscode+1)&$FF]d,"and try again."
 endif
