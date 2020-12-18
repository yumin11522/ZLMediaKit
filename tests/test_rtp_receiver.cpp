//
// Created by xiong on 2020/12/17.
//

#include <Rtsp/Rtsp.h>
#include <signal.h>
#include <Rtp/RtpProxyReceiver.h>
#include "Util/logger.h"
#include "Rtsp/UDPServer.h"
#include "Network/TcpServer.h"
#include "Common/config.h"
#include "Rtsp/RtspSession.h"
#include "Rtmp/RtmpSession.h"
#include "Http/HttpSession.h"

using namespace std;
using namespace toolkit;
using namespace mediakit;

bool static getSSRC(const char *data,int data_len, uint32_t &ssrc){
    if (data_len < 12) {
        return false;
    }
    uint32_t *ssrc_ptr = (uint32_t *) (data + 8);
    ssrc = ntohl(*ssrc_ptr);
    return true;
}

int main(int argc, char *argv[]) {
    //设置日志
    Logger::Instance().add(std::make_shared<ConsoleChannel>("ConsoleChannel"));
    //启动异步日志线程
    Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());
    loadIniConfig((exeDir() + "config.ini").data());
    TcpServer::Ptr rtspSrv(new TcpServer());
    TcpServer::Ptr rtmpSrv(new TcpServer());
    TcpServer::Ptr httpSrv(new TcpServer());
    rtspSrv->start<RtspSession>(554);//默认554
    rtmpSrv->start<RtmpSession>(1935);//默认1935
    httpSrv->start<HttpSession>(80);//默认80
    RtpProxyReceiver::Ptr rtpProxyReceiver = make_shared<RtpProxyReceiver>("test", "h264");

    //创建udp服务器
    Socket::Ptr udp_server = Socket::createSocket(nullptr, true);
    auto local_ip = "127.0.0.1";
    auto local_port = 5006;
    if (local_port == 0) {
        //随机端口，rtp端口采用偶数
        Socket::Ptr rtcp_server = Socket::createSocket(nullptr, true);
        auto pair = std::make_pair(udp_server, rtcp_server);
        makeSockPair(pair, local_ip);
        //取偶数端口
        udp_server = pair.first;
    } else if (!udp_server->bindUdpSock(local_port, local_ip)) {
        //用户指定端口
        throw std::runtime_error(StrPrinter << "bindUdpSock on " << local_ip << ":" << local_port << " failed:" << get_uv_errmsg(true));
    }

    //设置udp socket读缓存
    SockUtil::setRecvBuf(udp_server->rawFD(), 4 * 1024 * 1024);
    udp_server->setOnRead([udp_server, rtpProxyReceiver](const Buffer::Ptr &buf, struct sockaddr *addr, int) {
        uint32_t ssrc;
        getSSRC(buf->data(), buf->size(), ssrc);
        if (ssrc == 1234) {
            DebugL << "skip ssrc " << ssrc;
            return;
        }
        rtpProxyReceiver->inputRtp(buf->data(), buf->size());
    });

    //设置退出信号处理函数
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
}