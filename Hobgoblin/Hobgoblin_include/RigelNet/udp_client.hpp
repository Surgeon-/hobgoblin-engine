#ifndef UHOBGOBLIN_RN_UDP_CLIENT_HPP
#define UHOBGOBLIN_RN_UDP_CLIENT_HPP

#include <Hobgoblin_include/RigelNet/node.hpp>
#include <Hobgoblin_include/RigelNet/remote_info.hpp>
#include <Hobgoblin_include/RigelNet/udp_connector.hpp>
#include <Hobgoblin_include/Utility/NoCopyNoMove.hpp>

#include <SFML/Network.hpp>

#include <chrono>

#include <Hobgoblin_include/Private/pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

class RN_UdpClient : public RN_Node {
public:
    RN_UdpClient();
    RN_UdpClient(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort, std::string passphrase);

    ~RN_UdpClient();

    void connect(std::uint16_t localPort, sf::IpAddress serverIp, std::uint16_t serverPort, std::string passphrase);
    void disconnect();
    bool isConnected() const;
    void update();
    void updateWithoutUpload();

    // Utility:
    const RN_RemoteInfo& getServerInfo(PZInteger index) const;

private:
    detail::RN_UdpConnector _connector;
    sf::UdpSocket _mySocket;
    std::string _passphrase;
    std::chrono::microseconds _timeoutLimit = std::chrono::microseconds{0};
    bool _isConnected;

    void update(bool doUpload);
    void download();
};

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin_include/Private/pmacro_undef.hpp>

#endif // !UHOBGOBLIN_RN_UDP_CLIENT_HPP