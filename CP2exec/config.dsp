;rem file C:\Develop\CP2\CP2exec\config.dsp
;outpath          c:\data\
;outfilename     record.dat
ethernet        0xFFFFFFFF   ; ethernet address
freq            9240e6 ;stalo frequency (Hz)
;freq 0
velsign         1      ;use 1 if stalo < xmit, -1 if stalo > xmit
timingmode      0      ;0 = continuous, 1 = triggered, 2 = sync, 
;timingmode      1      ;0 = continuous, 1 = triggered, 2 = sync, 
delay           1    ; (no effect in mode 0) delay to first sample (special case: timingmode 2)
;number of range gates:  
;gates           150  ; CP2: LOTS of hits per PCI Bus transfer
;gates           1900  ;MAX for CP2?
gates            998  ;
;gates           850  ; 
;gates           660  ; CP2: gets 10 hits/packet -- easy network diagnosis
;gates           300  ;
;gates	20 ; 
;these dwells work at prf=3200Hz:
;hits           45  ;pulse pair integration (hits per dwell)
;hits            64  ;pulse pair integration (hits per dwell)
;hits             90  ;pulse pair integration (hits per dwell)
;hits            100   ; 30b/s @ 3.3333KHz
;hits            100   ; 20b/s @ 2KHz
;hits             50   ; 40b/s @ 2KHz
;hits             20   ; 100b/s @ 2KHz
hits             10   ; 100b/s @ 1KHz
;hits             48   ; ?b/s @ 2KHz
;hits             60   ; 
;hits             10   ; 100b/s @ 1KHz
;hits              8   ; 125b/s @ 1KHz
;hits              10   ; 100b/s @ 1KHz
;hits              20   ; 50b/s @ 1KHz
;hits             40   ; 50b/s @ 2KHz
;hits             48   ; 20b/s w/ 800Hz/1200Hz
;hits            120   ; 20b/s w/ 2000Hz/3000Hz
;hits             80   ; 30b/s w/ 2000Hz/3000Hz
;hits             48   ; 50b/s w/ 2000Hz/3000Hz
;hits             40   ; 100b/s @ 4KHz
;hits             18   ; 133.3333b/s w/ 2000Hz/3000Hz 18=9+9 hits
;hits             24   ; 100b/s w/ 2000Hz/3000Hz 24=12+12 hits
;hits             24   ; 100b/s @ 2.4KHz 
;hits             12   ; 200b/s w/ 2000Hz/3000Hz
;hits             20   ; 100b/s @ 2KHz
;hits            128   ; 25b/s @ 3.2KHz
;hits            256   ; 
;hits             32   ; 100b/s @ 3.2KHz
;hits             24   ; 133.3333b/s @ 3.2KHz
;hits             16   ; 200b/s @ 3.2KHz
;hits             34  ; ?b/s @ 3.2KHz
;hits             48   ; 66.66667b/s @ 3.2KHz
;hits             60   ; 50b/s @ 3KHz
;hits             50   ; 1 full cycle go.exe test ts data
;hits             30   ; 100b/s @ 3KHz
;hits             100   ; non-integer b/s
;hits             24   ; 125b/s @ 3.2KHz
;hits             64   ; 50b/s @ 3.2KHz
;hits            200   ; low b/s
; pulsewidths entered in 6MHz counts ... compute gate length accordingly. 
;rcvr_pulsewidth   14     ;2333ns pulses (350m)
;rcvr_pulsewidth 12    ;2000ns pulses (300m)
;rcvr_pulsewidth   8     ;1333ns pulses (200m)
rcvr_pulsewidth   6     ;1000ns pulses (150m)
;rcvr_pulsewidth   2     ;333ns pulses (50m)
;rcvr_pulsewidth  1    ;167ns pulses (25m)
; !note: xmit_pulsewidth is currently set in config.rdr
;xmit_pulsewidth  1    ;166ns pulses (25m) ;
xmit_pulsewidth  6     ;1us pulses (150m) ;
;xmit_pulsewidth  2    ;333ns pulses (50m) ;
;6MHz timebase counts:  
;prt		1875	;3200 hz prf
;prt		2000	;3000 Hz
;prt		3000	;2000 Hz 
prt		6000	;1000 Hz 
;prt		5000	;1200Hz
;prt		4000	;1500Hz
;prt		12000	;500 Hz 
;prt		60000	;100 Hz 
;prt		10000	;600 Hz 
;prt2		2000	;3000 Hz
;prt		7500	;800 Hz 
;prt2		5000	;1200 Hz
;prt		12500	;480 Hz 
;prt		2700	;2222.222... Hz 
;prt2		1800	;3333.333... Hz
;prt		1800	;3333.333... Hz
;prt		3000	;2000 Hz 
;prt		2500	;2400 Hz 
;prt		5000	;1200 Hz 
;prt		1500	;4000 Hz 
;prt2		2000	;3000 Hz
;prt2		1714	;~3.5KHz

tpdelay         8     ; test pulse delay in 6MHz counts (line-up G0)
tpwidth         17    ; (6 X pulsewidth + 4) test pulse delay in 100nS counts

trigger         on
testpulse		on
;tpdelay         14     ; test pulse delay in 100nS counts (line-up G0)
;tpdelay        24     ; test pulse delay in 100nS counts (line-up G0)
;tpwidth         28    ; (6 X pulsewidth + 4) test pulse delay in 100nS counts
;tpwidth        24     ; for 50m gates
gate0mode       off     ;high rate sampling of gate 0 (on or off)
phasecorrect    off     ;remove gate0 phase from all gates (for magnetron)
;phasecorrect    off   ;remove gate0 phase from all gates (for magnetron)
;clutterfilter   1      ;clutter filter id number
clutterfilter   0      ;clutter filter OFF
;clutter_start	15     ;start gate of clutter filtered region 
;clutter_end	18     ;end gate of clutter filtered region 
;clutter_start	135     ;multiplier for increment added to PIRAQ PCI DMA packet size 
;clutter_end	128     ;multiplier for increment added to host applications: udp packet size, #gates
;clutter_start	1     ;multiplier for #hits moved per piraq hardware fifo half-full interrupt: cf_s*gates <= 400 
;clutter_end	10     ;multiplier for ? 
clutter_start	3     ;offset to linear test data ramp set by Piraq DSP Executable
clutter_end	10     ;multiplier for ? 
;clutter_start	0     ;disable multiplier for PIRAQ PCI DMA packet size 
;clutter_end	0     ;disable multiplier for host applications udp packet size
;timeseries      off   ;time series on or off (not working)
ts_start_gate	-1     ; (<0 disables)
;ts_start_gate	100     ; (<0 disables)
;ts_end_gate	100    ; (<0 disables)
;tsgate          10    ;gate where time series is sampled (<0 disables)
startgate	123    ; 1st valid data gate
afcgain         1e5    ;use positive sign if high voltage = > 60MHz IF
;afchigh        9450e6  ;highest afc frequency
;afclow         9440e6  ;lowest afc frequency
locktime        10.0
debug           off     ;turns on certain debug printouts
;debug           on	;turns on certain debug printouts
;dataformat      0      ; DATA_SIMPLEPP: I,Q timeseries
;dataformat      16    ; PIRAQ_ABPDATA: rapidDOW
;dataformat      17    ; PIRAQ_ABPDATA_STAGGER_PRT: rapidDOW
dataformat      18    ; PIRAQ_CP2_TIMESERIES: CP2

meters_to_first_gate	100.0	; 1551
gate_spacing_meters		20.0	; 


; for 48 MHz operation, the timing reference is based on a 6 MHz clock.
; This will be changed to the "normal" 8 MHz clock when the firmware is
; updated to divide by 6 or 8 -- now it only divides by 8. -- E.L. 12/02
