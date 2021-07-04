
#include <SFML/System.hpp>

#include <algorithm>
#include <iostream>

#include "Controls_manager.hpp"

namespace {
using spempe::GameContext;
using spempe::KbKey;
using spempe::NetworkingManager;
} // namespace

RN_DEFINE_RPC(SetClientControls, RN_ARGS(PlayerControls&, controls)) {
    RN_NODE_IN_HANDLER().callIfClient(
        [](NetworkingManager::ClientType& client) {
            // ERROR
        });

    RN_NODE_IN_HANDLER().callIfServer(
        [&](NetworkingManager::ServerType& server) {
            auto& ctx = *(server.getUserData<GameContext>());
            auto& controlsMgr = GetControlsManager(ctx);

            const auto clientIndex = server.getSenderIndex();

            const auto latency = server.getClientConnector(clientIndex).getRemoteInfo().meanLatency;
            using TIME = std::remove_cv_t<decltype(latency)>;
            const auto dt = std::chrono::duration_cast<TIME>(ctx.getRuntimeConfig().getDeltaTime());
            const auto delaySteps = static_cast<int>(latency / dt) / 2;

            controlsMgr.putNewControls(server.getSenderIndex() + 1, controls, delaySteps);
        });
}

ControlsManager::ControlsManager(hg::QAO_RuntimeRef runtimeRef, hg::PZInteger playerCount, 
                                 hg::PZInteger inputDelayInSteps, hg::PZInteger historySize)
    : NonstateObject{runtimeRef, TYPEID_SELF, 0, "ControlsManager"} // TODO Priority for controls manager (Run after NetMgr?)
{
    _schedulers.reserve(playerCount);
    for (hg::PZInteger i = 0; i < playerCount; i += 1) {
        _schedulers.emplace_back(inputDelayInSteps);
        _schedulers.back().setDiscardIfOld(false);
    }
}

void ControlsManager::setInputDelay(hg::PZInteger inputDelayInSteps, hg::PZInteger historySize) {
    for (auto& scheduler : _schedulers) {
        scheduler.reset(inputDelayInSteps);
    }
}

PlayerControls ControlsManager::getCurrentControlsForPlayer(hg::PZInteger playerIndex) {
    return _schedulers[static_cast<std::size_t>(playerIndex)].getCurrentState();
}

void ControlsManager::putNewControls(hg::PZInteger playerIndex, const PlayerControls& controls, int delaySteps) {
    _schedulers[playerIndex].putNewState(controls, delaySteps);
}

void ControlsManager::_eventPreUpdate() {
    if (ctx().getMode() == GameContext::Mode::Initial) {
        // Game not yet initialized
        return;
    }
    if (ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex() == spempe::PLAYER_INDEX_UNKNOWN) {
        // Context is a client but not yet connected to the server
        return;
    }

    // Local controls (not needed on independent server):
    //if (ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex() != spempe::PLAYER_INDEX_LOCAL_PLAYER) {
    if (!ctx().isHeadless() &&
        ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex() >= 0) {
        auto& scheduler = _schedulers[ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex()];
        auto mousePos = ccomp<spempe::WindowManagerInterface>().getMousePos();
        scheduler.putNewState(PlayerControls{mousePos.x,
                                             mousePos.y,
                                             sf::Mouse::isButtonPressed(sf::Mouse::Left)
                                                && ccomp<spempe::WindowManagerInterface>().getWindowHasFocus(),
                                             ctx(DKeyboard).keyPressed(KbKey::A),
                                             ctx(DKeyboard).keyPressed(KbKey::D),
                                             ctx(DKeyboard).keyPressed(KbKey::W),
                                             ctx(DKeyboard).keyPressed(KbKey::S)});
    }

    for (auto& scheduler : _schedulers) {
        scheduler.scheduleNewStates();
        scheduler.advanceDownTo(std::max(1, ccomp<spempe::NetworkingManagerInterface>().getStateBufferingLength() * 2));
    }
}

void ControlsManager::_eventUpdate() {
    if (ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex() > 0 &&
        ccomp<spempe::NetworkingManagerInterface>().getClient().getServerConnector().getStatus()
        == hg::RN_ConnectorStatus::Connected) {
        auto& scheduler = _schedulers[ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex()];
        Compose_SetClientControls(ccomp<spempe::NetworkingManagerInterface>().getClient(), 
                                  0,
                                  scheduler.getLatestState());
    }
}

void ControlsManager::_eventPostUpdate() {
    for (auto& scheduler : _schedulers) {
        scheduler.advance();
    }
}