#ifndef TYPHON_FRAMEWORK_ENVIRONMENT_MANAGER_HPP
#define TYPHON_FRAMEWORK_ENVIRONMENT_MANAGER_HPP

#include <Hobgoblin/ChipmunkPhysics.hpp>
#include <Hobgoblin/Graphics.hpp>
#include <Hobgoblin/Utility/Grids.hpp>
#include <Hobgoblin/Utility/Packet.hpp>
#include <Hobgoblin/Utility/Rectangle.hpp>

#include <Typhon/Framework.hpp>

#include <unordered_map>

#include "__Experimental/Lighting.hpp"
#include "Typhon/Terrain/Terrain.hpp"

class EnvironmentManager : public StateObject, private spempe::NetworkingEventListener {
public:
    EnvironmentManager(hg::QAO_RuntimeRef rtRef);
    ~EnvironmentManager();

    void generate(hg::PZInteger width, hg::PZInteger height, float cellResolution);
    void destroy();
    void setCellType(hg::PZInteger x, hg::PZInteger y, Terrain::TypeId typeId);

    hg::PZInteger getTerrainRowCount() const;
    hg::PZInteger getTerrainColumnCount() const;

    // Light:
    LightingController::LightHandle addLight(float x, float y, LightingController::Color color, float radius);

protected:
    void onNetworkingEvent(const RN_Event& ev);

    void _eventPostUpdate() override;
    void _eventDraw1() override;

private:
    struct CellPhycsisData : Collideables::ITerrain {
        hg::cpShapeUPtr shape;
        hg::PZInteger x;
        hg::PZInteger y;
    };

    hg::util::RowMajorGrid<Terrain::TypeId> _typeIdGrid;
    hg::util::RowMajorGrid<CellPhycsisData> _physicsGrid;
    LightingController _lightingCtrl;
    float _cellResolution = 32.f; // TODO

    std::unordered_map<SpriteId, hg::gr::Multisprite> _spriteCache;

    void _setEnvironmentPhysicsParameters() const;
    void _resizeAllGrids(hg::PZInteger width, hg::PZInteger height);
    void _drawCell(hg::PZInteger x, hg::PZInteger y, hg::PZInteger quadIndex, sf::VertexArray& vertices);

    friend RN_HANDLER_SIGNATURE(ResizeTerrain, RN_ARGS(std::int32_t, width, std::int32_t, height));
    friend RN_HANDLER_SIGNATURE(SetTerrainRow, RN_ARGS(std::int32_t, rowIndex, hg::util::Packet&, packet));
};

#endif // !TYPHON_FRAMEWORK_ENVIRONMENT_MANAGER_HPP