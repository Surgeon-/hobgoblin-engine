#ifndef GLOBAL_PROGRAM_STATE_HPP
#define GLOBAL_PROGRAM_STATE_HPP

#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Utility/stopwatch.hpp>

#include <chrono>
#include <iostream>
#include <fstream>
#include <memory>
#include <thread>

#include "GameObjects/Framework/Game_object_framework.hpp"

#include "GameObjects/Managers/Controls_manager.hpp"
#include "GameObjects/Managers/Main_game_controller.hpp"
#include "GameObjects/Managers/Networking_manager.hpp"
#include "GameObjects/Managers/Terrain_manager.hpp"
#include "GameObjects/Managers/Window_manager.hpp"

#include "GameObjects/Gameplay/Player.hpp"

#include "Graphics/Sprites.hpp"

class GameContext {
public:
    static constexpr auto NETWORKING_PASSPHRASE = "beetlejuice";

    static constexpr int PLAYER_INDEX_UNKNOWN = -1;
    static constexpr int PLAYER_INDEX_NONE = -2;

    static constexpr int F_PRIVILEGED = 0x1;
    static constexpr int F_NETWORKING = 0x2;
    static constexpr int F_HEADLESS   = 0x4;

    enum class Mode : int {
        Initial    = 0,
        Server     = F_PRIVILEGED | F_NETWORKING | F_HEADLESS, 
        Client     = F_NETWORKING,
        Solo       = F_PRIVILEGED,
        GameMaster = F_PRIVILEGED | F_NETWORKING,
    };

    struct NetworkConfig {
        hg::PZInteger clientCount;
        sf::IpAddress serverIp;
        std::uint16_t serverPort;
        std::uint16_t localPort;
    };

    struct ResourceConfig {
        const hg::gr::SpriteLoader* spriteLoader;
    };

private:
    GameContext* _parentContext;
    Mode _mode;

public:
    int playerIndex = PLAYER_INDEX_UNKNOWN;
    hg::PZInteger syncBufferLength = 2;
    hg::PZInteger syncBufferHistoryLength = 1;

    NetworkConfig networkConfig;

    QAO_Runtime qaoRuntime;
    WindowManager windowMgr;
    NetworkingManager netMgr;
    MainGameController mainGameCtrl;
    ControlsManager controlsMgr;
    SynchronizedObjectManager syncObjMgr; // TODO This object isn't really a "manager"
    TerrainManager terrMgr;

    GameContext(const ResourceConfig& resourceConfig)
        // Essential:
        : _parentContext{nullptr}
        , _mode{Mode::Initial}
        , qaoRuntime{this}
        , windowMgr{qaoRuntime.nonOwning()}
        // Game-specific:
        , netMgr{qaoRuntime.nonOwning()}
        , mainGameCtrl{qaoRuntime.nonOwning()}
        , controlsMgr{qaoRuntime.nonOwning(), 4, syncBufferLength, syncBufferHistoryLength}
        , syncObjMgr{netMgr.getNode()}
        , terrMgr{qaoRuntime.nonOwning(), syncObjMgr, SYNC_ID_CREATE_MASTER}
        // Other:
        , _resourceConfig{resourceConfig}
    {
        netMgr.getNode().setUserData(this);

        // TODO Temp. - Create terrain
        _physicsSpace = cpSpaceNew();
    }

    ~GameContext() {
        qaoRuntime.eraseAllNonOwnedObjects();
        // TODO Clean up _physicsSpace
        terrMgr.generate(0, 0, 0);
        cpSpaceFree(_physicsSpace);
    }

    void configure(Mode mode);

    bool isPrivileged() const;
    bool isHeadless() const;
    bool hasNetworking() const;

    const ResourceConfig& getResourceConfig() const {
        return _resourceConfig;
    }

    cpSpace* getPhysicsSpace() const {
        return _physicsSpace;
    }

    int calcDelay(std::chrono::microseconds currentLatency) const { // TODO Mode to Utils class
        return static_cast<int>(currentLatency / std::chrono::microseconds{16'666});
    }

    int run();
    void stop();

    bool hasChildContext();
    int stopChildContext();
    void runChildContext(std::unique_ptr<GameContext> childContext);

private:
    // Child context stuff:
    std::unique_ptr<GameContext> _childContext;
    std::thread _childContextThread;
    int _childContextReturnValue;

    // Other:
    ResourceConfig _resourceConfig;
    cpSpace* _physicsSpace;
    bool _quit = false;

    static void runImpl(GameContext* context, int* retVal);
};

#endif // !GLOBAL_PROGRAM_STATE_HPP