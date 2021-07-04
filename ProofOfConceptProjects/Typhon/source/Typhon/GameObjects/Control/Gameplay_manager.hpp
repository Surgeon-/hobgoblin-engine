#ifndef TYPHON_GAME_OBJECTS_CONTROL_GAMEPLAY_MANAGER_HPP
#define TYPHON_GAME_OBJECTS_CONTROL_GAMEPLAY_MANAGER_HPP

#include <Typhon/Framework.hpp>

class GameplayManager 
    : public NonstateObject
    , private spempe::NetworkingEventListener {
public:
    GameplayManager(QAO_RuntimeRef runtimeRef);

    ~GameplayManager();

    // TODO bool isGamePaused() const noexcept;

    void restartGame();

protected:
    void _eventUpdate() override;
    void _eventPostUpdate() override;
    void _eventDrawGUI() override;

private:
    void onNetworkingEvent(const RN_Event& event) override;

    void _setPlayerPosition(hg::PZInteger playerIndex, float* x, float* y) const;
};

#endif // !TYPHON_GAME_OBJECTS_CONTROL_GAMEPLAY_MANAGER_HPP

