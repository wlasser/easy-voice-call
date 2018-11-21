#include "DownstreamProcessor.hpp"
#include "AudioCommon.hpp"

#include "../../audio-processing-module/independent_aec/src/echo_cancellation.h"
#include "Profiler.hpp"

#include "PacketLossConcealment_ZeroInsertion.hpp"
#include <memory>


void DownstreamProcessor::decodeOpusAndAecBufferFarend(const std::shared_ptr<NetPacket> netPacket, std::vector<short> &decodedPcm)
{
    std::vector<char> netBuff;
    netBuff.resize(netPacket->payloadLength());
    memcpy(netBuff.data(), netPacket->payload(), netPacket->payloadLength());
    decoder_->decode(netBuff, decodedPcm);

    if (needAec_) {
        std::vector<float> floatFarend(decodedPcm.size());
        for (auto i = 0u; i < decodedPcm.size(); i++) {
            floatFarend[i] = ((float)decodedPcm[i]) / (1 << 15);
        }
        for (int i = 0; i < blockSize; i += blockSize) {
            auto ret = webrtc::WebRtcAec_BufferFarend(aec_, &(floatFarend[i]), 160);
            if (ret) {
                throw;
            }
        }
    }


    {
        /// RUNTIME VERIFICATION
        /// UNEXPECTED: descending order
        auto currentTS = netPacket->timestamp();
        if (currentTS < largestReceivedMediaDataTimestamp_) {
            // unexpected data order!!!!
            LOGE << "currentTS < largestReceivedMediaDataTimestamp_";
            // TODO: AND THEN? THROW?
            throw;
        }
        else {
            largestReceivedMediaDataTimestamp_ = currentTS;
        }
    }

    {
        /// RUNTIME VERIFICATION
        /// SN 
        auto currentSn = netPacket->serialNumber();
        if (!lastReceivedAudioSn_) {
            if (lastReceivedAudioSn_ + 1 != currentSn) {
                LOGE << "lastReceivedAudioSn_ + 1 != currentSn";
                LOGE << "lastReceivedAudioSn_=" << lastReceivedAudioSn_;
                LOGE << "currentSn=" << currentSn;
            }
        }
        lastReceivedAudioSn_ = currentSn;
    }
}

DownstreamProcessor::DownstreamProcessor(bool needAec, void *aec, const std::string &audioOutDumpPath)
    : decoder_(&AudioDecoder::create())
    , needAec_(needAec)
    , aec_(aec)
{
    if (decoder_) {
        decoder_->reInit();
    }

    if (!audioOutDumpPath.empty()) {
        dumpMono16le16kHzPcmFile_ = std::make_shared<std::ofstream>(audioOutDumpPath, std::ofstream::binary /*don't miss std::ofstream::binary*/);
        if (!dumpMono16le16kHzPcmFile_->is_open()) {
            dumpMono16le16kHzPcmFile_ = nullptr;
        }
    }
}

DownstreamProcessor::~DownstreamProcessor()
{
    if (dumpMono16le16kHzPcmFile_) {
        if (dumpMono16le16kHzPcmFile_->is_open()) {
            dumpMono16le16kHzPcmFile_->close();
        }
    }
}

void DownstreamProcessor::append(const std::shared_ptr<NetPacket> &netPacket){
    std::vector<short> pcm;
    decodeOpusAndAecBufferFarend(netPacket, pcm);
    auto segment = std::make_shared<PcmSegment>();
    std::memcpy(segment->data(), pcm.data(), blockSize * sizeof(int16_t));
    jitterBuffer_.append(std::make_shared<std::tuple<uint32_t, std::shared_ptr<PcmSegment>>>(std::make_tuple<uint32_t, std::shared_ptr<PcmSegment>>( netPacket->timestamp(), std::move(segment ))));
}

void DownstreamProcessor::fetch(int16_t * const outData){
    auto data = jitterBuffer_.fetch();

    if (data == nullptr) {
        auto predicted = std::make_shared<PcmSegment>();
        if (m1_&&m2_) {
            PacketLossConcealment_ZeroInsertion plc;
            std::vector<std::shared_ptr<PcmSegment>> theList;
            theList.push_back(std::get<1>(*(m1_.get())));
            theList.push_back(std::get<1>(*(m2_.get())));
            plc.predict(theList, predicted);
        }

        data = std::make_shared<std::tuple<uint32_t, std::shared_ptr<PcmSegment>>>(
            std::make_tuple<uint32_t, std::shared_ptr<PcmSegment>>(0, std::move(predicted))
            );
    }

    m2_ = m1_;
    m1_ = data;




    // TODO: fade out for the fragment after ZeroInsertion

    if (m2_) {
        memcpy(outData, std::get<1>(*m2_)->data(), blockSize * sizeof(int16_t));
    }
    else {
        memset(outData, 0, blockSize * sizeof(int16_t));
    }


    if (dumpMono16le16kHzPcmFile_) {
        dumpMono16le16kHzPcmFile_->write((char*)outData, sizeof(int16_t)*blockSize);
    }

    Profiler::get().audioOutBufferSize_.addData(jitterBuffer_.read_available());
}