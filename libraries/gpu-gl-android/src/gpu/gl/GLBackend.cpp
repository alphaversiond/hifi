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

void GLBackend::initInput() {
    if(!_input._defaultVAO) {
        //glGenVertexArrays(1, &_input._defaultVAO);
        qDebug() << "TODO: GLBackend.cpp:initInput glGenVertexArrays";
    }
    // glBindVertexArray(_input._defaultVAO);
    qDebug() << "TODO: GLBackend.cpp:initInput glGenVertexArrays";
    (void) CHECK_GL_ERROR();
}


