
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <algorithm>
#include <iosfwd>
#include <stdexcept>
#include <vector>

struct Block {
    float x = -1.0;
    float y = -1.0;
    float z = -1.0;

    float xSize = 1.0;
    float ySize = 1.0;
    float zSize = 1.0;
};

sf::Sprite g_BlockSprite;

class WorldMatrix {
public:
    WorldMatrix(std::size_t xSize, std::size_t ySize, std::size_t zSize)
        : _xSize{xSize}
        , _ySize{ySize}
        , _zSize{zSize}
    {
        _blocks.resize(xSize * ySize * zSize);

        for (std::size_t x = 0; x < _xSize; x += 1) {
            for (std::size_t y = 0; y < _ySize; y += 1) {
                for (std::size_t z = 0; z < _zSize; z += 1) {
                    auto& block = at(x, y, z);
                    block.x = static_cast<float>(x);
                    block.y = static_cast<float>(y);
                    block.z = static_cast<float>(z);
                }
            }
        }
    }

    Block& at(std::size_t aX, std::size_t aY, std::size_t aZ) {
        return _blocks[(aX * _ySize * _zSize) + (aY * _zSize) + aZ];
    }

    Block& at_s(std::size_t aX, std::size_t aY, std::size_t aZ) {
        if (aX >= _xSize || aY >= _ySize || aZ >= _zSize) {
            throw std::invalid_argument{"Dimension not right"};
        }

        return _blocks.at((aX * _ySize * _zSize) + (aY * _zSize) + aZ);
    }

    void printBlocksInMemoryOrder() const {
        for (auto& block : _blocks) {
            std::printf("Block (%d, %d, %d)\n", (int)block.x, (int)block.y, (int)block.z);
        }
    }

    void printBlocksInDrawingOrder() const {
        std::vector<Block> sortedBlocks = [this]() {
            auto temp = _blocks;
            _sortToDrawOrder(temp);
            return temp;
        }();

        for (auto& block : sortedBlocks) {
            std::printf("Block (%d, %d, %d)\n", (int)block.x, (int)block.y, (int)block.z);
        }
    }

    void render(sf::RenderTarget& aRenderTarget) {
        std::vector<Block> sortedBlocks = [this]() {
            auto temp = _blocks;
            _sortToDrawOrder(temp);
            return temp;
        }();

        for (auto& block : sortedBlocks) {
            const float x = (block.y + block.x) * 16.f;
            const float y = (block.y - block.x) *  8.f - block.z * 16.f;

            if (block.y == 1.f) continue;

            g_BlockSprite.setPosition(x, y + 80.f);
            if (block.z == 0) {
                g_BlockSprite.setColor(sf::Color::Blue);
            }
            else {
                g_BlockSprite.setColor(sf::Color::White);
            }

            aRenderTarget.draw(g_BlockSprite);
        }
    }

private:
    std::size_t _xSize;
    std::size_t _ySize; 
    std::size_t _zSize;

    void _sortToDrawOrder(std::vector<Block>& aBlocks) const {
        std::sort(aBlocks.begin(), aBlocks.end(),
                  [](const Block& block1, const Block& block2) {
#if 0
                      const bool _1_in_front_of_2 =
                          (+block1.x + block1.xSize <= block2.x) ||
                          (-block1.y + block1.ySize <= -block2.y) /*||
                          (-block1.z + block1.zSize <= -block2.z)*/;

                      const bool _2_in_front_of_1 =
                          (+block2.x + block2.xSize <= block1.x) ||
                          (-block2.y + block2.ySize <= -block1.y) /*||
                          (-block2.z + block2.zSize <= -block1.z)*/;

                      if (_1_in_front_of_2 == _2_in_front_of_1) {
                          return std::addressof(block1) >= std::addressof(block2);
                      }

                      return !_1_in_front_of_2;
#endif
                      bool _1_in_front_of_2;
                      bool _2_in_front_of_1;

                      if (block1.x != block2.x || block1.y != block2.y) {
                          _1_in_front_of_2 = block1.x - block1.y < block2.x - block2.y;
                          _2_in_front_of_1 = block2.x - block2.y < block1.x - block1.y;
                      }
                      else {
                          _1_in_front_of_2 = block1.z > block2.z;
                          _2_in_front_of_1 = block2.z > block1.z;
                      }

                      if (_1_in_front_of_2 == _2_in_front_of_1) {
                          return std::addressof(block1) > std::addressof(block2);
                      }

                      return !_1_in_front_of_2;
                  });
    }

    std::vector<Block> _blocks;
};

#define WORLD_XSIZE  3
#define WORLD_YSIZE  3
#define WORLD_HEIGHT 3

int main(int argc, char* argv[]) {
    sf::Texture tex;
    tex.loadFromFile("C:\\Users\\Jovan\\Desktop\\block.png");
    g_BlockSprite.setTexture(tex, true);

    WorldMatrix world{WORLD_XSIZE, WORLD_YSIZE, WORLD_HEIGHT};

    std::printf("Blocks in memory order:\n");
    world.printBlocksInMemoryOrder();

    std::printf("Blocks in drawing order:\n");
    world.printBlocksInDrawingOrder();



    sf::RenderWindow window(sf::VideoMode(800, 800), "Isometrix");
    window.setFramerateLimit(60);

    window.setView(sf::View(sf::FloatRect(0.f, 0.f, 160.f, 160.f)));

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear();
        world.render(window);
        window.display();
    }



    return 0;
}