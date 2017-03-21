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
#include <CursorManager.h>

#include <gpu/Context.h>

#ifdef ANDROID
#include <QtOpenGL/QGLWidget>
#endif

const QString DaydreamDisplayPlugin::NAME("Daydream");

DaydreamDisplayPlugin::DaydreamDisplayPlugin() :
    _frame { gvr::Frame(NULL) } {
}

glm::uvec2 DaydreamDisplayPlugin::getRecommendedUiSize() const {
    auto window = _container->getPrimaryWidget();
    glm::vec2 windowSize = toGlm(window->size());
    return windowSize;
}


bool DaydreamDisplayPlugin::isSupported() const {
    return true;
}

void DaydreamDisplayPlugin::resetSensors() {
    _currentRenderFrameInfo.renderPose = glm::mat4(); // identity
}

void DaydreamDisplayPlugin::compositeLayers() {
    // updateCompositeFramebuffer();

    gpu::Batch compBatch;
    compBatch.enableStereo(false);
    compBatch.setViewportTransform(ivec4(uvec2(), getRecommendedRenderSize()));
    compBatch.setStateScissorRect(ivec4(uvec2(), getRecommendedRenderSize()));
    {
        PROFILE_RANGE_EX(render, "compositeScene", 0xff0077ff, (uint64_t)presentCount())
        compositeScene(compBatch);
    }

    {
        PROFILE_RANGE_EX(render, "compositeOverlay", 0xff0077ff, (uint64_t)presentCount())
        compositeOverlay(compBatch);
    }
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    if (compositorHelper->getReticleVisible()) {
        PROFILE_RANGE_EX(render, "compositePointer", 0xff0077ff, (uint64_t)presentCount())
        compositePointer(compBatch);
    }

    {
        PROFILE_RANGE_EX(render, "compositeExtra", 0xff0077ff, (uint64_t)presentCount())
        compositeExtra(compBatch);
    }
    {
        PROFILE_RANGE_EX(render, "compositeExecuteBatch", 0xff0077ff, (uint64_t)presentCount())
        _gpuContext->executeBatch(compBatch);
    }
}

void DaydreamDisplayPlugin::compositeScene(gpu::Batch& batch) {
    //batch.resetViewTransform();
    //batch.setProjectionTransform(mat4());
    batch.setPipeline(_simplePipeline);
    batch.setResourceTexture(0, _currentFrame->framebuffer->getRenderBuffer(0));
    batch.draw(gpu::TRIANGLE_STRIP, 4);
}

void DaydreamDisplayPlugin::prepareFrameBuffer() {
    GvrState *gvrState = GvrState::getInstance();
    _frame = gvrState->_swapchain->AcquireFrame();
    _frame.BindBuffer(0);
}

void DaydreamDisplayPlugin::internalPresent() {
    PROFILE_RANGE_EX(render, __FUNCTION__, 0xff00ff00, (uint64_t)presentCount())

 // Composite together the scene, overlay and mouse cursor
    //hmdPresent();
    GvrState *gvrState = GvrState::getInstance();
    gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
    pred_time.monotonic_system_time_nanos += 50000000; // 50ms

    gvr::Mat4f head_view = gvrState->_gvr_api->GetHeadSpaceFromStartSpaceRotation(pred_time);
    _frame.Unbind();
    {PROFILE_RANGE_EX(render, "gvrFrameSubmit", 0xff33ff22, (uint64_t)presentCount())
    _frame.Submit(gvrState->_viewport_list, head_view);
    }
    static int submitFrameCounter = 0;
        static long tsSec = 0L;
        long currentSec = static_cast<long int> (std::time(nullptr));
        if (tsSec != currentSec) {
            qDebug() << "[RENDER-METRIC] Render rate: " << submitFrameCounter << " fps";
            submitFrameCounter = 0;    
            tsSec = currentSec;
        }
        submitFrameCounter++;

    swapBuffers();
}

ivec4 DaydreamDisplayPlugin::getViewportForSourceSize(const uvec2& size) const {
    // screen preview mirroring
    auto window = _container->getPrimaryWidget();
    auto devicePixelRatio = window->devicePixelRatio();
    auto windowSize = toGlm(window->size());
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


    GvrState *gvrState = GvrState::getInstance();
    glm::quat orientation = toGlm(gvrState->_controller_state.GetOrientation());
    
    
    auto correctedLeftPose = daydreamControllerPoseToHandPose(true, orientation);
    auto correctedRightPose = daydreamControllerPoseToHandPose(false, orientation);
    
    std::array<glm::mat4, 2> handPoses;
    
    static const glm::quat HAND_TO_LASER_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_NEG_Y); // the angle between (0,0,1) and (0, -1, 0)

    handPoses[0] = glm::translate(glm::mat4(), correctedLeftPose.translation) * glm::mat4_cast(correctedLeftPose.rotation * HAND_TO_LASER_ROTATION);
    handPoses[1] = glm::translate(glm::mat4(), correctedRightPose.translation) * glm::mat4_cast(correctedRightPose.rotation * HAND_TO_LASER_ROTATION);

    withNonPresentThreadLock([&] {
        _uiModelTransform = DependencyManager::get<CompositorHelper>()->getModelTransform();
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
        
        _handPoses[0] = handPoses[0];//glm::translate(mat4(), vec3(0.1f, 0.3f, 0.0f));
/*      _handLasers[0].color = vec4(0, 0, 0, 0);
        _handLasers[0].mode = HandLaserMode::Overlay;
        _handLasers[0].direction = vec3(0,0,0);
*/

        _handPoses[1] = handPoses[1];
/*        _handLasers[1].color = vec4(0, 0, 0, 0);
        _handLasers[1].mode = HandLaserMode::None;
        _handLasers[1].direction = vec3(0,0,0);
*/
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
    _container->setFullscreen(nullptr, true);
    qDebug() << "DaydreamDisplayPlugin::internalActivate " << __gvr_context;
    GvrState::init(__gvr_context);
    GvrState *gvrState = GvrState::getInstance();

    if (gvrState->_gvr_api) {
        qDebug() << "Initialize _gvr_api GL " << gvrState;
        gvrState->_gvr_api->InitializeGl();
    }

    std::vector<gvr::BufferSpec> specs;
    specs.push_back(gvrState->_gvr_api->CreateBufferSpec());
    gvrState->_framebuf_size = gvrState->_gvr_api->GetMaximumEffectiveRenderTargetSize();

    //qDebug() << "_framebuf_size " << gvrState->_framebuf_size.width << ", " << gvrState->_framebuf_size.height; //  3426 ,  1770

    auto window = _container->getPrimaryWidget();
    glm::vec2 windowSize = toGlm(window->size());

    // Because we are using 2X MSAA, we can render to half as many pixels and
    // achieve similar quality. Scale each dimension by sqrt(2)/2 ~= 7/10ths.
    gvrState->_framebuf_size.width = windowSize.x;//(7 * gvrState->_framebuf_size.width) / 10;
    gvrState->_framebuf_size.height = windowSize.y; //(7 * gvrState->_framebuf_size.height) / 10;

    specs[0].SetSize(gvrState->_framebuf_size);
    specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
    specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
    specs[0].SetSamples(2);
    gvrState->_swapchain.reset(new gvr::SwapChain(gvrState->_gvr_api->CreateSwapChain(specs)));
    gvrState->_viewport_list.SetToRecommendedBufferViewports();


    resetEyeProjections(gvrState);

    _ipd = 0.0327499993f * 2.0f;

    _eyeOffsets[0][3] = vec4{ -0.0327499993, 0.0, 0.0149999997, 1.0 };
    _eyeOffsets[1][3] = vec4{ 0.0327499993, 0.0, 0.0149999997, 1.0 };

    _renderTargetSize = glm::vec2(gvrState->_framebuf_size.width ,  gvrState->_framebuf_size.height);

    // This must come after the initialization, so that the values calculated
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void DaydreamDisplayPlugin::updatePresentPose() {
    gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
    pred_time.monotonic_system_time_nanos += 50000000; // 50ms
    
    GvrState *gvrState = GvrState::getInstance();
    gvr::Mat4f head_view =
    gvrState->_gvr_api->GetHeadSpaceFromStartSpaceRotation(pred_time);

    glm::mat4 glmHeadView = glm::inverse(glm::make_mat4(&(MatrixToGLArray(head_view)[0])));

    _currentPresentFrameInfo.presentPose = glmHeadView;

    if (gvrState->_controller_state.GetApiStatus() == gvr_controller_api_status::GVR_CONTROLLER_API_OK &&
        gvrState->_controller_state.GetConnectionState() == gvr_controller_connection_state::GVR_CONTROLLER_CONNECTED) {

      if (gvrState->_controller_state.GetRecentered()) {
        resetEyeProjections(gvrState);
      }
    }
}

void DaydreamDisplayPlugin::resetEyeProjections(GvrState *gvrState) {
        gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
    pred_time.monotonic_system_time_nanos += 50000000; // 50ms

    gvr::Mat4f left_eye_view = gvrState->_gvr_api->GetEyeFromHeadMatrix(GVR_LEFT_EYE);
    gvr::Mat4f right_eye_view =gvrState->_gvr_api->GetEyeFromHeadMatrix(GVR_RIGHT_EYE);

    gvr::BufferViewport scratch_viewport(gvrState->_gvr_api->CreateBufferViewport());

    gvrState->_viewport_list.GetBufferViewport(0, &scratch_viewport);
    gvr::Mat4f proj_matrix = PerspectiveMatrixFromView(scratch_viewport.GetSourceFov(), 0.1, 1000.0);

    gvr::Mat4f mvp = MatrixMul(proj_matrix, left_eye_view);
    std::array<float, 16> mvpArr = MatrixToGLArray(mvp);

    _eyeProjections[0][0] = vec4{mvpArr[0],mvpArr[1],mvpArr[2],mvpArr[3]};
    _eyeProjections[0][1] = vec4{mvpArr[4],mvpArr[5],mvpArr[6],mvpArr[7]};
    _eyeProjections[0][2] = vec4{mvpArr[8],mvpArr[9],mvpArr[10],mvpArr[11]};
    _eyeProjections[0][3] = vec4{mvpArr[12],mvpArr[13],mvpArr[14],mvpArr[15]};

    gvrState->_viewport_list.GetBufferViewport(1, &scratch_viewport);
    
    proj_matrix = PerspectiveMatrixFromView(scratch_viewport.GetSourceFov(), 0.1, 1000.0);
    mvp = MatrixMul(proj_matrix, right_eye_view);
    mvpArr = MatrixToGLArray(mvp);

    _eyeProjections[1][0] = vec4{mvpArr[0],mvpArr[1],mvpArr[2],mvpArr[3]};
    _eyeProjections[1][1] = vec4{mvpArr[4],mvpArr[5],mvpArr[6],mvpArr[7]};
    _eyeProjections[1][2] = vec4{mvpArr[8],mvpArr[9],mvpArr[10],mvpArr[11]};
    _eyeProjections[1][3] = vec4{mvpArr[12],mvpArr[13],mvpArr[14],mvpArr[15]};

    for_each_eye([&](Eye eye) {
        _eyeInverseProjections[eye] = glm::inverse(_eyeProjections[eye]);
    });

    _cullingProjection = _eyeProjections[0];
}

void DaydreamDisplayPlugin::compositePointer() {}
void DaydreamDisplayPlugin::compositePointer(gpu::Batch& batch) {}


