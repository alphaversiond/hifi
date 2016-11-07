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

#include <Plugin.h>

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

void RenderableLeoPolyEntityItem::update(const quint64& now) {
    LeoPolyEntityItem::update(now);

    // TODO - place any "simulation" logic here
    // for example if SculptApp_Frame() needs to be called every frame, this is how it should be done

    //LeoPolyPlugin::Instance().SculptApp_Frame();  // maybe?
}


void RenderableLeoPolyEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLeoPolyEntityItem::render");
    assert(getType() == EntityTypes::LeoPoly);
    Q_ASSERT(args->_batch);

    if (_leoPolyDataDirty) {
        getMesh();
    }

    model::MeshPointer mesh;
    withReadLock([&] {
        mesh = _mesh;
    });

    if (!_pipeline) {
        gpu::ShaderPointer vertexShader = gpu::Shader::createVertex(std::string(polyvox_vert));
        gpu::ShaderPointer pixelShader = gpu::Shader::createPixel(std::string(polyvox_frag));

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), MATERIAL_GPU_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("xMap"), 0));
        slotBindings.insert(gpu::Shader::Binding(std::string("yMap"), 1));
        slotBindings.insert(gpu::Shader::Binding(std::string("zMap"), 2));

        gpu::ShaderPointer program = gpu::Shader::createProgram(vertexShader, pixelShader);
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        _pipeline = gpu::Pipeline::create(program, state);
    }

    gpu::Batch& batch = *args->_batch;
    batch.setPipeline(_pipeline);

    bool success;
    Transform transform = getTransformToCenter(success);
    if (!success) {
        return;
    }

    batch.setModelTransform(transform);
    batch.setInputFormat(mesh->getVertexFormat());
    batch.setInputBuffer(gpu::Stream::POSITION, mesh->getVertexBuffer());
    batch.setInputBuffer(gpu::Stream::NORMAL,
                         mesh->getVertexBuffer()._buffer,
                         sizeof(float) * 3,
                         mesh->getVertexBuffer()._stride);
    batch.setIndexBuffer(gpu::UINT32, mesh->getIndexBuffer()._buffer, 0);

    batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    batch.setResourceTexture(1, DependencyManager::get<TextureCache>()->getWhiteTexture());
    batch.setResourceTexture(2, DependencyManager::get<TextureCache>()->getWhiteTexture());

    int voxelVolumeSizeLocation = _pipeline->getProgram()->getUniforms().findLocation("voxelVolumeSize");
    // batch._glUniform3f(voxelVolumeSizeLocation, voxelVolumeSize.x, voxelVolumeSize.y, voxelVolumeSize.z);
    batch._glUniform3f(voxelVolumeSizeLocation, 16.0, 16.0, 16.0);

    batch.drawIndexed(gpu::TRIANGLES, (gpu::uint32)mesh->getNumIndices(), 0);
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

void RenderableLeoPolyEntityItem::getMesh() {
    EntityItemID entityUnderSculptID;

    // this appears to be storing the entity item ID in the leopoly plugin, is that true?
    if (LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1 != 0)
    {
        entityUnderSculptID.data1 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1;
        entityUnderSculptID.data2 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data2;
        entityUnderSculptID.data3 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data3;
        for (int i = 0; i < 8; i++)
            entityUnderSculptID.data4[i] = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data4[i];
    }

    if (getEntityItemID() == entityUnderSculptID) {
        _leoPolyDataDirty = false;
        return;
    }

    auto entity = std::static_pointer_cast<RenderableLeoPolyEntityItem>(getThisPointer());
    entity->withReadLock([&] {

        QtConcurrent::run([entity] {
            model::MeshPointer mesh(new model::Mesh());

            // ...

            // auto indexBuffer = std::make_shared<gpu::Buffer>(vecIndices.size() * sizeof(uint32_t),
            //                                                  (gpu::Byte*)vecIndices.data());
            // auto indexBufferPtr = gpu::BufferPointer(indexBuffer);
            // auto indexBufferView = new gpu::BufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW));
            // mesh->setIndexBuffer(*indexBufferView);

            // auto vertexBuffer = std::make_shared<gpu::Buffer>(verticesNormalsMaterials.size() *
            //                                                   sizeof(LeoPoly::PositionMaterialNormal),
            //                                                   (gpu::Byte*)verticesNormalsMaterials.data());
            // auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
            // gpu::Resource::Size vertexBufferSize = 0;
            // if (vertexBufferPtr->getSize() > sizeof(float) * 3) {
            //     vertexBufferSize = vertexBufferPtr->getSize() - sizeof(float) * 3;
            // }
            // auto vertexBufferView = new gpu::BufferView(vertexBufferPtr, 0, vertexBufferSize,
            //                                             sizeof(LeoPoly::PositionMaterialNormal),
            //                                             gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW));
            // mesh->setVertexBuffer(*vertexBufferView);
            // mesh->addAttribute(gpu::Stream::NORMAL,
            //                    gpu::BufferView(vertexBufferPtr,
            //                                    sizeof(float) * 3,
            //                                    vertexBufferPtr->getSize() - sizeof(float) * 3,
            //                                    sizeof(LeoPoly::PositionMaterialNormal),
            //                                    gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW)));

            entity->setMesh(mesh);
        });
    });
}


void RenderableLeoPolyEntityItem::setMesh(model::MeshPointer mesh) {
    // this catches the payload from getMesh
    withWriteLock([&] {
        _mesh = mesh;
    });
}
