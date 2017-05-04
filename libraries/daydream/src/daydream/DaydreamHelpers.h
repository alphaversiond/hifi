//
//  Created by Gabriel Calero & Cristian Duarte Davis on 2016/12/01
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <GLMHelpers.h>
#include "NumericalConstants.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <controllers/Forward.h>
#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_types.h"


std::array<float, 16> MatrixToGLArray(const gvr::Mat4f& matrix);

gvr::Mat4f MatrixMul(const gvr::Mat4f& m1, const gvr::Mat4f& m2);

gvr::Mat4f PerspectiveMatrixFromView(const gvr::Rectf& fov, float near_clip, float far_clip);

gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat);

controller::Pose daydreamControllerPoseToHandPose(bool isLeftHand, glm::quat rotation);

inline glm::quat toGlm(const gvr::ControllerQuat & dq) {
    return glm::make_quat(&dq.qx);
}
