#!/bin/bash

# video parameters
FPS=25
BITRATE=3300000

# output bitrates must always be higher than input bitrates
# add 15% headroom to prevent source video buffer overflow
let BITRATE2=BITRATE*115/100

# transmitter parameters
# used to calculate the channel bitrate
MODULATION="7000 4 2/3 1/4"

# set our final stream bitrate to be 95% of the
# transmitter channel bitrate
let TXBITRATE=$(./capacity.py $MODULATION)*95/100

# create all the fifos
mkfifo ts/camera.h264
mkfifo ts/camera.pes
mkfifo ts/camera.ts
mkfifo ts/muxed.ts
mkfifo ts/stamped.ts

# generate the dvb-t program information
# can set network name and channel name like this:
#./psi-config.py "RasPi Network" "RasPi 1"
./psi-config.py

# generate h.264 annex B video
raspivid -n -w 960 -h 720 -b $BITRATE -t 0 -fps $FPS -g 12 -pf high -ih -o ts/camera.h264 &

# wrap h.264 in an mpeg-pes stream
esvideorpi2pes ts/camera.h264 $FPS > ts/camera.pes &

# wrap pes video in mpeg-ts stream
pesvideo2ts 2064 $FPS 112 $BITRATE2 0 ts/camera.pes > ts/camera.ts &

# mux video and program info, and pad to CBR stream at tx bitrate
tscbrmuxer b:$BITRATE2 ts/camera.ts b:3008 ts/pat.ts b:3008 ts/pmt.ts b:1500 ts/sdt.ts b:1400 ts/nit.ts o:$TXBITRATE null.ts > ts/muxed.ts &

# recalculate the timestamps to be consistent and monotonic
tsstamp ts/muxed.ts $TXBITRATE > ts/stamped.ts &

# send the stream to the modulator
tsrfsend ts/stamped.ts 0 446500 $MODULATION 8 0 0 

