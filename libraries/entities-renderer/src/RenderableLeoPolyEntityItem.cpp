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

// Plugin.h(85) : warning C4091 : '__declspec(dllimport)' : ignored on left of 'LeoPlugin' when no variable is declared
#ifdef Q_OS_WIN
#pragma warning( push )
#pragma warning( disable : 4091 )
#endif

#include <Plugin.h>

#ifdef Q_OS_WIN
#pragma warning( pop )
#endif

gpu::PipelinePointer RenderableLeoPolyEntityItem::_pipeline = nullptr;


EntityItemPointer RenderableLeoPolyEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new RenderableLeoPolyEntityItem(entityID) };
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


// FIXME - this is probably cruft and can go...
void RenderableLeoPolyEntityItem::updateRegistrationPoint(const glm::vec3& value) {
    if (value != _registrationPoint) {
        EntityItem::updateRegistrationPoint(value);
    }
}

// FIXME - for now this is cruft and can probably go... but we will want to think about how physics works with these shapes
bool RenderableLeoPolyEntityItem::isReadyToComputeShape() {
    if (!EntityItem::isReadyToComputeShape()) {
        return false;
    }
    return true;
}

// FIXME - for now this is cruft and can probably go... but we will want to think about how physics works with these shapes
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

EntityItemID RenderableLeoPolyEntityItem::getCurrentlyEditingEntityID() {
    EntityItemID entityUnderSculptID;

    if (LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1 != 0) {
        entityUnderSculptID.data1 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data1;
        entityUnderSculptID.data2 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data2;
        entityUnderSculptID.data3 = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data3;
        for (int i = 0; i < 8; i++) {
            entityUnderSculptID.data4[i] = LeoPolyPlugin::Instance().CurrentlyUnderEdit.data4[i];
        }
    }
    return entityUnderSculptID;
}

void RenderableLeoPolyEntityItem::createShaderPipeline() {
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

void RenderableLeoPolyEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLeoPolyEntityItem::render");
    assert(getType() == EntityTypes::LeoPoly);
    Q_ASSERT(args->_batch);

    // if we don't have a _modelResource yet, then we can't render...
    if (!_modelResource) {
        initializeModelResource();
        return;
    }

    // FIXME - this is janky... 
    if (!_mesh) {
        getMesh();
    }

    model::MeshPointer mesh;
    withReadLock([&] {
        mesh = _mesh;
    });

    if (!_pipeline) {
        createShaderPipeline();
    }

    if (!mesh) {
        return;
    }

    gpu::Batch& batch = *args->_batch;
    batch.setPipeline(_pipeline);

    bool success;
    Transform transform = getTransformToCenter(success);
    if (!success) {
        return;
    }

    // get the bounds of the mesh, so we can scale it into the bounds of the entity
    auto numMeshParts = mesh->getNumParts();
    auto bounds = mesh->evalPartsBound(0, (numMeshParts-1));

    // determin the correct scale to fit mesh into entity bounds, set transform accordingly
    auto entityScale = getScale();
    auto meshBoundsScale = bounds.getScale();
    auto fitInBounds = entityScale / meshBoundsScale;
    transform.setScale(fitInBounds);

    // TODO - need to set registration point as well....

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

    // FIXME - this voxelVolumeSizeLocation is cruft.
    int voxelVolumeSizeLocation = _pipeline->getProgram()->getUniforms().findLocation("voxelVolumeSize");
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


void RenderableLeoPolyEntityItem::initializeModelResource() {
    // FIXME -- some open questions....
    //   1) what do we do if someone changes the URL while under edit?
    //   2) how do we managed the previous mesh resources associated with the old GeometryResource
    //
    //  _modelResource->getURL() ... this might be useful
    // 
    if (!_leoPolyURL.isEmpty()) {
        _modelResource = DependencyManager::get<ModelCache>()->getGeometryResource(_leoPolyURL);
    }
}

void RenderableLeoPolyEntityItem::getMesh() {
    EntityItemID entityUnderSculptID = getCurrentlyEditingEntityID();

    // FIXME -- this seems wrong, but It think I'm begining to understand it.
    // getMesh() will only produce a mesh of the current entity is not currently under edit.
    // if the current entity is under edit, then it's assumed that the _mesh will be updated
    // but calling updateGeometryFromLeoPlugin()...
    if (getEntityItemID() == entityUnderSculptID) {
        // FIXME-- maybe we should also call updateGeometryFromLeoPlugin() here...
        return;
    }

    if (!_modelResource || !_modelResource->isLoaded()) {
        // model not yet loaded... can't make a mesh from it yet...
        return;
    }

    //  const GeometryMeshes& getMeshes() const { return *_meshes; }
    auto meshes = _modelResource->getMeshes();

    // FIXME -- in the event that a model has multiple meshes, we should flatten them into a single mesh...
    if (meshes.size() > 1) {
        qDebug() << __FUNCTION__ << "WARNING- model resources with multiple meshes not yet supported...";
    }

    if (meshes.size() < 1) {
        model::MeshPointer emptyMesh(new model::Mesh()); // an empty mesh....
    } else {
        // FIXME- this is a bit of a hack to work around const-ness
        model::MeshPointer copyOfMesh(new model::Mesh(*meshes[0]));
        setMesh(copyOfMesh);
    }
}


void RenderableLeoPolyEntityItem::setUnderSculpting(bool value) {
    // TODO -- how do we want to enable/disable sculpting...
}


// This will take the _modelResource and convert it into a "flattened form" that can be used by the LeoPoly DLL
void RenderableLeoPolyEntityItem::importToLeoPoly() {

    // we need to have a _modelResource and make sure it's fully loaded before we can parse out it's contents
    // and flatten it into a LeoPoly friendly format.
    if (_modelResource && _modelResource->isLoaded()) {

        QVector<glm::vec3> vertices;
        QVector<glm::vec3> normals;
        QVector<glm::vec2> texCoords;
        QVector<int> indices;
        QVector<std::string> matStringsPerTriangles;
        QVector<unsigned short> matIndexesPerTriangles;

        auto geometry = _modelResource->getFBXGeometry();


        // FIXME -- use the correct URL from the property
        std::string baseUrl; //  = act->getURL().toString().toStdString().substr(0, act->getURL().toString().toStdString().find_last_of("\\/"));

        std::vector<LeoPlugin::IncomingMaterial> materialsToSend;
        foreach(const FBXMaterial mat, geometry.materials)
        {
            LeoPlugin::IncomingMaterial actMat;

            // FIXME - texture support?
            /*
            if (!mat.albedoTexture.filename.isEmpty() && mat.albedoTexture.content.isEmpty() &&
                !_textures.contains(mat.albedoTexture.filename))
            {
                actMat.diffuseTextureUrl = baseUrl + "//" + mat.albedoTexture.filename.toStdString();
            }
            */
            /*if (!mat.normalTexture.filename.isEmpty() && mat.normalTexture.content.isEmpty() &&
            !_textures.contains(mat.normalTexture.filename))
            {

            _texturesURLs.push_back(baseUrl + "\\" + mat.normalTexture.filename.toStdString());
            }
            if (!mat.specularTexture.filename.isEmpty() && mat.specularTexture.content.isEmpty() &&
            !_textures.contains(mat.specularTexture.filename))
            {
            _texturesURLs.push_back(baseUrl + "\\" + mat.specularTexture.filename.toStdString());
            }
            if (!mat.emissiveTexture.filename.isEmpty() && mat.emissiveTexture.content.isEmpty() &&
            !_textures.contains(mat.emissiveTexture.filename))
            {
            _texturesURLs.push_back(baseUrl + "\\" + mat.emissiveTexture.filename.toStdString());
            }*/
            for (int i = 0; i < 3; i++)
            {
                actMat.diffuseColor[i] = mat.diffuseColor[i];
                actMat.emissiveColor[i] = mat.emissiveColor[i];
                actMat.specularColor[i] = mat.specularColor[i];
            }
            actMat.diffuseColor[3] = 1;
            actMat.emissiveColor[3] = 0;
            actMat.specularColor[3] = 0;
            actMat.materialID = mat.materialID.toStdString();
            actMat.name = mat.name.toStdString();
            materialsToSend.push_back(actMat);
        }
        for (auto actmesh : geometry.meshes)
        {
            vertices.append(actmesh.vertices);
            normals.append(actmesh.normals);
            texCoords.append(actmesh.texCoords);
            for (auto subMesh : actmesh.parts)
            {
                int startIndex = indices.size();
                if (subMesh.triangleIndices.size() > 0)
                {
                    indices.append(subMesh.triangleIndices);
                }
                if (subMesh.quadTrianglesIndices.size() > 0)
                {
                    indices.append(subMesh.quadTrianglesIndices);
                }
                else
                    if (subMesh.quadIndices.size() > 0)
                    {
                        assert(subMesh.quadIndices.size() % 4 == 0);
                        for (int i = 0; i < subMesh.quadIndices.size() / 4; i++)
                        {
                            indices.push_back(subMesh.quadIndices[i * 4]);
                            indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                            indices.push_back(subMesh.quadIndices[i * 4 + 2]);

                            indices.push_back(subMesh.quadIndices[i * 4 + 3]);
                            indices.push_back(subMesh.quadIndices[i * 4]);
                            indices.push_back(subMesh.quadIndices[i * 4 + 1]);
                        }
                    }
                for (int i = 0; i < (indices.size() - startIndex) / 3; i++)
                {
                    matStringsPerTriangles.push_back(subMesh.materialID.toStdString());
                }

            }
        }
        float* verticesFlattened = new float[vertices.size() * 3];
        float *normalsFlattened = new float[normals.size() * 3];
        float *texCoordsFlattened = new float[texCoords.size() * 2];
        int *indicesFlattened = new int[indices.size()];
        for (int i = 0; i < vertices.size(); i++)
        {

            verticesFlattened[i * 3] = vertices[i].x;
            verticesFlattened[i * 3 + 1] = vertices[i].y;
            verticesFlattened[i * 3 + 2] = vertices[i].z;

            normalsFlattened[i * 3] = normals[i].x;
            normalsFlattened[i * 3 + 1] = normals[i].y;
            normalsFlattened[i * 3 + 2] = normals[i].z;

            texCoordsFlattened[i * 2] = texCoords[i].x;
            texCoordsFlattened[i * 2 + 1] = texCoords[i].y;
        }
        for (int i = 0; i < indices.size(); i++)
        {
            indicesFlattened[i] = indices[i];
        }
        matIndexesPerTriangles.resize(matStringsPerTriangles.size());
        for (unsigned int matInd = 0; matInd < materialsToSend.size(); matInd++)
        {
            for (int i = 0; i < matStringsPerTriangles.size(); i++)
            {
                if (matStringsPerTriangles[i] == materialsToSend[matInd].materialID)
                {
                    matIndexesPerTriangles[i] = matInd;
                }
            }
        }

         
        // Creates a sculptable mesh for the Leoengine from the given
        //   vertices array
        //   number of vertices passed
        //   indices array
        //   number of indices passed
        //   normals array
        //   number of normals passed
        //   Texture coordinates array
        //   number of texcoords passed
        //        world matrix??
        //   Materials
        //   Indices connecting the triangles
        // LEOPLUGIN_DLL_API void importFromRawData(
        //         float* vertices, unsigned int numVertices, int* indices, unsigned int numIndices, 
        //         float* normals, unsigned int numNormals, float* texCoords, unsigned int numTexCoords, 
        //         float worldMat[16], std::vector<IncomingMaterial> metrials, 
        //         std::vector<unsigned short> triangleMatInds);


        LeoPolyPlugin::Instance().importFromRawData(
                                        verticesFlattened, vertices.size(), 
                                        indicesFlattened, indices.size(), 
                                        normalsFlattened, normals.size(), 
                                        texCoordsFlattened, texCoords.size(), 
                                        const_cast<float*>(glm::value_ptr(glm::transpose(getTransform().getMatrix()))), 
                                        materialsToSend, matIndexesPerTriangles.toStdVector());
        delete[] verticesFlattened;
        delete[] indicesFlattened;
        delete[] normalsFlattened;
        delete[] texCoordsFlattened;
    }

}

// If the entity is currently under edit with LeoPoly, this method will ask the DLL for it's current mesh state
// and re-create our mesh for rendering...
void RenderableLeoPolyEntityItem::updateGeometryFromLeoPlugin() {

    // get the vertices, normals, and indices from LeoPoly
    float* vertices, *normals;
    int *indices;
    unsigned int numVertices, numNormals, numIndices;
    LeoPolyPlugin::Instance().getSculptMeshNUmberDatas(numVertices, numIndices, numNormals);
    vertices = new float[numVertices * 3];
    normals = new float[numNormals * 3];
    indices = new int[numIndices];
    LeoPolyPlugin::Instance().getRawSculptMeshData(vertices, indices, normals);

    // Create a model::Mesh from the flattened mesh from LeoPoly
    model::MeshPointer mesh(new model::Mesh());
    // bool needsMaterialLibrary = false;

    std::vector<VertexNormalMaterial> verticesNormalsMaterials;

    for (unsigned int i = 0; i < numVertices; i++) {
        glm::vec3 actVert = glm::vec3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
        glm::vec3 actNorm = glm::vec3(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);
        verticesNormalsMaterials.push_back(VertexNormalMaterial{actVert, actNorm, 0});
    }

    auto vertexBuffer = std::make_shared<gpu::Buffer>(verticesNormalsMaterials.size() * sizeof(VertexNormalMaterial),
                                                        (gpu::Byte*)verticesNormalsMaterials.data());
    auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
    gpu::Resource::Size vertexBufferSize = 0;

    // FIXME - Huh??? - seems to be setting the size for the vertext buffer, but this math is weird
    if (vertexBufferPtr->getSize() > sizeof(float) * 3) {
        vertexBufferSize = vertexBufferPtr->getSize() - sizeof(float) * 3;
    }

    auto vertexBufferView = new gpu::BufferView(vertexBufferPtr, 0, vertexBufferSize,
                                                sizeof(VertexNormalMaterial),
                                                gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW));
    mesh->setVertexBuffer(*vertexBufferView);
    mesh->addAttribute(gpu::Stream::NORMAL,
                            gpu::BufferView(vertexBufferPtr,
                                            sizeof(glm::vec3),
                                            vertexBufferPtr->getSize() - sizeof(glm::vec3), // FIXME - huh?
                                            sizeof(VertexNormalMaterial),
                                            gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW)));

    verticesNormalsMaterials.clear();


    std::vector<uint32_t> vecIndices;

    for (unsigned int i = 0; i < numIndices; i++) {
        vecIndices.push_back(indices[i]);
    }


    auto indexBuffer = std::make_shared<gpu::Buffer>(vecIndices.size() * sizeof(uint32_t), (gpu::Byte*)vecIndices.data());
    auto indexBufferPtr = gpu::BufferPointer(indexBuffer);

    auto indexBufferView = new gpu::BufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW));
    mesh->setIndexBuffer(*indexBufferView);

    vecIndices.clear();

    setMesh(mesh);

    delete[] vertices;
    delete[] normals;
    delete[] indices;
}


void RenderableLeoPolyEntityItem::setMesh(model::MeshPointer mesh) {
    // this catches the payload from getMesh
    withWriteLock([&] {
        _mesh = mesh;
    });
}
