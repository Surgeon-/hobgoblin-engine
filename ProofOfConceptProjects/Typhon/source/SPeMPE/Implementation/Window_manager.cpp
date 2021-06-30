
#include <SPeMPE/Include/Game_context.hpp>
#include <SPeMPE/Include/Window_manager.hpp>

#include <algorithm>
#include <iostream>

namespace jbatnozic {
namespace spempe {

WindowManager::WindowManager(hg::QAO_RuntimeRef runtimeRef)
    : NonstateObject{ runtimeRef, SPEMPE_TYPEID_SELF, 0, "spempe::WindowManager" }
    , _windowAdapter{ _window }
    , _mainRenderTextureAdapter{ _mainRenderTexture }
{
}

void WindowManager::create() {
    _window.create(sf::VideoMode(1280, 720), "Window");
    // vSync and Framerate limiter perform near identically as far as
    // time correctness is concerned, but the game is usually smoother
    // with vSync. TODO Make it a toggle in the settings.
    _window.setVerticalSyncEnabled(true);
    //_window.setFramerateLimit(60);

    _mainRenderTexture.create(1920, 1080);
    _mainRenderTexture.setSmooth(true);
    _mainRenderTextureAdapter.setViewCount(1);
    _mainRenderTextureAdapter.getView(0).setSize({ 1920, 1080 });
    _mainRenderTextureAdapter.getView(0).setCenter({ 1920 / 2, 1080 / 2 });
    _mainRenderTextureAdapter.getView(0).setViewport({ 0, 0, 1, 1 });
}

void WindowManager::init(sf::VideoMode windowVideoMode,
                         const sf::String& windowTitle,
                         sf::Vector2u mainRenderTextureSize,
                         sf::Uint32 windowStyle,
                         const sf::ContextSettings& windowContextSettings,
                         const sf::ContextSettings& mainRenderTextureContextSettings) {
    // Create window:
    _window.create(windowVideoMode, windowTitle, windowStyle, windowContextSettings);

    // Create main render texture:
    _mainRenderTexture.create(mainRenderTextureSize.x, mainRenderTextureSize.y, mainRenderTextureContextSettings);

    // Create default view:
    _mainRenderTextureAdapter.setViewCount(1);
    _mainRenderTextureAdapter.getView(0).setSize(static_cast<float>(mainRenderTextureSize.x),
                                                 static_cast<float>(mainRenderTextureSize.y));
    _mainRenderTextureAdapter.getView(0).setViewport({ 0.f, 0.f, 1.f, 1.f });
}

void WindowManager::initAsHeadless() {
    _isHeadless = true;
}

sf::RenderWindow& WindowManager::getWindow() {
    return _window;
}

sf::RenderTexture& WindowManager::getMainRenderTexture() {
    return _mainRenderTexture;
}

hg::gr::Canvas& WindowManager::getCanvas() {
    if (getRuntime()->getCurrentEvent() == hg::QAO_Event::DrawGUI) {
        return _windowAdapter;
    }
    else {
        return _mainRenderTextureAdapter;
    }
}

KbInputTracker& WindowManager::getKeyboardInput() {
    return _kbi;
}

void WindowManager::drawMainRenderTexture(DrawPosition drawPosition) {
    _mainRenderTexture.display();
    sf::Sprite mrtSprite{ _mainRenderTexture.getTexture() };
    const sf::Vector2u mrtSize = _mainRenderTexture.getSize();
    const sf::Vector2u winSize = _window.getSize();

    switch (drawPosition) {
    case DrawPosition::Stretch:
        mrtSprite.setScale({ static_cast<float>(winSize.x) / mrtSize.x,
                            static_cast<float>(winSize.y) / mrtSize.y });
        break;

    case DrawPosition::Fill:
    case DrawPosition::Fit:
    {
        float scale;
        if (drawPosition == DrawPosition::Fill) {
            scale = std::max(static_cast<float>(winSize.x) / mrtSize.x,
                             static_cast<float>(winSize.y) / mrtSize.y);
        }
        else {
            scale = std::min(static_cast<float>(winSize.x) / mrtSize.x,
                             static_cast<float>(winSize.y) / mrtSize.y);
        }
        mrtSprite.setScale({ scale, scale });
    }
    // FALLTHROUGH

    case DrawPosition::Centre:
        mrtSprite.setOrigin({ static_cast<float>(mrtSize.x) / 2.f,
                             static_cast<float>(mrtSize.y) / 2.f });
        mrtSprite.setPosition({ static_cast<float>(winSize.x) / 2.f,
                               static_cast<float>(winSize.y) / 2.f });
        break;

    default:
        assert(0 && "Unreachable");
    }

    _window.draw(mrtSprite);
}

void WindowManager::_eventPostUpdate() {
    _mainRenderTexture.clear(hg::gr::Color::Black);
}

void WindowManager::_eventDraw2() {
    _window.clear(sf::Color::Black);
    drawMainRenderTexture(DrawPosition::Fit);
}

void WindowManager::_eventFinalizeFrame() {
    if (!_isHeadless) {
        _finalizeFrameByDisplayingWindow();
    }
    else {
        _finalizeFrameBySleeping();
    }
}

// Views:

void WindowManager::setViewCount(hg::PZInteger viewCount) {
    _mainRenderTextureAdapter.setViewCount(viewCount);
}

hg::PZInteger WindowManager::getViewCount() const noexcept {
    return _mainRenderTextureAdapter.getViewCount();
}

sf::View& WindowManager::getView(hg::PZInteger viewIndex) {
    return _mainRenderTextureAdapter.getView(viewIndex);
}

const sf::View& WindowManager::getView(hg::PZInteger viewIndex) const {
    return _mainRenderTextureAdapter.getView(viewIndex);
}

void WindowManager::_finalizeFrameByDisplayingWindow() {
    _window.display();

    _kbi.prepForEvents();
    sf::Event ev;
    while (_window.pollEvent(ev)) {
        switch (ev.type) {
        case sf::Event::Closed:
            // TODO
            break;

        case sf::Event::KeyPressed:
        case sf::Event::KeyReleased:
            _kbi.onKeyEvent(ev);
            break;

        case sf::Event::Resized:
        {
            sf::FloatRect visibleArea(0, 0, ev.size.width, ev.size.height);
            _window.setView(sf::View(visibleArea));
        }
        break;

        case sf::Event::TextEntered:
            _kbi.onTextEvent(ev);
            break;

        default: (void)0;
        }
    }

    // I don't know if this is a good idea, but with vSync on, it waits in random places, including
    // in the draw events, which screws up timing measurements. So with this call, we force it to
    // synchronize during the finalizeFrame event.
    _finalizeFrameBySleeping();
}

void WindowManager::_finalizeFrameBySleeping() {
    using Time = std::chrono::microseconds;
    const auto deltaTime = std::chrono::duration_cast<Time>(ctx().getRuntimeConfig().getDeltaTime());
    const auto timePassed = _frameDurationStopwatch.getElapsedTime<Time>();

    if (timePassed < deltaTime) {
        hg::util::SuperPreciseSleep(deltaTime - timePassed);
    }

    _frameDurationStopwatch.restart();
}

} // namespace spempe
} // namespace jbatnozic