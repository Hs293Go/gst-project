# GST Project
A repo to collect my own *hand-rolled* C-based Gstreamer applications 

# Building
CMake is preferred to build these applications. Run:
```
mkdir build && cd build
cmake ..
make
```

# Dependencies
The [{fmt}](https://github.com/fmtlib/fmt) library is used for most string formatting and printing in anticipation of `std::format`'s inclusion in C++20. Install with:
```
$ git clone https://github.com/fmtlib/fmt.git
$ cd fmt
$ mkdir build && cd build
$ cmake .. 
$ sudo make install
```

Boost program options is used for argument parsing. Install with:
```
$ sudo apt-get install libboost-program-options-dev
```
# Projects
## gst_stream_udp
The first project in progress so far...

Stream video from another device (e.g. Raspberry Pi / Jetson Nano), encoded in H264 format, over UDP to a host. This application admits the following arguments.

```
Simple gstreamer launcher for Jetson Nano:
  --help                      Display help message
  --use-usb-camera            Set this option to use USB camera
  --use-nvmm                  Set this option to use DMA buffer on Jetson Nano
  -w [ --width ] arg (=640)   The width of the image in pixels
  -h [ --height ] arg (=480)  The height of the image in pixels
  --device arg (=/dev/video0) Device location
  --host arg (=127.0.0.1)     The host/IP/Multicast group to send the packets 
                              to
  --port arg (=8786)          The port to send the packets to 
```

An example of the pipeline to play the video on the other end is
```
$ gst-launch-1.0 -v udpsrc port=8787 ! application/x-rtp, encoding-name=H264, payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! autovideosink
```