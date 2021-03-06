#pragma once

#include <functional>
#include <memory>
#include "NetPacket.hpp"
#include "ReturnType.hpp"


class NetClient {
public:
    virtual ~NetClient(){}
    virtual ReturnType init(
        const std::string &serverHost,
        const std::string &serverPort,
        std::function<void(const NetClient &myClient, const std::shared_ptr<NetPacket> netPacket)> onDataReceived
    ) = 0;
    virtual ReturnType send(const NetPacket &netPacket) const = 0;
    virtual bool isConnected() const = 0;
};