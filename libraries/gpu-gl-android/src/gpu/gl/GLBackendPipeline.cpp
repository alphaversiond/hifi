//
//  GLBackendPipeline.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLShared.h"
#include "GLPipeline.h"
#include "GLShader.h"
#include "GLState.h"
#include "GLBuffer.h"
#include "GLTexture.h"

using namespace gpu;
using namespace gpu::gl;

void GLBackend::do_setPipeline(const Batch& batch, size_t paramOffset) {
}

void GLBackend::do_setUniformBuffer(const Batch& batch, size_t paramOffset) {
}

void GLBackend::do_setResourceTexture(const Batch& batch, size_t paramOffset) {
}

int GLBackend::ResourceStageState::findEmptyTextureSlot() const {
    // start from the end of the slots, try to find an empty one that can be used
/*    for (auto i = MAX_NUM_RESOURCE_TEXTURES - 1; i > 0; i--) {
        if (!_textures[i]) {
            return i;
        }
    }
*/
    return -1;
}

