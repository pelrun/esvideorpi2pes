#!/bin/bash

FPS=25
BITRATE=3300000
let BITRATE2=BITRATE*115/100

mkfifo camera.h264
mkfifo camera.pes
mkfifo camera.ts
mkfifo muxed.ts
mkfifo stamped.ts

raspivid -n -w 960 -h 720 -b $BITRATE -t 0 -fps $FPS -g 90 -pf high -ih -o camera.h264 &
esvideorpi2pes camera.h264 $FPS > camera.pes &
pesvideo2ts 2064 $FPS 112 $BITRATE2 0 camera.pes > camera.ts &
tscbrmuxer b:$BITRATE camera.ts b:3008 pat.ts b:3008 pmt.ts b:1500 sdt.ts b:1400 nit.ts o:6635000 null.ts > muxed.ts &
tsstamp muxed.ts 6635000 > stamped.ts &
tsrfsend stamped.ts 0 446500 7000 4 2/3 1/4 8 0 0
