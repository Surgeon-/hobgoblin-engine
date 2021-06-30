
#include <Hobgoblin/Common.hpp>
#include <SPeMPE/Include/Keyboard_input.hpp>

#include <cassert>

namespace jbatnozic {
namespace spempe {

KbInputTracker::KbInputTracker() {
    _controlBlocks.resize(static_cast<std::size_t>(KbKey::KeyCount));
}

void KbInputTracker::prepForEvents() {
    for (auto& cb : _controlBlocks) {
        cb.isPressedPrev = cb.isPressed;
        cb.justPressed = false;
    }
}

void KbInputTracker::onKeyEvent(sf::Event ev) {
    if (ev.type == sf::Event::KeyPressed) {
        if (ev.key.code == sf::Keyboard::Unknown) {
            return;
        }

        auto& cb = _controlBlocks.at(static_cast<std::size_t>(ev.key.code));
        if (!cb.cleared) {
            cb.isPressed = true;
            cb.justPressed = true;
        }
    }
    else if (ev.type == sf::Event::KeyReleased) {
        if (ev.key.code == sf::Keyboard::Unknown) {
            return;
        }

        auto& cb = _controlBlocks.at(static_cast<std::size_t>(ev.key.code));
        cb.isPressed = false;
        cb.cleared = false;
    }
    else {
        throw hg::TracedLogicError("Invalid event passed");
    }
}

void KbInputTracker::onTextEvent(sf::Event ev) {
    if (ev.type == sf::Event::TextEntered) {
        const sf::Uint32 character = ev.text.unicode;
        // In a backspace is typed, erase the last typed character
        if (character == '\b') {
            if (!inputString.isEmpty()) {
                inputString.erase(inputString.getSize() - 1);
            }
        }
        // Otherwise, append the new character to the string
        else {
            inputString += character;
            cullInputString();
        }
    }
    else {
        throw hg::TracedLogicError("Invalid event passed");
    }
}

bool KbInputTracker::keyPressed(KbKey key, KbMode mode) const {
    const std::size_t keyIndex = static_cast<std::size_t>(key);

    switch (mode) {
    default:
    case KbMode::Default:
        return _controlBlocks[keyIndex].isPressed;

    case KbMode::Edge:
        return _controlBlocks[keyIndex].isPressed && !_controlBlocks[keyIndex].isPressedPrev;

    case KbMode::Direct:
        return sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(key));

    case KbMode::Repeat:
        return _controlBlocks[keyIndex].justPressed;
    }
}

bool KbInputTracker::keyReleased(KbKey key, KbMode mode) const {
    const std::size_t keyIndex = static_cast<std::size_t>(key);

    switch (mode) {
    default:
    case KbMode::Default:
        return !_controlBlocks[keyIndex].isPressed;

    case KbMode::Edge:
        return !_controlBlocks[keyIndex].isPressed && _controlBlocks[keyIndex].isPressedPrev;

    case KbMode::Direct:
        return !sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(key));

    case KbMode::Repeat:
        return false;
    }
}

void KbInputTracker::setKeyPressed(KbKey key, bool pressed) {
    auto& cb = _controlBlocks.at(static_cast<std::size_t>(key));
    cb.isPressed = pressed;
    cb.cleared = false;
}

void KbInputTracker::clearKey(KbKey key) {
    auto& cb = _controlBlocks.at(static_cast<std::size_t>(key));
    if (cb.isPressed) {
        cb.isPressed = false;
        cb.cleared = true;
    }
}

void KbInputTracker::setInputStringMaxLength(hg::PZInteger maxLength) {
    _inputStringMaxLength = hg::ToSz(maxLength);
    cullInputString();
}

hg::PZInteger KbInputTracker::getInputStringMaxLength() {
    return _inputStringMaxLength;
}

void KbInputTracker::cullInputString() {
    if (inputString.getSize() > _inputStringMaxLength) {
        inputString.erase(_inputStringMaxLength, inputString.getSize() - _inputStringMaxLength);
    }
}

} // namespace spempe
} // namespace jbatnozic