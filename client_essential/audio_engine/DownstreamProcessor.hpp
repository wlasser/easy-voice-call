#pragma once

#include <memory>
#include "NetPacket.hpp"
#include "Logger.hpp"
#include "AudioDecoder.hpp"
#include "JitterBuffer.hpp"

//////////////////////////////////////////////////////////////////////////
// Decoder, Jitter buffer, Packet loss concealment
// 
// input: received AudioMessage(opus)
// output: audio endpoint
//////////////////////////////////////////////////////////////////////////
class DownstreamProcessor {
private:
    JitterBuffer jitterBuffer_;
    AudioDecoder *decoder_ = nullptr;
    bool needAec_ = false;
    uint32_t largestReceivedMediaDataTimestamp_ = 0;
    uint32_t lastReceivedAudioSn_ = 0;
    void *aec_ = nullptr;
    std::shared_ptr<std::ofstream> dumpMono16le16kHzPcmFile_ = nullptr;
private:
    void decodeOpusAndAecBufferFarend(const std::shared_ptr<NetPacket> netPacket, std::vector<short> &decodedPcm);
public:
    DownstreamProcessor(bool needAec, void *aec, const std::string &audioOutDumpPath);
    ~DownstreamProcessor();
    void append(const std::shared_ptr<NetPacket> &netPacket) { jitterBuffer_.append(netPacket);}
    void fetch(int16_t * const outData);
};