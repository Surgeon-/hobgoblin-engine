
#include <Hobgoblin/RigelNet.hpp>
#include <Hobgoblin/Common.hpp>

#include <Typhon/Framework.hpp>
#include <Typhon/GameObjects/UI/Main_menu.hpp>
#include <Typhon/Graphics/Sprites.hpp>
#include <Typhon/Terrain/Terrain.hpp>

#include <ZTCpp.hpp>

#include <cstdlib> // TODO Temp. for srand
#include <stdexcept>
#include <iostream>

#include "Main_impl.hpp"

namespace zt = jbatnozic::ztcpp;

class NodeInfo {
public:
  NodeInfo() = default;

  bool online = false;
  int networksJoinedCount = 0;
  uint64_t id = 0;
  zt::IpAddress ip4 = zt::IpAddress::ipv4Unspecified();
  zt::IpAddress ip6 = zt::IpAddress::ipv6Unspecified();
};

class ZeroTierEventHandler : public zt::IEventHandler {
public:
  ZeroTierEventHandler(NodeInfo& aNodeInfo)
    : _nodeInfo{aNodeInfo}
  {
  }

  void onAddressEvent(zt::EventCode::Address aEventCode, const zt::AddressDetails* aDetails) noexcept override {
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;

    if (aDetails && aEventCode == zt::EventCode::Address::AddedIPv4) {
      _nodeInfo.ip4 = aDetails->getIpAddress();
    }
  }

  void onNetworkEvent(zt::EventCode::Network aEventCode, const zt::NetworkDetails* aDetails) noexcept override {
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;

    if (aEventCode == zt::EventCode::Network::ReadyIPv4 || 
        aEventCode == zt::EventCode::Network::ReadyIPv6 ||
        aEventCode == zt::EventCode::Network::ReadyIPv4_IPv6) {
      _nodeInfo.networksJoinedCount += 1;
    }
    else if (aEventCode == zt::EventCode::Network::Down) {
      _nodeInfo.networksJoinedCount -= 1;
    }
  }

  void onNetworkInterfaceEvent(zt::EventCode::NetworkInterface aEventCode,
                               const zt::NetworkInterfaceDetails* aDetails) noexcept override{
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;
  }

  void onNetworkStackEvent(zt::EventCode::NetworkStack aEventCode,
                           const zt::NetworkStackDetails* aDetails) noexcept override {
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;
  }

  void onNodeEvent(zt::EventCode::Node aEventCode, const zt::NodeDetails* aDetails) noexcept override {
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;

    if (aEventCode == zt::EventCode::Node::Online) {
      _nodeInfo.online = true;
    }
    else if (aEventCode == zt::EventCode::Node::Online) {
      _nodeInfo.online = false;
    }
  }

  void onPeerEvent(zt::EventCode::Peer aEventCode, const zt::PeerDetails* aDetails) noexcept override {
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;
  }

  void onRouteEvent(zt::EventCode::Route aEventCode, const zt::RouteDetails* aDetails) noexcept override {
    std::cout << zt::EventDescription(aEventCode, aDetails) << std::endl;
  }

  void onUnknownEvent(int16_t aRawZeroTierEventCode) noexcept override {
    std::cout << "An unknown Zero Tier event was dispatched (" << aRawZeroTierEventCode << ")" << std::endl;
  }

private:
  NodeInfo& _nodeInfo;
};

NodeInfo localNode;
ZeroTierEventHandler eventHandler{localNode};

#define USE_ZT false

// #define MAIN_SHOULD_CATCH_EXCEPTIONS

int MainImpl::run(int argc, char* argv[]) 
#ifdef MAIN_SHOULD_CATCH_EXCEPTIONS
try
#endif // MAIN_SHOULD_CATCH_EXCEPTIONS
{
  if (USE_ZT) {
      std::string identityPath = "ZTNodeData";
      uint64_t nwid = strtoull("a09acf0233ceff5c", NULL, 16);
      int ztServicePort = 9994;
      zt::SetEventHandler(&eventHandler);

      printf("This node's identity is stored in %s\n", identityPath.c_str());

      printf("Starting ZeroTier service...\n");
      {
        const auto res = zt::StartService(identityPath, ztServicePort);
        ZTCPP_THROW_ON_ERROR(res, hg::TracedRuntimeError);
      }

      printf("Waiting for node to come online...\n");
      while (!localNode.online) { 
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
      }
  
      /*printf("Joining network %llx\n", nwid);
      printf("Don't forget to authorize this device in my.zerotier.com or the web API!\n");
      if ((err = zt::JoinNetwork(nwid)) != ZTS_ERR_OK) {
        printf("Unable to join network, error = %d. Exiting.\n", err);
        exit(1);
      }
  
      while (localNode.networksJoinedCount <= 0) {
        printf("Waiting to join network (networksJoinedCount = %d)...\n", localNode.networksJoinedCount);
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});
      }*/
      //std::this_thread::sleep_for(std::chrono::milliseconds{5000});
  }

	std::cout << GameConfig::GAME_NAME << ' ' << GameConfig::GAME_VERSION << '\n';

	// Setup:
	ResolveExecutionPriorities();
	hg::RN_IndexHandlers();
	std::srand(static_cast<unsigned>(std::time(NULL)));

	// To avoid the risk of initializing later from multiple threads
	// at the same time.
	Terrain::initializeSingleton();

	auto spriteLoader = LoadAllSprites();
	GameContext::ResourceConfig resConfig;
	resConfig.spriteLoader = &spriteLoader;
	GameContext::RuntimeConfig rtConfig{GameConfig::TARGET_FRAMERATE};
	_gameContext = std::make_unique<GameContext>(resConfig, rtConfig);
	ExtendGameContext(*_gameContext);
	QAO_PCreate<MainMenu>(&_gameContext->getQaoRuntime());

	// Run:
	const int retVal = _gameContext->runFor(-1);

	// Teardown:
	_gameContext.reset();

  if (USE_ZT) {
      printf("Shutting down service\n");
      zt::StopService();
      std::this_thread::sleep_for(std::chrono::milliseconds{1000});
      zt::SetEventHandler(nullptr);
  }

	// Return run's return value:
	return retVal;
}
#ifdef MAIN_SHOULD_CATCH_EXCEPTIONS
catch (hg::TracedException& ex) {
	std::cerr << "Unrecoverable traced exception caught in main: " << ex.what();
	ex.printStackTrace(std::cerr);
	return EXIT_FAILURE;
}
catch (std::exception& ex) {
	std::cerr << "Unrecoverable exception caught in main: " << ex.what();
	return EXIT_FAILURE;
}
catch (...) {
	std::cerr << "Unrecoverable exception caught in main: <unknown exception type>";
	return EXIT_FAILURE;
}
#endif // MAIN_SHOULD_CATCH_EXCEPTIONS