//
//  Created by Gabriel Calero & Cristian Duarte on 9/27/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLESBackend.h"


Q_LOGGING_CATEGORY(gpugleslogging, "hifi.gpu.gles")

using namespace gpu;
using namespace gpu::gles;

GLuint getFramebufferID(const FramebufferPointer& framebuffer) { }
GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer) { }

GLuint getBufferID(const Buffer& buffer) { }
GLBuffer* syncGPUObject(const Buffer& buffer) { }

GLuint getTextureID(const TexturePointer& texture, bool needTransfer = true) { }
GLTexture* syncGPUObject(const TexturePointer& texture, bool sync = true) { }

GLQuery* syncGPUObject(const Query& query) { }

// Draw Stage
void GLESBackend::do_draw(const Batch& batch, size_t paramOffset) { }
void GLESBackend::do_drawIndexed(const Batch& batch, size_t paramOffset) { }
void GLESBackend::do_drawInstanced(const Batch& batch, size_t paramOffset) { }
void GLESBackend::do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) { }
void GLESBackend::do_multiDrawIndirect(const Batch& batch, size_t paramOffset) { }
void GLESBackend::do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) { }
// Synchronize the state cache of this Backend with the actual real state of the GL Context
void GLESBackend::transferTransformState(const Batch& batch) const { };
void GLESBackend::initTransform() { };
