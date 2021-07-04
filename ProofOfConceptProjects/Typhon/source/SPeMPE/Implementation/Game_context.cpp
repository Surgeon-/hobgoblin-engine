
#include <Hobgoblin/Common.hpp>
#include <SPeMPE/Include/Game_context.hpp>

#include <algorithm>
#include <cassert>

namespace jbatnozic {
namespace spempe {

GameContext::RuntimeConfig::RuntimeConfig(hg::PZInteger targetFramerate, hg::PZInteger maxFramesBetweenDisplays)
    : _targetFramerate{ targetFramerate }
    , _deltaTime{ 1.0 / static_cast<double>(targetFramerate) }
    , _maxFramesBetweenDisplays{ maxFramesBetweenDisplays }
{
    assert(targetFramerate > 0);
    assert(maxFramesBetweenDisplays > 0);
}

hg::PZInteger GameContext::RuntimeConfig::getTargetFramerate() const noexcept {
    return _targetFramerate;
}

std::chrono::duration<double> GameContext::RuntimeConfig::getDeltaTime() const noexcept {
    return _deltaTime;
}

hg::PZInteger GameContext::RuntimeConfig::getMaxFramesBetweenDisplays() const noexcept {
    return _maxFramesBetweenDisplays;
}

GameContextExtensionData::~GameContextExtensionData() {
}

// ///////////////////////////////////////////////////////////////////////////////////////////// //

GameContext::GameContext(const ResourceConfig& resourceConfig, const RuntimeConfig& runtimeConfig)
    : _resourceConfig{resourceConfig}
    , _runtimeConfig{runtimeConfig}
    , _qaoRuntime{this}
    , _networkingManager{_qaoRuntime.nonOwning()} //! LEGACY
    , _syncObjReg{_networkingManager.getNode()} //! LEGACY
    , _components{100} // TODO
    , _extensionData{ nullptr }
{
    _networkingManager.getNode().setUserData(this);
}

//! LEGACY (old impl)
GameContext::~GameContext() {
    _qaoRuntime.eraseAllNonOwnedObjects();
    _extensionData.reset();
    _postStepActions.clear();
}

///////////////////////////////////////////////////////////////////////////
// CONFIGURATION                                                         //
///////////////////////////////////////////////////////////////////////////

//! LEGACY (old impl)
void GameContext::setToMode(Mode mode) {
    _mode = mode;

    // Configure local player index:
    if (_mode == Mode::Server || _mode == Mode::Initial) {
        _localPlayerIndex = PLAYER_INDEX_NONE;
    }
    else if (_mode == Mode::Client) {
        _localPlayerIndex = PLAYER_INDEX_UNKNOWN;
    }
    else {
        _localPlayerIndex = PLAYER_INDEX_LOCAL_PLAYER;
    }

    // Configure NetworkingManager:
    if (hasNetworking()) {
        if (isPrivileged()) {
            _networkingManager.initializeAsServer();
        }
        else {
            _networkingManager.initializeAsClient();
        }
    }
    else {
        // TODO _networkingManager.reset();
    }
    _syncObjReg.setNode(_networkingManager.getNode());
}

bool GameContext::isPrivileged() const {
    return ((static_cast<int>(_mode) & detail::GCMF_PRIV) != 0);
}

bool GameContext::isHeadless() const {
    return ((static_cast<int>(_mode) & detail::GCMF_HDLS) != 0);
}

bool GameContext::hasNetworking() const {
    return ((static_cast<int>(_mode) & detail::GCMF_NETW) != 0);
}

const GameContext::ResourceConfig& GameContext::getResourceConfig() const {
    return _resourceConfig;
}

const GameContext::RuntimeConfig& GameContext::getRuntimeConfig() const {
    return _runtimeConfig;
}

void GameContext::addPostStepAction(hg::PZInteger delay, std::function<void(GameContext&)> action) {
    if (isPrivileged()) {
        throw hg::TracedLogicError{"Cannot add a post step action on host context without the "
                                   "ALLOW_ON_HOST switch."};
    }

    _insertPostStepAction(std::move(action), delay);
}

void GameContext::addPostStepAction(hg::PZInteger delay, GameContext_AllowOnHost_Type,
                                    std::function<void(GameContext&)> action) {
    _insertPostStepAction(std::move(action), delay);
}

void GameContext::setLocalPlayerIndex(int index) {
    _localPlayerIndex = index;
}

int GameContext::getLocalPlayerIndex() const {
    return _localPlayerIndex;
}

void GameContext::setExtensionData(std::unique_ptr<GameContextExtensionData> extData) {
    _extensionData = std::move(extData);
}

GameContextExtensionData* GameContext::getExtensionData() const {
    return _extensionData.get();
}

///////////////////////////////////////////////////////////////////////////
// GAME OBJECT MANAGEMENT                                                //
///////////////////////////////////////////////////////////////////////////

hg::QAO_Runtime& GameContext::getQaoRuntime() {
    return _qaoRuntime;
}

///////////////////////////////////////////////////////////////////////////
// COMPONENT MANAGEMENT                                                  //
///////////////////////////////////////////////////////////////////////////

void GameContext::detachComponent(ContextComponent& aComponent) {
    _components.detachComponent(aComponent);

    _ownedComponents.erase(
        std::remove_if(_ownedComponents.begin(), _ownedComponents.end(),
        [&aComponent](std::unique_ptr<ContextComponent>& aCurr) {
            return aCurr.get() == &aComponent;
        }), _ownedComponents.end());
}

std::string GameContext::getComponentTableString(char aSeparator) const {
    return _components.toString(aSeparator);
}

NetworkingManager& GameContext::getNetworkingManager() {
    return _networkingManager;
}

SynchronizedObjectRegistry& GameContext::getSyncObjReg() {
    return _syncObjReg;
}

///////////////////////////////////////////////////////////////////////////
// EXECUTION                                                             //
///////////////////////////////////////////////////////////////////////////

int GameContext::runFor(int aSteps) {
    int rv;
    _quit.store(false);
    _runImpl(this, aSteps, &rv);
    return rv;
}

void GameContext::stop() {
    if (hasChildContext() && isChildContextJoinable()) {
        const auto childRv = stopAndJoinChildContext();
        // TODO Remove temporary cout
        std::cout << "Child context stopped with exit code " << childRv << '\n';
    }

    _quit.store(true);
}

const GameContext::PerformanceInfo& GameContext::getPerformanceInfo() const {
    return _performanceInfo;
}

int GameContext::getCurrentStepOrdinal() const {
    return _stepOrdinal;
}

///////////////////////////////////////////////////////////////////////////
// CHILD CONTEXT SUPPORT                                                 //
///////////////////////////////////////////////////////////////////////////

void GameContext::attachChildContext(std::unique_ptr<GameContext> aChildContext) {
    if (hasChildContext()) {
        throw hg::TracedLogicError{"A child context is already attached"};
    }
    _childContext = std::move(aChildContext);
    _childContext->_parentContext = this;
}

std::unique_ptr<GameContext> GameContext::detachChildContext() {
    if (hasChildContext() && isChildContextJoinable()) {
        throw hg::TracedLogicError("Cannot detach a running child context - stop and join it first");
    }

    return std::move(_childContext);
}

bool GameContext::hasChildContext() const {
    return (_childContext != nullptr);
}

bool GameContext::isChildContextJoinable() const {
    if (!hasChildContext()) {
        throw hg::TracedLogicError{"No child context is currently attached"};
    }
    return _childContextThread.joinable();
}

GameContext* GameContext::getChildContext() const {
    return _childContext.get();
}

void GameContext::startChildContext(int aSteps) {
    if (!hasChildContext()) {
        throw hg::TracedLogicError{"No child context is currently attached"};
    }

    if (_childContextThread.joinable()) {
        throw hg::TracedLogicError{"The previous child context must be stopped and joined first"};
    }

    _childContext->_quit.store(false);
    _childContextThread = std::thread{_runImpl, _childContext.get(), aSteps, &_childContextReturnValue};
}

int GameContext::stopAndJoinChildContext() {
    if (!hasChildContext()) {
        return _childContextReturnValue;
    }

    _childContext->stop();

    if (_childContextThread.joinable()) {
        _childContextThread.join();
    }

    return _childContextReturnValue;
}

///////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS                                                       //
///////////////////////////////////////////////////////////////////////////

namespace {

#ifdef NDEBUG
    // If we're in Debug mode, we don't want to catch and handle
    // exceptions, but rather let the IDE break the program.
    #define CATCH_EXCEPTIONS_TOP_LEVEL
#endif // !NDEBUG

int DoSingleQaoIteration(hg::QAO_Runtime& runtime, std::int32_t eventFlags) {
    runtime.startStep();
    bool done = false;
    do {
#ifdef CATCH_EXCEPTIONS_TOP_LEVEL
        try {
            runtime.advanceStep(done, eventFlags);
        }
        catch (std::exception& ex) {
            std::cout << "Exception caught: " << ex.what() << '\n';
            return 1;
        }
        catch (...) {
            std::cout << "Unknown exception caught!\n";
            return 2;
        }
#else
        runtime.advanceStep(done, eventFlags);
#endif
    } while (!done);

    return 0;
}

using hg::QAO_Event;
using hg::QAO_ALL_EVENT_FLAGS;

constexpr std::int32_t ToEventMask(QAO_Event::Enum ev) {
    return (1 << static_cast<std::int32_t>(ev));
}

constexpr std::int32_t QAO_EVENT_MASK_ALL_DRAWS = ToEventMask(QAO_Event::Draw1)
                                                | ToEventMask(QAO_Event::Draw2)
                                                | ToEventMask(QAO_Event::DrawGUI);

constexpr std::int32_t QAO_EVENT_MASK_FINALIZE = ToEventMask(QAO_Event::FinalizeFrame);

constexpr std::int32_t QAO_EVENT_MASK_ALL_EXCEPT_DRAW = QAO_ALL_EVENT_FLAGS & ~QAO_EVENT_MASK_ALL_DRAWS;

constexpr std::int32_t QAO_EVENT_MASK_ALL_EXCEPT_FINALIZE = QAO_ALL_EVENT_FLAGS & ~QAO_EVENT_MASK_FINALIZE;

constexpr std::int32_t QAO_EVENT_MASK_ALL_EXCEPT_DRAW_AND_FINALIZE = QAO_EVENT_MASK_ALL_EXCEPT_DRAW
                                                                   & QAO_EVENT_MASK_ALL_EXCEPT_FINALIZE;

using TimingDuration = std::chrono::duration<double, std::micro>;
using std::chrono::milliseconds;
using std::chrono::microseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;

} // namespace

void GameContext::_runImpl(hg::not_null<GameContext*> aContext,
                           int aMaxSteps,
                           hg::not_null<int*> aReturnValue) {
    if (aMaxSteps == 0) {
        return;
    }

    const auto maxFramesBetweenDisplays = aContext->_runtimeConfig.getMaxFramesBetweenDisplays();
    const TimingDuration deltaTime = aContext->_runtimeConfig.getDeltaTime();
    TimingDuration accumulatorTime = deltaTime;
    auto currentTime = steady_clock::now();

    PerformanceInfo perfInfo;
    hg::util::Stopwatch frameToFrameStopwatch;

    hg::PZInteger stepsCovered = 0;

    while (aContext->_quit.load() == false) {
        if (aMaxSteps > 0 && stepsCovered >= aMaxSteps) {
            break;
        }

        perfInfo.frameToFrameTime = frameToFrameStopwatch.restart<microseconds>();

        aContext->_stepOrdinal += 1;

        auto now = steady_clock::now();
        accumulatorTime += duration_cast<TimingDuration>(now - currentTime);
        currentTime = now;

        for (int i = 0; i < maxFramesBetweenDisplays; i += 1) {
            if (aMaxSteps > 0 && stepsCovered >= aMaxSteps) {
                break;
            }
            if ((accumulatorTime < deltaTime)) {
                break;
            }

            // Run all events except FinalizeFrame (and Draws if headless):
            {
                hg::util::Stopwatch stopwatch;
                if (aContext->isHeadless()) {
                    (*aReturnValue) = DoSingleQaoIteration(aContext->_qaoRuntime,
                                                           QAO_EVENT_MASK_ALL_EXCEPT_DRAW_AND_FINALIZE);
                }
                else {
                    (*aReturnValue) = DoSingleQaoIteration(aContext->_qaoRuntime,
                                                           QAO_EVENT_MASK_ALL_EXCEPT_FINALIZE);
                }
                if ((*aReturnValue) != 0) {
                    return;
                }
                perfInfo.updateAndDrawTime = stopwatch.getElapsedTime<microseconds>();
            }

            perfInfo.consecutiveUpdateLoops = i + 1;
            accumulatorTime -= deltaTime;
            stepsCovered += 1;
        } // End for

        // Prevent buildup in accumulator in case the program is not meeting time requirements
        accumulatorTime = std::min(accumulatorTime, deltaTime * 0.5);

        // FinalizeFrame event:
        {
            hg::util::Stopwatch stopwatch;
            (*aReturnValue) = DoSingleQaoIteration(aContext->_qaoRuntime, QAO_EVENT_MASK_FINALIZE);
            if ((*aReturnValue) != 0) {
                return;
            }
            perfInfo.finalizeTime = stopwatch.getElapsedTime<microseconds>();
        }

        // Do post step actions:
        aContext->_pollPostStepActions();

        // Record performance data:
        perfInfo.totalTime = perfInfo.updateAndDrawTime + perfInfo.finalizeTime;
        aContext->_performanceInfo = perfInfo;
    } // End while

    (*aReturnValue) = 0;
}

//! LEGACY
void GameContext::_insertPostStepAction(std::function<void(GameContext&)> action, hg::PZInteger delay) {
    const auto listIndex = hg::ToSz(delay);
    if (listIndex >= _postStepActions.size()) {
        _postStepActions.resize(listIndex + 1);
    }

    auto& list = _postStepActions[listIndex];

    list.push_back(action);
}

// LEGACY
void GameContext::_pollPostStepActions() {
    if (_postStepActions.empty()) {
        return;
    }

    auto& list = _postStepActions.front();

    for (auto& action : list) {
        action(*this);
    }

    _postStepActions.pop_front();
}

} // namespace spempe
} // namespace jbatnozic