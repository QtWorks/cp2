;rem This is the infamous config.dsp
;outpath         c:\data\
;outfilename     record.dat
ethernet        0xFFFFFFFF   ; ethernet address
freq            9240e6  ;stalo frequency (Hz)
velsign         1       ;use 1 if stalo < xmit, -1 if stalo > xmit
timingmode      0       ;0 = continuous, 1 = triggered, 2 = sync, 
delay           1       ; (no effect in mode 0) delay to first sample (special case: timingmode 2)
gates           950     ;
hits            10      ; 100b/s @ 1KHz
rcvr_pulsewidth 6       ;
xmit_pulsewidth 6       ;1us pulses (150m) ;
prt		6000	;1000 Hz 
;prt2		2000	;3000 Hz
tpdelay         8       ; test pulse delay in 6MHz counts (line-up G0)
tpwidth         17      ; (6 X pulsewidth + 4) test pulse delay in 100nS counts
trigger         on
testpulse	on
gate0mode       off     ;high rate sampling of gate 0 (on or off)
phasecorrect    off     ;remove gate0 phase from all gates (for magnetron)
;phasecorrect   off     ;remove gate0 phase from all gates (for magnetron)
;clutterfilter  1       ;clutter filter id number
clutterfilter   0       ;clutter filter OFF
clutter_start	3       ;offset to linear test data ramp set by Piraq DSP Executable
clutter_end	10      ;multiplier for ? 
;timeseries     off     ;time series on or off (not working)
ts_start_gate	-1      ; (<0 disables)
;ts_end_gate	100     ; (<0 disables)
;tsgate          10     ;gate where time series is sampled (<0 disables)
startgate	123     ; 1st valid data gate
afcgain         1e5     ;use positive sign if high voltage = > 60MHz IF
;afchigh        9450e6  ;highest afc frequency
;afclow         9440e6  ;lowest afc frequency
locktime        10.0
debug           off      ;turns on certain debug printouts
dataformat      18      ; PIRAQ_CP2_TIMESERIES: CP2
meters_to_first_gate	100.0	; 1551
gate_spacing_meters	20.0	; 


; for 48 MHz operation, the timing reference is based on a 6 MHz clock.
; This will be changed to the "normal" 8 MHz clock when the firmware is
; updated to divide by 6 or 8 -- now it only divides by 8. -- E.L. 12/02
