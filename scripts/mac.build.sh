
# opus-1.1.2, same as Ubuntu 16.04
pwd
which curl
curl --version
curl -O https://archive.mozilla.org/pub/opus/opus-1.1.2.tar.gz
tar -zxf opus-1.1.2.tar.gz
cd opus-1.1.2
./configure
make -j32
make install
cd ..


# portaudio
brew install portaudio
brew install qt

mkdir ../easy-voice-call-build
cd ../easy-voice-call-build
cmake ../easy-voice-call
make -j3





####### make artifact
# TODO: try to auto find the macdeployqt path?
/usr/local/Cellar/qt/5.11.2/bin/macdeployqt client_qt5
cd client_qt5
mkdir Contents/MacOS
mv client_qt5 Contents/MacOS/EVC
rm -rf CMakeFiles
rm -rf client_qt5_autogen
rm -f  Makefile
rm -f  cmake_install.cmake
cd ..
mv client_qt5 EVC.app
npm install -g appdmg
appdmg ../easy-voice-call/scripts/appdmg/spec.json EVC.dmg