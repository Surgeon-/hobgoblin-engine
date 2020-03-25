#ifndef CONTROLS_MANAGER_HPP
#define CONTROLS_MANAGER_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Utility/Autopack.hpp>

#include <cassert>
#include <deque>
#include <vector>

#include "Game_object_framework.hpp"

struct PlayerControls {
    bool left = false;
    bool right = false;
    bool up = false;

    HG_ENABLE_AUTOPACK(PlayerControls, left, right, up);

    void integrate(const PlayerControls& other) {
        left = left || other.left;
        right = right || other.right;
        up = up || other.up;
    }

    void unstuck(const std::deque<PlayerControls>& history) {
        PlayerControls dummy{};

        for (const auto& controls : history) {
            dummy.integrate(controls);
        }

        left = left && dummy.left;
        right = right && dummy.right;
        up = up && dummy.up;
    }
};

class ControlsScheduler {
public:
    ControlsScheduler(hg::PZInteger defaultDelayInSteps)
        : _newControlsDelay{0} 
    {
        setInputDelay(defaultDelayInSteps);
    }

    void setInputDelay(hg::PZInteger inputDelayInSteps) {
        assert(inputDelayInSteps >= 0);
        if (_controlsQueue.size() != inputDelayInSteps + 1) {
            _controlsQueue.clear();
            _controlsQueue.resize(inputDelayInSteps + 1);
            
            _rawHistory.clear();
            _rawHistory.resize(inputDelayInSteps + 1);
        }
    }

    const PlayerControls& getCurrentControls() const {
        return _controlsQueue.front();
    }

    const PlayerControls& getLatestControls() const {
        return _controlsQueue.back();
    }

    void advanceStep() {
        _controlsQueue.push_back(_controlsQueue.back());
        _controlsQueue.pop_front();
    }

    void putNewControls(const PlayerControls& newControls, std::chrono::microseconds delay) {
        _newControlsQueue.push_back(newControls);
        _newControlsDelay = delay;

        _rawHistory.pop_front();
        _rawHistory.push_back(newControls);
    }

    void integrateNewControls() {
        int offset = _newControlsDelay / std::chrono::microseconds{16'666}; // TODO Refactor magic number (deltaTime)
        int place = _controlsQueue.size() - (offset + 1);

        for (auto iter = _newControlsQueue.rbegin(); iter != _newControlsQueue.rend(); iter = std::next(iter)) {
            place = std::max(0, place);
            if (place == _controlsQueue.size() - 1) {
                _controlsQueue[place] = *iter;
            }
            else {
                _controlsQueue[place].integrate(*iter);
            }
            place -= 1;
        }

        _newControlsQueue.clear();

        for (auto& controls : _controlsQueue) {
            controls.unstuck(_rawHistory);
        }
    }

private:
    std::deque<PlayerControls> _controlsQueue;
    std::deque<PlayerControls> _rawHistory;
    std::vector<PlayerControls> _newControlsQueue;
    std::chrono::microseconds _newControlsDelay;
};

class ControlsManager : public GOF_Base {
public:
    ControlsManager(QAO_Runtime* runtime, hg::PZInteger playerCount, hg::PZInteger inputDelayInSteps);

    void setPlayerCount(hg::PZInteger playerCount);
    void setInputDelay(hg::PZInteger inputDelayInSteps);
    PlayerControls getCurrentControlsForPlayer(hg::PZInteger playerIndex);

    void putNewControls(hg::PZInteger playerIndex, const PlayerControls& controls, std::chrono::microseconds delay);

protected:
    void eventPreUpdate() override;
    void eventUpdate() override;
    void eventPostUpdate() override;

private:
    std::vector<ControlsScheduler> _schedulers;
};

#endif // !CONTROLS_MANAGER_HPP

