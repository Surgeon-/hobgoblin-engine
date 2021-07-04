
#include <SPeMPE/Include/Game_context.hpp>
#include <SPeMPE/Include/Window_manager.hpp>

#include <algorithm>
#include <iostream>

#define HARD_ASSERT(_expr_) do { if (!(_expr_)) { \
        throw ::jbatnozic::hobgoblin::TracedLogicError{#_expr_}; \
    } } while (false)

namespace jbatnozic {
namespace spempe {

WindowManager::WindowManager(hg::QAO_RuntimeRef aRuntimeRef,
                             int aExecutionPriority)
    : NonstateObject{aRuntimeRef, SPEMPE_TYPEID_SELF, aExecutionPriority, "jbatnozic::spempe::WindowManagerOne"}
    , _window{}
    , _mainRenderTexture{}
    , _windowToCanvasAdapter{}
    , _mainRenderTextureViewAdapter{}
{
}

///////////////////////////////////////////////////////////////////////////
// CONFIGURATION                                                         //
///////////////////////////////////////////////////////////////////////////

void WindowManager::setToHeadlessMode(const TimingConfig& aTimingConfig) {
    _headless = true;

    _mainRenderTextureViewAdapter.reset();
    _windowToCanvasAdapter.reset();
    _mainRenderTexture.reset();
    _window.reset();

    // Set timing parameters:
    _deltaTime = std::chrono::microseconds{1'000'000 / aTimingConfig.targetFramerate};
    _preciseTiming = aTimingConfig.preciseTiming;
}

void WindowManager::setToNormalMode(const WindowConfig& aWindowConfig,
                                    const MainRenderTextureConfig& aMainRenderTextureConfig,
                                    const TimingConfig& aTimingConfig) {
    _headless = false;

    // Create window:
    _window.emplace(aWindowConfig.videoMode,
                    aWindowConfig.title,
                    aWindowConfig.style,
                    aWindowConfig.openGlContextSettings);

    // Create main render texture:
    _mainRenderTexture.emplace();
    _mainRenderTexture->create(aMainRenderTextureConfig.size.x,
                               aMainRenderTextureConfig.size.y,
                               aMainRenderTextureConfig.openGlContextSettings);
    _mainRenderTexture->setSmooth(aMainRenderTextureConfig.smooth);

    // Create adapters:
    _windowToCanvasAdapter.emplace(*_window);
    _mainRenderTextureViewAdapter.emplace(*_mainRenderTexture);

    // Create default view:
    const auto w = static_cast<float>(aMainRenderTextureConfig.size.x);
    const auto h = static_cast<float>(aMainRenderTextureConfig.size.y);

    _mainRenderTextureViewAdapter->setViewCount(1);
    _mainRenderTextureViewAdapter->getView(0).setSize({w, h});
    _mainRenderTextureViewAdapter->getView(0).setCenter({w * 0.5f, h * 0.5f});
    _mainRenderTextureViewAdapter->getView(0).setViewport({0.f, 0.f, 1.f, 1.f});

    // Set timing parameters:
    _deltaTime = std::chrono::microseconds{1'000'000 / aTimingConfig.targetFramerate};
    _window->setFramerateLimit(aTimingConfig.framerateLimiter ? aTimingConfig.targetFramerate : 0);
    _window->setVerticalSyncEnabled(aTimingConfig.verticalSync);
    _preciseTiming = aTimingConfig.preciseTiming;
}

///////////////////////////////////////////////////////////////////////////
// WINDOW MANAGEMENT                                                     //
///////////////////////////////////////////////////////////////////////////

sf::RenderWindow& WindowManager::getWindow() {
    return *_window;
}

sf::Vector2u WindowManager::getWindowSize() const {
    return _window->getSize();
}

bool WindowManager::getWindowHasFocus() const {
    return _window->hasFocus();
}

///////////////////////////////////////////////////////////////////////////
// GRAPHICS & DRAWING                                                    //
///////////////////////////////////////////////////////////////////////////

sf::RenderTexture& WindowManager::getMainRenderTexture() {
    return *_mainRenderTexture;
}

hg::gr::Canvas& WindowManager::getCanvas() {
    HARD_ASSERT(!_headless);
    if (getRuntime()->getCurrentEvent() == hg::QAO_Event::DrawGUI) {
        return *_windowToCanvasAdapter;
    }
    else {
        return *_mainRenderTextureViewAdapter;
    }
}

void WindowManager::setMainRenderTextureDrawPosition(DrawPosition aDrawPosition) {
    _mainRenderTextureDrawPos = aDrawPosition;
}

///////////////////////////////////////////////////////////////////////////
// VIEWS                                                                 //
///////////////////////////////////////////////////////////////////////////

void WindowManager::setViewCount(hg::PZInteger aViewCount) {
    HARD_ASSERT(!_headless);
    _mainRenderTextureViewAdapter->setViewCount(aViewCount);
}

hg::PZInteger WindowManager::getViewCount() const {
    HARD_ASSERT(!_headless);
    return _mainRenderTextureViewAdapter->getViewCount();
}

sf::View& WindowManager::getView(hg::PZInteger aViewIndex) {
    HARD_ASSERT(!_headless);
    return _mainRenderTextureViewAdapter->getView(aViewIndex);
}

const sf::View& WindowManager::getView(hg::PZInteger aViewIndex) const {
    HARD_ASSERT(!_headless);
    return _mainRenderTextureViewAdapter->getView(aViewIndex);
}

///////////////////////////////////////////////////////////////////////////
// GUI                                                                   //
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// KEYBOARD & MOUSE INPUT                                                //
///////////////////////////////////////////////////////////////////////////

KbInputTracker& WindowManager::getKeyboardInput() {
    return _kbi;
}

///////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS                                                       //
///////////////////////////////////////////////////////////////////////////

void WindowManager::_eventPostUpdate() {
    if (!_headless) {
        _mainRenderTexture->clear(hg::gr::Color::Black); // TODO Parametrize colour
    }
}

void WindowManager::_eventDraw2() {
    if (!_headless) {
        _window->clear(sf::Color::Black); // TODO Parametrize colour
        _drawMainRenderTexture();
    }
}

void WindowManager::_eventFinalizeFrame() {
    if (!_headless) {
        _finalizeFrameByDisplayingWindow();
    }
    else {
        if (_preciseTiming) {
            _finalizeFrameBySleeping();
        }
    }
}

void WindowManager::_drawMainRenderTexture() {
    _mainRenderTexture->display();
    sf::Sprite mrtSprite{_mainRenderTexture->getTexture()};
    const sf::Vector2u mrtSize = _mainRenderTexture->getSize();
    const sf::Vector2u winSize = _window->getSize();

    switch (_mainRenderTextureDrawPos) {
    case DrawPosition::None:
        return;
        break;

    case DrawPosition::Fill:
    case DrawPosition::Fit:
    {
        float scale;
        if (_mainRenderTextureDrawPos == DrawPosition::Fill) {
            scale = std::max(static_cast<float>(winSize.x) / mrtSize.x,
                             static_cast<float>(winSize.y) / mrtSize.y);
        }
        else {
            scale = std::min(static_cast<float>(winSize.x) / mrtSize.x,
                             static_cast<float>(winSize.y) / mrtSize.y);
        }
        mrtSprite.setScale({scale, scale});
    }
    // FALLTHROUGH

    case DrawPosition::Centre:
        mrtSprite.setOrigin({static_cast<float>(mrtSize.x) * 0.5f,
                            static_cast<float>(mrtSize.y) * 0.5f});
        mrtSprite.setPosition({static_cast<float>(winSize.x) * 0.5f,
                              static_cast<float>(winSize.y) * 0.5f});
        break;

    case DrawPosition::Stretch:
        mrtSprite.setScale({static_cast<float>(winSize.x) / mrtSize.x,
                           static_cast<float>(winSize.y) / mrtSize.y});
        break;

    case DrawPosition::TopLeft:
        mrtSprite.setOrigin({0.f, 0.f});
        mrtSprite.setPosition({0.f, 0.f});
        break;

    default:
        assert(false && "Unreachable");
    }

    _window->draw(mrtSprite);
}

void WindowManager::_finalizeFrameByDisplayingWindow() {
    _window->display();

    _kbi.prepForEvents();
    sf::Event ev;
    while (_window->pollEvent(ev)) {
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
            sf::FloatRect visibleArea(0.f,
                                      0.f,
                                      static_cast<float>(ev.size.width),
                                      static_cast<float>(ev.size.height));
            _window->setView(sf::View(visibleArea));
        }
        break;

        case sf::Event::TextEntered:
            _kbi.onTextEvent(ev);
            break;

        default: (void)0;
        }
    }

    if (_preciseTiming) {
        // I don't know if this is a good idea, but with vSync on, it waits in random places, including
        // in the draw events, which screws up timing measurements. So with this call, we force it to
        // synchronize during the finalizeFrame event.
        _finalizeFrameBySleeping();
    }
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