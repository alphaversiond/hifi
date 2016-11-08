//
//  Created by Gabriel Calero & Cristian Duarte on 2016/11/03
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DaydreamDisplayPlugin.h"
#include <ViewFrustum.h>
#include <controllers/Pose.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/GLWidget.h>
#include <gpu/Frame.h>

#ifdef ANDROID
#include <QtOpenGL/QGLWidget>
#endif

const QString DaydreamDisplayPlugin::NAME("Daydream");

glm::uvec2 DaydreamDisplayPlugin::getRecommendedUiSize() const {
    return uvec2(2560, 1440);
}


bool DaydreamDisplayPlugin::isSupported() const {
    return true;
}

void DaydreamDisplayPlugin::resetSensors() {
    _currentRenderFrameInfo.renderPose = glm::mat4(); // identity
}

void DaydreamDisplayPlugin::internalPresent() {
    qDebug() << "[DaydreamDisplayPlugin] internalPresent";
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

 // Composite together the scene, overlay and mouse cursor
    hmdPresent();

    //if (!_disablePreview) {
        qDebug() << "[DaydreamDisplayPlugin] !_disablePreview";
        // screen preview mirroring
        auto sourceSize = _renderTargetSize;
/*        if (_monoPreview) {
            sourceSize.x >>= 1;
        }
*/
        qDebug() << "[DaydreamDisplayPlugin] sourceSize " << sourceSize; 
        float shiftLeftBy = getLeftCenterPixel() - (sourceSize.x / 2);
        float newWidth = sourceSize.x - shiftLeftBy;
        qDebug() << "[DaydreamDisplayPlugin] shiftLeftBy " << shiftLeftBy; 

        const unsigned int RATIO_Y = 9;
        const unsigned int RATIO_X = 16;
        glm::uvec2 originalClippedSize { newWidth, newWidth * RATIO_Y / RATIO_X };

        glm::ivec4 viewport = getViewportForSourceSize(sourceSize);
        glm::ivec4 scissor = viewport;

        render([&](gpu::Batch& batch) {

            //if (_monoPreview) {
                auto window = _container->getPrimaryWidget();
                float devicePixelRatio = window->devicePixelRatio();
                glm::vec2 windowSize = toGlm(window->size());
                windowSize *= devicePixelRatio;

                float windowAspect = aspect(windowSize);  // example: 1920 x 1080 = 1.78
                float sceneAspect = aspect(originalClippedSize); // usually: 1512 x 850 = 1.78


                bool scaleToWidth = windowAspect < sceneAspect;

                float ratio;
                int scissorOffset;

                if (scaleToWidth) {
                    ratio = (float)windowSize.x / (float)newWidth;
                } else {
                    ratio = (float)windowSize.y / (float)originalClippedSize.y;
                }

                float scaledShiftLeftBy = shiftLeftBy * ratio;

                int scissorSizeX = originalClippedSize.x * ratio;
                int scissorSizeY = originalClippedSize.y * ratio;

                int viewportSizeX = sourceSize.x * ratio;
                int viewportSizeY = sourceSize.y * ratio;
                int viewportOffset = ((int)windowSize.y - viewportSizeY) / 2;

                if (scaleToWidth) {
                    scissorOffset = ((int)windowSize.y - scissorSizeY) / 2;
                    scissor = ivec4(0, scissorOffset, scissorSizeX, scissorSizeY);
                    viewport = ivec4(-scaledShiftLeftBy, viewportOffset, viewportSizeX, viewportSizeY);
                } else {
                    scissorOffset = ((int)windowSize.x - scissorSizeX) / 2;
                    scissor = ivec4(scissorOffset, 0, scissorSizeX, scissorSizeY);
                    viewport = ivec4(scissorOffset - scaledShiftLeftBy, viewportOffset, viewportSizeX, viewportSizeY);
                }

                viewport.z *= 2;
            //}

            qDebug() << "[DaydreamDisplayPlugin] viewport" << viewport.x << "," << viewport.y << "," << viewport.z << "," << viewport.w;
            qDebug() << "[DaydreamDisplayPlugin] scissor" << scissor.x << "," << scissor.y << "," << scissor.z << "," << scissor.w;
            viewport = ivec4(0,0,2560,1440);
            batch.enableStereo(false);
            batch.resetViewTransform();
            batch.setFramebuffer(gpu::FramebufferPointer());
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(0));
            batch.setStateScissorRect(viewport); // was viewport
            batch.setViewportTransform(viewport);
            batch.setResourceTexture(0, _compositeFramebuffer->getRenderBuffer(0));
            batch.setPipeline(_presentPipeline);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });
        swapBuffers();
//    } 
/*
    
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.resetViewTransform();
        batch.setFramebuffer(gpu::FramebufferPointer());
        batch.setViewportTransform(ivec4(uvec2(0), getSurfacePixels()));
        batch.setResourceTexture(0, _compositeFramebuffer->getRenderBuffer(0));
        if (!_presentPipeline) {
            qDebug() << "OpenGLDisplayPlugin setting null _presentPipeline ";
        }

        batch.setPipeline(_presentPipeline);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
    swapBuffers();
    _presentRate.increment();
    */
}

ivec4 DaydreamDisplayPlugin::getViewportForSourceSize(const uvec2& size) const {
    // screen preview mirroring
    auto window = _container->getPrimaryWidget();
    qDebug() << "[DaydreamDisplayPlugin] window " << window; 
    auto devicePixelRatio = window->devicePixelRatio();
    auto windowSize = toGlm(window->size());
    qDebug() << "[DaydreamDisplayPlugin] windowSize " << windowSize; 
    windowSize *= devicePixelRatio;
    float windowAspect = aspect(windowSize);
    float sceneAspect = aspect(size);
    float aspectRatio = sceneAspect / windowAspect;

    uvec2 targetViewportSize = windowSize;
    if (aspectRatio < 1.0f) {
        targetViewportSize.x *= aspectRatio;
    } else {
        targetViewportSize.y /= aspectRatio;
    }

    uvec2 targetViewportPosition;
    if (targetViewportSize.x < windowSize.x) {
        targetViewportPosition.x = (windowSize.x - targetViewportSize.x) / 2;
    } else if (targetViewportSize.y < windowSize.y) {
        targetViewportPosition.y = (windowSize.y - targetViewportSize.y) / 2;
    }
    return ivec4(targetViewportPosition, targetViewportSize);
}

float DaydreamDisplayPlugin::getLeftCenterPixel() const {
    glm::mat4 eyeProjection = _eyeProjections[Left];
    glm::mat4 inverseEyeProjection = glm::inverse(eyeProjection);
    vec2 eyeRenderTargetSize = { _renderTargetSize.x / 2, _renderTargetSize.y };
    vec4 left = vec4(-1, 0, -1, 1);
    vec4 right = vec4(1, 0, -1, 1);
    vec4 right2 = inverseEyeProjection * right;
    vec4 left2 = inverseEyeProjection * left;
    left2 /= left2.w;
    right2 /= right2.w;
    float width = -left2.x + right2.x;
    float leftBias = -left2.x / width;
    float leftCenterPixel = eyeRenderTargetSize.x * leftBias;
    return leftCenterPixel;
}


bool DaydreamDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    _currentRenderFrameInfo = FrameInfo();
    _currentRenderFrameInfo.sensorSampleTime = secTimestampNow();
    _currentRenderFrameInfo.predictedDisplayTime = _currentRenderFrameInfo.sensorSampleTime;
    // FIXME simulate head movement
    //_currentRenderFrameInfo.renderPose = ;
    //_currentRenderFrameInfo.presentPose = _currentRenderFrameInfo.renderPose;

    withNonPresentThreadLock([&] {
        _uiModelTransform = DependencyManager::get<CompositorHelper>()->getModelTransform();
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
        
        _handPoses[0] = glm::translate(mat4(), vec3(-0.3f, 0.0f, 0.0f));
        _handLasers[0].color = vec4(1, 0, 0, 1);
        _handLasers[0].mode = HandLaserMode::Overlay;

        _handPoses[1] = glm::translate(mat4(), vec3(0.3f, 0.0f, 0.0f));
        _handLasers[1].color = vec4(0, 1, 1, 1);
        _handLasers[1].mode = HandLaserMode::Overlay;
    });
    return Parent::beginFrameRender(frameIndex);
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void DaydreamDisplayPlugin::customizeContext() {
    // glewContextInit undefined for android (why it isn't taking it from the ndk?) AND!!!
//#ifndef ANDROID
      //emit deviceConnected(getName());

    glewInit();
    glGetError(); // clear the potential error from glewExperimental
    Parent::customizeContext();
//#endif
}

bool DaydreamDisplayPlugin::internalActivate() {
    _ipd = 0.0327499993f * 2.0f;
/* This is the daydream projection matrix */
    _eyeProjections[0][0] = vec4{-0.000005,0.010428,0.847526,0.027109};
    _eyeProjections[0][1] = vec4{0.682452,0.162940,0.003716,0.000000};
    _eyeProjections[0][2] = vec4{-0.019558,-0.999806,-0.020130,-0.200020};
    _eyeProjections[0][3] = vec4{-0.019554,-0.999606,-0.020126,0.000000};

    _eyeProjections[1][0] = vec4{-0.001080,-0.044512,0.846420,-0.027109};
    _eyeProjections[1][1] = vec4{0.682452,0.162940,0.003716,0.000000};
    _eyeProjections[1][2] = vec4{-0.019558,-0.999806,-0.020130,-0.200020};
    _eyeProjections[1][3] = vec4{-0.019554,-0.999606,-0.020126,0.000000};

    _eyeProjections[0] = mat4();
    _eyeProjections[1] = mat4();
    
    //_eyeInverseProjections[0] = glm::inverse(_eyeProjections[0]);
    //_eyeInverseProjections[1] = glm::inverse(_eyeProjections[1]);
    //_eyeOffsets[0][3] = vec4{ -0.0327499993, 0.0, 0.0149999997, 1.0 };
    //_eyeOffsets[0][3] = vec4{ 0.0327499993, 0.0, 0.0149999997, 1.0 };
    _renderTargetSize = { 2560, 1440 }; // 3024x1680 
    _cullingProjection = _eyeProjections[0];
    // This must come after the initialization, so that the values calculated
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void DaydreamDisplayPlugin::updatePresentPose() {
    float yaw = 0.0f; //sinf(secTimestampNow()) * 0.5f;
    float pitch = 0.0f; // cosf(secTimestampNow()) * 0.25f;
    // Simulates head pose latency correction
    _currentPresentFrameInfo.presentPose = 
        glm::mat4_cast(glm::angleAxis(yaw, Vectors::UP)) * 
        glm::mat4_cast(glm::angleAxis(pitch, Vectors::RIGHT));
}
