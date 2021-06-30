#ifndef SPEMPE_WINDOW_MANAGER
#define SPEMPE_WINDOW_MANAGER

#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Utility/Time_utils.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <SPeMPE/Include/Game_object_framework.hpp>
#include <SPeMPE/Include/Keyboard_input.hpp>

namespace jbatnozic {
namespace spempe {

class WindowManager : public NonstateObject {
public:
    enum class DrawPosition {
        Fill,
        Fit,
        Stretch,
        Centre
    };

    WindowManager(hg::QAO_RuntimeRef runtimeRef);

    void init(sf::VideoMode windowVideoMode,
              const sf::String& windowTitle,
              sf::Vector2u mainRenderTextureSize,
              sf::Uint32 windowStyle = sf::Style::Default,
              const sf::ContextSettings& windowContextSettings = sf::ContextSettings{},
              const sf::ContextSettings& mainRenderTextureContextSettings = sf::ContextSettings{}
    );

    void initAsHeadless();

    void create(); // TODO Temp.

    sf::RenderWindow& getWindow(); // TODO Temp.
    sf::RenderTexture& getMainRenderTexture();
    hg::gr::Canvas& getCanvas();

    void drawMainRenderTexture(DrawPosition drawPosition); // TODO Temp. - To private

    // Input:
    KbInputTracker& getKeyboardInput();

    // TODO MouseInputTracker& getMouseInput();

    sf::Vector2f getMousePos(hg::PZInteger viewIndex = 0) const { // TODO - To .cpp file
        auto pixelPos = sf::Mouse::getPosition(_window);
        return _window.mapPixelToCoords(pixelPos, getView(viewIndex));
    }

    // GUI:
    void setGuiScale();
    void getGuiScale() const;
    void getGuiWidth() const;
    void getGuiHeight() const;

    // Views:
    void setViewCount(hg::PZInteger viewCount);
    hg::PZInteger getViewCount() const noexcept;
    sf::View& getView(hg::PZInteger viewIndex = 0);
    const sf::View& getView(hg::PZInteger viewIndex = 0) const;

protected:
    void _eventPostUpdate() override;
    void _eventDraw2() override;
    void _eventFinalizeFrame() override;

private:
    sf::RenderWindow _window;
    sf::RenderTexture _mainRenderTexture;
    hg::gr::CanvasAdapter _windowAdapter;
    hg::gr::MultiViewRenderTargetAdapter _mainRenderTextureAdapter;
    KbInputTracker _kbi;
    hg::util::Stopwatch _frameDurationStopwatch;
    bool _isHeadless = false;

    void _finalizeFrameByDisplayingWindow();
    void _finalizeFrameBySleeping();
};

} // namespace spempe
} // namespace jbatnozic

#endif // !SPEMPE_WINDOW_MANAGER

