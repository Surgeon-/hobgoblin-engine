
#include <Hobgoblin/Common.hpp>
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

NetworkingManager::NetworkingManager(hg::QAO_RuntimeRef aRuntimeRef,
                                     int aExecutionPriority,
                                     hg::PZInteger aStateBufferingLength)
    : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "jbatnozic::spempe::NetworkingManager"}
    , _node{nullptr}
    , _syncObjReg{nullptr}
    , _stateBufferingLength{aStateBufferingLength}
{
}

///////////////////////////////////////////////////////////////////////////
// CONFIGURATION                                                         //
///////////////////////////////////////////////////////////////////////////

void NetworkingManager::setToMode(Mode aMode) {
    if (_mode == aMode) {
        return;
    }

    _mode = aMode;

    switch (_mode) {
    case Mode::Uninitialized:
        _localPlayerIndex.reset();
        _node.reset();
        _syncObjReg.reset();
        break;

    case Mode::Dummy:
        _localPlayerIndex = PLAYER_INDEX_LOCAL_PLAYER;
        _node = hg::RN_ServerFactory::createDummyServer();
        _node->setUserData(&ctx());
        _syncObjReg = std::make_unique<SynchronizedObjectRegistry>(*_node, _stateBufferingLength);
        break;

    case Mode::Server:
        _localPlayerIndex = PLAYER_INDEX_LOCAL_PLAYER;
        _node = hg::RN_ServerFactory::createServer(hg::RN_Protocol::UDP, "pass"); // TODO Parametrize !!!!!!!!!!!
        _node->setUserData(&ctx());
        getServer().setRetransmitPredicate(RETRANSMIT_PREDICATE);
        _syncObjReg = std::make_unique<SynchronizedObjectRegistry>(*_node, _stateBufferingLength);
        break;

    case Mode::Client:
        _localPlayerIndex = PLAYER_INDEX_UNKNOWN;
        _node = hg::RN_ClientFactory::createClient(hg::RN_Protocol::UDP, "pass");
        _node->setUserData(&ctx());
        getClient().setRetransmitPredicate(RETRANSMIT_PREDICATE);
        _syncObjReg = std::make_unique<SynchronizedObjectRegistry>(*_node, _stateBufferingLength);
        break;

    default: {}
    }

    // TODO !!!!!!!!!!!!!!!!!
}

NetworkingManagerInterface::Mode NetworkingManager::getMode() const {
    return _mode;
}

bool NetworkingManager::isUninitialized() const {
    return _mode == Mode::Uninitialized;
}

bool NetworkingManager::isDummy() const {
    return _mode == Mode::Dummy;
}

bool NetworkingManager::isServer() const {
    return _mode == Mode::Server;
}

bool NetworkingManager::isClient() const {
    return _mode == Mode::Client;
}

///////////////////////////////////////////////////////////////////////////
// NODE ACCESS                                                           //
///////////////////////////////////////////////////////////////////////////

NetworkingManagerInterface::NodeType& NetworkingManager::getNode() const {
    return *_node;
}

NetworkingManagerInterface::ServerType& NetworkingManager::getServer() const {
    assert(isServer());
    return static_cast<ServerType&>(getNode());
}

NetworkingManagerInterface::ClientType& NetworkingManager::getClient() const {
    assert(isClient());
    return static_cast<ClientType&>(getNode());
}

///////////////////////////////////////////////////////////////////////////
// LISTENER MANAGEMENT                                                   //
///////////////////////////////////////////////////////////////////////////

void NetworkingManager::addEventListener(NetworkingEventListener& aListener) {
    for (const auto listener : _eventListeners) {
        if (listener == &aListener) {
            return;
        }
    }
    _eventListeners.push_back(&aListener);
}

void NetworkingManager::removeEventListener(NetworkingEventListener& aListener) {
    _eventListeners.erase(
        std::remove_if(_eventListeners.begin(), _eventListeners.end(),
        [&aListener](const NetworkingEventListener* aCurr) {
            return aCurr == &aListener;
        }), _eventListeners.end());
}

///////////////////////////////////////////////////////////////////////////
// SYNCHRONIZATION                                                       //
///////////////////////////////////////////////////////////////////////////

SynchronizedObjectRegistry& NetworkingManager::getSyncObjReg() {
    if (isUninitialized()) {
        throw hg::TracedLogicError("Method call on Uninitialized NetworkingManager");
    }
    return *_syncObjReg;
}

hg::PZInteger NetworkingManager::getStateBufferingLength() const {
    if (isUninitialized()) {
        throw hg::TracedLogicError("Method call on Uninitialized NetworkingManager");
    }
    return _syncObjReg->getDefaultDelay();
}

void NetworkingManager::setStateBufferingLength(hg::PZInteger aNewStateBufferingLength) {
    if (isUninitialized()) {
        throw hg::TracedLogicError("Method call on Uninitialized NetworkingManager");
    }
    _stateBufferingLength = aNewStateBufferingLength;
    _syncObjReg->setDefaultDelay(aNewStateBufferingLength);
}

///////////////////////////////////////////////////////////////////////////
// MISC.                                                                 //
///////////////////////////////////////////////////////////////////////////

int NetworkingManager::getLocalPlayerIndex() {
    if (isUninitialized()) {
        throw hg::TracedLogicError("Method call on Uninitialized NetworkingManager");
    }
    return *_localPlayerIndex;
}

///////////////////////////////////////////////////////////////////////////
// PROTECTED & PRIVATE METHODS                                           //
///////////////////////////////////////////////////////////////////////////

void NetworkingManager::_eventPreUpdate() {
    if (isUninitialized()) {
        return;
    }

    _node->update(hg::RN_UpdateMode::Receive);
    _handleNetworkingEvents();
}

void NetworkingManager::_eventPostUpdate() {
    if (isUninitialized()) {
        return;
    }

    // Update all Synchronized objects
    if (_node->isServer()) {
        _syncObjReg->syncStateUpdates();
    }

    _node->update(hg::RN_UpdateMode::Send);
    _handleNetworkingEvents();
}

void NetworkingManager::_handleNetworkingEvents() {
    // TODO Temporary couts (move to logger)

    using hg::RN_Event;
    RN_Event event;
    while (_node->pollEvent(event)) {
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
                    _syncObjReg->syncCompleteState(*ev.clientIndex);
                }
                else {
                    std::cout << "Connected to server\n";
                    _localPlayerIndex = (getClient().getClientIndex() + 1);
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