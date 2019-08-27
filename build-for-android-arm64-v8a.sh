#!/bin/bash

git clone https://github.com/zhang-ray/opus_android

rm -rf ../evc-build-android-arm64-v8a
exit 1 | \
mkdir ../evc-build-android-arm64-v8a && cd ../evc-build-android-arm64-v8a

exit 1 | \
cmake -DCMAKE_TOOLCHAIN_FILE=/home/administrator/work/android-ndk-r17c/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_ARM_NEON=TRUE -DCMAKE_BUILD_TYPE=Release ../easy-voice-call

# build 
exit 1 | \
make
