//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_GLShared_h
#define hifi_gpu_GLShared_h

#include <gl/Config.h>
#include <gpu/Forward.h>
#include <gpu/Format.h>
#include <gpu/Context.h>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(gpugllogging)

namespace gpu { namespace gl { 

bool checkGLError(const char* name = nullptr);
bool checkGLErrorDebug(const char* name = nullptr);

} } // namespace gpu::gl 

#define CHECK_GL_ERROR() gpu::gl::checkGLErrorDebug(__FUNCTION__)

#endif



