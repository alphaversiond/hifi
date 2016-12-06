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

controller::Pose daydreamControllerPoseToHandPose(bool isLeftHand, const mat4& mat, const vec3& linearVelocity, const vec3& angularVelocity) {
    // When the sensor-to-world rotation is identity the coordinate axes look like this:
    //
    //                       user
    //                      forward
    //                        -z
    //                         |
    //                        y|      user
    //      y                  o----x right
    //       o-----x         user
    //       |                up
    //       |
    //       z
    //
    //     Rift

    // From ABOVE the hand canonical axes looks like this:
    //
    //      | | | |          y        | | | |
    //      | | | |          |        | | | |
    //      |     |          |        |     |
    //      |left | /  x---- +      \ |right|
    //      |     _/          z      \_     |
    //       |   |                     |   |
    //       |   |                     |   |
    //

    // So when the user is in Rift space facing the -zAxis with hands outstretched and palms down
    // the rotation to align the Touch axes with those of the hands is:
    //
    //    touchToHand = halfTurnAboutY * quaterTurnAboutX

    // Due to how the Touch controllers fit into the palm there is an offset that is different for each hand.
    // You can think of this offset as the inverse of the measured rotation when the hands are posed, such that
    // the combination (measurement * offset) is identity at this orientation.
    //
    //    Qoffset = glm::inverse(deltaRotation when hand is posed fingers forward, palm down)
    //
    // An approximate offset for the Touch can be obtained by inspection:
    //
    //    Qoffset = glm::inverse(glm::angleAxis(sign * PI/2.0f, zAxis) * glm::angleAxis(PI/4.0f, xAxis))
    //
    // So the full equation is:
    //
    //    Q = combinedMeasurement * touchToHand
    //
    //    Q = (deltaQ * QOffset) * (yFlip * quarterTurnAboutX)
    //
    //    Q = (deltaQ * inverse(deltaQForAlignedHand)) * (yFlip * quarterTurnAboutX)
    static const glm::quat yFlip = glm::angleAxis(PI, Vectors::UNIT_Y);
    static const glm::quat quarterX = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_X);
    static const glm::quat touchToHand = yFlip * quarterX;

    static const glm::quat leftQuarterZ = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat rightQuarterZ = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat eighthX = glm::angleAxis(PI / 4.0f, Vectors::UNIT_X);

    static const glm::quat leftRotationOffset = glm::inverse(leftQuarterZ * eighthX) * touchToHand;
    static const glm::quat rightRotationOffset = glm::inverse(rightQuarterZ * eighthX) * touchToHand;

    // this needs to match the leftBasePosition in tutorial/viveControllerConfiguration.js:21
    static const float CONTROLLER_LATERAL_OFFSET = 0.0381f;
    static const float CONTROLLER_VERTICAL_OFFSET = 0.0495f;
    static const float CONTROLLER_FORWARD_OFFSET = 0.1371f;
    static const glm::vec3 CONTROLLER_OFFSET(CONTROLLER_LATERAL_OFFSET, CONTROLLER_VERTICAL_OFFSET, CONTROLLER_FORWARD_OFFSET);

    static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
    static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;

    auto translationOffset = (isLeftHand ? leftTranslationOffset : rightTranslationOffset);
    auto rotationOffset = (isLeftHand ? leftRotationOffset : rightRotationOffset);

    glm::vec3 position = extractTranslation(mat);
    glm::quat rotation = glm::normalize(glm::quat_cast(mat));

    position += rotation * translationOffset;
    rotation = rotation * rotationOffset;

    // transform into avatar frame
    auto result = controller::Pose(position, rotation);
    // handle change in velocity due to translationOffset
    result.velocity = linearVelocity + glm::cross(angularVelocity, position - extractTranslation(mat));
    result.angularVelocity = angularVelocity;
    result.valid = true;
    return result;
}

