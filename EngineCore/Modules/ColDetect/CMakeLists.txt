# ===== COMPONENT SETUP =====

set(COMPONENT_NAME "Hobgoblin.ColDetect")

# ===== GATHER SOURCES ======

set(COMPONENT_SOURCES
  "Source/Qtree_collision_domain.cpp"
)

# ===== TARGET SETUP =====

add_library(${COMPONENT_NAME} STATIC ${COMPONENT_SOURCES})
target_include_directories(${COMPONENT_NAME} PUBLIC "Include/")

target_link_libraries(${COMPONENT_NAME}
PUBLIC
  "Hobgoblin.Private"
  "Hobgoblin.Common"
  "Hobgoblin.Utility"
  "CONAN_PKG::sfml"
)