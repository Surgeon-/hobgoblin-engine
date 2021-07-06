#include <Hobgoblin/Utility/Autopack.hpp>
#include <Hobgoblin/Utility/State_scheduler.hpp>
#include <SPeMPE/SPeMPE.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace hg  = ::jbatnozic::hobgoblin;
namespace spe = ::jbatnozic::spempe;
using namespace hg::qao; // All names from QAO are prefixed with QAO_
using namespace hg::rn;  // All names from RigelNet are prefixed with RN_

using MNetworking = spe::NetworkingManagerInterface;
using MWindow     = spe::WindowManagerInterface;

#define PRIORITY_NETWORKMGR   15
#define PRIPRITY_GAMEPLAYMGR  10
#define PRIORITY_PLAYERAVATAR  5
#define PRIORITY_WINDOWMGR     0

#define STATE_BUFFERING_LENGTH 3

///////////////////////////////////////////////////////////////////////////
// PLAYER CONTROLS                                                       //
///////////////////////////////////////////////////////////////////////////

struct PlayerControls {
    bool left = false, right = false, up = false, down = false;
    HG_ENABLE_AUTOPACK(PlayerControls, left, right, up, down);
};

///////////////////////////////////////////////////////////////////////////
// MAIN GAMEPLAY CONTROLLER INTERFACE                                    //
///////////////////////////////////////////////////////////////////////////

class GameplayManagerInterface : public spe::ContextComponent {
public:
    virtual const PlayerControls& getPlayerControls(hg::PZInteger aForPlayerIndex) const = 0;

private:
    SPEMPE_CTXCOMP_TAG("GameplayManagerInterface");
};

using MGameplay = GameplayManagerInterface;

///////////////////////////////////////////////////////////////////////////
// PLAYER "AVATARS"                                                      //
///////////////////////////////////////////////////////////////////////////

struct PlayerAvatar_VisibleState {
    float x, y;
    int owningPlayerIndex = spe::PLAYER_INDEX_UNKNOWN;
    bool hidden = true;
    HG_ENABLE_AUTOPACK(PlayerAvatar_VisibleState, x, y, owningPlayerIndex, hidden);
};

class PlayerAvatar : public spe::SynchronizedObject<PlayerAvatar_VisibleState> {
public:
    PlayerAvatar(QAO_RuntimeRef aRuntimeRef,
                 spe::RegistryId aRegId,
                 spe::SyncId aSyncId)
        : SyncObjSuper{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIORITY_PLAYERAVATAR,
                       "PlayerAvatar", aRegId, aSyncId}
    {
    }

    void init(int aOwningPlayerIndex) {
        assert(isMasterObject());

        auto& self = _getCurrentState();
        self.x = 400.f;
        self.y = 400.f;
        self.owningPlayerIndex = aOwningPlayerIndex;
        self.hidden = false;
    }

private:
    void _eventUpdate(spe::IfMaster) override {
        auto& self = _getCurrentState();
        assert(self.owningPlayerIndex >= 0);

        auto& controls = ccomp<MGameplay>().getPlayerControls(self.owningPlayerIndex);
        self.x += (5.f * ((float)controls.right - (float)controls.left));
        self.y += (5.f * ((float)controls.down  - (float)controls.up));
    }

    void _eventUpdate(spe::IfDummy) override {
        SPEMPE_SYNCOBJ_BEGIN_EVENT_UPDATE_OVERRIDE();
        auto& self = _getCurrentState();
        if (!self.hidden && sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            std::cout << self.x << ' ' << self.y << std::endl;
        }
    }

    void _eventDraw1() override {
        const auto& self = _getCurrentState();
        if (self.hidden || self.owningPlayerIndex < 0) {
            return;
        }

        sf::CircleShape circle{20.f};
        circle.setFillColor(hg::gr::Color::Red);
        circle.setPosition({self.x, self.y});
        ccomp<MWindow>().getCanvas().draw(circle);
    }

    void _syncCreateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncUpdateImpl(spe::SyncDetails& aSyncDetails) const override;
    void _syncDestroyImpl(spe::SyncDetails& aSyncDetails) const override;
};

SPEMPE_GENERATE_DEFAULT_SYNC_HANDLERS(PlayerAvatar, (CREATE, UPDATE, DESTROY));

void PlayerAvatar::_syncCreateImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_CREATE_DEFAULT_IMPL(PlayerAvatar, aSyncDetails);
}

void PlayerAvatar::_syncUpdateImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_UPDATE_DEFAULT_IMPL(PlayerAvatar, aSyncDetails);
}

void PlayerAvatar::_syncDestroyImpl(spe::SyncDetails& aSyncDetails) const {
    SPEMPE_SYNC_DESTROY_DEFAULT_IMPL(PlayerAvatar, aSyncDetails);
}

///////////////////////////////////////////////////////////////////////////
// MAIN GAMEPLAY CONTROLLER IMPLEMENTATION                               //
///////////////////////////////////////////////////////////////////////////

class GameplayManager 
    : public  GameplayManagerInterface
    , public  spe::NonstateObject
    , private spe::NetworkingEventListener {
public:
    GameplayManager(QAO_RuntimeRef aRuntimeRef)
        : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, PRIPRITY_GAMEPLAYMGR, "GameplayManager"}
    {
        ccomp<MNetworking>().addEventListener(*this);
        for (int i = 0; i < 20; i += 1) { // TODO
            _schedulers.emplace_back(STATE_BUFFERING_LENGTH);  // TODO
        }
    }

    ~GameplayManager() {
        ccomp<MNetworking>().removeEventListener(*this);
    }

    void pushNewPlayerControls(hg::PZInteger aForPlayerIndex,
                               const PlayerControls& aNewControls,
                               int aDelaySteps) {
        _schedulers.at(aForPlayerIndex).putNewState(aNewControls, aDelaySteps);
    }

    const PlayerControls& getPlayerControls(hg::PZInteger aForPlayerIndex) const override {
        return _schedulers.at(aForPlayerIndex).getCurrentState();
    }

private:
    std::vector<hg::util::StateScheduler<PlayerControls>> _schedulers;

    void _eventPreUpdate() {
        if (ctx().isPrivileged()) {
            for (auto& scheduler : _schedulers) {
                scheduler.scheduleNewStates();
                scheduler.advanceDownTo(std::max(1, STATE_BUFFERING_LENGTH * 2)); // TODO Temp.
            }
        }
    }

    void _eventUpdate() override;

    void _eventPostUpdate() {
        if (ctx().isPrivileged()) {
            for (auto& scheduler : _schedulers) {
                scheduler.advance();
            }
        }
    }

    void onNetworkingEvent(const hg::RN_Event& ev) override {
        if (ccomp<MNetworking>().isClient()) {
            return;
        }

        ev.visit(
            [](const RN_Event::BadPassphrase& ev) {
            },
            [](const RN_Event::ConnectAttemptFailed& ev) {
            },
            [this](const RN_Event::Connected& ev) {
                QAO_PCreate<PlayerAvatar>(getRuntime(),
                                          ccomp<MNetworking>().getRegistryId(),
                                          spe::SYNC_ID_NEW)->init(*ev.clientIndex + 1);
            },
            [](const RN_Event::Disconnected& ev) {
                // TODO Remove player avatar
            }
        );
    }
};

RN_DEFINE_RPC(PushPlayerControls, RN_ARGS(PlayerControls&, aControls)) {
    RN_NODE_IN_HANDLER().callIfServer(
        [&](RN_ServerInterface& aServer) {
            auto sp = SPEMPE_GET_SYNC_PARAMS(aServer);
            auto& gpMgr = sp.context.getComponent<GameplayManager>();
            gpMgr.pushNewPlayerControls(sp.senderIndex + 1, aControls, sp.meanLatencyInSteps);
        });
}

void GameplayManager::_eventUpdate() {
    if (!ctx().isPrivileged() &&
        ccomp<MNetworking>().getLocalPlayerIndex() >= 0 &&
        ccomp<MNetworking>().getClient().getServerConnector().getStatus() == hg::RN_ConnectorStatus::Connected) {
        PlayerControls controls{
            sf::Keyboard::isKeyPressed(sf::Keyboard::A),
            sf::Keyboard::isKeyPressed(sf::Keyboard::D),
            sf::Keyboard::isKeyPressed(sf::Keyboard::W),
            sf::Keyboard::isKeyPressed(sf::Keyboard::S)
        };

        Compose_PushPlayerControls(ccomp<MNetworking>().getClient(),
                                   RN_COMPOSE_FOR_ALL, controls);
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        ctx().stop();
    }
}

///////////////////////////////////////////////////////////////////////////
// GAME CONFIG                                                           //
///////////////////////////////////////////////////////////////////////////

#define WINDOW_WIDTH           800
#define WINDOW_HEIGHT          800
#define FRAMERATE               60

enum class GameMode {
    Server, Client
};

bool MyRetransmitPredicate(hg::PZInteger aCyclesSinceLastTransmit,
                           std::chrono::microseconds aTimeSinceLastSend,
                           std::chrono::microseconds aCurrentLatency) {
    return 1;
}

std::unique_ptr<spe::GameContext> MakeGameContext(GameMode aGameMode,
                                                  std::uint16_t aLocalPort,
                                                  std::uint16_t aRemotePort, 
                                                  std::string aRemoteIp,
                                                  hg::PZInteger aPlayerCount)
{
    auto context = std::make_unique<spe::GameContext>(
        spe::GameContext::RuntimeConfig{std::chrono::duration<double>(1.0 / FRAMERATE)});
    context->setToMode((aGameMode == GameMode::Server) ? spe::GameContext::Mode::Server
                                                       : spe::GameContext::Mode::Client);

    // Create and attach a Window manager
    auto winMgr = std::make_unique<spe::WindowManagerOne>(context->getQAORuntime().nonOwning(), 
                                                          PRIORITY_WINDOWMGR);
    if (aGameMode == GameMode::Server) {
        winMgr->setToHeadlessMode(spe::WindowManagerInterface::TimingConfig{FRAMERATE});
    }
    else {
        winMgr->setToNormalMode(
            spe::WindowManagerInterface::WindowConfig{
                sf::VideoMode{WINDOW_WIDTH, WINDOW_WIDTH},
                "SPeMPE Minimal Multiplayer",
                sf::Style::Fullscreen
            },
            spe::WindowManagerInterface::MainRenderTextureConfig{{WINDOW_HEIGHT, WINDOW_HEIGHT}},
            spe::WindowManagerInterface::TimingConfig{
                FRAMERATE,
                static_cast<bool>((aGameMode == GameMode::Server) ? 0 : 0), /* Framerate limiter */
                static_cast<bool>((aGameMode == GameMode::Server) ? 0 : 1), /* V-Sync */
                static_cast<bool>((aGameMode == GameMode::Server) ? 1 : 0)  /* Previce timing */
            }
        );
    }

    context->attachAndOwnComponent(std::move(winMgr));

    // Create and attach a Networking manager
    auto netMgr = std::make_unique<spe::NetworkingManagerOne>(context->getQAORuntime().nonOwning(), 
                                                              PRIORITY_NETWORKMGR,
                                                              STATE_BUFFERING_LENGTH);
    if (aGameMode == GameMode::Server) {
        netMgr->setToMode(spe::NetworkingManagerOne::Mode::Server);
        auto& server = netMgr->getServer();
        server.setTimeoutLimit(std::chrono::seconds{5});
        server.setRetransmitPredicate(&MyRetransmitPredicate);
        server.start(aLocalPort);
        // TODO playerCount unused!

        std::printf("Server started on port %d\n", (int)server.getLocalPort());
    }
    else {
        netMgr->setToMode(spe::NetworkingManagerOne::Mode::Client);
        auto& client = netMgr->getClient();
        client.setTimeoutLimit(std::chrono::seconds{5});
        client.setRetransmitPredicate(&MyRetransmitPredicate);
        client.connect(aLocalPort, aRemoteIp, aRemotePort);

        std::printf("Client started on port %d (connecting to %s:%d)\n",
                    (int)client.getLocalPort(), aRemoteIp.c_str(), (int)aRemotePort);
    }
    context->attachAndOwnComponent(std::move(netMgr));

    // Create and attach a Gameplay manager
    auto gpMgr = std::make_unique<GameplayManager>(context->getQAORuntime().nonOwning());
    context->attachAndOwnComponent(std::move(gpMgr));

    return context;
}

/* SERVER:
 *   mmp.exe server <local-port> <player-count>
 * 
 * CLIENT:
 *   mmp.exe client <local-port> <server-ip> <server-port>
 *
 */
int main(int argc, char* argv[]) {
    RN_IndexHandlers();

    // Parse command line arguments:
    GameMode gameMode;
    std::uint16_t localPort = 0;
    std::uint16_t remotePort = 0;
    hg::PZInteger playerCount = 1;
    std::string remoteIp = "";

    if (argc != 4 && argc != 5) {
        std::puts("Invalid argument count");
        return EXIT_FAILURE;
    }
    const std::string gameModeStr = argv[1];
    if (gameModeStr == "server") {
        gameMode    = GameMode::Server;
        localPort   = std::stoi(argv[2]);
        playerCount = std::stoi(argv[3]);
    }
    else if (gameModeStr == "client") {
        gameMode   = GameMode::Client;
        localPort  = std::stoi(argv[2]);
        remoteIp   = argv[3];
        remotePort = std::stoi(argv[4]);
    }
    else {
        std::puts("Game mode must be either 'server' or 'client'");
        return EXIT_FAILURE;
    }

    if (!(playerCount >= 1 && playerCount < 20)) {
        std::puts("Player count must be between 1 and 20");
        return EXIT_FAILURE;
    }

    // Start the game:
    auto context = MakeGameContext(gameMode, localPort, remotePort,
                                   std::move(remoteIp), playerCount);
    const int status = context->runFor(-1);
    return status;
}

#if 0
int main() {
    float x = 400.f, y = 400.f;

    sf::RenderWindow window(sf::VideoMode(800, 800), "SFML works!");
    //window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    sf::CircleShape shape(30.f);
    shape.setFillColor(sf::Color::Green);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();

        x += (5.f) * ((int)sf::Keyboard::isKeyPressed(sf::Keyboard::D) - 
                      (int)sf::Keyboard::isKeyPressed(sf::Keyboard::A));

        y += (5.f) * ((int)sf::Keyboard::isKeyPressed(sf::Keyboard::S) - 
                      (int)sf::Keyboard::isKeyPressed(sf::Keyboard::W));

        shape.setPosition(x, y);
        window.draw(shape);

        window.display();
    }

    return 0;
}
#endif