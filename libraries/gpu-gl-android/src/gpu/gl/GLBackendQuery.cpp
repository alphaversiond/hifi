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
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    GLQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        glGetInteger64v(GL_TIMESTAMP_EXT, (GLint64*)&glquery->_batchElapsedTime);
        if (timeElapsed) {
            glBeginQuery(GL_TIME_ELAPSED_EXT, glquery->_endqo);
        } else {
            PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC) eglGetProcAddress("glQueryCounterEXT");
            if (glQueryCounterEXT != NULL) {
                glQueryCounterEXT(glquery->_beginqo, GL_TIMESTAMP_EXT);
            }
        }
        (void)CHECK_GL_ERROR();
    }
}

void GLBackend::do_endQuery(const Batch& batch, size_t paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    GLQuery* glquery = syncGPUObject(*query);
    if (glquery) {
        if (timeElapsed) {
            glEndQuery(GL_TIME_ELAPSED_EXT);
        } else {
            PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC) eglGetProcAddress("glQueryCounterEXT");
            if (glQueryCounterEXT != NULL) {
                glQueryCounterEXT(glquery->_endqo, GL_TIMESTAMP_EXT);
            }
        }
        GLint64 now;
        glGetInteger64v(GL_TIMESTAMP_EXT, &now);
        glquery->_batchElapsedTime = now - glquery->_batchElapsedTime;

        (void)CHECK_GL_ERROR();
    }
}

void GLBackend::do_getQuery(const Batch& batch, size_t paramOffset) {
    auto query = batch._queries.get(batch._params[paramOffset]._uint);
    GLQuery* glquery = syncGPUObject(*query);
    if (glquery) { 
        //glGetQueryObjectui64v(glquery->_endqo, GL_QUERY_RESULT_AVAILABLE, &glquery->_result);
        qDebug() << "TODO: GLBackendQuery.cpp.cpp:do_getQuery glGetQueryObjectui64v";
        if (glquery->_result == GL_TRUE) {
            if (timeElapsed) {
                //glGetQueryObjectui64v(glquery->_endqo, GL_QUERY_RESULT, &glquery->_result);
                qDebug() << "TODO: GLBackendQuery.cpp.cpp:do_getQuery glGetQueryObjectui64v";
            } else {
                GLuint64 start, end;
                //glGetQueryObjectui64v(glquery->_beginqo, GL_QUERY_RESULT, &start);
                qDebug() << "TODO: GLBackendQuery.cpp.cpp:do_getQuery glGetQueryObjectui64v";
                //glGetQueryObjectui64v(glquery->_endqo, GL_QUERY_RESULT, &end);
                qDebug() << "TODO: GLBackendQuery.cpp.cpp:do_getQuery glGetQueryObjectui64v";

                glquery->_result = end - start;
            }
            query->triggerReturnHandler(glquery->_result, glquery->_batchElapsedTime);
        }
        (void)CHECK_GL_ERROR();
    }
}

void GLBackend::resetQueryStage() {
}
