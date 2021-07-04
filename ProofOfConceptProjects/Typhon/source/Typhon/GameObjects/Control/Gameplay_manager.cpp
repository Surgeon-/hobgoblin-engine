
#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Graphics.hpp>
#include <SFML/System.hpp>

#include <algorithm>
#include <cstdlib> // TODO Temp. for std::rand
#include <iostream>
#include <sstream>

#include "Environment_manager.hpp"
#include "Gameplay_manager.hpp"
#include "../Gameplay/Physics_bullet.hpp"
#include "../Gameplay/Physics_player.hpp"

namespace {

RN_DEFINE_RPC(RequestGameRestart, RN_ARGS()) {
    RN_NODE_IN_HANDLER().callIfClient(
        [](NetworkingManager::ClientType& client) {
            throw RN_IllegalMessage{"Client cannot receive game restart requests."};
        });

    RN_NODE_IN_HANDLER().callIfServer(
        [&](NetworkingManager::ServerType& server) {
            auto& ctx = *(server.getUserData<GameContext>());
            auto& gameplayMgr = GetGameplayManager(ctx);
            gameplayMgr.restartGame();
        });
}

RN_DEFINE_RPC(GameStartAnnouncement, RN_ARGS()) {
    RN_NODE_IN_HANDLER().callIfClient(
        [](NetworkingManager::ClientType& client) {
            std::cout << "*** A new game has started. ***\n";
        });

    RN_NODE_IN_HANDLER().callIfServer(
        [&](NetworkingManager::ServerType& server) {
            throw RN_IllegalMessage{"Host cannot receive game start announcements."};
        });
}

} // namespace

GameplayManager::GameplayManager(QAO_RuntimeRef runtimeRef)
    : NonstateObject{runtimeRef, TYPEID_SELF, *PEXEPR_GAMEPLAY_MGR, "GameplayManager"}
{
    ctx(MNetworking).addEventListener(this);
}

GameplayManager::~GameplayManager() {
    ctx(MNetworking).removeEventListener(this);
}

void GameplayManager::restartGame() {
    if (ctx().isPrivileged()) {
        // Clean up current session (delete all players and bullets):
        auto iter = getRuntime()->begin();
        while (iter != getRuntime()->end()) {
            auto* obj = *iter;
            iter = std::next(iter);

            auto& typeInfo = obj->getTypeInfo();
            if (typeInfo == typeid(PhysicsBullet) || typeInfo == typeid(PhysicsPlayer)) {
                QAO_PDestroy(obj);
            }
        }

        // Create new players:
        if (ctx().hasNetworking()) {
            auto& server = ctx(MNetworking).getServer();

            for (hg::PZInteger i = 0; i < server.getSize(); i += 1) {
                const auto& clientStatus = server.getClientConnector(i).getStatus();
                if (clientStatus == RN_ConnectorStatus::Connected) {
                    // TODO Temp -- give an init() method to PhysicsPlayer
                    PhysicsPlayer::VisibleState vs;
                    vs.playerIndex = i + 1;
                    _setPlayerPosition(vs.playerIndex, &vs.x, &vs.y);
                    QAO_PCreate<PhysicsPlayer>(getRuntime(), ctx().getSyncObjReg(), SYNC_ID_NEW, vs);
                }
            }
        }

        if (ctx().getLocalPlayerIndex() == spempe::PLAYER_INDEX_LOCAL_PLAYER) {
            // TODO Temp -- give an init() method to PhysicsPlayer
            PhysicsPlayer::VisibleState vs;
            vs.playerIndex = spempe::PLAYER_INDEX_LOCAL_PLAYER;
            _setPlayerPosition(vs.playerIndex, &vs.x, &vs.y);
            QAO_PCreate<PhysicsPlayer>(getRuntime(), ctx().getSyncObjReg(), SYNC_ID_NEW, vs);
        }

        // Announce:
        Compose_GameStartAnnouncement(ctx(MNetworking).getNode(), RN_COMPOSE_FOR_ALL);
        if (!ctx().isHeadless()) {
            std::cout << "*** A new game has started. ***\n";
        }
    }
    else {
        // Not privileged:
    }
}

void GameplayManager::_eventUpdate() {
    // Exit:
    if (ctx(DKeyboard).keyPressed(KbKey::Escape)) {
        ctx().stop();
    }

    // Game restart:
    if (ctx(DKeyboard).keyPressed(KbKey::Backspace, spempe::KbMode::Edge)) {
        if (ctx().isPrivileged()) {
            restartGame();
        }
        else {
            Compose_RequestGameRestart(ctx(MNetworking).getNode(), RN_COMPOSE_FOR_ALL);
        }
    }

    // Camera movement:
    if (!ctx().isHeadless()) {
        auto& view = ccomp<spempe::WindowManagerInterface>().getView();

        const bool left = ctx(DKeyboard).keyPressed(KbKey::J);
        const bool right = ctx(DKeyboard).keyPressed(KbKey::L);
        const bool up = ctx(DKeyboard).keyPressed(KbKey::I);
        const bool down = ctx(DKeyboard).keyPressed(KbKey::K);

        if (!left && !right && !up && !down) return;

        const float moveSpeed = 16.f;

        view.move({moveSpeed * static_cast<float>(right - left), moveSpeed * static_cast<float>(down - up)});
    }
}

void GameplayManager::_eventPostUpdate() {
    cpSpaceStep(ctx(DPhysicsSpace), ctx().getRuntimeConfig().getDeltaTime().count());
}

void GameplayManager::_eventDrawGUI() {
    // DRAW PERFORMANCE INFO:
    {
        sf::Text text;
        text.setFont(hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TitilliumRegular));
        text.setPosition({16.f, 16.f});
        text.setFillColor(hg::gr::Color::White);
        text.setOutlineColor(hg::gr::Color::Black);
        text.setOutlineThickness(1.f);
        text.setCharacterSize(16);

        auto& perfInfo = ctx().getPerformanceInfo();
        std::stringstream sstream;
        sstream
            << "Local context performance (microsec):\n"
            << "    tUpdate: " << perfInfo.updateAndDrawTime.count() << " (x" << perfInfo.consecutiveUpdateLoops << ")\n"
            << "    tFinalize: " << perfInfo.finalizeTime.count() << '\n'
            << "    tTotal: " << perfInfo.totalTime.count() << '\n'
            << "    tFTF: " << perfInfo.frameToFrameTime.count() << "\n\n";

        if (ctx().hasChildContext()) {
            auto& perfInfo = ctx().getChildContext()->getPerformanceInfo();
            sstream
                << "Child context performance (microsec):\n"
                << "    tUpdate: " << perfInfo.updateAndDrawTime.count() << " (x" << perfInfo.consecutiveUpdateLoops << ")\n"
                << "    tFinalize: " << perfInfo.finalizeTime.count() << '\n'
                << "    tTotal: " << perfInfo.totalTime.count() << '\n'
                << "    tFTF: " << perfInfo.frameToFrameTime.count() << "\n\n";
        }

        text.setString(sstream.str());

        ccomp<spempe::WindowManagerInterface>().getCanvas().draw(text);
    }

    // DRAW PLAYERS LEFT:
    {
        const float guiWidth = static_cast<float>(ccomp<spempe::WindowManagerInterface>().getWindowSize().x);

        sf::Text text;
        text.setFont(hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TitilliumRegular));
        text.setPosition({guiWidth - 200.f, 16.f});
        text.setFillColor(hg::gr::Color::White);
        text.setOutlineColor(hg::gr::Color::Black);
        text.setOutlineThickness(4.f);
        text.setCharacterSize(28);

        const auto playerCount = std::count_if(std::begin(*getRuntime()), std::end(*getRuntime()),
                                               [](QAO_Base* object) -> bool {
                                                   return object->getTypeInfo() == typeid(PhysicsPlayer);
                                               });

        text.setString("Players left: " + std::to_string(playerCount));

        ccomp<spempe::WindowManagerInterface>().getCanvas().draw(text);
    }
}

void GameplayManager::onNetworkingEvent(const RN_Event& event_) {
    //event_.visit(
    //    [](const RN_Event::BadPassphrase& ev) {
    //    },
    //    [](const RN_Event::ConnectAttemptFailed& ev) {
    //    },
    //    [this](const RN_Event::Connected& ev) {
    //    },
    //    [](const RN_Event::Disconnected& ev) {
    //    }
    //);
}

void GameplayManager::_setPlayerPosition(hg::PZInteger playerIndex, float* x, float* y) const {
    const auto& environMgr = ctx(MEnvironment);

    const float cellResolution = 32.f; // TODO TEMP
    const float minX = 4.f * cellResolution;
    const float minY = 4.f * cellResolution;
    const float maxX = environMgr.getTerrainColumnCount() * cellResolution - 4.f * cellResolution;
    const float maxY = environMgr.getTerrainRowCount() * cellResolution - 4.f * cellResolution;

    //switch (playerIndex % 4) {
    //case 0:
    //    *x = minX;
    //    *y = minY;
    //    break;

    //case 1:
    //    *x = maxX;
    //    *y = maxY;
    //    break;

    //case 2:
    //    *x = maxX;
    //    *y = minY;
    //    break;

    //case 3:
    //    *x = minX;
    //    *y = maxY;
    //    break;
    //}

    const float noramalizedRandX = static_cast<float>(std::rand()) / RAND_MAX;
    const float noramalizedRandY = static_cast<float>(std::rand()) / RAND_MAX;

    *x = minX + (maxX - minX) * noramalizedRandX;
    *y = minY + (maxY - minY) * noramalizedRandY;
}
