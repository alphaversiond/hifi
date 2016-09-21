//
//  Created by Bradley Austin Davis on 2016/05/14
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLShared.h"

#include <mutex>

#include <QtCore/QThread>

#include <GPUIdent.h>
#include <NumericalConstants.h>
#include <fstream>

Q_LOGGING_CATEGORY(gpugllogging, "hifi.gpu.gl")

namespace gpu { namespace gl {

bool checkGLError(const char* name) {
    /*GLenum error = glGetError();
    if (!error) {
        return false;
    } else {
        switch (error) {
        case GL_INVALID_ENUM:
            qCDebug(gpugllogging) << "GLBackend::" << name << ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_INVALID_VALUE:
            qCDebug(gpugllogging) << "GLBackend" << name << ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
            break;
        case GL_INVALID_OPERATION:
            qCDebug(gpugllogging) << "GLBackend" << name << ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            qCDebug(gpugllogging) << "GLBackend" << name << ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
            break;
        case GL_OUT_OF_MEMORY:
            qCDebug(gpugllogging) << "GLBackend" << name << ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
            break;
        case GL_STACK_UNDERFLOW:
            qCDebug(gpugllogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
            break;
        case GL_STACK_OVERFLOW:
            qCDebug(gpugllogging) << "GLBackend" << name << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
            break;
        }
        return true;
    } AND!!! */
    return false;
}

bool checkGLErrorDebug(const char* name) {
#ifdef DEBUG
    return checkGLError(name);
#else
    Q_UNUSED(name);
    return false;
#endif
}

} }

using namespace gpu;


