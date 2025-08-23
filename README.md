# WebRTC AEC3 fully working demo using Qt6 Audio
Tested on macbook (vmware windows/linux)

It uses slightly modified [AEC3 Extracted From WebRTC](https://github.com/ewan-xu/AEC3)

no AEC
![no AEC](https://github.com/khomin/webrtc_aec3_qt6_cmake/blob/main/demo/wave1.png)

with AEC
![with AEC](https://github.com/khomin/webrtc_aec3_qt6_cmake/blob/main/demo/wave2.png)

How to use
```bash
git clone https://github.com/khomin/webrtc_aec3_qt6_cmake.git --recurse-submodules
cd ./webrtc_aec3_qt6_cmake
mkdir build && cd ./build

cmake ../
make -j8
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

## Sponsorship
If you found it helpful, consider donating

Ethereum/Arbitrum  `0xD17F0EBf046ab5C0d5a92BDFddf24c38b526AD9F`

## License
MIT