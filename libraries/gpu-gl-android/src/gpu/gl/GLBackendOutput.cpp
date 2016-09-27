//
//  GLBackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLShared.h"
// TODO: restore #include "GLFramebuffer.h"

#include <QtGui/QImage>

using namespace gpu;
using namespace gpu::gl;

/*void GLBackend::syncOutputStateCache() {
    GLint currentFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);

    _output._drawFBO = currentFBO;
    _output._framebuffer.reset();
}

void GLBackend::resetOutputStage() {
    if (_output._framebuffer) {
        _output._framebuffer.reset();
        _output._drawFBO = 0;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
}
*/
void GLBackend::do_setFramebuffer(const Batch& batch, size_t paramOffset) { }

void GLBackend::do_clearFramebuffer(const Batch& batch, size_t paramOffset) { }

void GLBackend::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
    /*auto readFBO = getFramebufferID(srcFramebuffer);
    if (srcFramebuffer && readFBO) {
        if ((srcFramebuffer->getWidth() < (region.x + region.z)) || (srcFramebuffer->getHeight() < (region.y + region.w))) {
          qCDebug(gpugllogging) << "GLBackend::downloadFramebuffer : srcFramebuffer is too small to provide the region queried";
          return;
        }
    }

    if ((destImage.width() < region.z) || (destImage.height() < region.w)) {
          qCDebug(gpugllogging) << "GLBackend::downloadFramebuffer : destImage is too small to receive the region of the framebuffer";
          return;
    }

    GLenum format = GL_BGRA;
    if (destImage.format() != QImage::Format_ARGB32) {
          qCDebug(gpugllogging) << "GLBackend::downloadFramebuffer : destImage format must be FORMAT_ARGB32 to receive the region of the framebuffer";
          return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, getFramebufferID(srcFramebuffer));
    glReadPixels(region.x, region.y, region.z, region.w, format, GL_UNSIGNED_BYTE, destImage.bits());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    (void) CHECK_GL_ERROR();*/
}

GLuint GLBackend::getFramebufferID(const FramebufferPointer& framebuffer) {

}
