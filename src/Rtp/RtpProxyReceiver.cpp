//
// Created by xiong on 2020/12/17.
//

#include "RtpProxyReceiver.h"

namespace mediakit {

RtpProxyReceiver::RtpProxyReceiver(string app, string stream) {
    _track = std::make_shared<SdpTrack>();
    _track->_interleaved = 0;
    _track->_samplerate = 90000;
    _track->_type = TrackVideo;
    _muxer = std::make_shared<MultiMediaSourceMuxer>(DEFAULT_VHOST, app, stream, 0, true, true, true, true);
}

bool RtpProxyReceiver::inputRtp(const char *data, int data_len) {
    _last_rtp_time.resetTime();
    bool ret = handleOneRtp(0, _track->_type, _track->_samplerate, (unsigned char *) data, data_len);
    return ret;
}

void RtpProxyReceiver::onRtpSorted(const RtpPacket::Ptr &rtp, int) {
    if (rtp->sequence != _sequence + 1) {
        WarnL << rtp->sequence << " != " << _sequence << "+1";
    }
    _sequence = rtp->sequence;
    if (!_videoTrack) {
        _videoTrack = std::make_shared<H264Track>();
        _muxer->addTrack(_videoTrack);
    }
    if (!_h264RtpDecoder) {
        _h264RtpDecoder = make_shared<H264RtpDecoder>();
        _h264RtpDecoder->addDelegate(std::make_shared<FrameWriterInterfaceHelper>([&](const Frame::Ptr &frame) {
            _muxer->inputFrame(frame);
        }));

    }
    _h264RtpDecoder->inputRtp(rtp);
}

bool RtpProxyReceiver::alive() {
    if (_last_rtp_time.elapsedTime() / 1000 < 5) {
        return true;
    }
    return false;
}

int RtpProxyReceiver::totalReaderCount() {
    return _muxer->totalReaderCount();
}

void RtpProxyReceiver::setListener(const std::weak_ptr<MediaSourceEvent> &listener) {
    _muxer->setMediaListener(listener);
}

}