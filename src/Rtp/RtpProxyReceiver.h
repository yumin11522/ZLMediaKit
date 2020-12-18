//
// Created by xiong on 2020/12/17.
//

#ifndef ZLMEDIAKIT_RTPPROXY_H
#define ZLMEDIAKIT_RTPPROXY_H

#include <Extension/H264Rtp.h>
#include "Rtsp/RtpReceiver.h"
#include "Common/MultiMediaSourceMuxer.h"

using namespace mediakit;

namespace mediakit {

class RtpProxyReceiver : public RtpReceiver {
public:
    typedef std::shared_ptr<RtpProxyReceiver> Ptr;

    RtpProxyReceiver(string app, string stream);

    ~RtpProxyReceiver() {};

    bool inputRtp(const char *data, int data_len);

    bool alive();

    int totalReaderCount();

    void setListener(const std::weak_ptr<MediaSourceEvent> &listener);

protected:
    void onRtpSorted(const RtpPacket::Ptr &rtp, int track_index) override;

private:
    SdpTrack::Ptr _track;
    uint16_t _sequence = 0;
    MultiMediaSourceMuxer::Ptr _muxer;
    Ticker _last_rtp_time;
    uint32_t _dts = 0;
    H264RtpDecoder::Ptr _h264RtpDecoder;
    VideoTrack::Ptr _videoTrack;
};
}

#endif //ZLMEDIAKIT_RTPPROXY_H
