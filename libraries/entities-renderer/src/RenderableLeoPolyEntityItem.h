//
//  RenderableLeoPolyEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 2016-11-5.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableLeoPolyEntityItem_h
#define hifi_RenderableLeoPolyEntityItem_h

#include <QSemaphore>
#include <atomic>

#include <TextureCache.h>

#include "LeoPolyEntityItem.h"
#include "RenderableEntityItem.h"
#include "gpu/Context.h"

class LeoPolyPayload {
public:
    LeoPolyPayload(EntityItemPointer owner) : _owner(owner), _bounds(AABox()) { }
    typedef render::Payload<LeoPolyPayload> Payload;
    typedef Payload::DataPointer Pointer;

    EntityItemPointer _owner;
    AABox _bounds;
};

namespace render {
   template <> const ItemKey payloadGetKey(const LeoPolyPayload::Pointer& payload);
   template <> const Item::Bound payloadGetBound(const LeoPolyPayload::Pointer& payload);
   template <> void payloadRender(const LeoPolyPayload::Pointer& payload, RenderArgs* args);
}


class RenderableLeoPolyEntityItem : public LeoPolyEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableLeoPolyEntityItem(const EntityItemID& entityItemID);

    virtual ~RenderableLeoPolyEntityItem();

    virtual void somethingChangedNotification() override {
        // This gets called from EnityItem::readEntityDataFromBuffer every time a packet describing
        // this entity comes from the entity-server.  It gets called even if nothing has actually changed
        // (see the comment in EntityItem.cpp).  If that gets fixed, this could be used to know if we
        // need to redo the voxel data.
        // _needsModelReload = true;
    }

    void render(RenderArgs* args) override;
    virtual bool supportsDetailedRayIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance,
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const override;

    virtual ShapeType getShapeType() const override;
    virtual bool shouldBePhysical() const override { return !isDead(); }
    virtual bool isReadyToComputeShape() override;
    virtual void computeShapeInfo(ShapeInfo& info) override;

    virtual bool addToScene(EntityItemPointer self,
                            std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges) override;
    virtual void removeFromScene(EntityItemPointer self,
                                 std::shared_ptr<render::Scene> scene,
                                 render::PendingChanges& pendingChanges) override;

    virtual void updateRegistrationPoint(const glm::vec3& value) override;

    bool isTransparent() override { return false; }

private:
    const int MATERIAL_GPU_SLOT = 3;
    render::ItemID _myItem{ render::Item::INVALID_ITEM_ID };
    static gpu::PipelinePointer _pipeline;

    ShapeInfo _shapeInfo;
};

#endif // hifi_RenderableLeoPolyEntityItem_h
