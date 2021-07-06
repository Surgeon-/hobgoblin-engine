#ifndef UHOBGOBLIN_UTIL_STATE_SCHEDULER_SIMPLE_HPP
#define UHOBGOBLIN_UTIL_STATE_SCHEDULER_SIMPLE_HPP

#include <Hobgoblin/Common.hpp>

#include <algorithm>
#include <cassert>
#include <vector>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace util {

template <class taState>
class SimpleStateScheduler {
public:
    SimpleStateScheduler(PZInteger aDefaultDelay);

    // Main:

    void reset(PZInteger aDefaultDelay);

    void putNewState(const taState& aNewState, PZInteger aDelay = 0);

    void scheduleNewStates();

    void advance();

    // Extra/utility:

    PZInteger getDefaultDelay() const noexcept;

    void setDefaultDelay(PZInteger aNewDefaultDelay);

    // Access stored states:

    taState& getCurrentState();
    taState& getLatestState();

    const taState& getCurrentState() const;
    const taState& getLatestState() const;

    typename std::vector<taState>::iterator begin();
    typename std::vector<taState>::iterator end();

    typename std::vector<taState>::const_iterator cbegin() const;
    typename std::vector<taState>::const_iterator cend() const;

private:
    std::vector<taState> _stateBuffer;

    PZInteger _defaultDelay; // Individual buffer size = _defaultDelay + 1

    int _newStatesBufferHand = 0; // Points to the latest state written to

    // 'Blue' states are ones that were received explicitly from the host
    // (were not inferred). The variable '_bluePos' tells us the position
    // of the latest such state we have in the buffer of scheduled states,
    // if any (it is set to 'BLUE_POS_NONE' otherwise). When scheduling
    // new states, if there are any existing blue states, the new states
    // go right after them (we can do this because Rigel reorders all
    // network packets into their original order).
    // It's important to keep BLUE_POS_NONE as -2 and not -1, because a
    // value of -1 signals that the next received state can be scheduled
    // to position 0.
    #define BLUE_POS_NONE (-2)
    int _bluePos = BLUE_POS_NONE;

    PZInteger _newStatesCount = 0;
    PZInteger _newStatesDelay = 0;

    PZInteger _individualBufferSize() const {
        return _defaultDelay + 1;
    }

    taState& _scheduledStateAt(PZInteger aIndex) {
        assert(aIndex < _individualBufferSize());
        return _stateBuffer[ToSz(aIndex % _individualBufferSize())];
    }

    const taState& _scheduledStateAt(PZInteger aIndex) const {
        assert(aIndex < _individualBufferSize());
        return _stateBuffer[ToSz(aIndex % _individualBufferSize())];
    }

    taState& _newStateAt(PZInteger aIndex) {
        assert(aIndex < _individualBufferSize());
        return _stateBuffer[ToSz(_individualBufferSize() + 
                                 (aIndex % _individualBufferSize()))];
    }

    const taState& _newStateAt(PZInteger aIndex) const {
        assert(aIndex < _individualBufferSize());
        return _stateBuffer[ToSz(_individualBufferSize() +
            (aIndex % _individualBufferSize()))];
    }
};

template <class taState>
SimpleStateScheduler<taState>::SimpleStateScheduler(PZInteger aDefaultDelay)
    : _defaultDelay{aDefaultDelay}
{
    // First half of the buffer is for the already scheduled states.
    // The second half is for the newly received ones.
    _stateBuffer.resize(ToSz(_defaultDelay + 1) * 2u);
}

// Main:

template <class taState>
void SimpleStateScheduler<taState>::reset(PZInteger aDefaultDelay) {
    _defaultDelay = aDefaultDelay;

    _stateBuffer.clear();
    _stateBuffer.resize(ToSz(_defaultDelay + 1) * 2u);

    _newStatesBufferHand = 0;
    _bluePos = BLUE_POS_NONE;

    _newStatesCount = 0;
    _newStatesDelay = 0;
}

template <class taState>
void SimpleStateScheduler<taState>::putNewState(const taState& aNewState, PZInteger aDelay) {
    _newStatesBufferHand = (_newStatesBufferHand + 1) % _individualBufferSize();
    _newStateAt(_newStatesBufferHand) = aNewState;
    _newStatesDelay = aDelay;
    _newStatesCount += 1;
}

template <class taState>
void SimpleStateScheduler<taState>::scheduleNewStates() {
    if (_newStatesCount == 0) {
        return;
    }

    // The algorithm assumes there will be at most 1 state update per frame
    // (unit of delay is a frame)
    _newStatesDelay = std::max(_newStatesDelay, _newStatesCount - 1);

    // Set a temporary 'hand' to the oldest received state
    int currNew = _newStatesBufferHand - std::min(_newStatesCount - 1, _individualBufferSize());
    if (currNew < 0) {
        currNew += _individualBufferSize();
    }

    // Determine where the oldest state should go
    int pos;
    if (_bluePos != BLUE_POS_NONE &&
        _newStatesCount + _bluePos < _individualBufferSize()) {
        // Classic StateScheduler behaviour (chain blue states)
        pos = _bluePos + 1;
    }
    else {
        // Default SimpleStateScheduler behaviour
        pos = _individualBufferSize() - 1 - _newStatesDelay;
    }

    // Transfer states
    _bluePos = BLUE_POS_NONE;
    for (PZInteger i = 0; i < _newStatesCount; i += 1) {
        if (pos >= 0) {
            _scheduledStateAt(pos) = _newStateAt(currNew);
            _bluePos = pos;
        }

        pos += 1;
        currNew = (currNew + 1) % _individualBufferSize();
    }

    // Fill the rest of the buffer with the newest state
    for (PZInteger i = std::max(pos, 0); i < _individualBufferSize(); i += 1) {
        _scheduledStateAt(i) = _newStateAt(_newStatesBufferHand);
    }
    
    // Prepare for next cycle
    _newStatesCount = 0;
}

template <class taState>
void SimpleStateScheduler<taState>::advance() {
    for (PZInteger i = 0; i < _individualBufferSize() - 1; i += 1) {
        _stateBuffer[ToSz(i)] = _stateBuffer[ToSz(i + 1)];
    }

    if (_bluePos != BLUE_POS_NONE) {
        _bluePos -= 1;
    }
}

// Extra/utility:

template <class taState>
PZInteger SimpleStateScheduler<taState>::getDefaultDelay() const noexcept {
    return _defaultDelay;
}

template <class taState>
void SimpleStateScheduler<taState>::setDefaultDelay(PZInteger aNewDefaultDelay) {
    // TODO Temporary quick-n-dirty implementation
    reset(aNewDefaultDelay);
}

// Access states:

template <class taState>
taState& SimpleStateScheduler<taState>::getCurrentState() {
    return _scheduledStateAt(0);
}

template <class taState>
taState& SimpleStateScheduler<taState>::getLatestState() {
    return _newStateAt(_newStatesBufferHand);
}

template <class taState>
const taState& SimpleStateScheduler<taState>::getCurrentState() const {
    return _scheduledStateAt(0);
}

template <class taState>
const taState& SimpleStateScheduler<taState>::getLatestState() const {
    return _newStateAt(_newStatesBufferHand);
}

template <class taState>
typename std::vector<taState>::iterator SimpleStateScheduler<taState>::begin() {
    return _stateBuffer.begin();
}

template <class taState>
typename std::vector<taState>::iterator SimpleStateScheduler<taState>::end() {
    return _stateBuffer.begin() + _defaultDelay;
}

template <class taState>
typename std::vector<taState>::const_iterator SimpleStateScheduler<taState>::cbegin() const {
    return _stateBuffer.cbegin();
}

template <class taState>
typename std::vector<taState>::const_iterator SimpleStateScheduler<taState>::cend() const {
    return _stateBuffer.begin() + _defaultDelay;
}

#undef BLUE_POS_NONE

} // namespace util
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_UTIL_STATE_SCHEDULER_SIMPLE_HPP