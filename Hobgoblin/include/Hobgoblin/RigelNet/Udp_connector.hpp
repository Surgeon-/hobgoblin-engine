#ifndef UHOBGOBLIN_RN_UDP_CONNECTOR_HPP
#define UHOBGOBLIN_RN_UDP_CONNECTOR_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/RigelNet/Node.hpp>
#include <Hobgoblin/RigelNet/Packet.hpp>
#include <Hobgoblin/RigelNet/Remote_info.hpp>

#include <Hobgoblin/Utility/Stopwatch.hpp>

#include <SFML/System/Clock.hpp>
#include <SFML/Network.hpp>

#include <chrono>
#include <cstdint>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

enum class RN_ConnectorStatus {
    Disconnected,
    Accepting,
    Connecting,
    Connected
};

namespace detail {

struct TaggedPacket {
    enum Tag {
        DefaultTag,
    // SEND:
        ReadyForSending = DefaultTag,
        NotAcknowledged,
        Acknowledged,
    // RECV:
        WaitingForData = DefaultTag,
        ReadyForUnpacking,
        Unpacked,
    };

    RN_Packet packet;
    util::Stopwatch stopwatch;
    Tag tag = DefaultTag;
};

class RN_UdpConnector {
public:
    RN_UdpConnector(sf::UdpSocket& socket, const std::string& passphrase);

    void tryAccept(sf::IpAddress addr, std::uint16_t port, RN_Packet& packet);
    void connect(sf::IpAddress addr, std::uint16_t port);
    void disconnect(bool notfiyRemote);

    void checkForTimeout();
    void send(RN_Node& node);
    void receivedPacket(RN_Packet& packet);
    void handleDataMessages(RN_Node& node); // TODO - Do I need this?
    void sendAcks();
    
    const RN_RemoteInfo& getRemoteInfo() const noexcept;
    RN_ConnectorStatus getStatus() const noexcept;
    PZInteger getSendBufferSize() const;
    PZInteger getRecvBufferSize() const;

    void appendToNextOutgoingPacket(const void *data, std::size_t sizeInBytes);

private:
    RN_RemoteInfo _remoteInfo;
    std::chrono::microseconds _timeoutLimit = std::chrono::microseconds{0};
    sf::UdpSocket& _socket;
    const std::string& _passphrase;
    RN_ConnectorStatus _status;

    std::deque<TaggedPacket> _sendBuffer;
    std::deque<TaggedPacket> _recvBuffer;
    std::uint32_t _sendBufferHeadIndex;
    std::uint32_t _recvBufferHeadIndex;

    std::vector<std::uint32_t> _ackOrdinals;

    void cleanUp();
    void reset();
    bool connectionTimedOut() const;
    void uploadAllData();
    void prepareAck(std::uint32_t ordinal);
    void receivedAck(std::uint32_t ordinal);
    void initializeSession();
    void prepareNextOutgoingPacket();
    void receiveDataMessage(RN_Packet& packet);
    
    void processHelloPacket(RN_Packet& packet);
    void processConnectPacket(RN_Packet& packet);
    void processDisconnectPacket(RN_Packet& packet);
    void processDataPacket(RN_Packet& packet);
    void processAcksPacket(RN_Packet& packet);
};

} // namespace detail
} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>

#endif // !UHOBGOBLIN_RN_UDP_CONNECTOR_HPP
