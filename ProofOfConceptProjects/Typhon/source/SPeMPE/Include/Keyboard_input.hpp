#ifndef SPEMPE_KEYBOARD_INPUT_HPP
#define SPEMPE_KEYBOARD_INPUT_HPP

#include <Hobgoblin/Common.hpp>
#include <SFML/System/String.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <vector>

namespace jbatnozic {
namespace spempe {

enum class KbKey {
    Unknown = sf::Keyboard::Unknown,   ///< Unhandled key
    A = sf::Keyboard::A,         ///< The A key
    B = sf::Keyboard::B,         ///< The B key
    C = sf::Keyboard::C,         ///< The C key
    D = sf::Keyboard::D,         ///< The D key
    E = sf::Keyboard::E,         ///< The E key
    F = sf::Keyboard::F,         ///< The F key
    G = sf::Keyboard::G,         ///< The G key
    H = sf::Keyboard::H,         ///< The H key
    I = sf::Keyboard::I,         ///< The I key
    J = sf::Keyboard::J,         ///< The J key
    K = sf::Keyboard::K,         ///< The K key
    L = sf::Keyboard::L,         ///< The L key
    M = sf::Keyboard::M,         ///< The M key
    N = sf::Keyboard::N,         ///< The N key
    O = sf::Keyboard::O,         ///< The O key
    P = sf::Keyboard::P,         ///< The P key
    Q = sf::Keyboard::Q,         ///< The Q key
    R = sf::Keyboard::R,         ///< The R key
    S = sf::Keyboard::S,         ///< The S key
    T = sf::Keyboard::T,         ///< The T key
    U = sf::Keyboard::U,         ///< The U key
    V = sf::Keyboard::V,         ///< The V key
    W = sf::Keyboard::W,         ///< The W key
    X = sf::Keyboard::X,         ///< The X key
    Y = sf::Keyboard::Y,         ///< The Y key
    Z = sf::Keyboard::Z,         ///< The Z key
    Num0 = sf::Keyboard::Num0,      ///< The 0 key
    Num1 = sf::Keyboard::Num1,      ///< The 1 key
    Num2 = sf::Keyboard::Num2,      ///< The 2 key
    Num3 = sf::Keyboard::Num3,      ///< The 3 key
    Num4 = sf::Keyboard::Num4,      ///< The 4 key
    Num5 = sf::Keyboard::Num5,      ///< The 5 key
    Num6 = sf::Keyboard::Num6,      ///< The 6 key
    Num7 = sf::Keyboard::Num7,      ///< The 7 key
    Num8 = sf::Keyboard::Num8,      ///< The 8 key
    Num9 = sf::Keyboard::Num9,      ///< The 9 key
    Escape = sf::Keyboard::Escape,    ///< The Escape key
    LControl = sf::Keyboard::LControl,  ///< The left Control key
    LShift = sf::Keyboard::LShift,    ///< The left Shift key
    LAlt = sf::Keyboard::LAlt,      ///< The left Alt key
    LSystem = sf::Keyboard::LSystem,   ///< The left OS specific key: window (Windows and Linux), apple (MacOS X), ...
    RControl = sf::Keyboard::RControl,  ///< The right Control key
    RShift = sf::Keyboard::RShift,    ///< The right Shift key
    RAlt = sf::Keyboard::RAlt,      ///< The right Alt key
    RSystem = sf::Keyboard::RSystem,   ///< The right OS specific key: window (Windows and Linux), apple (MacOS X), ...
    Menu = sf::Keyboard::Menu,      ///< The Menu key
    LBracket = sf::Keyboard::LBracket,  ///< The [ key
    RBracket = sf::Keyboard::RBracket,  ///< The ] key
    Semicolon = sf::Keyboard::Semicolon, ///< The ; key
    Comma = sf::Keyboard::Comma,     ///< The , key
    Period = sf::Keyboard::Period,    ///< The . key
    Quote = sf::Keyboard::Quote,     ///< The ' key
    Slash = sf::Keyboard::Slash,     ///< The / key
    Backslash = sf::Keyboard::Backslash, ///< The \ key
    Tilde = sf::Keyboard::Tilde,     ///< The ~ key
    Equal = sf::Keyboard::Equal,     ///< The = key
    Hyphen = sf::Keyboard::Hyphen,    ///< The - key (hyphen)
    Space = sf::Keyboard::Space,     ///< The Space key
    Enter = sf::Keyboard::Enter,     ///< The Enter/Return keys
    Backspace = sf::Keyboard::Backspace, ///< The Backspace key
    Tab = sf::Keyboard::Tab,       ///< The Tabulation key
    PageUp = sf::Keyboard::PageUp,    ///< The Page up key
    PageDown = sf::Keyboard::PageDown,  ///< The Page down key
    End = sf::Keyboard::End,       ///< The End key
    Home = sf::Keyboard::Home,      ///< The Home key
    Insert = sf::Keyboard::Insert,    ///< The Insert key
    Delete = sf::Keyboard::Delete,    ///< The Delete key
    Add = sf::Keyboard::Add,       ///< The + key
    Subtract = sf::Keyboard::Subtract,  ///< The - key (minus, usually from numpad)
    Multiply = sf::Keyboard::Multiply,  ///< The * key
    Divide = sf::Keyboard::Divide,    ///< The / key
    Left = sf::Keyboard::Left,      ///< Left arrow
    Right = sf::Keyboard::Right,     ///< Right arrow
    Up = sf::Keyboard::Up,        ///< Up arrow
    Down = sf::Keyboard::Down,      ///< Down arrow
    Numpad0 = sf::Keyboard::Numpad0,   ///< The numpad 0 key
    Numpad1 = sf::Keyboard::Numpad1,   ///< The numpad 1 key
    Numpad2 = sf::Keyboard::Numpad2,   ///< The numpad 2 key
    Numpad3 = sf::Keyboard::Numpad3,   ///< The numpad 3 key
    Numpad4 = sf::Keyboard::Numpad4,   ///< The numpad 4 key
    Numpad5 = sf::Keyboard::Numpad5,   ///< The numpad 5 key
    Numpad6 = sf::Keyboard::Numpad6,   ///< The numpad 6 key
    Numpad7 = sf::Keyboard::Numpad7,   ///< The numpad 7 key
    Numpad8 = sf::Keyboard::Numpad8,   ///< The numpad 8 key
    Numpad9 = sf::Keyboard::Numpad9,   ///< The numpad 9 key
    F1 = sf::Keyboard::F1,        ///< The F1 key
    F2 = sf::Keyboard::F2,        ///< The F2 key
    F3 = sf::Keyboard::F3,        ///< The F3 key
    F4 = sf::Keyboard::F4,        ///< The F4 key
    F5 = sf::Keyboard::F5,        ///< The F5 key
    F6 = sf::Keyboard::F6,        ///< The F6 key
    F7 = sf::Keyboard::F7,        ///< The F7 key
    F8 = sf::Keyboard::F8,        ///< The F8 key
    F9 = sf::Keyboard::F9,        ///< The F9 key
    F10 = sf::Keyboard::F10,       ///< The F10 key
    F11 = sf::Keyboard::F11,       ///< The F11 key
    F12 = sf::Keyboard::F12,       ///< The F12 key
    F13 = sf::Keyboard::F13,       ///< The F13 key
    F14 = sf::Keyboard::F14,       ///< The F14 key
    F15 = sf::Keyboard::F15,       ///< The F15 key
    Pause = sf::Keyboard::Pause,     ///< The Pause key

    KeyCount, ///< Keep last -- the total number of supported keyboard keys
};

enum class KbMode {
    Default,
    Edge,
    Direct,
    Repeat
};

class KbInputTracker {
public:
    KbInputTracker();

    void prepForEvents();
    void onKeyEvent(sf::Event ev);
    void onTextEvent(sf::Event ev);

    bool keyPressed(KbKey key, KbMode mode = KbMode::Default) const;
    bool keyReleased(KbKey key, KbMode mode = KbMode::Default) const;

    void setKeyPressed(KbKey key, bool pressed);
    void clearKey(KbKey key);

    sf::String inputString;
    void setInputStringMaxLength(hg::PZInteger maxLength);
    hg::PZInteger getInputStringMaxLength();

private:
    struct KeyControlBlock {
        bool isPressed = false;
        bool isPressedPrev = false;
        bool justPressed = false;
        bool cleared = false;
    };

    std::vector<KeyControlBlock> _controlBlocks;
    std::size_t _inputStringMaxLength = 256;

    void cullInputString();
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_KEYBOARD_INPUT_HPP

