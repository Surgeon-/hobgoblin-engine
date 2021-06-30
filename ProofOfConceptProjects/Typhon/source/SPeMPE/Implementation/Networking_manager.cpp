
#include <SPeMPE/Include/Networking_manager.hpp>
#include <SPeMPE/Include/Game_context.hpp>

#include <cassert>
#include <chrono>
#include <iostream>

namespace jbatnozic {
namespace spempe {

namespace {

const auto RETRANSMIT_PREDICATE =
[](hg::PZInteger cyclesSinceLastTransmit,
   std::chrono::microseconds timeSinceLastTransmit,
   std::chrono::microseconds currentLatency)
{
    return (timeSinceLastTransmit >= 2 * currentLatency) || cyclesSinceLastTransmit >= 3;
    //return 1; // Maximize user experience (super bandwidth-unfriendly)
};

using namespace hg::qao;
using namespace hg::rn;

} // namespace

NetworkingManager::NetworkingManager(QAO_RuntimeRef runtimeRef)
    : NonstateObject{runtimeRef, SPEMPE_TYPEID_SELF, 0, "spempe::NetworkingManager"}
{
    _node = hg::RN_ServerFactory::createDummyServer();
}

void NetworkingManager::initializeAsServer() {
    _node = hg::RN_ServerFactory::createServer(RN_Protocol::UDP, "pass123", 1, 2048, RN_NetworkingStack::Default);
    getServer().setRetransmitPredicate(RETRANSMIT_PREDICATE);
    getServer().setUserData(&ctx());
    _state = State::Server;
}

void NetworkingManager::initializeAsClient() {
    _node = hg::RN_ClientFactory::createClient(hg::RN_Protocol::UDP, "pass123", 2048, RN_NetworkingStack::Default);
    getClient().setRetransmitPredicate(RETRANSMIT_PREDICATE);
    getClient().setUserData(&ctx());
    _state = State::Client;
}

bool NetworkingManager::isServer() const noexcept {
    return _state == State::Server;
}

bool NetworkingManager::isClient() const noexcept {
    return _state == State::Client;
}

RN_NodeInterface& NetworkingManager::getNode() {
    assert(_node);
    return *_node;
}

NetworkingManager::ServerType& NetworkingManager::getServer() {
    assert(_node && _node->isServer());
    return static_cast<ServerType&>(*_node);
}

NetworkingManager::ClientType& NetworkingManager::getClient() {
    assert(_node && !_node->isServer());
    return static_cast<ClientType&>(*_node);
}

void NetworkingManager::_eventPreUpdate() {
    getNode().update(RN_UpdateMode::Receive);
    handleEvents();
}

void NetworkingManager::_eventPostUpdate() {
    // Update all Synchronized objects
    if (_node->isServer()) {
        ctx().getSyncObjReg().syncStateUpdates();
    }

    getNode().update(RN_UpdateMode::Send);
    handleEvents();
}

void NetworkingManager::addEventListener(EventListener* listener) {
    _eventListeners.push_back(listener);
}

void NetworkingManager::removeEventListener(EventListener* listener) {
    _eventListeners.remove_if(
        [=](EventListener* listener_) {
            return listener_ == listener;
        });
}

void NetworkingManager::handleEvents() {
    RN_Event event;
    while (getNode().pollEvent(event)) {
        event.visit(
            [](const RN_Event::BadPassphrase& ev) {
                std::cout << "Bad passphrase\n";
            },
            [](const RN_Event::ConnectAttemptFailed& ev) {
                std::cout << "Connection attempt failed\n";
            },
            [this](const RN_Event::Connected& ev) {
                if (_node->isServer()) {
                    std::cout << "New client connected\n";
                    ctx().getSyncObjReg().syncCompleteState(*ev.clientIndex);
                }
                else {
                    std::cout << "Connected to server\n";
                    ctx().setLocalPlayerIndex(getClient().getClientIndex() + 1);
                }
            },
            [](const RN_Event::Disconnected& ev) {
                std::cout << "Disconnected (message: " << ev.message << ")\n";
            }
            );

        for (auto& listener : _eventListeners) {
            listener->onNetworkingEvent(event);
        }
    }
}

} // namespace spempe
} // namespace jbatnozic