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

GvrState::GvrState(gvr_context *ctx) :
    _gvr_context(ctx),
    _gvr_api(gvr::GvrApi::WrapNonOwned(_gvr_context)),
    _viewport_list(_gvr_api->CreateEmptyBufferViewportList()),
    _scratch_viewport(_gvr_api->CreateBufferViewport()) {

}


/* TODO: check what matrix system to use overall and see if this is needed */
std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix) {
  // Note that this performs a *tranpose* to a column-major matrix array, as
  // expected by GL.
  std::array<float, 16> result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result[j * 4 + i] = matrix.m[i][j];
    }
  }
  return result;
}

gvr::Mat4f MatrixMul(const gvr::Mat4f& m1, const gvr::Mat4f& m2) {
  gvr::Mat4f result;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k) {
        result.m[i][j] += m1.m[i][k] * m2.m[k][j];
      }
    }
  }
  return result;
}

gvr::Mat4f PerspectiveMatrixFromView(const gvr::Rectf& fov,
                                            float near_clip, float far_clip) {
  gvr::Mat4f result;
  const float x_left = -tan(fov.left * M_PI / 180.0f) * near_clip;
  const float x_right = tan(fov.right * M_PI / 180.0f) * near_clip;
  const float y_bottom = -tan(fov.bottom * M_PI / 180.0f) * near_clip;
  const float y_top = tan(fov.top * M_PI / 180.0f) * near_clip;
  const float zero = 0.0f;

  //CHECK(x_left < x_right && y_bottom < y_top && near_clip < far_clip &&
    //     near_clip > zero && far_clip > zero);
  const float X = (2 * near_clip) / (x_right - x_left);
  const float Y = (2 * near_clip) / (y_top - y_bottom);
  const float A = (x_right + x_left) / (x_right - x_left);
  const float B = (y_top + y_bottom) / (y_top - y_bottom);
  const float C = (near_clip + far_clip) / (near_clip - far_clip);
  const float D = (2 * near_clip * far_clip) / (near_clip - far_clip);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      result.m[i][j] = 0.0f;
    }
  }
  result.m[0][0] = X;
  result.m[0][2] = A;
  result.m[1][1] = Y;
  result.m[1][2] = B;
  result.m[2][2] = C;
  result.m[2][3] = D;
  result.m[3][2] = -1;

  return result;
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
        qDebug() << "[DaydreamDisplayPlugin] sourceSize " << sourceSize; // 2560, 1440
        float shiftLeftBy = getLeftCenterPixel() - (sourceSize.x / 2);   //        - 640 =  getLeftCenterPixel - 1280         => getLeftCenterPixel = 640
        float newWidth = sourceSize.x - shiftLeftBy;
        qDebug() << "[DaydreamDisplayPlugin] shiftLeftBy " << shiftLeftBy;  // -640

        const unsigned int RATIO_Y = 9;
        const unsigned int RATIO_X = 16;
        glm::uvec2 originalClippedSize { newWidth, newWidth * RATIO_Y / RATIO_X };

        glm::ivec4 viewport = getViewportForSourceSize(sourceSize);
        glm::ivec4 scissor = viewport;

        gvr::Frame frame = _gvrState->_swapchain->AcquireFrame();
        frame.BindBuffer(0);

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
            viewport = ivec4(0,0,windowSize);
            batch.enableStereo(false);
            batch.resetViewTransform();
            //batch.setFramebuffer(gpu::FramebufferPointer());
            batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(0));
            batch.setStateScissorRect(scissor); // was viewport
            batch.setViewportTransform(viewport);
            batch.setResourceTexture(0, _compositeFramebuffer->getRenderBuffer(0));
            batch.setPipeline(_presentPipeline);
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        });

        gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
        pred_time.monotonic_system_time_nanos += 50000000; // 50ms

        gvr::Mat4f head_view = _gvrState->_gvr_api->GetHeadSpaceFromStartSpaceRotation(pred_time);

        frame.Unbind();
        frame.Submit(_gvrState->_viewport_list, head_view);

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
    qDebug() << "[DaydreamDisplayPlugin] internalActivate with _gvr_context " << __gvr_context;
    _container->setFullscreen(nullptr, true);
    _gvrState = new GvrState(__gvr_context);

    if (_gvrState->_gvr_api) {
        qDebug() << "Initialize _gvr_api GL";
        _gvrState->_gvr_api->InitializeGl();

        _gvrState->_controller_api.reset(new gvr::ControllerApi);

        _gvrState->_controller_api->Init(gvr::ControllerApi::DefaultOptions(), _gvrState->_gvr_context);
        _gvrState->_controller_api->Resume();
    }
    qDebug() << "[DaydreamDisplayPlugin] internalActivate with _gvr_api " << _gvrState->_gvr_api->GetTimePointNow().monotonic_system_time_nanos;

    std::vector<gvr::BufferSpec> specs;
    specs.push_back(_gvrState->_gvr_api->CreateBufferSpec());
    _gvrState->_framebuf_size = _gvrState->_gvr_api->GetMaximumEffectiveRenderTargetSize();

    qDebug() << "_framebuf_size " << _gvrState->_framebuf_size.width << ", " << _gvrState->_framebuf_size.height;
    // Because we are using 2X MSAA, we can render to half as many pixels and
    // achieve similar quality. Scale each dimension by sqrt(2)/2 ~= 7/10ths.
    _gvrState->_framebuf_size.width = (7 * _gvrState->_framebuf_size.width) / 10;
    _gvrState->_framebuf_size.height = (7 * _gvrState->_framebuf_size.height) / 10;

    specs[0].SetSize(_gvrState->_framebuf_size);
    specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
    specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
    specs[0].SetSamples(2);
    _gvrState->_swapchain.reset(new gvr::SwapChain(_gvrState->_gvr_api->CreateSwapChain(specs)));
    _gvrState->_viewport_list.SetToRecommendedBufferViewports();


    resetEyeProjections();

    _ipd = 0.0327499993f * 2.0f;

    _eyeOffsets[0][3] = vec4{ -0.0327499993, 0.0, 0.0149999997, 1.0 };
    _eyeOffsets[1][3] = vec4{ 0.0327499993, 0.0, 0.0149999997, 1.0 };

    auto window = _container->getPrimaryWidget();
    glm::vec2 windowSize = toGlm(window->size());

    _renderTargetSize = windowSize; // 3024x1680 
    // This must come after the initialization, so that the values calculated
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    return Parent::internalActivate();
}

void DaydreamDisplayPlugin::updatePresentPose() {
    gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
    pred_time.monotonic_system_time_nanos += 50000000; // 50ms
    gvr::Mat4f head_view =
      _gvrState->_gvr_api->GetHeadSpaceFromStartSpaceRotation(pred_time);

    glm::mat4 glmHeadView = glm::inverse(glm::make_mat4(&(MatrixToGLArray(head_view)[0])));

    //glm::quat rotation = extractRotation(glmHeadView, true);

    //glm::vec3 angles = safeEulerAngles(rotation);
    //qDebug() << "angles  : " << angles.x << "," << angles.y << "," << angles.z ;

    //float yaw = angles.y; //sinf(secTimestampNow()) * 0.5f;
    //float pitch = angles.x; // cosf(secTimestampNow()) * 0.25f;
    // Simulates head pose latency correction
    _currentPresentFrameInfo.presentPose = glmHeadView;
       // glm::mat4_cast(glm::angleAxis(yaw, Vectors::UP)) * 
       // glm::mat4_cast(glm::angleAxis(pitch, Vectors::RIGHT));

    const int32_t old_status = _gvrState->_controller_state.GetApiStatus();
    const int32_t old_connection_state = _gvrState->_controller_state.GetConnectionState();

    // Read current controller state.
    _gvrState->_controller_state.Update(*_gvrState->_controller_api);

  // Print new API status and connection state, if they changed.
  if (_gvrState->_controller_state.GetApiStatus() != old_status ||
      _gvrState->_controller_state.GetConnectionState() != old_connection_state) {
    qDebug() << "[DAYDREAM-CONTROLLER]: Controller API status: "<<
         gvr_controller_api_status_to_string(_gvrState->_controller_state.GetApiStatus()) << ", connection state: " <<
         gvr_controller_connection_state_to_string(
             _gvrState->_controller_state.GetConnectionState());
  }

  _gvrState->_controller_api->Resume();

  bool isTouching = _gvrState->_controller_state.IsTouching();

  if (isTouching) {
      gvr_vec2f touchPos = _gvrState->_controller_state.GetTouchPos();
      qDebug() << "[DAYDREAM-CONTROLLER]: Touching x:" << touchPos.x << " y:" << touchPos.y;

  }

    bool appbutton = _gvrState->_controller_state.GetButtonUp(gvr::kControllerButtonApp);
  if (appbutton) {
        qDebug() << "[DAYDREAM-CONTROLLER]: App button pressed";
  }

  if (_gvrState->_controller_state.GetRecentered()) {
      resetEyeProjections();
  }

  gvr::ControllerQuat orientation = _gvrState->_controller_state.GetOrientation();
  qDebug() << "[DAYDREAM-CONTROLLER]: Orientation: " << orientation.qx << "," << orientation.qy << "," << orientation.qz << "," << orientation.qw;

}

void DaydreamDisplayPlugin::resetEyeProjections() {
        gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
    pred_time.monotonic_system_time_nanos += 50000000; // 50ms
    gvr::Mat4f head_view =
      _gvrState->_gvr_api->GetHeadSpaceFromStartSpaceRotation(pred_time);

    gvr::Mat4f left_eye_view =
        MatrixMul(_gvrState->_gvr_api->GetEyeFromHeadMatrix(GVR_LEFT_EYE), head_view);
    gvr::Mat4f right_eye_view =
        MatrixMul(_gvrState->_gvr_api->GetEyeFromHeadMatrix(GVR_RIGHT_EYE), head_view);

    _gvrState->_viewport_list.GetBufferViewport(0, &_gvrState->_scratch_viewport);
    gvr::Mat4f proj_matrix =
    PerspectiveMatrixFromView(_gvrState->_scratch_viewport.GetSourceFov(), 0.1, 1000.0);

    gvr::Mat4f mvp = MatrixMul(proj_matrix, left_eye_view);
    std::array<float, 16> mvpArr = MatrixToGLArray(mvp);

    _eyeProjections[0][0] = vec4{mvpArr[0],mvpArr[1],mvpArr[2],mvpArr[3]};
    _eyeProjections[0][1] = vec4{mvpArr[4],mvpArr[5],mvpArr[5],mvpArr[7]};
    _eyeProjections[0][2] = vec4{mvpArr[8],mvpArr[9],mvpArr[10],mvpArr[11]};
    _eyeProjections[0][3] = vec4{mvpArr[12],mvpArr[13],mvpArr[14],mvpArr[15]};

    mvp = MatrixMul(proj_matrix, right_eye_view);
    mvpArr = MatrixToGLArray(mvp);

    _eyeProjections[1][0] = vec4{mvpArr[0],mvpArr[1],mvpArr[2],mvpArr[3]};
    _eyeProjections[1][1] = vec4{mvpArr[4],mvpArr[5],mvpArr[5],mvpArr[7]};
    _eyeProjections[1][2] = vec4{mvpArr[8],mvpArr[9],mvpArr[10],mvpArr[11]};
    _eyeProjections[1][3] = vec4{mvpArr[12],mvpArr[13],mvpArr[14],mvpArr[15]};

    for_each_eye([&](Eye eye) {
        _eyeInverseProjections[eye] = glm::inverse(_eyeProjections[eye]);
    });

    _cullingProjection = _eyeProjections[0];

}


