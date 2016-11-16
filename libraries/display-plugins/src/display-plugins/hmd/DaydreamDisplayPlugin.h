//
//  Created by Gabriel Calero & Cristian Duarte on 2016/11/03
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "HmdDisplayPlugin.h"

class GvrState {

public:
    GvrState(gvr_context *ctx);

    gvr_context* _gvr_context;
    std::unique_ptr<gvr::GvrApi> _gvr_api;

    std::unique_ptr<gvr::SwapChain> _swapchain;
    std::unique_ptr<gvr::ControllerApi> _controller_api;
    gvr::BufferViewportList _viewport_list;
    gvr::BufferViewport _scratch_viewport;
    // Size of the offscreen framebuffer.
    gvr::Sizei _framebuf_size;

    // The last controller state (updated once per frame).
    gvr::ControllerState _controller_state;
};


// TODO: move this to plugins and add it as dependency ... somewhere for android
class DaydreamDisplayPlugin : public HmdDisplayPlugin {
    using Parent = HmdDisplayPlugin;

public:
    const QString& getName() const override { return NAME; }
    grouping getGrouping() const override { return DEVELOPER; }

    //void init() override;

    bool isSupported() const override;
    void resetSensors() override final;
    bool beginFrameRender(uint32_t frameIndex) override;
    float getTargetFrameRate() const override { return 90; }
    glm::uvec2 getRecommendedUiSize() const override final;
    void internalPresent() override;

protected:
    void updatePresentPose() override;
    void hmdPresent() override {}
    bool isHmdMounted() const override { return true; }
    void customizeContext() override;
    bool internalActivate() override;
private:
    static const QString NAME;
    float getLeftCenterPixel() const;
    ivec4 getViewportForSourceSize(const uvec2& size) const;

    GvrState * _gvrState;
};

