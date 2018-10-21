#include "Worker.hpp"


#include "evc/Factory.hpp"
#include "evc/TcpClient.hpp"
#include "evc/AudioVolume.hpp"


const char *constPort ="1222";

Worker::~Worker(){
    try{
        syncStop();
    }
    catch(std::exception &e){
        qDebug() << e.what();
    }
}

bool Worker::initCodec(){
    decoder = &(Factory::get().createAudioDecoder());
    encoder = &(Factory::get().createAudioEncoder());

    if (encoder->reInit()){
        if (decoder->reInit()) {
            return true;
        }
    }
    return false;
}

bool Worker::initDevice(std::function<void(const std::string &, const std::string &)> reportInfo,
                        std::function<void(const uint8_t)> reportMicVolume,
                        std::function<void(const uint8_t)> reportSpkVolume){
    device_ = &(Factory::get().create());

    std::string micInfo;
    std::string spkInfo;
    if (device_->init(micInfo, spkInfo)){
        reportInfo(micInfo, spkInfo);
        micVolumeReporter_ = reportMicVolume;
        spkVolumeReporter_ = reportSpkVolume;
        return true;
    }
    return false;
}

void Worker::asyncStart(const std::string &host, std::function<void (const NetworkState &, const std::string &)> toggleState){
    syncStop();
    netThread_.reset(new std::thread(std::bind(&Worker::syncStart, this, host, toggleState)));
}

void Worker::syncStart(const std::string &host,
                        std::function<void(const NetworkState &newState, const std::string &extraMessage)> toggleState
                          ){


    try{
        gotoStop_ = false;
        bool isLogin = false;
        TcpClient client(
                    host.c_str(),
                    constPort,
                    [&](TcpClient *_TcpClient, const NetPacket& netPacket){
            // on Received Data
            switch (netPacket.payloadType()){
                case NetPacket::PayloadType::HeartBeatRequest:{
                    _TcpClient->send(NetPacket(NetPacket::PayloadType::HeartBeatResponse));
                    break;
                }
                case NetPacket::PayloadType::LoginResponse: {
                    isLogin = true;
                    qDebug() << "&isLogin=" << &isLogin << "\t" << __FUNCTION__;
                    break;
                }
                case NetPacket::PayloadType::AudioMessage: {
                    std::vector<char> netBuff;
                    netBuff.resize(netPacket.payloadLength());
                    memcpy(netBuff.data(), netPacket.payload(), netPacket.payloadLength());
                    std::vector<short> decodedPcm;
                    decoder->decode(netBuff, decodedPcm);
                    auto ret = device_->write(decodedPcm);
                    if (spkVolumeReporter_){
                        static SuckAudioVolume sav;
                        spkVolumeReporter_(sav.calculate(decodedPcm));
                    }
                    if (!ret) {
                        std::cout << ret.message() << std::endl;
                    }
                    break;
                }
            }
        });


        /// TCP/UDP connecting phase
        /// timeout: 100ms
        {
            //
            bool isOK = false;
            for (int i = 0; i < 10; i++){
                if (client.isConnected()){
                    isOK = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (!isOK) {
                toggleState(NetworkState::Disconnected, "Could not connect to Server...");
                return;
            }
        }


        /// App Login phase
        /// timeout: 300ms
        {
            client.send(NetPacket(NetPacket::PayloadType::LoginRequest));
            for (int i = 0; i < 10; i++){
                if (isLogin){
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }

        qDebug() << "&isLogin=" << &isLogin << "\t" << __FUNCTION__;
        if (!isLogin){
            toggleState(NetworkState::Disconnected, "Could not Login to Server...");
            return;
        }

        toggleState(NetworkState::Connected, "");

        // sending work
        for (;!gotoStop_;){
            const auto blockSize = 1920;
            std::vector<short> micBuffer(blockSize);
            /// TODO:
            /// in practice, device_->read would be unblock in first several blocks
            /// so let's clear microphone's buffer on first time???
            auto ret = device_->read(micBuffer);
            if (!ret){
                break;
            }
            if (micVolumeReporter_){
                static SuckAudioVolume sav;
                micVolumeReporter_(sav.calculate(micBuffer));
            }

            std::vector<char> outData;
            auto retEncode = encoder->encode(micBuffer, outData);
            if (!retEncode){
                std::cout << retEncode.message() << std::endl;
                break;
            }

            client.send(NetPacket(NetPacket::PayloadType::AudioMessage, outData));



            // send heartbeat
            {
                static auto lastTimeStamp = std::chrono::system_clock::now();
                auto now = std::chrono::system_clock::now();
                auto elapsed = now - lastTimeStamp;
                if (elapsed > std::chrono::seconds(10)){
                    client.send(NetPacket(NetPacket::PayloadType::HeartBeatRequest));
                    lastTimeStamp = std::chrono::system_clock::now();
                }
            }
        }


        /// App logout
        client.send(NetPacket(NetPacket::PayloadType::LogoutRequest));


    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    //        emit updateUiState(NetworkState::Disconnected);

    /* ... here is the expensive or blocking operation ... */
    //        emit resultReady(result);

}
