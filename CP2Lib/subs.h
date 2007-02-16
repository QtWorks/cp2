#pragma once

#include "config.h"

#define C       2.99792458E8
#define M_PI    3.141592654 
#define TWOPI   6.283185307

/* the bits of the status register */
#define STAT0_SW_RESET    0x0001
#define STAT0_TRESET      0x0002
#define STAT0_TMODE       0x0004
#define STAT0_DELAY_SIGN  0x0008
#define STAT0_EVEN_TRIG   0x0010
#define STAT0_ODD_TRIG    0x0020
#define STAT0_EVEN_TP     0x0040
#define STAT0_ODD_TP      0x0080
#define STAT0_PCLED       0x0100
#define STAT0_GLEN0       0x0200
#define STAT0_GLEN1       0x0400
#define STAT0_GLEN2       0x0800
#define STAT0_GLEN3       0x1000

#define STAT1_PLL_CLOCK   0x0001
#define STAT1_PLL_LE      0x0002
#define STAT1_PLL_DATA    0x0004
#define STAT1_WATCHDOG    0x0008
#define STAT1_PCI_INT     0x0010
#define STAT1_EXTTRIGEN   0x0020
#define STAT1_FIRST_TRIG  0x0040
#define STAT1_PHASELOCK   0x0080
#define STAT1_SPARE0      0x0100
#define STAT1_SPARE1      0x0200
#define STAT1_SPARE2      0x0400
#define STAT1_SPARE3      0x0800


/* macro's for writing to the status register */
#define STATUS0(card,a)        (card->GetControl()->SetValue_StatusRegister0(a)) // *card->status0 = (a)
#define STATUSRD0(card,a)      (card->GetControl()->GetBit_StatusRegister0(a)) // (*card->status0 & (a))
#define STATSET0(card,a)       (card->GetControl()->SetBit_StatusRegister0(a))  // *card->status0 |= (a)
#define STATCLR0(card,a)       (card->GetControl()->UnSetBit_StatusRegister0(a)) // *card->status0 &= ~(a)
#define STATTOG0(card,a)       (if(STATUSRD0(card,a)) { STATCLR0(card,a);}else{STATSET0(card,a);}) // *card->status0 ^= (a)
#define STATUS1(card,a)        (card->GetControl()->SetValue_StatusRegister1(a)) // *card->status0 = (a)
#define STATUSRD1(card,a)      (card->GetControl()->GetBit_StatusRegister1(a)) // (*card->status0 & (a))
#define STATSET1(card,a)       (card->GetControl()->SetBit_StatusRegister1(a))  // *card->status0 |= (a)
#define STATCLR1(card,a)       (card->GetControl()->UnSetBit_StatusRegister1(a)) // *card->status0 &= ~(a)
#define STATTOG1(card,a)       if(STATUSRD0(card,a)) { STATCLR1(card,a);}else{STATSET1(card,a);} // *card->status0 ^= (a)
#define STATPLL(card,a)        STATUS1(card,STATUSRD1(card,0xFFF8) |  (a))


/* FIR filter register definitions */
#define	APATH_REG0		0x00
#define	APATH_REG1		0x01
#define	BPATH_REG0		0x02
#define	BPATH_REG1		0x03
#define	CASCADE_REG	0x04
#define	COUNTER_REG	0x05
#define	GAIN_REG		0x06
#define	OUTPUT_REG	0x07
#define	SNAP_REGA		0x08
#define	SNAP_REGB		0x09
#define	SNAP_REGC		0x0A
#define	ONE_SHOT		0x0B
#define	NEW_MODES		0x0C
#define	PATHA_COEFF	0x80
#define	PATHB_COEFF	0xC0

