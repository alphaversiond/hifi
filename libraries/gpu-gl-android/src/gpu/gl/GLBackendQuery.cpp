//
//  GLBackendQuery.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 7/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLQuery.h"

using namespace gpu;
using namespace gpu::gl;

// Eventually, we want to test with TIME_ELAPSED instead of TIMESTAMP
#ifdef Q_OS_MAC
static bool timeElapsed = true;
#else
static bool timeElapsed = false;
#endif

void GLBackend::do_beginQuery(const Batch& batch, size_t paramOffset) {

}

void GLBackend::do_endQuery(const Batch& batch, size_t paramOffset) {

}

void GLBackend::do_getQuery(const Batch& batch, size_t paramOffset) {

}

void GLBackend::releaseQuery(GLuint id) const {

}

