//
//  RenderableLeoPolyEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 2016-11-5.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>
#include <QObject>
#include <QByteArray>
#include <QtConcurrent/QtConcurrentRun>
#include <glm/gtx/transform.hpp>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <Model.h>
#include <PerfStat.h>
#include <render/Scene.h>

#include "model/Geometry.h"
#include "EntityTreeRenderer.h"
#include "polyvox_vert.h"
#include "polyvox_frag.h"
#include "RenderableLeoPolyEntityItem.h"
#include "EntityEditPacketSender.h"
#include "PhysicalEntitySimulation.h"

gpu::PipelinePointer RenderableLeoPolyEntityItem::_pipeline = nullptr;


EntityItemPointer RenderableLeoPolyEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity{ new RenderableLeoPolyEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

RenderableLeoPolyEntityItem::RenderableLeoPolyEntityItem(const EntityItemID& entityItemID) :
    LeoPolyEntityItem(entityItemID)
{
}

RenderableLeoPolyEntityItem::~RenderableLeoPolyEntityItem() {
}

bool RenderableLeoPolyEntityItem::findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                                              bool& keepSearching, OctreeElementPointer& element,
                                                              float& distance, BoxFace& face, glm::vec3& surfaceNormal,
                                                              void** intersectedObject, bool precisionPicking) const
{
    if (!precisionPicking) {
        // just intersect with bounding box
        return true;
    }

    // TODO
    return true;
}

ShapeType RenderableLeoPolyEntityItem::getShapeType() const {
    return SHAPE_TYPE_BOX;
}

void RenderableLeoPolyEntityItem::updateRegistrationPoint(const glm::vec3& value) {
    if (value != _registrationPoint) {
        EntityItem::updateRegistrationPoint(value);
    }
}

bool RenderableLeoPolyEntityItem::isReadyToComputeShape() {
    if (!EntityItem::isReadyToComputeShape()) {
        return false;
    }
    return true;
}

void RenderableLeoPolyEntityItem::computeShapeInfo(ShapeInfo& info) {
    info.setParams(getShapeType(), 0.5f * getDimensions());
    adjustShapeInfoByRegistration(info);
}

void RenderableLeoPolyEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLeoPolyEntityItem::render");
    assert(getType() == EntityTypes::LeoPoly);
    Q_ASSERT(args->_batch);

    static glm::vec4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
    gpu::Batch& batch = *args->_batch;
    bool success;
    auto shapeTransform = getTransformToCenter(success);
    if (success) {
        batch.setModelTransform(shapeTransform); // we want to include the scale as well
        DependencyManager::get<GeometryCache>()->renderWireCubeInstance(batch, greenColor);
    }
}

bool RenderableLeoPolyEntityItem::addToScene(EntityItemPointer self,
                                             std::shared_ptr<render::Scene> scene,
                                             render::PendingChanges& pendingChanges) {
    _myItem = scene->allocateID();

    auto renderItem = std::make_shared<LeoPolyPayload>(getThisPointer());
    auto renderData = LeoPolyPayload::Pointer(renderItem);
    auto renderPayload = std::make_shared<LeoPolyPayload::Payload>(renderData);

    render::Item::Status::Getters statusGetters;
    makeEntityItemStatusGetters(getThisPointer(), statusGetters);
    renderPayload->addStatusGetters(statusGetters);

    pendingChanges.resetItem(_myItem, renderPayload);

    return true;
}

void RenderableLeoPolyEntityItem::removeFromScene(EntityItemPointer self,
                                                  std::shared_ptr<render::Scene> scene,
                                                  render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_myItem);
    render::Item::clearID(_myItem);
}

namespace render {
    template <> const ItemKey payloadGetKey(const LeoPolyPayload::Pointer& payload) {
        return ItemKey::Builder::opaqueShape();
    }

    template <> const Item::Bound payloadGetBound(const LeoPolyPayload::Pointer& payload) {
        if (payload && payload->_owner) {
            auto leoPolyEntity = std::dynamic_pointer_cast<RenderableLeoPolyEntityItem>(payload->_owner);
            bool success;
            auto result = leoPolyEntity->getAABox(success);
            if (!success) {
                return render::Item::Bound();
            }
            return result;
        }
        return render::Item::Bound();
    }

    template <> void payloadRender(const LeoPolyPayload::Pointer& payload, RenderArgs* args) {
        if (args && payload && payload->_owner) {
            payload->_owner->render(args);
        }
    }
}
