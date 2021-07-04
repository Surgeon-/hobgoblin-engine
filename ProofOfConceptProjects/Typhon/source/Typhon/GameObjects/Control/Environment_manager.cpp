
#include <algorithm>
#include <cstdlib>
#include <vector>

#include "Environment_manager.hpp"

// TODO Temp.
#include "../Gameplay/Physics_player.hpp"

namespace {

std::vector<cpVect> GetShapeVertices(Terrain::CellShape shape, cpFloat cellResolution) {
    // IMPORTANT: The vertices must be in counter clockwise order and must form a convex shape
    std::vector<cpVect> rv;

    switch (shape) {
    case Terrain::CellShape::Empty:
        // Do nothing
        break;

    case Terrain::CellShape::FullSquare:
        rv.push_back(cpv(0., 0.));
        rv.push_back(cpv(0., 1.));
        rv.push_back(cpv(1., 1.));
        rv.push_back(cpv(1., 0.));
        rv.push_back(cpv(0., 0.));
        break;

    default:
        assert(0 && "Unreachable");
    }

    for (auto& vect : rv) {
        vect = cpvmult(vect, cellResolution);
    }

    return rv;
}

using spempe::GameContext;
using spempe::NetworkingManager;

} // namespace

RN_DEFINE_RPC(ResizeTerrain, RN_ARGS(std::int32_t, width, std::int32_t, height)) {
    RN_NODE_IN_HANDLER().callIfClient(
        [&](NetworkingManager::ClientType& client) {
            auto& ctx = *client.getUserData<GameContext>();
            auto& envMgr = GetEnvironmentManager(ctx);

            envMgr._resizeAllGrids(width, height);          
        });

    RN_NODE_IN_HANDLER().callIfServer(
        [](NetworkingManager::ServerType& server) {
            // ERROR
        });
}

RN_DEFINE_RPC(SetTerrainRow, RN_ARGS(std::int32_t, rowIndex, hg::util::Packet&, packet)) {
    RN_NODE_IN_HANDLER().callIfClient(
        [&](NetworkingManager::ClientType& client) {
            auto& ctx = *client.getUserData<GameContext>();
            auto& envMgr = GetEnvironmentManager(ctx);

            for (hg::PZInteger x = 0; x < envMgr.getTerrainColumnCount(); x += 1) {
                std::int16_t terrainTypeId;
                packet >> terrainTypeId;
                envMgr.setCellType(x, rowIndex, static_cast<Terrain::TypeId>(terrainTypeId));
            }          
        });

    RN_NODE_IN_HANDLER().callIfServer(
        [](NetworkingManager::ServerType& server) {
            // ERROR
        });
}

EnvironmentManager::EnvironmentManager(QAO_RuntimeRef rtRef)
    : StateObject{rtRef, TYPEID_SELF, *PEXEPR_ENVIRON_MGR, "TerrainManager"}
    , _lightingCtrl{0, 0, 32.f, hg::gr::Color{5, 5, 10}}
{
    ccomp<spempe::NetworkingManagerInterface>().addEventListener(*this);
}

EnvironmentManager::~EnvironmentManager() {
    ccomp<spempe::NetworkingManagerInterface>().removeEventListener(*this);
}

void EnvironmentManager::generate(hg::PZInteger width, hg::PZInteger height, float cellResolution) {
    auto* space = ctx(DPhysicsSpace);

    _resizeAllGrids(width, height);

    for (hg::PZInteger y = 0; y < getTerrainRowCount(); y += 1) {
        for (hg::PZInteger x = 0; x < getTerrainColumnCount(); x += 1) {
            const bool isBorder = (x <= 1 || y <= 1 ||
                                   x >= getTerrainColumnCount() - 2 ||
                                   y >= getTerrainRowCount() - 2);

            const int padding = 12;

            const int isInner = (x > padding && y > padding &&
                                 x < getTerrainColumnCount() - padding &&
                                 y < getTerrainRowCount() - padding);

            if (isBorder || (isInner && (std::rand() % 1000) < 20)) {
                setCellType(x, y, Terrain::TypeId::EnergyShield);
            }
            else {
                setCellType(x, y, Terrain::TypeId::Space);
            }
        }
    }

    // _cellResolution = cellResolution; TODO

    _setEnvironmentPhysicsParameters();
}

void EnvironmentManager::destroy() {
    _resizeAllGrids(0, 0);
    _cellResolution = 0.f;
}

void EnvironmentManager::setCellType(hg::PZInteger x, hg::PZInteger y, Terrain::TypeId typeId) {
    auto& tprop = Terrain::getTypeProperties(typeId);

    // Type ID:
    _typeIdGrid[y][x] = typeId;

    // Physics shape:
    auto vertices = GetShapeVertices(tprop.shape, _cellResolution);
    if (!vertices.empty()) {
        auto* space = ctx(DPhysicsSpace);
        auto* body = cpSpaceGetStaticBody(space);

        for (auto& vertex : vertices) {
            vertex.x += cpFloat{_cellResolution} * x;
            vertex.y += cpFloat{_cellResolution} * y;
        }

        auto* shape = cpPolyShapeNew(body,
                                     static_cast<int>(vertices.size()),
                                     vertices.data(),
                                     cpTransformIdentity,
                                     0.0);

        auto& physicsData = _physicsGrid[y][x];

        physicsData.shape.reset(cpSpaceAddShape(space, shape));
        cpShapeSetElasticity(shape, 0.5);
        Collideables::initTerrain(shape, physicsData);
        cpSpaceReindexShape(space, shape);
    }
    else {
        _physicsGrid[y][x].shape.reset();
    }
    
    // Lighting:
    if (tprop.shape == Terrain::CellShape::FullSquare) {
        _lightingCtrl.setCellIsWall(x, y, true);
    }
    else {
        _lightingCtrl.setCellIsWall(x, y, false);
    }
}

hg::PZInteger EnvironmentManager::getTerrainRowCount() const {
    return _typeIdGrid.getHeight();
}

hg::PZInteger EnvironmentManager::getTerrainColumnCount() const {
    return _typeIdGrid.getWidth();
}

LightingController::LightHandle EnvironmentManager::addLight(float x, float y, 
                                                             LightingController::Color color, float radius) {
    return _lightingCtrl.addLight(x, y, color, radius);
}

void EnvironmentManager::onNetworkingEvent(const RN_Event& ev) {
    if (!ctx().isPrivileged()) {
        return;
    }
    
    ev.visit(
        [this](const RN_Event::Connected& ev) {
            auto& node = ccomp<spempe::NetworkingManagerInterface>().getNode();
            auto receiverIndex = *ev.clientIndex;

            if (ccomp<spempe::NetworkingManagerInterface>().getServer().getClientConnector(receiverIndex).getStatus() !=
                RN_ConnectorStatus::Connected) {
                // If we have both Connected and Disconnected queued up, it means this
                // client is actually no longer connected..
                return;
            }

            Compose_ResizeTerrain(node, receiverIndex, getTerrainColumnCount(), getTerrainRowCount());

            for (hg::PZInteger y = 0; y < getTerrainRowCount(); y += 1) {
                hg::util::Packet packet;
                for (hg::PZInteger x = 0; x < getTerrainColumnCount(); x += 1) {
                    packet << static_cast<std::int16_t>(_typeIdGrid[y][x]);
                }
                Compose_SetTerrainRow(node, receiverIndex, y, packet);
            }
        },
        [](const auto&) {}
    );
}

void EnvironmentManager::_eventPostUpdate() {
    //_lightingCtrl.render();
    //_lightingCtrl.smooth();
}

void EnvironmentManager::_eventDraw1() {
    auto& view = ccomp<spempe::WindowManagerInterface>().getView();
    hg::PZInteger startX = std::max(0, (int)std::floor((view.getCenter().x - view.getSize().x / 2.f) / _cellResolution));
    hg::PZInteger startY = std::max(0, (int)std::floor((view.getCenter().y - view.getSize().y / 2.f) / _cellResolution));

    hg::PZInteger endX = std::min(getTerrainColumnCount(),
                                  (int)std::ceil((view.getCenter().x + view.getSize().x / 2.f) / _cellResolution));
    hg::PZInteger endY = std::min(getTerrainRowCount(),
                                  (int)std::ceil((view.getCenter().y + view.getSize().y / 2.f) / _cellResolution));

    auto vertexCount = (endY - startY) * (endX - startX) * 4;
    sf::VertexArray vertices{sf::PrimitiveType::Quads, hg::ToSz(vertexCount)};

    for (hg::PZInteger y = startY; y < endY; y += 1) {
        for (hg::PZInteger x = startX; x < endX; x += 1) {
            _drawCell(x, y, ((y - startY) * (endX - startX) + (x - startX)) * 4, vertices);
        }
    }

    if (vertexCount > 0) {
        sf::RenderStates states;
        states.texture = (*_spriteCache.begin()).second.getSubsprite(0).getTexture(); // TODO Fragile!

        ccomp<spempe::WindowManagerInterface>().getCanvas().draw(vertices, states);
    }
}

void EnvironmentManager::_setEnvironmentPhysicsParameters() const {
    //cpSpaceSetDamping(ctx(DPhysicsSpace), 0.1);
}

void EnvironmentManager::_resizeAllGrids(hg::PZInteger width, hg::PZInteger height) {
    _typeIdGrid.resize(width, height);
    _physicsGrid.resize(width, height);
    _lightingCtrl.resize(width, height, _cellResolution);

    for (hg::PZInteger y = 0; y < getTerrainRowCount(); y += 1) {
        for (hg::PZInteger x = 0; x < getTerrainColumnCount(); x += 1) {
            _physicsGrid[y][x].x = x;
            _physicsGrid[y][x].y = y;
        }
    }
}

void EnvironmentManager::_drawCell(hg::PZInteger x, hg::PZInteger y, hg::PZInteger quadIndex, 
                                   sf::VertexArray& vertices) {
    auto typeId = _typeIdGrid[y][x];

    auto& tprop = Terrain::getTypeProperties(typeId);
    
    auto iter = _spriteCache.find(tprop.spriteId);
    if (iter == _spriteCache.end()) {
        auto pair = _spriteCache.insert(std::make_pair(tprop.spriteId, ctx(DSprite, tprop.spriteId)));
        assert(pair.second);
        iter = pair.first;
    }

    const sf::Sprite& sprite = (*iter).second.getSubsprite(0);
    sf::Vertex* quad = &(vertices[quadIndex]);

    {
        const auto xx = static_cast<float>(x * _cellResolution);
        const auto yy = static_cast<float>(y * _cellResolution);
        const auto ww = static_cast<float>(sprite.getLocalBounds().width);
        const auto hh = static_cast<float>(sprite.getLocalBounds().height);

        quad[0].position = {xx, yy};
        quad[1].position = {xx, yy + hh};
        quad[2].position = {xx + ww, yy + hh};
        quad[3].position = {xx + ww, yy};
    }

    {
        const auto xx = static_cast<float>(sprite.getTextureRect().left);
        const auto yy = static_cast<float>(sprite.getTextureRect().top);
        const auto ww = static_cast<float>(sprite.getTextureRect().width);
        const auto hh = static_cast<float>(sprite.getTextureRect().height);

        quad[0].texCoords = {xx, yy};
        quad[1].texCoords = {xx, yy + hh};
        quad[2].texCoords = {xx + ww, yy + hh};
        quad[3].texCoords = {xx + ww, yy};
    }

    quad[0].color = quad[1].color = quad[1].color = quad[1].color = hg::gr::Color::White;
}