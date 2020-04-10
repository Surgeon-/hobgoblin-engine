#ifndef UHOBGOBLIN_RN_UDP_CLIENT_HPP
#define UHOBGOBLIN_RN_UDP_CLIENT_HPP

#include <Hobgoblin/RigelNet/Client.hpp>
#include <Hobgoblin/RigelNet/Node.hpp>
#include <Hobgoblin/RigelNet/Remote_info.hpp>
#include <Hobgoblin/RigelNet/Udp_connector.hpp>
#include <Hobgoblin/Utility/NoCopyNoMove.hpp>

#include <SFML/Network.hpp>

#include <chrono>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

class RN_UdpClient : public RN_Client<RN_UdpClient, detail::RN_UdpConnector> {
public:
    explicit RN_UdpClient();
    explicit RN_UdpClient(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort, 
                          std::string passphrase);

    ~RN_UdpClient();

    void connect(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort, std::string passphrase);
    void disconnect();
    void update(RN_UpdateMode mode);

    // Utility:
    const detail::RN_UdpConnector& getServer() const;
    PZInteger getClientIndex() const;

    std::chrono::microseconds getTimeoutLimit() const;
    void setTimeoutLimit(std::chrono::microseconds limit);

    void setRetransmitPredicate(RetransmitPredicate pred);

    const std::string& getPassphrase() const;

protected:
    void compose(int receiver, const void* data, std::size_t sizeInBytes) override;
    void compose(RN_ComposeForAllType receiver, const void* data, std::size_t sizeInBytes) override { /* TODO */ }

private:
    detail::RN_UdpConnector _connector;
    sf::UdpSocket _mySocket;
    std::string _passphrase;
    std::chrono::microseconds _timeoutLimit = std::chrono::microseconds{0};
    RetransmitPredicate _retransmitPredicate;
    bool _running = false;

    void updateReceive();
    void updateSend();
};

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_RN_UDP_CLIENT_HPP
