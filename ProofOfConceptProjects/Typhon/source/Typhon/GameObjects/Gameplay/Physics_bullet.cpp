
#include <Hobgoblin/Graphics.hpp>
#include <Typhon/Graphics/Sprites.hpp>

#include <algorithm>
#include <cmath>

#include "Physics_bullet.hpp"

SPEMPE_GENERATE_CANNONICAL_HANDLERS(PhysicsBullet);

SPEMPE_GENERATE_CANNONICAL_SYNC_IMPLEMENTATIONS(PhysicsBullet);

static void customDampingVelocityFunc(cpBody* body, cpVect gravity, cpFloat damping, cpFloat dt) {
    //cpBodySetAngularVelocity(body, cpBodyGetAngularVelocity(body) * 0.9);
    cpBodyUpdateVelocity(body, cpv(0.0, 0.0), 1.0, dt);
}

PhysicsBullet::PhysicsBullet(QAO_RuntimeRef rtRef, SynchronizedObjectRegistry& syncObjReg, SyncId syncId,
                             const VisibleState& initialState)
    : SynchronizedObject{rtRef, SPEMPE_TYPEID_SELF, *PEXEPR_ENTITIES_BELOW, "PhysicsBullet", syncObjReg, syncId}
    , _ssch{ccomp<spempe::NetworkingManagerInterface>().getSyncObjReg().getDefaultDelay()}
{
    for (auto& state : _ssch) {
        state = initialState;
    }

    if (isMasterObject()) {
        auto* space = ctx(DPhysicsSpace);

        const cpFloat radius = 4.0;
        const cpFloat mass = 0.03;
        const cpFloat moment = cpMomentForCircle(mass, 0.0, radius, cpv(0, 0));

        _body = hg::cpBodyUPtr{cpSpaceAddBody(space, cpBodyNew(mass, moment))};
        _shape = hg::cpShapeUPtr{cpSpaceAddShape(space, cpCircleShapeNew(_body.get(), radius, cpv(0.0, 0.0)))};
        cpBodySetPosition(_body.get(), cpv(initialState.x, initialState.y));
        cpShapeSetElasticity(_shape.get(), 0.75);
        cpBodySetVelocityUpdateFunc(_body.get(), customDampingVelocityFunc);
        Collideables::initProjectile(_shape.get(), *this);
    }
}

PhysicsBullet::~PhysicsBullet() {
    if (isMasterObject()) {
        syncDestroy();
    }
}

void PhysicsBullet::initWithSpeed(const Collideables::ICreature* creator, double direction, double speed) {
    const cpVect force = cpv(std::cos(direction) * speed, std::sin(direction) * speed);
    cpBodyApplyImpulseAtLocalPoint(_body.get(), force, cpv(0, 0));
    _creator = creator;
}

void PhysicsBullet::destroySelfIn(int steps) {
    ctx().addPostStepAction(std::max(steps, 0),
                            [this](GameContext&) {
                                QAO_PDestroy(this);
                            });
}

void PhysicsBullet::cannonicalSyncApplyUpdate(const VisibleState& state, int delay) {
    _ssch.putNewState(state, delay);
}

cpBody* PhysicsBullet::getPhysicsBody() const {
    return _body.get();
}

void PhysicsBullet::_eventUpdate() {
    if (ctx().isPrivileged()) {
        if (_hitSomething) {
            QAO_PDestroy(this);
        }
    }
    else{
        _ssch.scheduleNewStates();
        _ssch.advance();
        _ssch.advanceDownTo(std::max(1, ccomp<spempe::NetworkingManagerInterface>().getSyncObjReg().getDefaultDelay() * 2));
    }
}

void PhysicsBullet::_eventPostUpdate() {   
    if (ctx().isPrivileged()) {
        auto& self = _ssch.getCurrentState();
        auto physicsPos = cpBodyGetPosition(_body.get());
        self.x = static_cast<float>(physicsPos.x);
        self.y = static_cast<float>(physicsPos.y);
    }
}

void PhysicsBullet::_eventDraw1() {
    auto& canvas = ccomp<spempe::WindowManagerInterface>().getCanvas();
    auto& self = _ssch.getCurrentState();

    auto sprite = ctx(DSprite, SpriteId::Plasma);
    auto sprBounds = sprite.getSubsprite(0).getLocalBounds();

    sprite.setPosition({self.x, self.y});
    sprite.setColor(sf::Color{self.rgbaColor});
    sprite.setOrigin(sprBounds.width / 2, sprBounds.height / 2);
    sprite.setScale({1.5f, 1.5f});
    canvas.draw(sprite);
}

bool PhysicsBullet::collisionBegin(Collideables::ICreature* other, cpArbiter* arbiter) const {
    if (_hitSomething || _creator == other) {
        return REJECT_COLLISION;
    }
    return ACCEPT_COLLISION;
}

void PhysicsBullet::collisionPostSolve(Collideables::ICreature* other, cpArbiter* arbiter) {
    _hitSomething = true;
}

void PhysicsBullet::collisionSeparate(Collideables::ICreature* other, cpArbiter* arbiter) {
    if (_creator == other) {
        _creator = nullptr;
    }
}

void PhysicsBullet::collisionPostSolve(Collideables::ITerrain* other, cpArbiter* arbiter) {
    _hitSomething = true;
}
