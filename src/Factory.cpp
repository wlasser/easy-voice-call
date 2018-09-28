#include "evc/Factory.hpp"
#if 0
#include "evc/Alsa.hpp"


AudioDevice &Factory::create() {
    return (AudioDevice &)(Alsa::get());
}

#endif

// use PortAudio backend
#include "evc/PortAudio.hpp"
AudioDevice &Factory::create() {
    return (AudioDevice &)(PortAudio::get());
}

#include "evc/OpusEnc.hpp"
AudioEncoder &Factory::createAudioEncoder() {
    return (AudioEncoder &)(OpusEnc::get());
}

#include "evc/OpusDec.hpp"
AudioDecoder &Factory::createAudioDecoder() {
    return (AudioDecoder &)(OpusDec::get());
}
