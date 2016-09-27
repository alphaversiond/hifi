//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Android_Backend_h
#define hifi_gpu_Android_Backend_h

#include <assert.h>
#include <functional>
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

#include <gl/Config.h>

#include <QtCore/QLoggingCategory>
#include <gpu/Forward.h>

#include <gpu/Context.h>
#include "GLShared.h"

namespace gpu { namespace gl {

//class GLBackend : public gpu::Backend {
class GLBackend : public Backend, public std::enable_shared_from_this<GLBackend> {

    using Parent = gpu::Backend;
    // Context Backend static interface required
    friend class gpu::Context;
    static void init() {}
    //static BackendPointer createBackend();
    //static gpu::Backend* createBackend() { return new GLBackend(); }
    static BackendPointer createBackend();
    // making it public static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) { return true; }

public:
    GLBackend() { } /*: Parent() { } was protected*/
    ~GLBackend() { }

    static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings = Shader::BindingSet());

    void setCameraCorrection(const Mat4& correction);
    void render(const Batch& batch) final override;

    // This call synchronize the Full Backend cache with the current GLState
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    void syncCache() final override;

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer,
                                     const Vec4i& region, QImage& destImage) final override;

    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    static const int MAX_NUM_INPUT_BUFFERS = 16;

    size_t getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 12;
    size_t getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    size_t getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

    // Draw Stage
    virtual void do_draw(const Batch& batch, size_t paramOffset);
    virtual void do_drawIndexed(const Batch& batch, size_t paramOffset);
    virtual void do_drawInstanced(const Batch& batch, size_t paramOffset);
    virtual void do_drawIndexedInstanced(const Batch& batch, size_t paramOffset);
    virtual void do_multiDrawIndirect(const Batch& batch, size_t paramOffset);
    virtual void do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset);

    // Input Stage
    virtual void do_setInputFormat(const Batch& batch, size_t paramOffset) final;
    virtual void do_setInputBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setIndexBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setIndirectBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_generateTextureMips(const Batch& batch, size_t paramOffset) final;

    // Transform Stage
    virtual void do_setModelTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setViewTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setProjectionTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setViewportTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setDepthRangeTransform(const Batch& batch, size_t paramOffset) final;

    // Uniform Stage
    virtual void do_setUniformBuffer(const Batch& batch, size_t paramOffset) final;

    // Resource Stage
    virtual void do_setResourceTexture(const Batch& batch, size_t paramOffset) final;

    // Pipeline Stage
    virtual void do_setPipeline(const Batch& batch, size_t paramOffset) final;

    // Output stage
    virtual void do_setFramebuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_clearFramebuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_blit(const Batch& batch, size_t paramOffset);

    // Query section
    virtual void do_beginQuery(const Batch& batch, size_t paramOffset) final;
    virtual void do_endQuery(const Batch& batch, size_t paramOffset) final;
    virtual void do_getQuery(const Batch& batch, size_t paramOffset) final;

    // Reset stages
    virtual void do_resetStages(const Batch& batch, size_t paramOffset) final;

    virtual void do_runLambda(const Batch& batch, size_t paramOffset) final;

    virtual void do_startNamedCall(const Batch& batch, size_t paramOffset) final;
    virtual void do_stopNamedCall(const Batch& batch, size_t paramOffset) final;

    virtual void do_pushProfileRange(const Batch& batch, size_t paramOffset) final;
    virtual void do_popProfileRange(const Batch& batch, size_t paramOffset) final;

    // TODO: As long as we have gl calls explicitely issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    virtual void do_glActiveBindTexture(const Batch& batch, size_t paramOffset) final;

    virtual void do_glUniform1i(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform1f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform2f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4iv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset) final;

    virtual void do_glColor4f(const Batch& batch, size_t paramOffset) final;

    // The State setters called by the GLState::Commands when a new state is assigned
    virtual void do_setStateFillMode(int32 mode) final;
    virtual void do_setStateCullMode(int32 mode) final;
    virtual void do_setStateFrontFaceClockwise(bool isClockwise) final;
    virtual void do_setStateDepthClampEnable(bool enable) final;
    virtual void do_setStateScissorEnable(bool enable) final;
    virtual void do_setStateMultisampleEnable(bool enable) final;
    virtual void do_setStateAntialiasedLineEnable(bool enable) final;
    virtual void do_setStateDepthBias(Vec2 bias) final;
    virtual void do_setStateDepthTest(State::DepthTest test) final;
    virtual void do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest) final;
    virtual void do_setStateAlphaToCoverageEnable(bool enable) final;
    virtual void do_setStateSampleMask(uint32 mask) final;
    virtual void do_setStateBlend(State::BlendFunction blendFunction) final;
    virtual void do_setStateColorWriteMask(uint32 mask) final;
    virtual void do_setStateBlendFactor(const Batch& batch, size_t paramOffset) final;
    virtual void do_setStateScissorRect(const Batch& batch, size_t paramOffset) final;

    virtual GLuint getFramebufferID(const FramebufferPointer& framebuffer);
    virtual GLuint getTextureID(const TexturePointer& texture, bool needTransfer = true);
    virtual GLuint getBufferID(const Buffer& buffer);
    virtual GLuint getQueryID(const QueryPointer& query);
    virtual bool isTextureReady(const TexturePointer& texture);

    virtual void releaseBuffer(GLuint id, Size size) const;
    virtual void releaseTexture(GLuint id, Size size) const;
    virtual void releaseFramebuffer(GLuint id) const;
    virtual void releaseShader(GLuint id) const;
    virtual void releaseProgram(GLuint id) const;
    virtual void releaseQuery(GLuint id) const;

protected:
    explicit GLBackend(bool syncCache) : Parent() { }
    virtual void initInput() final;

    void recycle() const override;
    static const size_t INVALID_OFFSET = (size_t)-1;

    void renderPassTransfer(const Batch& batch);
    void renderPassDraw(const Batch& batch);

    struct InputStageState {
        bool _invalidFormat { true };
        Stream::FormatPointer _format;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation { 0 };

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;
        BuffersState _invalidBuffers { 0 };

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;
        std::vector<GLuint> _bufferVBOs;

        glm::vec4 _colorAttribute{ 0.0f };

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset { 0 };
        Type _indexBufferType { UINT32 };
        
        BufferPointer _indirectBuffer;
        Offset _indirectBufferOffset{ 0 };
        Offset _indirectBufferStride{ 0 };

        GLuint _defaultVAO { 0 };

        InputStageState() :
            _buffers(_invalidBuffers.size()),
            _bufferOffsets(_invalidBuffers.size(), 0),
            _bufferStrides(_invalidBuffers.size(), 0),
            _bufferVBOs(_invalidBuffers.size(), 0) {}
    } _input;


    // Allows for correction of the camera pose to account for changes
    // between the time when a was recorded and the time(s) when it is 
    // executed
    struct CameraCorrection {
        Mat4 correction;
        Mat4 correctionInverse;
    };

    struct TransformStageState {
        using CameraBufferElement = TransformCamera;
        using TransformCameras = std::vector<CameraBufferElement>;

        TransformCamera _camera;
        TransformCameras _cameras;

        mutable std::map<std::string, GLvoid*> _drawCallInfoOffsets;

        GLuint _objectBuffer { 0 };
        GLuint _cameraBuffer { 0 };
        GLuint _drawCallInfoBuffer { 0 };
        GLuint _objectBufferTexture { 0 };
        size_t _cameraUboSize { 0 };
        bool _viewIsCamera{ false };
        bool _skybox { false };
        Transform _view;
        CameraCorrection _correction;

        Mat4 _projection;
        Vec4i _viewport { 0, 0, 1, 1 };
        Vec2 _depthRange { 0.0f, 1.0f };
        bool _invalidView { false };
        bool _invalidProj { false };
        bool _invalidViewport { false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        mutable List::const_iterator _camerasItr;
        mutable size_t _currentCameraOffset{ INVALID_OFFSET };

        void preUpdate(size_t commandIndex, const StereoState& stereo);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void bindCurrentCamera(int stereoSide) const;
    } _transform;

    struct ResourceStageState {
        std::array<TexturePointer, MAX_NUM_RESOURCE_TEXTURES> _textures;
        //Textures _textures { { MAX_NUM_RESOURCE_TEXTURES } };
        int findEmptyTextureSlot() const;
    } _resource;

};

} }

//gpu::BackendPointer gpu::gl::GLBackend::createBackend() { return std::make_shared<GLBackend>(); }

#endif
