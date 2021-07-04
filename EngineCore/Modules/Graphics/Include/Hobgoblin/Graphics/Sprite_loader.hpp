#ifndef UHOBGOBLIN_GR_SPRITE_LOADER_HPP
#define UHOBGOBLIN_GR_SPRITE_LOADER_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Graphics/Multisprite.hpp>
#include <Hobgoblin/Graphics/Texture_packing.hpp>
#include <Hobgoblin/Utility/Rectangle.hpp>

#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_BEGIN
namespace gr {

using TextureHandle = int;

constexpr struct AutoIndexType {} AUTO_INDEX;

// TODO use std::filesystem::path instead of path strings
class SpriteLoader {
public:

    TextureHandle addTexture(PZInteger width, PZInteger height);
    sf::Texture& getTexture(TextureHandle textureHandle);

    // Loading from file:
    SpriteLoader& loadFromFile(TextureHandle textureHandle, PZInteger spriteIndex,
                               PZInteger subspriteIndex, const std::string& filePath);

    SpriteLoader& loadFromFile(TextureHandle textureHandle, PZInteger spriteIndex,
                               AutoIndexType, const std::string& filePath);

    SpriteLoader& loadFromFile(TextureHandle textureHandle, std::string spriteName,
                               PZInteger subspriteIndex, const std::string& filePath);

    SpriteLoader& loadFromFile(TextureHandle textureHandle, std::string spriteName,
                               AutoIndexType, const std::string& filePath);

    // Loading from directory:
    SpriteLoader& loadFromDirectory(TextureHandle textureHandle, PZInteger spriteIndex, const std::string& filePath);
    SpriteLoader& loadFromDirectory(TextureHandle textureHandle, std::string spriteName, const std::string& filePath);

    // Loading from memory:
    // TODO loadFromMemory

    // Search loaded sprites:
    const Multisprite& getSprite(PZInteger spriteIndex) const;
    const Multisprite& getSprite(const std::string& spriteName) const;

    // Other:
    void finalize(TexturePackingHeuristic heuristic);
    void clear();

private:
    struct SpriteData {
        Multisprite multisprite;

        struct PackRequest {
            sf::Image image;
            TextureHandle textureHandle;
            util::Rectangle<PZInteger> rextureRect;
            bool unused = true;
        };

        std::vector<PackRequest> packRequests;
    };

    std::vector<sf::Texture> _textures;
    std::vector<SpriteData> _indexedSprites;
    std::unordered_map<std::string, SpriteData> _mappedSprites;
    bool _isFinalized = false;

    void assertNotFinalized() const;
};

} // namespace gr
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_GR_SPRITE_LOADER_HPP