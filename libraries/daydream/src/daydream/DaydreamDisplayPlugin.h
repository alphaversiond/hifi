//
//  Created by Gabriel Calero & Cristian Duarte on 2016/11/03
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "display-plugins/hmd/HmdDisplayPlugin.h"
#include "../DaydreamPlugin.h"
#include "DaydreamHelpers.h"


// TODO: move this to plugins and add it as dependency ... somewhere for android
class DaydreamDisplayPlugin : public HmdDisplayPlugin {
    using Parent = HmdDisplayPlugin;

public:
    DaydreamDisplayPlugin();
    const QString getName() const override { return NAME; }
    grouping getGrouping() const override { return DEVELOPER; }

    //void init() override;

    bool isSupported() const override;
    void resetSensors() override final;
    bool beginFrameRender(uint32_t frameIndex) override;
    float getTargetFrameRate() const override { return 60; }
    glm::uvec2 getRecommendedUiSize() const override final;
    void internalPresent() override;
    void prepareFrameBuffer() override;

protected:
    void updatePresentPose() override;
    void hmdPresent() override {}
    bool isHmdMounted() const override { return true; }
    void customizeContext() override;
    bool internalActivate() override;
    void compositeLayers() override;
    void compositePointer() override;

    void compositePointer(gpu::Batch& batch);
    void compositeScene(gpu::Batch& batch);

private:
    static const QString NAME;
    float getLeftCenterPixel() const;
    ivec4 getViewportForSourceSize(const uvec2& size) const;
    void resetEyeProjections(GvrState *);
    gvr::Frame _frame;
};

