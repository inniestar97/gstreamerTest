# BUILD

```bash
mkdir build && cd build && cmake .. && make
```

# PLAY

```bash
gst-launch-1.0 filesrc location="./Meantime-spacehog.mp4" ! decodebin name=dmux \
    dmux. ! queue ! audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=2,layout=interleaved,rate=44100 ! udpsink host=127.0.0.1 port=12121 \
	dmux. ! queue ! x264enc ! video/x-h264,stream-format=byte-stream ! udpsink host=127.0.0.1 port=13131 
```

In build file,

```bash
./gstreamerTest
```