
#include <Hobgoblin/RigelNet/Packet_wrapper.hpp>
#include <Hobgoblin/RigelNet/Udp_server.hpp>

#include <cassert>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn
{

RN_UdpServer::RN_UdpServer(PZInteger size)
    : RN_Server<RN_UdpServer, detail::RN_UdpConnector>{RN_NodeType::UdpServer}
{
    _clients.reserve(static_cast<std::size_t>(size));
    for (PZInteger i = 0; i < size; i += 1) {
        _clients.emplace_back(_mySocket, _timeoutLimit, _passphrase, detail::EventFactory{SELF, i});
    }

    _mySocket.setBlocking(false);
}

RN_UdpServer::RN_UdpServer()
    : RN_UdpServer{1}
{
}

RN_UdpServer::RN_UdpServer(PZInteger size, std::uint16_t port, std::string passphrase)
    : RN_UdpServer{size} 
{
    start(port, std::move(passphrase));
}

RN_UdpServer::~RN_UdpServer() {
    // TODO
}

void RN_UdpServer::start(std::uint16_t localPort, std::string passphrase) {
    assert(_running == false);
    auto status = _mySocket.bind(localPort);
    assert(status == sf::Socket::Done); // TODO - Throw exception on failure
    _passphrase = std::move(passphrase);
    _running = true;
}

void RN_UdpServer::stop() {
    // TODO
}

bool RN_UdpServer::isRunning() const {
    return _running;
}

void RN_UdpServer::update(RN_UpdateMode mode) {
    if (!_running) {
        return;
    }

    if (!_eventQueue.empty()) {
        // TODO Error
    }

    switch (mode) {
    case RN_UpdateMode::Receive:
        updateReceive();
        break;

    case RN_UpdateMode::Send:
        updateSend();
        break;

    default:
        assert(0 && "Unreachable");
        break;
    }
}

// Client management:
// TODO getClinet

void RN_UdpServer::swapClients(PZInteger index1, PZInteger index2) {
    // TODO
}

// Utility:
int RN_UdpServer::getSenderIndex() const {
    return _senderIndex;
}

std::uint16_t RN_UdpServer::getLocalPort() const {
    return _mySocket.getLocalPort();
}

PZInteger RN_UdpServer::getSize() const {
    return static_cast<PZInteger>(_clients.size());
}

void RN_UdpServer::resize(PZInteger newSize) {
    // TODO
}

std::chrono::microseconds RN_UdpServer::getTimeoutLimit() const {
    return _timeoutLimit;
}

void RN_UdpServer::setTimeoutLimit(std::chrono::microseconds limit) {
    _timeoutLimit = limit;
}

const std::string& RN_UdpServer::getPassphrase() const {
    return _passphrase;
}

// Protected:
void RN_UdpServer::compose(int receiver, const void* data, std::size_t sizeInBytes) {
    // TODO Temp - Check if client is even connected!
    _clients[receiver].appendToNextOutgoingPacket(data, sizeInBytes);
}

// Private:

void RN_UdpServer::updateReceive() {
    detail::RN_PacketWrapper packetWrap;
    sf::IpAddress senderIp;
    std::uint16_t senderPort;

    while (_mySocket.receive(packetWrap.packet, senderIp, senderPort) == sf::Socket::Status::Done) {
        const int senderConnectorIndex = findConnector(senderIp, senderPort);

        if (senderConnectorIndex != -1) {
            _senderIndex = senderConnectorIndex;
            _clients[senderConnectorIndex].receivedPacket(packetWrap);
        }
        else {
            handlePacketFromUnknownSender(senderIp, senderPort, packetWrap);
        }

        packetWrap.packet.clear();
    }

    for (PZInteger i = 0; i < getSize(); i += 1) {
        auto& client = _clients[i];
        
        if (client.getStatus() == RN_ConnectorStatus::Connected) {
            client.sendAcks();
        }
        if (client.getStatus() != RN_ConnectorStatus::Disconnected) {
            _senderIndex = i;
            client.handleDataMessages(SELF);
        }
        if (client.getStatus() != RN_ConnectorStatus::Disconnected) {
            client.checkForTimeout();
        }
    }
    _senderIndex = -1;
}

void RN_UdpServer::updateSend() {
    for (auto& client : _clients) {
        if (client.getStatus() == RN_ConnectorStatus::Disconnected) {
            continue;
        }
        client.send(SELF);
    }
}

int RN_UdpServer::findConnector(sf::IpAddress addr, std::uint16_t port) const {
    for (int i = 0; i < getSize(); i += 1) {
        const auto& remote = getClient(i).getRemoteInfo();
        if (remote.port == port && remote.ipAddress == addr) {
            return i;
        }
    }
    return -1;
}

void RN_UdpServer::handlePacketFromUnknownSender(sf::IpAddress senderIp, std::uint16_t senderPort, detail::RN_PacketWrapper& packetWrap) {
    for (PZInteger i = 0; i < getSize(); i += 1) {
        auto& connector = _clients[i];
        if (connector.getStatus() == RN_ConnectorStatus::Disconnected) {
            connector.setClientIndex(i);
            if (!connector.tryAccept(senderIp, senderPort, packetWrap)) {
                // TODO Notify of error
            }
            return;
        }
    }
    // TODO Send disconnect message (no room left)
}

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
