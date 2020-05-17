
#include <Hobgoblin/RigelNet.hpp>

#include "GameContext/Game_config.hpp"
#include "GameObjects/Framework/Execution_priorities.hpp"
#include "GameObjects/Framework/Game_object_framework.hpp"
#include "GameObjects/UI/Main_menu.hpp"
#include "Graphics/Sprites.hpp"
#include "Main/Main_impl.hpp"
#include "Terrain/Terrain.hpp"

// TODO catch exceptions in main

int MainImpl::run(int argc, char* argv[]) {
	// Setup:
	ResolveExecutionPriorities();
	hg::RN_IndexHandlers();

	// To avoid the risk of initializing later from multiple threads
	// at the same time.
	Terrain::initializeSingleton();

	auto spriteLoader = LoadAllSprites();
	GameContext::ResourceConfig resConfig;
	resConfig.spriteLoader = &spriteLoader;
	_gameContext = std::make_unique<GameContext>(resConfig, GameConfig::TARGET_FRAMERATE);
	QAO_PCreate<MainMenu>(_gameContext->qaoRuntime);

	// Run:
	const int rv = _gameContext->run();

	// Teardown:
	_gameContext.reset();

	// Return run's return value:
	return rv;
}