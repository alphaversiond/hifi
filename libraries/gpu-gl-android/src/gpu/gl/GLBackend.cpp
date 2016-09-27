//
//  GLBackend.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"

#include <gl/QOpenGLContextWrapper.h>

/*  TODO: just to test, init glewForAndroid somewhere */
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
/* */

#include "GLShader.h"

using namespace gpu;
using namespace gpu::gl;

static GLBackend* INSTANCE{ nullptr };
static const char* GL_BACKEND_PROPERTY_NAME = "com.highfidelity.gl.backend";

//static BackendPointer GLBackend::createBackend() { return std::make_shared<GLBackend>(); }
BackendPointer GLBackend::createBackend() {
    // FIXME provide a mechanism to override the backend for testing
    // Where the gpuContext is initialized and where the TRUE Backend is created and assigned
    auto version = QOpenGLContextWrapper::currentContextVersion();
    std::shared_ptr<GLBackend> result;
	qDebug() << "Current context version is " << version;
	result = std::make_shared<gpu::gl::GLBackend>();
   
    result->initInput(); 
    //result->initTransform(); // TODO: uncomment

    INSTANCE = result.get();
    void* voidInstance = &(*result);
    qApp->setProperty(GL_BACKEND_PROPERTY_NAME, QVariant::fromValue(voidInstance));

    //gl::GLTexture::initTextureTransferHelper(); // TODO: uncomment
    return result;
}

GLBackend& getBackend() {
    if (!INSTANCE) {
        INSTANCE = static_cast<GLBackend*>(qApp->property(GL_BACKEND_PROPERTY_NAME).value<void*>());
    }
    return *INSTANCE;
}

bool GLBackend::makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {
    return GLShader::makeProgram(getBackend(), shader, slotBindings);
}

void GLBackend::setCameraCorrection(const Mat4& correction) { }

void GLBackend::syncCache() { }

void GLBackend::do_draw(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_drawIndexed(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_drawInstanced(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_multiDrawIndirect(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_blit(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_resetStages(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_runLambda(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_startNamedCall(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_stopNamedCall(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_pushProfileRange(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_popProfileRange(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glActiveBindTexture(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform1i(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform1f(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform2f(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform3f(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform4f(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform3fv(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform4fv(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniform4iv(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_glColor4f(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_setStateFillMode(int32 mode) { }

void GLBackend::do_setStateCullMode(int32 mode) { }

void GLBackend::do_setStateFrontFaceClockwise(bool isClockwise) { }

void GLBackend::do_setStateDepthClampEnable(bool enable) { }

void GLBackend::do_setStateScissorEnable(bool enable) { }

void GLBackend::do_setStateMultisampleEnable(bool enable) { }

void GLBackend::do_setStateAntialiasedLineEnable(bool enable) { }

void GLBackend::do_setStateDepthBias(Vec2 bias) { }

void GLBackend::do_setStateDepthTest(State::DepthTest test) { }

void GLBackend::do_setStateStencil(State::StencilActivation activation, State::StencilTest testFront, State::StencilTest testBack) { }

void GLBackend::do_setStateAlphaToCoverageEnable(bool enable) { }

void GLBackend::do_setStateSampleMask(uint32 mask) { }

void GLBackend::do_setStateBlend(State::BlendFunction function) { }

void GLBackend::do_setStateColorWriteMask(uint32 mask) { }

void GLBackend::do_setStateBlendFactor(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_setStateScissorRect(const Batch& batch, size_t paramOffset) { }

GLuint GLBackend::getQueryID(const QueryPointer& query) { }

void GLBackend::releaseBuffer(GLuint id, Size size) const { }

void GLBackend::releaseTexture(GLuint id, Size size) const { }

void GLBackend::releaseFramebuffer(GLuint id) const { }

void GLBackend::releaseShader(GLuint id) const { }

void GLBackend::releaseProgram(GLuint id) const { }

void GLBackend::recycle() const {
  /*  {
        std::vector<GLuint> ids;
        std::list<std::pair<GLuint, Size>> buffersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_buffersTrash, buffersTrash);
        }
        ids.reserve(buffersTrash.size());
        for (auto pair : buffersTrash) {
            ids.push_back(pair.first);
            decrementBufferGPUCount();
            updateBufferGPUMemoryUsage(pair.second, 0);
        }
        if (!ids.empty()) {
            glDeleteBuffers((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<GLuint> framebuffersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_framebuffersTrash, framebuffersTrash);
        }
        ids.reserve(framebuffersTrash.size());
        for (auto id : framebuffersTrash) {
            ids.push_back(id);
        }
        if (!ids.empty()) {
            glDeleteFramebuffers((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<std::pair<GLuint, Size>> texturesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_texturesTrash, texturesTrash);
        }
        ids.reserve(texturesTrash.size());
        for (auto pair : texturesTrash) {
            ids.push_back(pair.first);
            decrementTextureGPUCount();
            updateTextureGPUMemoryUsage(pair.second, 0);
        }
        if (!ids.empty()) {
            glDeleteTextures((GLsizei)ids.size(), ids.data());
        }
    }

    {
        std::list<GLuint> programsTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_programsTrash, programsTrash);
        }
        for (auto id : programsTrash) {
            glDeleteProgram(id);
        }
    }

    {
        std::list<GLuint> shadersTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_shadersTrash, shadersTrash);
        }
        for (auto id : shadersTrash) {
            glDeleteShader(id);
        }
    }

    {
        std::vector<GLuint> ids;
        std::list<GLuint> queriesTrash;
        {
            Lock lock(_trashMutex);
            std::swap(_queriesTrash, queriesTrash);
        }
        ids.reserve(queriesTrash.size());
        for (auto id : queriesTrash) {
            ids.push_back(id);
        }
        if (!ids.empty()) {
            glDeleteQueries((GLsizei)ids.size(), ids.data());
        }
    }
    */
}

void GLBackend::render(const Batch& batch) {
/*    _transform._skybox = _stereo._skybox = batch.isSkyboxEnabled();
    // Allow the batch to override the rendering stereo settings
    // for things like full framebuffer copy operations (deferred lighting passes)
    bool savedStereo = _stereo._enable;
    if (!batch.isStereoEnabled()) {
        _stereo._enable = false;
    }
    
    {
        PROFILE_RANGE("Transfer");
        renderPassTransfer(batch);
    }

    {
        PROFILE_RANGE(_stereo._enable ? "Render Stereo" : "Render");
        renderPassDraw(batch);
    }

    // Restore the saved stereo state for the next batch
    _stereo._enable = savedStereo;
*/}

void GLBackend::renderPassTransfer(const Batch& batch) { }

void GLBackend::renderPassDraw(const Batch& batch) { }

void GLBackend::initInput() {
    // TODO: just to test, init glewForAndroid somewhere
    
    if(!_input._defaultVAO) {
        qDebug() << "TODO: GLBackend.cpp:initInput glGenVertexArrays";
        glGenVertexArrays(1, &_input._defaultVAO);
    }
    qDebug() << "TODO: GLBackend.cpp:initInput glGenVertexArrays";
    //PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOESPROC) eglGetProcAddress("glBindVertexArrayOES");
    //glBindVertexArrayOES(_input._defaultVAO);
    (void) CHECK_GL_ERROR();
}


