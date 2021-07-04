#ifndef SPEMPE_NETWORKING_MANAGER_HPP
#define SPEMPE_NETWORKING_MANAGER_HPP

#include <Hobgoblin/RigelNet.hpp>
#include <Hobgoblin/RigelNet_Macros.hpp>
#include <SPeMPE/Include/Game_object_framework.hpp>
#include <SPeMPE/Include/Networking_manager_interface.hpp>

#include <list>
#include <memory>
#include <optional>

namespace jbatnozic {
namespace spempe {

//! One concrete implementation of NetworkingManagerInterface.
//! TODO: Tip on setting execution priority?
class NetworkingManager
    : public NetworkingManagerInterface
    , public NonstateObject {
public:
    NetworkingManager(hg::QAO_RuntimeRef aRuntimeRef,
                      int aExecutionPriority,
                      hg::PZInteger aStateBufferingLength);

    ///////////////////////////////////////////////////////////////////////////
    // CONFIGURATION                                                         //
    ///////////////////////////////////////////////////////////////////////////

    void setToMode(Mode aMode) override;

    Mode getMode() const override;

    bool isUninitialized() const override;
    bool isDummy() const override;
    bool isServer() const override;
    bool isClient() const override;

    ///////////////////////////////////////////////////////////////////////////
    // NODE ACCESS                                                           //
    ///////////////////////////////////////////////////////////////////////////

    NodeType& getNode() const override;

    ServerType& getServer() const override;

    ClientType& getClient() const override;

    ///////////////////////////////////////////////////////////////////////////
    // LISTENER MANAGEMENT                                                   //
    ///////////////////////////////////////////////////////////////////////////

    void addEventListener(NetworkingEventListener& aListener) override;

    void removeEventListener(NetworkingEventListener& aListener) override;

    ///////////////////////////////////////////////////////////////////////////
    // SYNCHRONIZATION                                                       //
    ///////////////////////////////////////////////////////////////////////////

    SynchronizedObjectRegistry& getSyncObjReg() override;

    hg::PZInteger getStateBufferingLength() const override;

    void setStateBufferingLength(hg::PZInteger aNewStateBufferingLength) override;

    ///////////////////////////////////////////////////////////////////////////
    // MISC.                                                                 //
    ///////////////////////////////////////////////////////////////////////////

    int getLocalPlayerIndex() override;

private:
    Mode _mode = Mode::Uninitialized;
    std::optional<int> _localPlayerIndex = PLAYER_INDEX_NONE;

    std::unique_ptr<hg::RN_NodeInterface> _node;
    std::unique_ptr<SynchronizedObjectRegistry> _syncObjReg;

    std::vector<NetworkingEventListener*> _eventListeners;

    hg::PZInteger _stateBufferingLength;

    void _eventPreUpdate() override;
    void _eventPostUpdate() override;

    void _handleNetworkingEvents();
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_NETWORKING_MANAGER_HPP

