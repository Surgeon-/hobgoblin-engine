#ifndef UHOBGOBLIN_RN_UDP_SERVER_HPP
#define UHOBGOBLIN_RN_UDP_SERVER_HPP

#include <Hobgoblin/RigelNet/Node.hpp>
#include <Hobgoblin/RigelNet/Remote_info.hpp>
#include <Hobgoblin/RigelNet/Server.hpp>
#include <Hobgoblin/RigelNet/Udp_connector.hpp>
#include <Hobgoblin/Utility/No_copy_no_move.hpp>

#include <SFML/Network.hpp>

#include <chrono>
#include <vector>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

class RN_UdpServer : public RN_Server<RN_UdpServer, detail::RN_UdpConnector> {
public:
    explicit RN_UdpServer();
    explicit RN_UdpServer(PZInteger size);
    explicit RN_UdpServer(PZInteger size, std::uint16_t port, std::string passphrase);

    ~RN_UdpServer();

    void start(std::uint16_t localPort, std::string passphrase);
    void stop();
    bool isRunning() const;
    void update(RN_UpdateMode mode) override;

    // Client management:
    const RN_Connector<detail::RN_UdpConnector>& getClient(PZInteger clientIndex) const;

    void swapClients(PZInteger index1, PZInteger index2);

    void kick(PZInteger index); // TODO To getClient().kick(), add reason

    // Utility:
    int getSenderIndex() const;
    std::uint16_t getLocalPort() const;
    PZInteger getSize() const;
    void resize(PZInteger newSize);

    std::chrono::microseconds getTimeoutLimit() const;
    void setTimeoutLimit(std::chrono::microseconds limit);

    void setRetransmitPredicate(RetransmitPredicate pred);

    const std::string& getPassphrase() const;

protected:
    void compose(int receiver, const void* data, std::size_t sizeInBytes) override;
    void compose(RN_ComposeForAllType receiver, const void* data, std::size_t sizeInBytes) override;

private:
    std::vector<detail::RN_UdpConnector> _clients;
    sf::UdpSocket _mySocket;
    std::string _passphrase;
    std::chrono::microseconds _timeoutLimit = std::chrono::microseconds{0};
    RetransmitPredicate _retransmitPredicate;
    int _senderIndex = -1;
    bool _running = false;

    void updateReceive();
    void updateSend();
    int  findConnector(sf::IpAddress addr, std::uint16_t port) const;
    void handlePacketFromUnknownSender(sf::IpAddress senderIp, std::uint16_t senderPort, detail::RN_PacketWrapper& packetWrap);
};

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_RN_UDP_SERVER_HPP
