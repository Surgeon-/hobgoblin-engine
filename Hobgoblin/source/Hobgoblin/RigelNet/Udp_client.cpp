
#include <Hobgoblin/RigelNet/Udp_client.hpp>

#include <cassert>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

RN_UdpClient::RN_UdpClient()
    : RN_Node{RN_NodeType::UdpClient}
    , _connector{_mySocket, _passphrase}
{
    _mySocket.setBlocking(false);
}

RN_UdpClient::RN_UdpClient(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort, std::string passphrase)
    : RN_UdpClient()
{
    connect(localPort, serverIp, serverPort, std::move(passphrase));
}

RN_UdpClient::~RN_UdpClient() {
    // TODO
}

void RN_UdpClient::connect(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort, std::string passphrase) {
    auto status = _mySocket.bind(localPort);
    assert(status == sf::Socket::Done); // TODO - Throw exception on failure
    _passphrase = std::move(passphrase);
    _connector.connect(serverIp, serverPort);
    _running = true;
}

void RN_UdpClient::disconnect() {
    // TODO
    _connector.disconnect(false); // TODO [configurable]
    _running = false;
}

void RN_UdpClient::update(RN_UpdateMode mode) {
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

const RN_RemoteInfo& RN_UdpClient::getServerInfo() const {
    return _connector.getRemoteInfo();
}

RN_ConnectorStatus RN_UdpClient::getConnectorStatus() const {
    return _connector.getStatus();
}

PZInteger RN_UdpClient::getSendBufferSize() const {
    return _connector.getSendBufferSize();
}

PZInteger RN_UdpClient::getRecvBufferSize() const {
    return _connector.getRecvBufferSize();
}

// Protected

void RN_UdpClient::compose(int receiver, const void* data, std::size_t sizeInBytes) {
    // TODO Temp.
    _connector.appendToNextOutgoingPacket(data, sizeInBytes);
}

// Private

void RN_UdpClient::updateReceive() {
    RN_Packet packet;
    sf::IpAddress senderIp;
    std::uint16_t senderPort;

    while (_mySocket.receive(packet, senderIp, senderPort) == sf::Socket::Status::Done) {
        if (senderIp == _connector.getRemoteInfo().ipAddress
            && senderPort == _connector.getRemoteInfo().port) {
            _connector.receivedPacket(packet);
        }
        else {
            // handlePacketFromUnknownSender(senderIp, senderPort, packet); TODO
        }
        packet.clear();
    }

    if (_connector.getStatus() == RN_ConnectorStatus::Connected) {
        _connector.sendAcks();
    }
    if (_connector.getStatus() != RN_ConnectorStatus::Disconnected) {
        _connector.handleDataMessages(Self);
    }
    if (_connector.getStatus() != RN_ConnectorStatus::Disconnected) {
        _connector.checkForTimeout();
    }
}

void RN_UdpClient::updateSend() {
    _connector.send(Self);
}

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
