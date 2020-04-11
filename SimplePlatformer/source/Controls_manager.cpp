
#include <SFML/System.hpp>

#include "Controls_manager.hpp"
#include "Global_program_state.hpp"

#include <iostream>

RN_DEFINE_HANDLER(SetClientControls, RN_ARGS(PlayerControls&, controls)) {
    RN_NODE_IN_HANDLER().visit(
        [](NetworkingManager::ClientType& client) {
            // ERROR
        },
        [&](NetworkingManager::ServerType& server) {
            auto& global = *server.getUserData<GlobalProgramState>();
            auto& controlsMgr = global.controlsMgr;

            const auto clientIndex = server.getSenderIndex();
            const std::chrono::microseconds delay = server.getClient(clientIndex).getRemoteInfo().latency / 2LL;

            controlsMgr.putNewControls(server.getSenderIndex() + 1, controls, delay);
        }
    );
}

ControlsManager::ControlsManager(QAO_RuntimeRef runtimeRef, hg::PZInteger playerCount, 
                                 hg::PZInteger inputDelayInSteps, hg::PZInteger historySize)
    : GOF_Base{runtimeRef, TYPEID_SELF, 30, "ControlsManager"} // Run after NetMgr
{
    _schedulers.reserve(playerCount);
    for (hg::PZInteger i = 0; i < playerCount; i += 1) {
        _schedulers.emplace_back(inputDelayInSteps, historySize);
    }
}

void ControlsManager::setInputDelay(hg::PZInteger inputDelayInSteps, hg::PZInteger historySize) {
    for (auto& scheduler : _schedulers) {
        scheduler.reset(inputDelayInSteps, historySize);
    }
}

PlayerControls ControlsManager::getCurrentControlsForPlayer(hg::PZInteger playerIndex) {
    return _schedulers[static_cast<std::size_t>(playerIndex)].getCurrentControls();
}

void ControlsManager::putNewControls(hg::PZInteger playerIndex, const PlayerControls& controls,
                                     std::chrono::microseconds delay) {
    // TODO Temp. implementation
    _schedulers[playerIndex].putNewControls(controls, delay);
}

void ControlsManager::eventPreUpdate() {
    if (global().playerIndex == -1) {
        return;
    }

    // Local controls:
    auto& scheduler = _schedulers[global().playerIndex];
    bool focus = true; //global().windowMgr.window.hasFocus(); TODO Temp.
    scheduler.putNewControls(PlayerControls{focus && sf::Keyboard::isKeyPressed(sf::Keyboard::A),
                                            focus && sf::Keyboard::isKeyPressed(sf::Keyboard::D),
                                            focus && sf::Keyboard::isKeyPressed(sf::Keyboard::W)},
                             std::chrono::microseconds{0});

    for (auto& scheduler : _schedulers) {
        scheduler.integrateNewControls();
    }
}

void ControlsManager::eventUpdate() {
    if (global().playerIndex > 0 && 
        global().netMgr.getClient().getServer().getStatus() == RN_ConnectorStatus::Connected) {
        auto& scheduler = _schedulers[global().playerIndex];
        RN_Compose_SetClientControls(global().netMgr.getClient(), 0, scheduler.getLatestControls());
    }
}

void ControlsManager::eventPostUpdate() {
    for (auto& scheduler : _schedulers) {
        scheduler.advanceStep();
    }
}