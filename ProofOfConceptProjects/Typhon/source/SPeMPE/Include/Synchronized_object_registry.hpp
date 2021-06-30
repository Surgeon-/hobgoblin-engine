#ifndef SPEMPE_SYNCHRONIZED_OBJECT_REGISTRY_HPP
#define SPEMPE_SYNCHRONIZED_OBJECT_REGISTRY_HPP

#include <Hobgoblin/RigelNet.hpp>
#include <Hobgoblin/Utility/No_copy_no_move.hpp>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace jbatnozic {
namespace spempe {

using SyncId = std::int64_t;
constexpr SyncId SYNC_ID_NEW = 0;

class SynchronizedObject;

class SynchronizedObjectRegistry 
    : public hg::util::NonCopyable
    , public hg::util::NonMoveable {
public:
    SynchronizedObjectRegistry(hg::RN_NodeInterface& node);

    void setNode(hg::RN_NodeInterface& node);

    SyncId registerMasterObject(SynchronizedObject* object);
    void registerDummyObject(SynchronizedObject* object, SyncId masterSyncId);
    void unregisterObject(SynchronizedObject* object);

    SynchronizedObject* getMapping(SyncId syncId) const;

    void syncObjectCreate(const SynchronizedObject* object);
    void syncObjectUpdate(const SynchronizedObject* object);
    void syncObjectDestroy(const SynchronizedObject* object);

    void syncStateUpdates();
    void syncCompleteState(hg::PZInteger clientIndex);

private:
    std::unordered_map<SyncId, SynchronizedObject*> _mappings;
    std::unordered_set<const SynchronizedObject*> _newlyCreatedObjects;
    std::unordered_set<const SynchronizedObject*> _alreadyUpdatedObjects;
    std::unordered_set<const SynchronizedObject*> _alreadyDestroyedObjects;
    std::vector<hg::PZInteger> _recepientVec;
    SyncId _syncIdCounter = 2;
    hg::RN_NodeInterface* _node;
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_SYNCHRONIZED_OBJECT_REGISTRY_HPP

