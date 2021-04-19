
#include "Udp_server_impl.hpp"

#include <Hobgoblin/Utility/Exceptions.hpp>

#include <cassert>
#include <utility>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

RN_UdpServerImpl::RN_UdpServerImpl(std::string passphrase,
                                   PZInteger size,
                                   RN_NetworkingStack networkingStack)
    : _socket{RN_Protocol::UDP, networkingStack}
    , _passphrase{std::move(passphrase)}
    , _retransmitPredicate{DefaultRetransmitPredicate}
{
    _socket.init(65536); // TODO Magic number

    _clients.reserve(static_cast<std::size_t>(size));
    for (PZInteger i = 0; i < size; i += 1) {
        auto connector = std::make_unique<RN_UdpConnectorImpl>(
            _socket,
            _timeoutLimit,
            _passphrase,
            _retransmitPredicate,
            detail::EventFactory{_eventQueue, i});

        _clients.push_back(std::move(connector));
    }
}

RN_UdpServerImpl::~RN_UdpServerImpl() {
    stop();
}

///////////////////////////////////////////////////////////////////////////
// SERVER CONTROL                                                        //
///////////////////////////////////////////////////////////////////////////

void RN_UdpServerImpl::start(std::uint16_t localPort) {
    assert(_running == false);

    _socket.bind(sf::IpAddress::Any, localPort);

    _running = true;
}

void RN_UdpServerImpl::stop() {
    // TODO disconnect all connectors
    for (auto& client : _clients) {
        if (client->getStatus() != RN_ConnectorStatus::Disconnected) {
            client->disconnect(false); // TODO make configurable
        }
    }

    // Safe to call multiple times
    _socket.close();
}

void RN_UdpServerImpl::resize(PZInteger newSize) {
    if (newSize <= stopz(_clients.size())) {
        // TODO Add support for downsizing
        return;
    }

    PZInteger i = stopz(_clients.size());
    while (i < newSize) {
        auto connector = std::make_unique<RN_UdpConnectorImpl>(
            _socket,
            _timeoutLimit,
            _passphrase,
            _retransmitPredicate,
            detail::EventFactory{_eventQueue, i});

        _clients.push_back(std::move(connector));
        i += 1;
    }
}

void RN_UdpServerImpl::setTimeoutLimit(std::chrono::microseconds limit) {
    _timeoutLimit = limit;
}

void RN_UdpServerImpl::setRetransmitPredicate(RN_RetransmitPredicate pred) {
    _retransmitPredicate = pred;
}

void RN_UdpServerImpl::update(RN_UpdateMode mode) {
    if (!_running) {
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

bool RN_UdpServerImpl::pollEvent(RN_Event& ev) {
    if (_eventQueue.empty()) {
        return false;
    }
    ev = _eventQueue.front();
    _eventQueue.pop_front();
    return true;
}

///////////////////////////////////////////////////////////////////////////
// CLIENT MANAGEMENT                                                     //
///////////////////////////////////////////////////////////////////////////

const RN_ConnectorInterface& RN_UdpServerImpl::getClientConnector(PZInteger clientIndex) const {
    return *(_clients[clientIndex]);
}

void RN_UdpServerImpl::swapClients(PZInteger index1, PZInteger index2) {
    // TODO
}

void RN_UdpServerImpl::kickClient(PZInteger index) {
    // TODO
}

///////////////////////////////////////////////////////////////////////////
// STATE INSPECTION                                                      //
///////////////////////////////////////////////////////////////////////////

bool RN_UdpServerImpl::isRunning() const {
    return _running;
}

PZInteger RN_UdpServerImpl::getSize() const {
    return static_cast<PZInteger>(_clients.size());
}

const std::string& RN_UdpServerImpl::getPassphrase() const {
    return _passphrase;
}

std::chrono::microseconds RN_UdpServerImpl::getTimeoutLimit() const {
    return _timeoutLimit;
}

std::uint16_t RN_UdpServerImpl::getLocalPort() const {
    return _socket.getLocalPort();
}

bool RN_UdpServerImpl::isServer() const noexcept {
    return true;
}

RN_Protocol RN_UdpServerImpl::getProtocol() const noexcept {
    return _socket.getProtocol();
}

RN_NetworkingStack RN_UdpServerImpl::getNetworkingStack() const noexcept {
    return _socket.getNetworkingStack();
}




int RN_UdpServerImpl::getSenderIndex() const {
    return _senderIndex;
}

///////////////////////////////////////////////////////////////////////////
// PRIVATE IMPLEMENTATION                                                //
///////////////////////////////////////////////////////////////////////////

void RN_UdpServerImpl::_updateReceive() {
    detail::RN_PacketWrapper packetWrap;
    sf::IpAddress senderIp;
    std::uint16_t senderPort;

    bool keepReceiving = true;
    while (keepReceiving) {
        switch (_socket.recv(packetWrap.packet, senderIp, senderPort)) {
        case decltype(_socket)::Status::OK:
            {
                const int senderConnectorIndex = _findConnector(senderIp, senderPort);

                if (senderConnectorIndex != -1) {
                    _senderIndex = senderConnectorIndex;
                    _clients[senderConnectorIndex]->receivedPacket(packetWrap);
                }
                else {
                    _handlePacketFromUnknownSender(senderIp, senderPort, packetWrap);
                }

                packetWrap.packet.clear();
            }
            break;

        case decltype(_socket)::Status::NotReady:
            // Nothing left to receive for now
            keepReceiving = false;
            break;

        case decltype(_socket)::Status::Disconnected:
        default:
            // Realistically these won't ever happen
            assert(false && "Unreachable");
        }
    }

    for (PZInteger i = 0; i < getSize(); i += 1) {
        auto& client = _clients[i];
        
        if (client->getStatus() == RN_ConnectorStatus::Connected) {
            client->sendAcks();
        }
        if (client->getStatus() != RN_ConnectorStatus::Disconnected) {
            _senderIndex = i;
            client->handleDataMessages(SELF, _currentPacket);
        }
        if (client->getStatus() != RN_ConnectorStatus::Disconnected) {
            client->checkForTimeout();
        }
    }
    _senderIndex = -1;
}

void RN_UdpServerImpl::_updateSend() {
    for (auto& client : _clients) {
        if (client->getStatus() == RN_ConnectorStatus::Disconnected) {
            continue;
        }
        client->send();
    }
}

int RN_UdpServerImpl::_findConnector(sf::IpAddress addr, std::uint16_t port) const {
    for (int i = 0; i < getSize(); i += 1) {
        const auto& remote = getClientConnector(i).getRemoteInfo();
        if (remote.port == port && remote.ipAddress == addr) {
            return i;
        }
    }
    return -1;
}

void RN_UdpServerImpl::_handlePacketFromUnknownSender(sf::IpAddress senderIp, std::uint16_t senderPort, detail::RN_PacketWrapper& packetWrap) {
    for (PZInteger i = 0; i < getSize(); i += 1) {
        auto& connector = _clients[i];
        if (connector->getStatus() == RN_ConnectorStatus::Disconnected) {
            connector->setClientIndex(i);
            if (!connector->tryAccept(senderIp, senderPort, packetWrap)) {
                // TODO Notify of error
            }
            return;
        }
    }
    // TODO Send disconnect message (no room left)
}

void RN_UdpServerImpl::_compose(RN_ComposeForAllType receiver, const void* data, std::size_t sizeInBytes) {
    for (auto& client : _clients) {
        if (client->getStatus() == RN_ConnectorStatus::Connected) {
            client->appendToNextOutgoingPacket(data, sizeInBytes);
        }
    }
}

void RN_UdpServerImpl::_compose(PZInteger receiver, const void* data, std::size_t sizeInBytes) {
    if (_clients[receiver]->getStatus() != RN_ConnectorStatus::Connected) {
        throw util::TracedLogicError("Client is not connected; cannot compose messages");
    }
    _clients[receiver]->appendToNextOutgoingPacket(data, sizeInBytes);
}

detail::RN_PacketWrapper* RN_UdpServerImpl::_getCurrentPacketWrapper() {
    return _currentPacket;
}

void RN_UdpServerImpl::_setUserData(util::AnyPtr userData) {
    _userData = userData;
}

util::AnyPtr RN_UdpServerImpl::_getUserData() const {
    return _userData;
}

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>