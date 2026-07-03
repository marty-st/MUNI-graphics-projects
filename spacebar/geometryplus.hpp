#pragma once

#include "frame.hpp"
#include "geometry.hpp"
#include "material.hpp"

using geometry_ptr = std::shared_ptr<Geometry>;

class GeometryPlus
{
public:
    geometry_ptr geometry;
    frame_ptr frame;
    Material material;
    
    bool has_texture = false;
    /* These bools added later - not included in constructors */
    bool animated = false;
    bool lightsource = false;

    GLuint bound_texture = 0;
    /* Position in Geometries vector */
    std::size_t index = 0;

    GeometryPlus() : frame{std::make_shared<Frame>()}, geometry{std::make_shared<Geometry>()} {}

    GeometryPlus(Geometry &&_geometry) : geometry{std::make_shared<Geometry>(std::move(_geometry))}, frame{std::make_shared<Frame>()} {}
    GeometryPlus(geometry_ptr _geometry) : geometry{_geometry}, frame{std::make_shared<Frame>()} {}

    GeometryPlus(Frame _frame) : geometry{std::make_shared<Geometry>()}, frame{std::make_shared<Frame>(_frame)} {}
    GeometryPlus(frame_ptr _frame) : geometry{std::make_shared<Geometry>()}, frame{_frame} {}

    /* Geometry Variants */
    GeometryPlus(Geometry &&_geometry, Frame _frame, Material _material) 
        : geometry{std::make_shared<Geometry>(std::move(_geometry))}, frame{std::make_shared<Frame>(_frame)}, material{_material} {}

    GeometryPlus(geometry_ptr _geometry, Frame _frame, Material _material) 
        : geometry{_geometry}, frame{std::make_shared<Frame>(_frame)}, material{_material} {}

    /* Frame Variants */
    GeometryPlus(Geometry &&_geometry, frame_ptr _frame, Material _material) 
        : geometry{std::make_shared<Geometry>(std::move(_geometry))}, frame{_frame}, material{_material} {}

    GeometryPlus(geometry_ptr _geometry, frame_ptr _frame, Material _material) 
        : geometry{_geometry}, frame{_frame}, material{_material} {}

    // TO DO - not fully
    /*Deep Copy */
    GeometryPlus(const GeometryPlus& other) : frame{std::make_shared<Frame>(other.getFrameCopy())}, geometry{std::make_shared<Geometry>(other.getGeometryRef())} {}
    /* Swap Geometries */
    GeometryPlus(GeometryPlus&& other)
        : GeometryPlus() { frame = other.frame; geometry->swap_fields(*this->geometry.get(), *other.geometry.get()); }

    const Frame getFrameCopy() const { return *frame.get(); }
    const Geometry &getGeometryRef() const { return *geometry.get(); }

    const glm::mat4 getModelMatrix() const { return frame->getModelMat(); }
    const glm::mat3 getNormalMatrix() const { return glm::mat3(glm::transpose(glm::inverse(frame->getModelMat()))); }

    static Geometry from_file(std::filesystem::path path, bool normalize = true, bool centralize = true);

    void draw() { geometry->draw(); }
};