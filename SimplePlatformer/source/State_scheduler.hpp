#ifndef STATE_SCHEDULER_HPP
#define STATE_SCHEDULER_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Utility/Exceptions.hpp>
#include <Hobgoblin/Utility/Math.hpp>

#include <algorithm>
#include <deque>
#include <vector>

template <class TState>
class StateScheduler {
public:
    StateScheduler(hg::PZInteger defaultDelay);

    // Main:

    void reset(hg::PZInteger defaultDelay);

    void putNewState(const TState& newState, hg::PZInteger delay = 0);

    void scheduleNewStates();

    void advance();

    void setDiscardIfOld(bool discardIfTooOld);

    // Extra/utility:

    void advanceDownTo(hg::PZInteger maxSize);

    hg::PZInteger getDefaultDelay() const noexcept;

    // Access stored states:

    TState& getCurrentState();
    TState& getLatestState();

    const TState& getCurrentState() const;
    const TState& getLatestState() const;

    typename std::deque<TState>::iterator begin();
    typename std::deque<TState>::iterator end();

    typename std::deque<TState>::const_iterator cbegin() const;
    typename std::deque<TState>::const_iterator cend() const;

private:
    std::deque<TState> _stateBuffer;
    hg::PZInteger _stateBufferMinSize;

    std::vector<TState> _newStates;
    hg::PZInteger _newStatesDelay;

    // NOTE: "Blue" states are those that have been put into the scheduler
    // explicitly by the user. Inferred states are "Red", though they are not
    // explicitly tracked.
    TState _latestBlueState;
    int _blueTailPos = -1;

    bool _discardIfTooOld = false;

    void setAt(const TState& state, hg::PZInteger pos) {
        if (hg::ToPz(_stateBuffer.size()) <= pos) {
            _stateBuffer.resize(hg::ToSz(pos + 1));
        }

        _stateBuffer[hg::ToSz(pos)] = state;
    }
    
    void extendLatest() {
        for (int i = _blueTailPos + 1; i < _stateBufferMinSize; i += 1) {
            _stateBuffer[hg::ToSz(i)] = _latestBlueState;
        }
    }
};

template <class TState>
StateScheduler<TState>::StateScheduler(hg::PZInteger defaultDelay)
    : _stateBufferMinSize{defaultDelay + 1}
    , _blueTailPos{-1}
{
    _stateBuffer.resize(hg::ToSz(_stateBufferMinSize));
}

// Main:

template <class TState>
void StateScheduler<TState>::reset(hg::PZInteger defaultDelay) {
    _stateBuffer.clear();
    _stateBuffer.resize(hg::ToSz(defaultDelay + 1));

    _stateBufferMinSize = defaultDelay + 1;

    _newStates.clear();

    _blueTailPos = -1;
}

template <class TState>
void StateScheduler<TState>::putNewState(const TState& newState, hg::PZInteger delay) {
    if (_blueTailPos != -1) {
        setAt(newState, _blueTailPos + 1);
        _blueTailPos += 1;
    }
    else {
        _newStates.push_back(newState);
        _newStatesDelay = delay;
    }

    _latestBlueState = newState;
}

template <class TState>
void StateScheduler<TState>::scheduleNewStates() {
    if (_newStates.empty()) {
        return;
    }

    int pos = _stateBufferMinSize - _newStatesDelay - hg::ToPz(_newStates.size());

    if (pos >= 0) {
        for (auto& state : _newStates) {
            setAt(state, pos);
            pos += 1;
        }
        _blueTailPos = pos - 1;
    }
    else if (_discardIfTooOld) {
        hg::PZInteger stateSelector = -1 * pos;
        pos = 0;

        while (stateSelector < hg::ToPz(_newStates.size())) {
            setAt(_newStates[hg::ToSz(stateSelector)], pos);
            stateSelector += 1;
            pos += 1;
        }
        _blueTailPos = pos - 1;
    }
    else {
        pos = 0;

        for (auto& state : _newStates) {
            setAt(state, pos);
            pos += 1;
        }
        _blueTailPos = pos - 1;
    }

    _newStates.clear();

    extendLatest();
}

template <class TState>
void StateScheduler<TState>::advance() {
    if (hg::ToPz(_stateBuffer.size()) <= _stateBufferMinSize) {
        _stateBuffer.push_back(_latestBlueState);
    }

    if (_blueTailPos >= 0) {
        _blueTailPos -= 1;
    }

    _stateBuffer.pop_front();
}

template <class TState>
void StateScheduler<TState>::setDiscardIfOld(bool discardIfTooOld) {
    _discardIfTooOld = discardIfTooOld;
}

// Extra/utility:

template <class TState>
void StateScheduler<TState>::advanceDownTo(hg::PZInteger maxSize) {
    if (maxSize < _stateBufferMinSize) {
        throw hg::util::TracedLogicError("MaxSize must be at least (defaultDelay + 1)");
    }

    while (hg::ToPz(_stateBuffer.size()) > maxSize) {
        advance();
    }
}

template <class TState>
hg::PZInteger StateScheduler<TState>::getDefaultDelay() const noexcept {
    return _stateBufferMinSize - 1;
}

// Access states:

template <class TState>
TState& StateScheduler<TState>::getCurrentState() {
    return _stateBuffer.front();
}

template <class TState>
TState& StateScheduler<TState>::getLatestState() {
    return _latestBlueState;
}

template <class TState>
const TState& StateScheduler<TState>::getCurrentState() const {
    return _stateBuffer.front();
}

template <class TState>
const TState& StateScheduler<TState>::getLatestState() const {
    return _latestBlueState;
}

template <class TState>
typename std::deque<TState>::iterator StateScheduler<TState>::begin() {
    return _stateBuffer.begin();
}

template <class TState>
typename std::deque<TState>::iterator StateScheduler<TState>::end() {
    return _stateBuffer.end();
}

template <class TState>
typename std::deque<TState>::const_iterator StateScheduler<TState>::cbegin() const {
    return _stateBuffer.cbegin();
}

template <class TState>
typename std::deque<TState>::const_iterator StateScheduler<TState>::cend() const {
    return _stateBuffer.cend();
}

#endif // !STATE_SCHEDULER_HPP

