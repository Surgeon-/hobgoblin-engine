#ifndef SPEMPE_NETWORKING_MANAGER_HPP
#define SPEMPE_NETWORKING_MANAGER_HPP

#include <Hobgoblin/RigelNet.hpp>
#include <Hobgoblin/RigelNet_Macros.hpp>
#include <SPeMPE/Include/Game_object_framework.hpp>

#include <list>
#include <memory>

namespace jbatnozic {
namespace spempe {

class NetworkingManager : public NonstateObject {
public:
    using ServerType = hg::RN_ServerInterface;
    using ClientType = hg::RN_ClientInterface;

    NetworkingManager(hg::QAO_RuntimeRef runtimeRef);

    void initializeAsServer();
    void initializeAsClient();

    bool isServer() const noexcept;
    bool isClient() const noexcept;

    hg::RN_NodeInterface& getNode();
    ServerType& getServer();
    ClientType& getClient();

    class EventListener {
    public:
        virtual void onNetworkingEvent(const hg::RN_Event& ev) = 0;
    };

    void addEventListener(EventListener* listener);
    void removeEventListener(EventListener* listener);

protected:
    void _eventPreUpdate() override;
    void _eventPostUpdate() override;

private:
    enum class State {
        NotInitialized,
        Server,
        Client
    };

    std::unique_ptr<hg::RN_NodeInterface> _node;
    State _state = State::NotInitialized;

    std::list<EventListener*> _eventListeners;

    void handleEvents();
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_NETWORKING_MANAGER_HPP

