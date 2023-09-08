# WebRTC AEC3 fully working demo using Qt6 Audio
Tested on macbook

It uses AEC3 [Extracted From WebRTC](https://github.com/ewan-xu/AEC3)

![demo](https://github.com/khomin/electron_camera_ffmpeg/blob/master/demo.png)

![no AEC](https://github.com/khomin/electron_camera_ffmpeg/blob/main/demo/wave1.png)
![with AEC](https://github.com/khomin/electron_camera_ffmpeg/blob/main/demo/wave2.png)

How to use
```bash
git clone https://github.com/khomin/AEC3_cmake.git --recurse-submodules
cd ./AEC3_cmake
mkdir build && cd ./build

cmake ../
make -j4
./aec3_qt6 
```

Expected log
```
starting...
IoAudio: start
WARNING: QApplication was not created in the main() thread.
IoAudio: input device: MacBook Pro Microphone
IoAudio: out device: MacBook Pro Speakers
IoAudio: sink size: 8192
IoAudio: source size: 4096
```

In case you see:
 'Could not find a package configuration file provided by "QT" with any of'
 
Add prefix path to your Qt before cmake ../
```
export CMAKE_PREFIX_PATH=~/Qt/6.5.2/macos
```

## License
MIT