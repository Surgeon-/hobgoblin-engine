
cmake_minimum_required(VERSION 3.14)

project("TheRavine" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Conan setup
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Collect all sources
set(THERAVINE_SOURCE_ALL
    "Source/Main.cpp"
    "Source/World.cpp"
)

# Compilation configuration
add_compile_definitions("HOBGOBLIN_SHORT_NAMESPACE")

set(THERAVINE_TARGET_NAME
    ${PROJECT_NAME}
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    link_directories(../../EngineCore/Build/Hobgoblin-x64-Debug/lib)
else()
    link_directories(../../EngineCore/Build/Hobgoblin-x64-Release/lib)
endif()

add_executable(${THERAVINE_TARGET_NAME}
    ${THERAVINE_SOURCE_ALL}
)

target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/ChipmunkPhysics/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/Common/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/Graphics/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/Math/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/Preprocessor/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/Private/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/QAO/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/RigelNet/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC ../../EngineCore/Modules/Utility/Include)
target_include_directories(${THERAVINE_TARGET_NAME} PUBLIC source)

target_link_libraries(${THERAVINE_TARGET_NAME} PUBLIC
    "Hobgoblin.Common"
    "Hobgoblin.Utility"
    "Hobgoblin.ChipmunkPhysics"
    "Hobgoblin.Graphics"
    "Hobgoblin.QAO"
    "Hobgoblin.RigelNet"
    "CONAN_PKG::sfml"
)

# if(WIN32)
#     target_link_libraries(${THERAVINE_TARGET_NAME} PRIVATE legacy_stdio_definitions.lib)
# endif()

# set_target_properties(${TYPHON_TARGET_NAME} PROPERTIES LINK_FLAGS "/PROFILE")