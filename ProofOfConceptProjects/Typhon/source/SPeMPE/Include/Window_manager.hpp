#ifndef SPEMPE_WINDOW_MANAGER
#define SPEMPE_WINDOW_MANAGER

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <SPeMPE/Include/Game_object_framework.hpp>
#include <SPeMPE/Include/Keyboard_input.hpp>
#include <SPeMPE/Include/Window_manager_interface.hpp>

#include <optional>

namespace jbatnozic {
namespace spempe {

namespace hg = jbatnozic::hobgoblin;

class WindowManager
    : public WindowManagerInterface
    , public NonstateObject {
public:
    WindowManager(hg::QAO_RuntimeRef aRuntimeRef,
                  int aExecutionPriority);

    ///////////////////////////////////////////////////////////////////////////
    // CONFIGURATION                                                         //
    ///////////////////////////////////////////////////////////////////////////

    void setToHeadlessMode(const TimingConfig& aTimingConfig) override;

    void setToNormalMode(const WindowConfig& aWindowConfig,
                         const MainRenderTextureConfig& aMainRenderTextureConfig,
                         const TimingConfig& aTimingConfig) override;

    ///////////////////////////////////////////////////////////////////////////
    // WINDOW MANAGEMENT                                                     //
    ///////////////////////////////////////////////////////////////////////////

    //! LEGACY
    [[deprecated]]
    sf::RenderWindow& getWindow(); // TODO Temp.

    sf::Vector2u getWindowSize() const override;

    bool getWindowHasFocus() const override;

    ///////////////////////////////////////////////////////////////////////////
    // GRAPHICS & DRAWING                                                    //
    ///////////////////////////////////////////////////////////////////////////

    //! LEGACY
    [[deprecated]]
    sf::RenderTexture& getMainRenderTexture();

    hg::gr::Canvas& getCanvas() override;

    void drawMainRenderTexture(DrawPosition drawPosition); // TODO Temp. - To private

    void setMainRenderTextureDrawPosition(DrawPosition aDrawPosition) override;

    ///////////////////////////////////////////////////////////////////////////
    // VIEWS                                                                 //
    ///////////////////////////////////////////////////////////////////////////

    void setViewCount(hg::PZInteger viewCount) override;

    hg::PZInteger getViewCount() const override;

    sf::View& getView(hg::PZInteger viewIndex = 0) override;

    const sf::View& getView(hg::PZInteger viewIndex = 0) const override;

    ///////////////////////////////////////////////////////////////////////////
    // GUI                                                                   //
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // KEYBOARD & MOUSE INPUT                                                //
    ///////////////////////////////////////////////////////////////////////////

    KbInputTracker& getKeyboardInput();

    // TODO MouseInputTracker& getMouseInput();

    sf::Vector2f getMousePos(hg::PZInteger viewIndex = 0) const override { // TODO - To .cpp file
        auto pixelPos = sf::Mouse::getPosition(*_window);
        return _window->mapPixelToCoords(pixelPos, getView(viewIndex));
    }

private:
    // Configuration:
    bool _headless;

    std::chrono::microseconds _deltaTime;
    bool _preciseTiming = true;
    hg::util::Stopwatch _frameDurationStopwatch;

    // Window management:
    std::optional<sf::RenderWindow> _window;

    // Graphics & drawing:
    std::optional<sf::RenderTexture> _mainRenderTexture;
    std::optional<hg::gr::CanvasAdapter> _windowToCanvasAdapter;

    DrawPosition _mainRenderTextureDrawPos = DrawPosition::Fit;

    // Views:
    std::optional<hg::gr::MultiViewRenderTargetAdapter> _mainRenderTextureViewAdapter;

    // GUI:

    // Keyboard & mouse input:
    KbInputTracker _kbi;

    void _eventPostUpdate() override;
    void _eventDraw2() override;
    void _eventFinalizeFrame() override;

    void _drawMainRenderTexture();
    void _finalizeFrameByDisplayingWindow();
    void _finalizeFrameBySleeping();
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_WINDOW_MANAGER

