cmake_minimum_required(VERSION 3.14)

project("SPeMPE" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ===== CONAN INTEGRATION =====

include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup(TARGETS)

# ===== OVERLAY SETUP =====

set(OVERLAY_NAME "spempe")

# ===== GATHER SOURCES ======

set(OVERLAY_SOURCES
  "Source/Context_components.cpp"
  "Source/Game_object_bases.cpp"
  "Source/Game_context.cpp"
  "Source/Networking_manager_one.cpp"
  "Source/Synchronized_object_registry.cpp"
  "Source/Window_manager_one.cpp"

  # "Source/Temp_demo.cpp"
)

# ===== TARGET SETUP =====

add_library(${OVERLAY_NAME} STATIC ${OVERLAY_SOURCES})
target_include_directories(${OVERLAY_NAME} PUBLIC "Include/")

# Hobgoblin includes
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/ChipmunkPhysics/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/Common/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/Graphics/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/Math/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/Preprocessor/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/Private/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/QAO/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/RigelNet/Include")
target_include_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Modules/Utility/Include")

# Hobgoblin libs
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Build/Hobgoblin-x64-Debug/lib")
else()
    target_link_directories(${OVERLAY_NAME} PUBLIC "../../EngineCore/Build/Hobgoblin-x64-Release/lib")
endif()

target_link_libraries(${OVERLAY_NAME}
PUBLIC
  "Hobgoblin.Common"
  "Hobgoblin.Graphics"
  "Hobgoblin.QAO"
  "Hobgoblin.RigelNet"
  "Hobgoblin.Utility"
  "CONAN_PKG::sfml"
  "CONAN_PKG::ztcpp"
)

# ===== BUILD TESTS =====

set(OVERLAY_TEST_NAME "spempe-test")

set(OVERLAY_TEST_SOURCES
  "Test/SPeMPE_test.cpp"
)

add_executable(${OVERLAY_TEST_NAME} ${OVERLAY_TEST_SOURCES})

target_link_libraries(${OVERLAY_TEST_NAME}
PUBLIC
  "${OVERLAY_NAME}"
  "CONAN_PKG::gtest"
)

# ===== BUILD EXAMPLES =====

add_executable("minimal-multiplayer"
  "Examples/Minimal_multiplayer.cpp"
)

target_link_libraries("minimal-multiplayer"
PUBLIC
  "${OVERLAY_NAME}"
)

add_executable("Temp"
  "Examples/Temp.cpp"
)

target_link_libraries("Temp"
PUBLIC
  "CONAN_PKG::sfml"
)