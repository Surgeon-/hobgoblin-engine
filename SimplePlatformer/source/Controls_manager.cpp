
#include <SFML/System.hpp>

#include "Controls_manager.hpp"
#include "Game_context.hpp"

#include <iostream>

RN_DEFINE_HANDLER(SetClientControls, RN_ARGS(PlayerControls&, controls)) {
    RN_NODE_IN_HANDLER().visit(
        [](NetworkingManager::ClientType& client) {
            // ERROR
        },
        [&](NetworkingManager::ServerType& server) {
            auto& global = *server.getUserData<GameContext>();
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

void ControlsManager::putNewControls(hg::PZInteger playerIndex, const PlayerControls& controls,
                                     std::chrono::microseconds delay) {
    // TODO Temp. implementation
    _schedulers[playerIndex].putNewState(controls, (delay / std::chrono::microseconds{16'666}));
    //_schedulers[playerIndex].putNewState(controls, delay);
}

void ControlsManager::eventPreUpdate() {
    if (ctx().playerIndex == GameContext::PLAYER_INDEX_UNKNOWN) {
        return;
    }

    // Local controls (not needed on independent server):
    if (ctx().playerIndex != GameContext::PLAYER_INDEX_NONE) {
        auto& scheduler = _schedulers[ctx().playerIndex];
        //scheduler.putNewState(PlayerControls{1, 1, 1});
        scheduler.putNewState(PlayerControls{kbi().keyPressed(KbKey::A),
                                             kbi().keyPressed(KbKey::D),
                                             kbi().keyPressed(KbKey::W)});
    }

    for (auto& scheduler : _schedulers) {
        scheduler.scheduleNewStates();
        scheduler.advanceDownTo(ctx().syncBufferLength * 2);
    }
}

void ControlsManager::eventUpdate() {
    if (ctx().playerIndex > 0 && 
        ctx().netMgr.getClient().getServer().getStatus() == RN_ConnectorStatus::Connected) {
        auto& scheduler = _schedulers[ctx().playerIndex];
        Compose_SetClientControls(ctx().netMgr.getClient(), 0, scheduler.getLatestState());
    }
}

void ControlsManager::eventPostUpdate() {
    for (auto& scheduler : _schedulers) {
        scheduler.advance();
    }

    //auto& scheduler = _schedulers[0];

    //int i = 0;
    //for (auto& item : scheduler) {
    //    i += 1;
    //    std::cout << (int)item.up << ' ';
    //    if (i == 10) break;

    //}
    //std::cout << "(" << int{scheduler.end() - scheduler.begin()} << ")\n"; 
    //std::cout.flush();
}