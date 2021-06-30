
#include <SPeMPE/Include/Game_context.hpp>
#include <SPeMPE/Include/Game_object_framework.hpp>
#include <SPeMPE/Include/Keyboard_input.hpp>
#include <SPeMPE/Include/Networking_manager.hpp>
#include <SPeMPE/Include/Window_manager.hpp>

#include <cassert>

namespace jbatnozic {
namespace spempe {

GameContext& GameObjectBase::ctx() const {
    assert(getRuntime() && "Game object not registered to any runtime!");
    return *(getRuntime()->getUserData<GameContext>());
}

// ///////////////////////////////////////////////////////////////////////////////////////////// //

SynchronizedObject::SynchronizedObject(hg::QAO_RuntimeRef runtimeRef, const std::type_info& typeInfo,
                                       int executionPriority, std::string name,
                                       SynchronizedObjectRegistry& syncObjReg, SyncId syncId)
    : StateObject{runtimeRef, typeInfo, executionPriority, std::move(name)}
    , _syncObjReg{syncObjReg}
    , _syncId{(syncId == SYNC_ID_NEW) ? _syncObjReg.registerMasterObject(this) : syncId}
{
    _syncObjReg.registerDummyObject(this, _syncId);
}

SynchronizedObject::~SynchronizedObject() {
    _syncObjReg.unregisterObject(this);
}

SyncId SynchronizedObject::getSyncId() const noexcept {
    return _syncId & ~std::int64_t{1};
}

bool SynchronizedObject::isMasterObject() const noexcept {
    return (_syncId & 1) != 0;
}

void SynchronizedObject::syncCreate() const {
    if (!isMasterObject()) {
        throw hg::TracedLogicError("Dummy objects cannot request synchronization!");
    }
    _syncObjReg.syncObjectCreate(this);
}

void SynchronizedObject::syncUpdate() const {
    if (!isMasterObject()) {
        throw hg::TracedLogicError("Dummy objects cannot request synchronization!");
    }
    _syncObjReg.syncObjectUpdate(this);
}

void SynchronizedObject::syncDestroy() const {
    if (!isMasterObject()) {
        throw hg::TracedLogicError("Dummy objects cannot request synchronization!");
    }
    _syncObjReg.syncObjectDestroy(this);
}

} // namespace spempe
} // namespace jbatnozic