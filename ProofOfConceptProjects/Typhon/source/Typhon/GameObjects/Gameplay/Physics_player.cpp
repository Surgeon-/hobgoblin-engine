
#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Math.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../Control/Controls_manager.hpp"
#include "../Control/Environment_manager.hpp"
#include "Physics_bullet.hpp"
#include "Physics_player.hpp"

namespace {

RN_DEFINE_RPC(PlayerDeathAnnouncement, RN_ARGS(hg::PZInteger, playerIndex)) {
    RN_NODE_IN_HANDLER().callIfServer(
        [&](NetworkingManager::ServerType& server) {
            throw RN_IllegalMessage{"Host cannot receive death announcements"};
        });

    RN_NODE_IN_HANDLER().callIfClient(
        [&](NetworkingManager::ClientType& client) {
            std::cout << "Player " << playerIndex << " has died.\n";
        });
}

void customDampingVelocityFunc(cpBody* body, cpVect gravity, cpFloat damping, cpFloat dt) {
    cpBodySetAngularVelocity(body, cpBodyGetAngularVelocity(body) * 0.9);
    cpBodyUpdateVelocity(body, cpv(0.0, 0.0), 1.0, dt);
}

std::string MakeBarString(hg::PZInteger amount) {
    std::string rv;
    rv.resize(hg::ToSz(amount), '|');
    return rv;
}

const hg::gr::Color* PLAYER_COLORS[] = {
    &hg::gr::Color::White,
    &hg::gr::Color::Red,
    &hg::gr::Color::Green,
    &hg::gr::Color::Yellow,
    &hg::gr::Color::Blue,
    &hg::gr::Color::Orange,
    &hg::gr::Color::Purple,
    &hg::gr::Color::Fuchsia,
    &hg::gr::Color::Brown,
    &hg::gr::Color::Teal,
    &hg::gr::Color::Khaki
};

} // namespace

SPEMPE_GENERATE_CANNONICAL_HANDLERS(PhysicsPlayer);

SPEMPE_GENERATE_CANNONICAL_SYNC_IMPLEMENTATIONS(PhysicsPlayer);

PhysicsPlayer::PhysicsPlayer(QAO_RuntimeRef rtRef, SynchronizedObjectRegistry& syncObjReg, SyncId syncId,
                             const VisibleState& initialState)
    : SynchronizedObject{rtRef, SPEMPE_TYPEID_SELF, *PEXEPR_ENTITIES_ABOVE, "PhysicsPlayer", syncObjReg, syncId}
    , _ssch{ccomp<spempe::NetworkingManagerInterface>().getStateBufferingLength()}
{
    for (auto& state : _ssch) {
        state = initialState;
        if (isMasterObject()) {
            state.hidden = false;
        }
    }

    if (ctx().isPrivileged()) {
        auto* space = ctx(DPhysicsSpace);

        cpFloat mass = 1.0;
        cpFloat moment = cpMomentForBox(mass, 58.0, 38.0);

        _body.reset(cpSpaceAddBody(space, cpBodyNew(mass, moment)));
        _shape.reset(cpSpaceAddShape(space, cpBoxShapeNew(_body.get(), 58.0, 38.0, 1.0)));
        cpBodySetPosition(_body.get(), cpv(initialState.x, initialState.y));
        cpShapeSetElasticity(_shape.get(), 0.5);
        cpBodySetVelocityUpdateFunc(_body.get(), customDampingVelocityFunc);
        Collideables::initCreature(_shape.get(), *this);
    }

    _lightHandle = ctx(MEnvironment).addLight(initialState.x, initialState.y, hg::gr::Color::AntiqueWhite, 8.f);
}

PhysicsPlayer::~PhysicsPlayer() {
    if (isMasterObject()) {
        syncDestroy();
    }
}

void PhysicsPlayer::_eventUpdate() {
    using hg::math::AngleD;
    using hg::math::PointDirection;

    if (ctx().isPrivileged()) {
        if (_invulCounter > 0) {
            _invulCounter -= 1;
        }

        if (_stunCounter > 0) {
            _stunCounter -= 1;
        }

        if (_health <= 0.0) {
            Compose_PlayerDeathAnnouncement(ccomp<spempe::NetworkingManagerInterface>().getNode(),
                                            RN_COMPOSE_FOR_ALL,
                                            _ssch.getCurrentState().playerIndex);
            if (!ctx().isHeadless()) {
                std::cout << "Player " << _ssch.getCurrentState().playerIndex << " has died.\n";
            }
            QAO_PDestroy(this);
            return;
        }

        _shield = std::min(_shield + SHIELD_REGEN_RATE, MAX_SHIELD);

        auto& self = _ssch.getCurrentState();
        PlayerControls controls = ctx(MControls).getCurrentControlsForPlayer(self.playerIndex);

        const cpFloat left = controls.left;
        const cpFloat right = controls.right;
        const cpFloat up = controls.up;
        const cpFloat down = controls.down;

        // Propulsion
        cpBody* const body = _body.get();
        const cpVect pos = cpBodyGetPosition(body);
        const cpFloat propulsionStrength = 400.0;

        if (up) {
            const cpVect force = cpvmult(cpBodyGetRotation(body), propulsionStrength);
            cpBodyApplyForceAtWorldPoint(body, force, pos);
        }

        if (down) {
            const cpVect force = cpvmult(cpBodyGetRotation(body), -0.5 * propulsionStrength);
            cpBodyApplyForceAtWorldPoint(body, force, pos);
        }

        if (left) {
            const cpVect force = cpvmult(cpBodyGetRotation(body), 0.5 * propulsionStrength);
            cpBodyApplyForceAtWorldPoint(body, cpvrperp(force), pos);
        }

        if (right) {
            const cpVect force = cpvmult(cpBodyGetRotation(body), 0.5 * propulsionStrength);
            cpBodyApplyForceAtWorldPoint(body, cpvperp(force), pos);
        }

        // Rotation
        if (_stunCounter == 0) {
            // Manual turning:
#if 0
            if (left) {
                const auto rotForce = cpv(0.0, -15.0);
                cpBodyApplyForceAtLocalPoint(body, rotForce, cpv(12.0, 0.0));
                cpBodyApplyForceAtLocalPoint(body, cpvneg(rotForce), cpvzero);
            }
            else if (right) {
                const auto rotForce = cpv(0.0, +15.0);
                cpBodyApplyForceAtLocalPoint(body, rotForce, cpv(12.0, 0.0));
                cpBodyApplyForceAtLocalPoint(body, cpvneg(rotForce), cpvzero);
            }
#else
            const AngleD currentRotation = AngleD::fromVector(cpBodyGetRotation(body).x,
                                                              cpBodyGetRotation(body).y);

            const auto selfPos  = cpBodyGetPosition(body);
            const auto mousePos = cpv(controls.mouseX, controls.mouseY);

            const AngleD targetRotation = PointDirection<double>(selfPos, mousePos);

            const AngleD diff = currentRotation.shortestDistanceTo(targetRotation);

            const cpFloat rcsStrength = 1000.0;

            cpVect rotForce;
            if (diff.asRadians() < 0.0) {
                rotForce = cpv(0.0, -rcsStrength);
            }
            else {
                rotForce = cpv(0.0, +rcsStrength);
            }

            rotForce = rotForce * std::sin(0.5 * std::abs(diff.asRadians()));

            cpBodyApplyForceAtLocalPoint(body, rotForce, cpv(12.0, 0.0));
            cpBodyApplyForceAtLocalPoint(body, cpvneg(rotForce), cpvzero);

            //cpVect turningN = cpv(1.0, 1.1);
            ////if (right) {
            ////    turningN = cpv(1.0, 1.0);
            ////}
            ////else {
            ////    turningN = cpvzero;
            ////}

            //const cpFloat dot = cpvdot(turningN, cpvnormalize(cpBodyGetRotation(body)));
            //const cpFloat cross = cpvcross(turningN, cpvnormalize(cpBodyGetRotation(body)));

            //cpVect rotN;
            //if (cross <= 0) {
            //    rotN = cpvperp(cpvnormalize(cpBodyGetRotation(body)));
            //}
            //else {
            //    rotN = cpvrperp(cpvnormalize(cpBodyGetRotation(body)));
            //}

            //cpVect rotF = cpvmult(rotN, 300 * (1 - dot));
            //cpBodyApplyForceAtLocalPoint(body, rotF, cpv(1.0, 1.0));
            //cpBodyApplyForceAtLocalPoint(body, cpvneg(rotF), cpvzero);
#endif
        }

        // FIRING PLASMA:
        if (CountPeriodic(&_fireCounter, 7, controls.fire)) {
        //if (controls.fire) {
            auto selfVel = cpBodyGetVelocity(_body.get());
            auto selfPos = cpBodyGetPosition(_body.get());
            auto selfRot = cpBodyGetRotation(_body.get());

            PhysicsBullet::VisibleState vs;
            vs.x = selfPos.x + (static_cast<float>(std::rand()) / RAND_MAX) * 24.0 - 12.0;
            vs.y = selfPos.y + (static_cast<float>(std::rand()) / RAND_MAX) * 24.0 - 12.0;
            vs.rgbaColor = PLAYER_COLORS[self.playerIndex]->toInteger();
            auto* bullet = QAO_PCreate<PhysicsBullet>(getRuntime(), ccomp<spempe::NetworkingManagerInterface>().getSyncObjReg(), SYNC_ID_NEW, vs);
            //bullet->initWithSpeed(this, std::atan2(controls.mouseY - selfPos.y, controls.mouseX - selfPos.x), 50.0);
            bullet->initWithSpeed(this, 0.0, 0.0);

            cpBody* bulletBody = bullet->getPhysicsBody();
            cpFloat bulletMass = cpBodyGetMass(bulletBody);
            const cpFloat bulletSpeed = 36.0;

            // Match ship's speed:
            cpBodyApplyImpulseAtLocalPoint(bulletBody, cpvmult(selfVel, bulletMass), cpvzero);

            // Launch projectile:
            constexpr double aimingAngle = hg::math::DegToRad(20.0);

            const AngleD selfAngle = AngleD::fromVector(selfRot.x, selfRot.y);

            const AngleD desiredLaunchAngle = PointDirection<double>(selfPos, cpv(controls.mouseX, controls.mouseY));

            const AngleD desiredAngleOffset = selfAngle.shortestDistanceTo(desiredLaunchAngle);

            const AngleD actualAngleOffset = AngleD::fromRadians(
                hg::math::Clamp(desiredAngleOffset.asRadians(), -aimingAngle, +aimingAngle)
            );

            const AngleD actualLaunchAngle = selfAngle + actualAngleOffset;

            const auto actualLaunchAngleCp = cpv(actualLaunchAngle.asNormalizedVector().x, 
                                                 actualLaunchAngle.asNormalizedVector().y);

            cpBodyApplyImpulseAtLocalPoint(bulletBody, cpvmult(cpvnormalize(actualLaunchAngleCp), bulletSpeed), 
                                           cpvzero);

            // Action and reaction baby
            cpBodyApplyImpulseAtWorldPoint(_body.get(), cpvmult(cpvnormalize(actualLaunchAngleCp), -3.0), selfPos);
        }
    }
    else {
        _ssch.scheduleNewStates();
        _ssch.advance();
        _ssch.advanceDownTo(std::max(1, ccomp<spempe::NetworkingManagerInterface>().getStateBufferingLength() * 2));
    }

    // MOVE CAMERA:
    if (!ctx().isHeadless()) {
        auto& view = ccomp<spempe::WindowManagerInterface>().getView();
        auto& self = _ssch.getCurrentState();

        if (self.playerIndex == ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex()) {
            auto dist = hg::math::EuclideanDist<float>({self.x, self.y}, view.getCenter());
            if (dist < 4.f) {
                view.setCenter({self.x, self.y});
            }
            else {
                auto direction = atan2(self.y - view.getCenter().y, self.x - view.getCenter().x);
                view.move({std::cosf(direction) * dist / 2.f, std::sinf(direction) * dist / 2.f});
            }
            view.setCenter(/*std::roundf*/(view.getCenter().x), /*std::roundf*/(view.getCenter().y));
        }
    }
}

void PhysicsPlayer::_eventPostUpdate() {
    auto& self = _ssch.getCurrentState();
    if (ctx().isPrivileged()) {
        auto physicsPos = cpBodyGetPosition(_body.get());
        self.x = static_cast<float>(physicsPos.x);
        self.y = static_cast<float>(physicsPos.y);
        self.health = static_cast<float>(_health);
        self.shield = static_cast<float>(_shield);

        auto physicsRot = cpBodyGetRotation(_body.get());
        self.angle = static_cast<float>(hg::math::AngleD::fromVector(physicsRot.x, physicsRot.y).asRadians());
    }

    _lightHandle.setPosition(self.x, self.y);
}

void PhysicsPlayer::_eventDraw1() {
    auto& canvas = ccomp<spempe::WindowManagerInterface>().getCanvas();
    auto& self = _ssch.getCurrentState();

    auto glow = ctx(DSprite, SpriteId::WhiteGlow).getSubsprite(0);
    glow.setOrigin(glow.getLocalBounds().width / 2, glow.getLocalBounds().height / 2);
    glow.setPosition(self.x, self.y);
    glow.setColor(*PLAYER_COLORS[self.playerIndex]);
    glow.setScale({1.25f, 1.25f});
    ccomp<spempe::WindowManagerInterface>().getCanvas().draw(glow);

    auto sprite = ctx(DSprite, SpriteId::Ship).getSubsprite(0);

    sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
    sprite.setPosition(self.x, self.y);
    sprite.setRotation(hg::math::RadToDeg(self.angle) + 90.f);
    ccomp<spempe::WindowManagerInterface>().getCanvas().draw(sprite);
}

void PhysicsPlayer::_eventDrawGUI() {
    auto& self = _ssch.getCurrentState();

    if (self.playerIndex == ccomp<spempe::NetworkingManagerInterface>().getLocalPlayerIndex()) {
        const auto guiHeight = ccomp<spempe::WindowManagerInterface>().getWindowSize().y;

        // Shield:
        {
            sf::Text text;
            text.setPosition(20.f, guiHeight - 52.f);
            text.setFont(hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TitilliumRegular));
            text.setFillColor(hg::gr::Color::LightBlue);
            text.setOutlineColor(hg::gr::Color::Black);
            text.setOutlineThickness(4.f);
            text.setCharacterSize(16);
            text.setString(MakeBarString(static_cast<int>(100.0 * self.shield / MAX_SHIELD)));

            ccomp<spempe::WindowManagerInterface>().getCanvas().draw(text);
        }

        // Health:
        {
            sf::Text text;
            text.setPosition(20.f, guiHeight - 32.f);
            text.setFont(hg::gr::BuiltInFonts::getFont(hg::gr::BuiltInFonts::TitilliumRegular));
            text.setFillColor(hg::gr::Color::Red);
            text.setOutlineColor(hg::gr::Color::Black);
            text.setOutlineThickness(4.f);
            text.setCharacterSize(16);
            text.setString(MakeBarString(static_cast<int>(100.0 * self.health / MAX_HEALTH)));

            ccomp<spempe::WindowManagerInterface>().getCanvas().draw(text);
        }
    }
}

namespace {
constexpr double DAMAGE_MULTIPLIER = 1.0 / 8000.0;
} // namespace

void PhysicsPlayer::collisionPostSolve(Collideables::ICreature* terr, cpArbiter* arb) {
    const double damage = cpArbiterTotalKE(arb) * DAMAGE_MULTIPLIER;
    if (damage >= 10.0) {
        _stunCounter = STUN_STEPS_AFTER_COLLISION * damage / 100.0;
    }
    _takeDamage(cpArbiterTotalKE(arb) * DAMAGE_MULTIPLIER);
}

void PhysicsPlayer::collisionPostSolve(Collideables::IProjectile* proj, cpArbiter* arb) {
    _takeDamage(25.0);
}

void PhysicsPlayer::collisionPostSolve(Collideables::ITerrain* terr, cpArbiter* arb) {
    const double damage = cpArbiterTotalKE(arb) * DAMAGE_MULTIPLIER;
    if (damage >= 10.0) {
        _stunCounter = STUN_STEPS_AFTER_COLLISION * damage / 100.0;
    }
    _takeDamage(cpArbiterTotalKE(arb) * DAMAGE_MULTIPLIER);
}

void PhysicsPlayer::_takeDamage(double damage) {
    if (_invulCounter > 0) {
        return;
    }

    if (damage <= _shield) {
        _shield -= damage;
        return;
    }

    _health -= (damage - _shield);
    _shield = 0.0;
}