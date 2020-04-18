
#include <Hobgoblin/Utility/Math.hpp>

#include <cmath>
#include <iostream>

#include "Global_program_state.hpp"
#include "Player.hpp"

namespace {
using hg::util::EuclideanDist;
} // namespace

RN_DEFINE_HANDLER(CreatePlayer, RN_ARGS(SyncId, syncId, Player::State&, state)) {
    RN_NODE_IN_HANDLER().visit(
        [=](NetworkingManager::ClientType& client) {
            auto& global = *client.getUserData<GlobalProgramState>();
            auto& runtime = global.qaoRuntime;
            auto& syncObjMapper = global.syncObjMgr;
            QAO_PCreate<Player>(&runtime, syncObjMapper, syncId, state.x, state.y, state.playerIndex);
        },
        [](NetworkingManager::ServerType& server) {
            // ERROR
        }
    );
}

RN_DEFINE_HANDLER(UpdatePlayer, RN_ARGS(SyncId, syncId, Player::State&, state)) {
    RN_NODE_IN_HANDLER().visit(
        [=](NetworkingManager::ClientType& client) {
            auto& global = *client.getUserData<GlobalProgramState>();
            auto& runtime = global.qaoRuntime;
            auto& syncObjMapper = global.syncObjMgr;
            auto* player = static_cast<Player*>(syncObjMapper.getMapping(syncId));

            const std::chrono::microseconds delay = client.getServer().getRemoteInfo().latency / 2LL;
            player->_ssch.putNewState(state, global.calcDelay(delay));
        },
        [](NetworkingManager::ServerType& server) {
            // ERROR
        }
    );
}

RN_DEFINE_HANDLER(DestroyPlayer, RN_ARGS(SyncId, syncId)) {
    RN_NODE_IN_HANDLER().visit(
        [=](NetworkingManager::ClientType& client) {
            auto& global = *client.getUserData<GlobalProgramState>();
            auto& runtime = global.qaoRuntime;
            auto& syncObjMapper = global.syncObjMgr;
            auto* player = static_cast<Player*>(syncObjMapper.getMapping(syncId));
            QAO_PDestroy(player);
        },
        [](NetworkingManager::ServerType& server) {
            // ERROR
        }
    );
}

void Player::syncCreateImpl(RN_Node& node, const std::vector<hg::PZInteger>& rec) const {
    Compose_CreatePlayer(node, rec, getSyncId(), _ssch.getCurrentState());
}

void Player::syncUpdateImpl(RN_Node& node, const std::vector<hg::PZInteger>& rec) const {
    Compose_UpdatePlayer(node, rec, getSyncId(), _ssch.getCurrentState()); // TODO
}

void Player::syncDestroyImpl(RN_Node& node, const std::vector<hg::PZInteger>& rec) const {
    Compose_DestroyPlayer(node, rec, getSyncId()); // TODO
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Player::Player(QAO_Runtime* runtime, SynchronizedObjectManager& syncObjMapper, SyncId syncId,
               float x, float y, hg::PZInteger playerIndex)
    : GOF_SynchronizedObject{runtime, TYPEID_SELF, 75, "Player", syncObjMapper, syncId}
    , _ssch{global().syncBufferLength, global().syncBufferHistoryLength, false, false}
{
    for (auto& state : _ssch) {
        state.playerIndex = playerIndex;
        state.x = x;
        state.y = y;
    }
    _doppelganger.x = x;
    _doppelganger.y = y;
}

void Player::eventPreUpdate() {
    if (!global().isHost()) {
        _ssch.advance();
    }
}

void Player::eventUpdate() {
    if (global().isHost()) {
        move(_ssch.getCurrentState());
    }
    else {
        _ssch.scheduleNewStates();
        
        // Try to predict doppelganger movement:
        move(_doppelganger);

        // Interpolate doppelganger state:
        auto& self = _ssch.getCurrentState();
        float dist = EuclideanDist<float>({_doppelganger.x, _doppelganger.y}, {self.x, self.y});
        if (dist <= 5.f) { // TODO Magic number
            _doppelganger = self;
        }
        else {
            double theta = std::atan2(self.y - _doppelganger.y, self.x - _doppelganger.x);
            _doppelganger.x += std::cos(theta) * dist * 0.5f;
            _doppelganger.y += std::sin(theta) * dist * 0.5f;
        }
    }
}

void Player::eventDraw1() {
    static const sf::Color COLORS[] = {sf::Color::Blue, sf::Color::Red, sf::Color::Green, sf::Color::Yellow};

    if (global().isHost()) {
        auto& self = _ssch.getCurrentState();
        sf::RectangleShape rect{{self.width, self.height}};

        rect.setFillColor(COLORS[self.playerIndex]);
        rect.setPosition(self.x, self.y);
        global().windowMgr.getCanvas().draw(rect);
    }
    else {
        auto& self = _ssch.getCurrentState();
        sf::RectangleShape rect{{self.width, self.height}};

        rect.setFillColor(COLORS[self.playerIndex]);
        rect.setPosition(_doppelganger.x, _doppelganger.y);
        global().windowMgr.getCanvas().draw(rect);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) return; // TODO Temp.

        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(sf::Color::Black);
        rect.setOutlineThickness(2.0f);
        rect.setPosition(self.x, self.y);
        global().windowMgr.getCanvas().draw(rect);
    }
}

void Player::move(State& self) {
    PlayerControls controls = global().controlsMgr.getCurrentControlsForPlayer(self.playerIndex);

    if (self.y < static_cast<float>(800) - self.height) {
        self.yspeed += GRAVITY;
    }
    else {
        self.y = static_cast<float>(800) - self.height;
        self.yspeed = 0.f;
    }

    if (controls.up && !oldUp) {
        self.yspeed -= JUMP_POWER;
    }

    self.xspeed = (static_cast<float>(controls.right) - static_cast<float>(controls.left)) * MAX_SPEED;

    self.x += self.xspeed;
    self.y += self.yspeed;

    oldUp = controls.up;
}