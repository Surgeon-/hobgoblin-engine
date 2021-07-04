#ifndef UHOBGOBLIN_GR_TEXTURE_PACKING_HPP
#define UHOBGOBLIN_GR_TEXTURE_PACKING_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Utility/Rectangle.hpp>
#include <SFML/Graphics.hpp>

#include <vector>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace gr {

enum class TexturePackingHeuristic {
	BestShortSideFit,
	BestLongSideFit,
	BestAreaFit,
	BottomLeftRule,
	ContactPointRule
};

std::vector<util::Rectangle<PZInteger>> PackTexture(sf::Texture& texture, 
                                                    const std::vector<sf::Image*>& images,
                                                    TexturePackingHeuristic heuristic, 
                                                    float* occupancy = nullptr);

} // namespace gr
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_GR_TEXTURE_PACKING_HPP