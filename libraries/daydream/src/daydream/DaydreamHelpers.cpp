//
//  Created by Bradley Austin Davis on 2015/11/01
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "daydreamHelpers.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <controllers/Pose.h>
#include <QtCore/QDebug>

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

gvr::Mat4f ControllerQuatToMatrix(const gvr::ControllerQuat& quat) {
  const float x = quat.qx;
  const float x2 = quat.qx * quat.qx;
  const float y = quat.qy;
  const float y2 = quat.qy * quat.qy;
  const float z = quat.qz;
  const float z2 = quat.qz * quat.qz;
  const float w = quat.qw;
  const float xy = quat.qx * quat.qy;
  const float xz = quat.qx * quat.qz;
  const float xw = quat.qx * quat.qw;
  const float yz = quat.qy * quat.qz;
  const float yw = quat.qy * quat.qw;
  const float zw = quat.qz * quat.qw;

  const float m11 = 1.0f - 2.0f * y2 - 2.0f * z2;
  const float m12 = 2.0f * (xy - zw);
  const float m13 = 2.0f * (xz + yw);
  const float m21 = 2.0f * (xy + zw);
  const float m22 = 1.0f - 2.0f * x2 - 2.0f * z2;
  const float m23 = 2.0f * (yz - xw);
  const float m31 = 2.0f * (xz - yw);
  const float m32 = 2.0f * (yz + xw);
  const float m33 = 1.0f - 2.0f * x2 - 2.0f * y2;

  return {
    m11,  m12,  m13,  0.0f,
    m21,  m22,  m23,  0.0f,
    m31,  m32,  m33,  0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
}

controller::Pose daydreamControllerPoseToHandPose(bool isLeftHand, glm::quat rotation) {
    static const glm::quat LASER_TO_HAND_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_Y); // the angle between (0,0,1) and (0, 1, 0)

    static const glm::quat leftRotationOffset = LASER_TO_HAND_ROTATION;
    static const glm::quat rightRotationOffset = LASER_TO_HAND_ROTATION;  

    static const glm::vec3 leftTranslationOffset = glm::vec3(-0.1, -0.5, -0.1);
    static const glm::vec3 rightTranslationOffset = glm::vec3(0.1, -0.5, -0.1);

    auto translationOffset = (isLeftHand ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (isLeftHand ? leftRotationOffset : rightRotationOffset);

    controller::Pose pose;
    pose.translation = glm::vec3(0.0, 0.0, 0.0);
    pose.translation += rotation * translationOffset;
    pose.rotation = rotation * rotationOffset;
    pose.angularVelocity = glm::vec3(0.0, 0.0, 0.0); // toGlm(handPose.AngularVelocity);
    pose.velocity = glm::vec3(0.0, 0.0, 0.0); // toGlm(handPose.LinearVelocity);
    pose.valid = true;
    return pose;
}


/*

    auto translationOffset = (hand == ovrHand_Left ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (hand == ovrHand_Left ? leftRotationOffset : rightRotationOffset);

    glm::quat rotation = toGlm(handPose.ThePose.Orientation);

    controller::Pose pose;
    pose.translation = toGlm(handPose.ThePose.Position);
    pose.translation += rotation * translationOffset;
    pose.rotation = rotation * rotationOffset;
    pose.angularVelocity = toGlm(handPose.AngularVelocity);
    pose.velocity = toGlm(handPose.LinearVelocity);
    pose.valid = true;

*/

