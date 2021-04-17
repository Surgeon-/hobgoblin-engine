
#include "Udp_client_impl.hpp"

#include <cassert>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

RN_UdpClientImpl::RN_UdpClientImpl(std::string aPassphrase,
                                   RN_NetworkingStack aNetworkingStack)
    : _socket{RN_Protocol::UDP, aNetworkingStack}
    , _connector{_socket, _timeoutLimit, _passphrase, _retransmitPredicate, detail::EventFactory{_eventQueue}}
    , _passphrase{std::move(aPassphrase)}
    , _retransmitPredicate{DefaultRetransmitPredicate}
{
    _socket.init(65536);
}

RN_UdpClientImpl::~RN_UdpClientImpl() {
    // TODO
}

///////////////////////////////////////////////////////////////////////////
// CLIENT CONTROL                                                        //
///////////////////////////////////////////////////////////////////////////

void RN_UdpClientImpl::connect(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort) {
    assert(!_running || _connector.getStatus() == RN_ConnectorStatus::Disconnected);
    
    _socket.bind(sf::IpAddress::Any, localPort);
    _connector.connect(serverIp, serverPort);
    _running = true;
}

void RN_UdpClientImpl::disconnect(bool notifyRemote) {
    _connector.disconnect(notifyRemote);
    _running = false;
}

void RN_UdpClientImpl::setTimeoutLimit(std::chrono::microseconds limit) {
    _timeoutLimit = limit;
}

void RN_UdpClientImpl::update(RN_UpdateMode mode) {
    if (!_running) {
        return;
    }
    else if (_connector.getStatus() == RN_ConnectorStatus::Disconnected) {
        _running = false;
        return;
    }

    switch (mode) {
    case RN_UpdateMode::Receive:
        _updateReceive();
        break;

    case RN_UpdateMode::Send:
        _updateSend();
        break;

    default:
        assert(0 && "Unreachable");
        break;
    }
}

bool RN_UdpClientImpl::pollEvent(RN_Event& ev) {
    if (_eventQueue.empty()) {
        return false;
    }
    ev = _eventQueue.front();
    _eventQueue.pop_front();
    return true;
}

///////////////////////////////////////////////////////////////////////////
// STATE INSPECTION                                                      //
///////////////////////////////////////////////////////////////////////////

bool RN_UdpClientImpl::isRunning() const {
    return _running;
}

const std::string& RN_UdpClientImpl::getPassphrase() const {
    return _passphrase;
}

std::uint16_t RN_UdpClientImpl::getLocalPort() const {
    return _socket.getLocalPort();
}

std::chrono::microseconds RN_UdpClientImpl::getTimeoutLimit() const {
    return _timeoutLimit;
}

const RN_ConnectorInterface& RN_UdpClientImpl::getServerConnector() const {
    return _connector;
}
PZInteger RN_UdpClientImpl::getClientIndex() const {
    assert(_running && _connector.getStatus() == RN_ConnectorStatus::Connected);
    return *_connector.getClientIndex();
}


bool RN_UdpClientImpl::isServer() const noexcept {
    return false;
}

RN_Protocol RN_UdpClientImpl::getProtocol() const noexcept {
    return _socket.getProtocol();
}

RN_NetworkingStack RN_UdpClientImpl::getNetworkingStack() const noexcept {
    return _socket.getNetworkingStack();
}







void RN_UdpClientImpl::setRetransmitPredicate(RetransmitPredicate pred) {
    _retransmitPredicate = pred;
}

///////////////////////////////////////////////////////////////////////////
// PRIVATE IMPLEMENTATION                                                //
///////////////////////////////////////////////////////////////////////////

void RN_UdpClientImpl::_updateReceive() {
    detail::RN_PacketWrapper packetWrap;
    sf::IpAddress senderIp;
    std::uint16_t senderPort;

    while (_socket.recv(packetWrap.packet, senderIp, senderPort)) {
        if (senderIp == _connector.getRemoteInfo().ipAddress
            && senderPort == _connector.getRemoteInfo().port) {
            _connector.receivedPacket(packetWrap);
        }
        else {
            // handlePacketFromUnknownSender(senderIp, senderPort, packet); TODO
        }
        packetWrap.packet.clear();
    }

    if (_connector.getStatus() == RN_ConnectorStatus::Connected) {
        _connector.sendAcks();
    }
    if (_connector.getStatus() != RN_ConnectorStatus::Disconnected) {
        _connector.handleDataMessages(SELF, _currentPacket);
    }
    if (_connector.getStatus() != RN_ConnectorStatus::Disconnected) {
        _connector.checkForTimeout();
    }
}

void RN_UdpClientImpl::_updateSend() {
    _connector.send();
}

void RN_UdpClientImpl::_compose(int receiver, const void* data, std::size_t sizeInBytes) {
    _connector.appendToNextOutgoingPacket(data, sizeInBytes);
}

void RN_UdpClientImpl::_compose(RN_ComposeForAllType receiver, const void* data, std::size_t sizeInBytes) {
    _connector.appendToNextOutgoingPacket(data, sizeInBytes);
}

detail::RN_PacketWrapper* RN_UdpClientImpl::_getCurrentPacketWrapper() {
    return _currentPacket;
}

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
