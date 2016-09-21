//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Null_Backend_h
#define hifi_gpu_Null_Backend_h

#include <assert.h>
#include <functional>
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

//#include <gl/Config.h>

#include <QtCore/QLoggingCategory>

#include <gpu/Context.h>

namespace gpu { namespace gl {

class GLBackend : public gpu::Backend {
    using Parent = gpu::Backend;
    // Context Backend static interface required
    friend class gpu::Context;
    static void init() {}
    //static BackendPointer createBackend();
    //static gpu::Backend* createBackend() { return new GLBackend(); }
    static BackendPointer createBackend() { return std::make_shared<GLBackend>(); }
    // making it public static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) { return true; }
protected:
    explicit GLBackend(bool syncCache) : Parent() { }
    
public:
    GLBackend() { } /*: Parent() { } was protected*/
    ~GLBackend() { }

    void setCameraCorrection(const Mat4& correction) { } // 48

    void render(const Batch& batch) final { }

    // This call synchronize the Full Backend cache with the current GLState
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    void syncCache() final { }

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) final { }

    virtual void recycle() const {}

    virtual bool isTextureReady(const TexturePointer& texture) { return false; } // 170

    static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings = Shader::BindingSet()) { return false; } // cpp 71
};

} }

//gpu::BackendPointer gpu::gl::GLBackend::createBackend() { return std::make_shared<GLBackend>(); }

#endif
