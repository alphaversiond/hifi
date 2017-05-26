//
//  PrioVRPlugin.cpp
//  input-plugins/src/input-plugins
//
//  Created by William Brown and Thijs Wenker on 2016/10/29
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PrioVRPlugin.h"

#include <controllers/UserInputMapper.h>
#include <QLoggingCategory>
#include <PathUtils.h>
#include <DebugDraw.h>
#include <cassert>
#include <NumericalConstants.h>
#include <StreamUtils.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")


const char* PrioVRPlugin::NAME { "PrioVR" };
const char* PrioVRPlugin::PRIOVR_ID_STRING { "YOST PrioVR" };


// indices of joints of the PrioVR standard skeleton.
// this has been carried over from the documentation of the YOST Skele - wb 2016/10/29
// added back a bunch of things that were chopped / removed out of HASTE - wb 2016/1/12
enum PrioVRJointIndex {
    Hips,
    Spine2,
    Spine1,
    Spine,
    Neck,
    Head,
    HeadEnd,
    LeftShoulder,
    LeftUpperArm,
    LeftLowerArm,
    LeftHand,
    LeftHandThumb1,
    LeftHandThumb2,
    LeftHandThumb3,
//    LeftHandThumbEnd,
    LeftHandIndex1,
    LeftHandIndex2,
    LeftHandIndex3,
    LeftHandIndexEnd,
    LeftHandMiddle1,
    LeftHandMiddle2,
    LeftHandMiddle3,
    LeftHandMiddleEnd,
    LeftHandRing1,
    LeftHandRing2,
    LeftHandRing3,
    LeftHandRingEnd,
    LeftHandPinky1,
    LeftHandPinky2,
    LeftHandPinky3,
    LeftHandPinkyEnd,
   // LeftJoystick,
    RightShoulder,
    RightUpperArm,
    RightLowerArm,
    RightHand,
    RightHandThumb1,
    RightHandThumb2,
    RightHandThumb3,
//    RightHandThumbEnd,
    RightHandIndex1,
    RightHandIndex2,
    RightHandIndex3,
    RightHandIndexEnd,
    RightHandMiddle1,
    RightHandMiddle2,
    RightHandMiddle3,
    RightHandMiddleEnd,
    RightHandRing1,
    RightHandRing2,
    RightHandRing3,
    RightHandRingEnd,
    RightHandPinky1,
    RightHandPinky2,
    RightHandPinky3,
    RightHandPinkyEnd,
    // RightJoystick,
    LeftUpperLeg,
    LeftLowerLeg,
    LeftFoot,
//    LeftFootHeel,
//    LeftFootBall,
    RightUpperLeg,
    RightLowerLeg,
    RightFoot,
//    RightFootHeel,
//    RightFootBall,
    Size
};
// added back a bunch of things that were chopped / removed out of HASTE - wb 2016/1/12
static controller::StandardPoseChannel priovrJointIndexToPoseIndexMap[PrioVRJointIndex::Size] = {
    controller::HIPS,    
    controller::SPINE3,
    controller::SPINE2,
    controller::SPINE1,
    controller::SPINE,
    controller::NECK,
    controller::HEAD,
    controller::LEFT_SHOULDER,
    controller::LEFT_ARM,
    controller::LEFT_FORE_ARM,
    controller::LEFT_HAND,
    controller::LEFT_HAND_THUMB2,
    controller::LEFT_HAND_THUMB3,
    controller::LEFT_HAND_THUMB4,
    controller::LEFT_HAND_INDEX1,
    controller::LEFT_HAND_INDEX2,
    controller::LEFT_HAND_INDEX3,
    controller::LEFT_HAND_INDEX4,
    controller::LEFT_HAND_MIDDLE1,
    controller::LEFT_HAND_MIDDLE2,
    controller::LEFT_HAND_MIDDLE3,
    controller::LEFT_HAND_MIDDLE4,
    controller::LEFT_HAND_RING1,
    controller::LEFT_HAND_RING2,
    controller::LEFT_HAND_RING3,
    controller::LEFT_HAND_RING4,
    controller::LEFT_HAND_PINKY1,
    controller::LEFT_HAND_PINKY2,
    controller::LEFT_HAND_PINKY3,
    controller::LEFT_HAND_PINKY4,
    controller::RIGHT_SHOULDER,
    controller::RIGHT_ARM,
    controller::RIGHT_FORE_ARM,
    controller::RIGHT_HAND,
    controller::RIGHT_HAND_THUMB2,
    controller::RIGHT_HAND_THUMB3,
    controller::RIGHT_HAND_THUMB4,
    controller::RIGHT_HAND_INDEX1,
    controller::RIGHT_HAND_INDEX2,
    controller::RIGHT_HAND_INDEX3,
    controller::RIGHT_HAND_INDEX4,
    controller::RIGHT_HAND_MIDDLE1,
    controller::RIGHT_HAND_MIDDLE2,
    controller::RIGHT_HAND_MIDDLE3,
    controller::RIGHT_HAND_MIDDLE4,
    controller::RIGHT_HAND_RING1,
    controller::RIGHT_HAND_RING2,
    controller::RIGHT_HAND_RING3,
    controller::RIGHT_HAND_RING4,
    controller::RIGHT_HAND_PINKY1,
    controller::RIGHT_HAND_PINKY2,
    controller::RIGHT_HAND_PINKY3,
    controller::RIGHT_HAND_PINKY4,
    controller::LEFT_UP_LEG,
    controller::LEFT_LEG,
    controller::LEFT_FOOT,
    controller::RIGHT_UP_LEG,
    controller::RIGHT_LEG,
    controller::RIGHT_FOOT    
};
// added back a bunch of things that were chopped / removed out of HASTE - wb 2016/1/12
static YOST_SKELETON_BONE yostSkeletonToPrioVRJointIndex[PrioVRJointIndex::Size] = {
    YOST_SKELETON_BONE_HIPS,                    /**< This will get the standard bone name for the hips */
    YOST_SKELETON_BONE_LOWER_SPINE_2,           /**< This will get the standard bone name for the second lower spine */
    YOST_SKELETON_BONE_LOWER_SPINE_1,           /**< This will get the standard bone name for the first lower spine */
    YOST_SKELETON_BONE_SPINE,                   /**< This will get the standard bone name for the spine */
    YOST_SKELETON_BONE_NECK,                    /**< This will get the standard bone name for the neck */
    YOST_SKELETON_BONE_HEAD,                    /**< This will get the standard bone name for the head */
    YOST_SKELETON_BONE_HEAD_END,                /**< This will get the standard bone name for the head end */
    YOST_SKELETON_BONE_LEFT_SHOULDER,           /**< This will get the standard bone name for the left shoulder */
    YOST_SKELETON_BONE_LEFT_UPPER_ARM,          /**< This will get the standard bone name for the left upper arm */
    YOST_SKELETON_BONE_LEFT_LOWER_ARM,          /**< This will get the standard bone name for the left lower arm */
    YOST_SKELETON_BONE_LEFT_HAND,               /**< This will get the standard bone name for the left hand */
    YOST_SKELETON_BONE_LEFT_HAND_THUMB_1,       /**< This will get the standard bone name for the first left hand thumb */
    YOST_SKELETON_BONE_LEFT_HAND_THUMB_2,       /**< This will get the standard bone name for the second left hand thumb */
    YOST_SKELETON_BONE_LEFT_HAND_THUMB_3,       /**< This will get the standard bone name for the third left hand thumb */
//    YOST_SKELETON_BONE_LEFT_HAND_THUMB_END,     /**< This will get the standard bone name for the end left hand thumb */
    YOST_SKELETON_BONE_LEFT_HAND_INDEX_1,       /**< This will get the standard bone name for the first left hand index */
    YOST_SKELETON_BONE_LEFT_HAND_INDEX_2,       /**< This will get the standard bone name for the second left hand index */
    YOST_SKELETON_BONE_LEFT_HAND_INDEX_3,       /**< This will get the standard bone name for the third left hand index */
    YOST_SKELETON_BONE_LEFT_HAND_INDEX_END,     /**< This will get the standard bone name for the end left hand index */
    YOST_SKELETON_BONE_LEFT_HAND_MIDDLE_1,      /**< This will get the standard bone name for the first left hand middle */
    YOST_SKELETON_BONE_LEFT_HAND_MIDDLE_2,      /**< This will get the standard bone name for the second left hand middle */
    YOST_SKELETON_BONE_LEFT_HAND_MIDDLE_3,      /**< This will get the standard bone name for the third left hand middle */
    YOST_SKELETON_BONE_LEFT_HAND_MIDDLE_END,    /**< This will get the standard bone name for the end left hand middle */
    YOST_SKELETON_BONE_LEFT_HAND_RING_1,        /**< This will get the standard bone name for the first left hand ring */
    YOST_SKELETON_BONE_LEFT_HAND_RING_2,        /**< This will get the standard bone name for the second left hand ring */
    YOST_SKELETON_BONE_LEFT_HAND_RING_3,        /**< This will get the standard bone name for the third left hand ring */
    YOST_SKELETON_BONE_LEFT_HAND_RING_END,      /**< This will get the standard bone name for the end left hand ring */
    YOST_SKELETON_BONE_LEFT_HAND_PINKY_1,       /**< This will get the standard bone name for the first left hand pinky */
    YOST_SKELETON_BONE_LEFT_HAND_PINKY_2,       /**< This will get the standard bone name for the second left hand pinky */
    YOST_SKELETON_BONE_LEFT_HAND_PINKY_3,       /**< This will get the standard bone name for the third left hand pinky */
    YOST_SKELETON_BONE_LEFT_HAND_PINKY_END,     /**< This will get the standard bone name for the end left hand pinky */
 //   YOST_SKELETON_BONE_LEFT_JOYSTICK,           /**< This will get the standard bone name for the left joystick */
    YOST_SKELETON_BONE_RIGHT_SHOULDER,          /**< This will get the standard bone name for the right shoulder */
    YOST_SKELETON_BONE_RIGHT_UPPER_ARM,         /**< This will get the standard bone name for the right upper arm */
    YOST_SKELETON_BONE_RIGHT_LOWER_ARM,         /**< This will get the standard bone name for the right lower arm */
    YOST_SKELETON_BONE_RIGHT_HAND,              /**< This will get the standard bone name for the right hand */
    YOST_SKELETON_BONE_RIGHT_HAND_THUMB_1,      /**< This will get the standard bone name for the first right hand thumb */
    YOST_SKELETON_BONE_RIGHT_HAND_THUMB_2,      /**< This will get the standard bone name for the second right hand thumb */
    YOST_SKELETON_BONE_RIGHT_HAND_THUMB_3,      /**< This will get the standard bone name for the third right hand thumb */
//    YOST_SKELETON_BONE_RIGHT_HAND_THUMB_END,    /**< This will get the standard bone name for the end right hand thumb */
    YOST_SKELETON_BONE_RIGHT_HAND_INDEX_1,      /**< This will get the standard bone name for the first right hand index */
    YOST_SKELETON_BONE_RIGHT_HAND_INDEX_2,      /**< This will get the standard bone name for the second right hand index */
    YOST_SKELETON_BONE_RIGHT_HAND_INDEX_3,      /**< This will get the standard bone name for the third right hand index */
    YOST_SKELETON_BONE_RIGHT_HAND_INDEX_END,    /**< This will get the standard bone name for the end right hand index */
    YOST_SKELETON_BONE_RIGHT_HAND_MIDDLE_1,     /**< This will get the standard bone name for the first right hand middle */
    YOST_SKELETON_BONE_RIGHT_HAND_MIDDLE_2,     /**< This will get the standard bone name for the second right hand middle */
    YOST_SKELETON_BONE_RIGHT_HAND_MIDDLE_3,     /**< This will get the standard bone name for the third right hand middle */
    YOST_SKELETON_BONE_RIGHT_HAND_MIDDLE_END,   /**< This will get the standard bone name for the end right hand middle */
    YOST_SKELETON_BONE_RIGHT_HAND_RING_1,       /**< This will get the standard bone name for the first right hand ring */
    YOST_SKELETON_BONE_RIGHT_HAND_RING_2,       /**< This will get the standard bone name for the second right hand ring */
    YOST_SKELETON_BONE_RIGHT_HAND_RING_3,       /**< This will get the standard bone name for the third right hand ring */
    YOST_SKELETON_BONE_RIGHT_HAND_RING_END,     /**< This will get the standard bone name for the end right hand ring */
    YOST_SKELETON_BONE_RIGHT_HAND_PINKY_1,      /**< This will get the standard bone name for the first right hand pinky */
    YOST_SKELETON_BONE_RIGHT_HAND_PINKY_2,      /**< This will get the standard bone name for the second right hand pinky */
    YOST_SKELETON_BONE_RIGHT_HAND_PINKY_3,      /**< This will get the standard bone name for the third right hand pinky */
    YOST_SKELETON_BONE_RIGHT_HAND_PINKY_END,    /**< This will get the standard bone name for the end right hand pinky */
 //   YOST_SKELETON_BONE_RIGHT_JOYSTICK,           /**< This will get the standard bone name for the right joystick */
    YOST_SKELETON_BONE_LEFT_UPPER_LEG,          /**< This will get the standard bone name for the left upper leg */
    YOST_SKELETON_BONE_LEFT_LOWER_LEG,          /**< This will get the standard bone name for the left lower leg */
    YOST_SKELETON_BONE_LEFT_FOOT,               /**< This will get the standard bone name for the left foot */
//    YOST_SKELETON_BONE_LEFT_FOOT_HEEL,          /**< This will get the standard bone name for the left foot heel */
//    YOST_SKELETON_BONE_LEFT_FOOT_BALL,          /**< This will get the standard bone name for the left foot ball */
    YOST_SKELETON_BONE_RIGHT_UPPER_LEG,         /**< This will get the standard bone name for the right upper leg */
    YOST_SKELETON_BONE_RIGHT_LOWER_LEG,         /**< This will get the standard bone name for the right lower leg */
    YOST_SKELETON_BONE_RIGHT_FOOT              /**< This will get the standard bone name for the right foot */
//    YOST_SKELETON_BONE_RIGHT_FOOT_HEEL,         /**< This will get the standard bone name for the right foot heel */
//    YOST_SKELETON_BONE_RIGHT_FOOT_BALL,         /**< This will get the standard bone name for the right foot ball */
};
// added back a bunch of things that were chopped / removed out of HASTE - wb 2016/1/12
static const char* controllerJointNames[PrioVRJointIndex::Size] = {
    "Hips",
    "Spine3",
    "Spine2",
    "Spine1",
    "Spine",    
    "Neck",
    "Head",
    "LeftShoulder",
    "LeftArm",
    "LeftForeArm",
    "LeftHand",
    "LeftHandThumb1",
    "LeftHandThumb2",
    "LeftHandThumb3",
//    "LeftHandThumb4",
    "LeftHandIndex1",
    "LeftHandIndex2",
    "LeftHandIndex3",
    "LeftHandIndex4",
    "LeftHandMiddle1",
    "LeftHandMiddle2",
    "LeftHandMiddle3",
    "LeftHandMiddle4",
    "LeftHandRing1",
    "LeftHandRing2",
    "LeftHandRing3",
    "LeftHandRing4",
    "LeftHandPinky1",
    "LeftHandPinky2",
    "LeftHandPinky3",
    "LeftHandPinky4",
    "RightShoulder",
    "RightArm",
    "RightForeArm",
    "RightHand",
    "RightHandThumb1",
    "RightHandThumb2",
    "RightHandThumb3",
//    "RightHandThumb4",
    "RightHandIndex1",
    "RightHandIndex2",
    "RightHandIndex3",
    "RightHandIndex4",
    "RightHandMiddle1",
    "RightHandMiddle2",
    "RightHandMiddle3",
    "RightHandMiddle4",
    "RightHandRing1",
    "RightHandRing2",
    "RightHandRing3",
    "RightHandRing4",
    "RightHandPinky1",
    "RightHandPinky2",
    "RightHandPinky3",
    "RightHandPinky4",
    "LeftUpLeg",
    "LeftLeg",
    "LeftFoot",
    "RightUpLeg",
    "RightLeg",
    "RightFoot"    
};

static controller::StandardPoseChannel priovrJointIndexToPoseIndex(PrioVRJointIndex i) {
    assert(i >= 0 && i < PrioVRJointIndex::Size);
    if (i >= 0 && i < PrioVRJointIndex::Size) {
        return priovrJointIndexToPoseIndexMap[i];
    } else {
        return (controller::StandardPoseChannel)0; // not sure what to do here, but don't crash!
    }
}

// convert between YXZ perception euler angles in degrees to quaternion
// this is the default setting in the Axis perception server.

static quat eulerToQuat(const vec3& e) {
    // euler.x and euler.y are swaped, WTF.
    return (glm::angleAxis(e.x * RADIANS_PER_DEGREE, Vectors::UNIT_Y) *
            glm::angleAxis(e.y * RADIANS_PER_DEGREE, Vectors::UNIT_X) *
            glm::angleAxis(e.z * RADIANS_PER_DEGREE, Vectors::UNIT_Z));
}

//
// PrioVRPlugin
// the following functions are PerceptionNeuron funcitons that we will mod for PrioVR
// isSupported();  - required
// activate();     - API activation - status: OCT 31st - functional in Interface.exe
// deactivate();   - API deactivation
// pluginUpdate(); - this is where we update the mapping
// saveSettings(); - leave this as is
// loadSettings(); - leave this as is


bool PrioVRPlugin::isSupported() const {
    return true;
}

// wb: lots of init going on in here... basically normal setup stuff
// wb: see also YOST streaming_data.c located in */API_SDK/Prio_API/example
bool PrioVRPlugin::activate() {
    InputPlugin::activate();

    // register with userInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);
   
    // Create a skeleton using the user's sex and age
    uint8_t is_male = 1;    // <-- Edit this to your user's sex
    uint32_t age = 34;      // <-- Edit this to your user's age
    //qCDebug(inputplugins) << "====Creating Skeleton for a %s, %d Years Old====\n", is_male ? "Male" : "Female", age;
    _skeletonID = yostskel_createStandardSkeletonWithAge(is_male, age);
    if (_skeletonID == YOST_SKELETON_INVALID_ID) {
        qCDebug(inputplugins) << "Failed to create a skeleton for a " << (is_male ? "Male" : "Female") << ", " << age
            << " years old";
        return false;
    }
    qCDebug(inputplugins) << "Created a skeleton for a " << (is_male ? "Male" : "Female") << ", " << age << " years old";

    // Sets the skeleton to the standard T Pose
    qCDebug(inputplugins) << "====Setting a Pose for Skeleton====";
    YOST_SKELETON_ERROR error = yostskel_setSkeletonToStandardTPose(_skeletonID);
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
        return false;
    }
    qCDebug(inputplugins) << "Skeleton is now in a \"T\" pose";

    // Create a Prio Processor
    qCDebug(inputplugins) << "====Creating a Prio Processor====";

    int offset = 0;
    prio_findPorts(PRIO_FIND_ALL_KNOWN);
    prio_ComPort port;
    port.port_name = new char[64];
    _prioDeviceID = YOST_SKELETON_INVALID_ID;
    while (prio_getPort(port.port_name, offset, &port.device_type) == PRIO_NO_ERROR &&
           _prioDeviceID == YOST_SKELETON_INVALID_ID) {
        _prioDeviceID = yostskel_createPrioProcessorWithComOffset(PRIO_TYPE::PRIO_ALL, offset);
        qCDebug(inputplugins) << "offset" << offset;
        qCDebug(inputplugins) << "prio_id" << _prioDeviceID;
        qCDebug(inputplugins) << "port_name" << port.port_name;
        qCDebug(inputplugins) << "device_type" << port.device_type;
        offset++;
    }    
    if (_prioDeviceID == YOST_SKELETON_INVALID_ID) {
        qCDebug(inputplugins) << "Failed to create Prio Processor";
        return false;
    }
    qCDebug(inputplugins) << "Created a Prio Processor";

    // Add the Prio Proccesor to the internal skeleton
    // Adding a Processor to the skeleton allows the Processor to manipulate the bones of the skeleton
    qCDebug(inputplugins) << "====Adding the Prio Processor to the Skeleton====\n";
    error = yostskel_addProcessorToSkeleton(_skeletonID, YOST_SKELETON_PROCESSOR_LIST_END, _prioDeviceID);
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: %s\n" << yost_skeleton_error_string[error];
        return false;
    }
    qCDebug(inputplugins) << "Added Prio Processor(" << _prioDeviceID << ") to Skeleton(" << _skeletonID << ")";

    // The standard Prio Layout uses the YOST Skeleton standard bone names
    qCDebug(inputplugins) << "====Mapping the Skeleton to Match the Prio Pro Suit====";
    error = yostskel_setStandardDeviceXmlMapPrioProLayout(_skeletonID);
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
        return false;
    }
    qCDebug(inputplugins) << "Skeleton set up for Prio Pro Suit";

    // Finding the root bone
    qCDebug(inputplugins) << "====Setting up skeleton bones====";
    char nameBuffer[256];
    error = yostskel_getRootBoneName(_skeletonID, nameBuffer, 256);
    if (error != YOST_SKELETON_NO_ERROR)
    {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
        return false;
    }
    qCDebug(inputplugins) << "Root Bone: " << nameBuffer;

    // Get the standard bone names and put them in a array
    // Doing this makes it easy to loop through all the bones needed
    //qCDebug(inputplugins) << "Getting standard bone names for Prio Lite suit";
    
    // The following should return YOST_SKELETON_NO_ERROR as long as the buffer size is large enough
    for (uint8_t i = 0; i < PrioVRJointIndex::Size; i++) {
       error = yostskel_getStandardBoneName(yostSkeletonToPrioVRJointIndex[i], _jointNames[i], 32);
    }

    //wb: we can also add the joysticks using similar methods...


    qCDebug(inputplugins) << "====Calibrating the Prio Suit====";
    error = yostskel_calibratePrioProcessor(_prioDeviceID, 0.0f);
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
        return false;
    }
    qCDebug(inputplugins) << "Prio calibration complete";

    // Sets up the standard streaming of data for the Prio suit
    qCDebug(inputplugins) << "====Setting up the Standard Prio Streaming====\n";
    error = yostskel_setupStandardStreamPrioProcessor(_prioDeviceID);
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
        return false;
    }

    qCDebug(inputplugins) << "====Start Streaming====";
    error = yostskel_startPrioProcessor(_prioDeviceID);
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
    }

    qCDebug(inputplugins) << "PrioVRPlugin: INITIALIZED";
    
    return true;
}

// wb: turn it off, clean it up
void PrioVRPlugin::deactivate() {
    // unregister from userInputMapper
    if (_inputDevice->_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_inputDevice->_deviceID);
    }
    
    // Tells the Prio Processor to stop, stopping the stream data
    qCDebug(inputplugins) << "====Stop Streaming====";
    YOST_SKELETON_ERROR error = yostskel_stopPrioProcessor(_prioDeviceID);
    if (error == YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "Streaming has stopped!";
    } else {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
    }
    // Destroy the Skeleton to clean up any allocated memory and clean up any attached processors
    yostskel_destroySkeleton(_skeletonID);

    // Reset API to clean up allocated memory
    qCDebug(inputplugins) << "====Resetting API====";
    error = yostskel_resetSkeletalApi();
    if (error != YOST_SKELETON_NO_ERROR) {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
    }
    InputPlugin::deactivate();
}

void PrioVRPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    // Loop through all bone names and return the orientation
    // Only bones active in the Prio layout will be returned
    const int QUAT_SIZE = 4;
    const int VEC3_SIZE = 3;
    
    float quatBuffer[PrioVRJointIndex::Size][QUAT_SIZE];
    float positionBuffer[PrioVRJointIndex::Size][VEC3_SIZE];
    _joints.resize(PrioVRJointIndex::Size, PrioVRJoint());
    YOST_SKELETON_ERROR error = yostskel_update(_skeletonID);
    if (error == YOST_SKELETON_NO_ERROR) {
        for (uint8_t i = 0; i < PrioVRJointIndex::Size; i++) {
            // qCDebug(inputplugins) << "Bone: " << _jointNames[i];
            //Gets the orientation of a given bone
            //param[out] quat4 A float array denoting a quaternion(X, Y, Z, W).
            error = yostskel_getBoneOrientation(_skeletonID, _jointNames[i], quatBuffer[i]);
            if (error == YOST_SKELETON_NO_ERROR) {
                /* qCDebug(inputplugins) << "Quat: " << quatBuffer[i][0] << "," << quatBuffer[i][1] << "," << quatBuffer[i][2]
                     << "," << quatBuffer[i][3];  */
            }
            else {
                //  qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
            }
            error = yostskel_getBonePosition(_skeletonID, _jointNames[i], positionBuffer[i]);
            if (error == YOST_SKELETON_NO_ERROR) {
                /* qCDebug(inputplugins) << "Pos:" << positionBuffer[i][0] << "," << positionBuffer[i][1] << "," <<
                    positionBuffer[i][2]; */
            }
            else {
                //  qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
            }

            if (error == YOST_SKELETON_NO_ERROR) {
                PrioVRJoint jointdata;
                jointdata.position = glm::vec3(positionBuffer[i][0], positionBuffer[i][1], positionBuffer[i][2]);
                jointdata.rotation = glm::quat(quatBuffer[i][1], quatBuffer[i][2], quatBuffer[i][3], quatBuffer[i][0]);
                _joints[i] = jointdata;
            }
            
        }
    } else {
        qCDebug(inputplugins) << "ERROR: " << yost_skeleton_error_string[error];
    }

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData, _joints, _prevJoints);
    });

    _prevJoints = _joints;
}

void PrioVRPlugin::saveSettings() const {
    InputPlugin::saveSettings();
}

void PrioVRPlugin::loadSettings() {
    InputPlugin::loadSettings();
}

//
// InputDevice
// getAvailableInputs();      - this is where we count inside enum of Poses inside 'controller'
// getDefaultMappingConfig(); - maps the skeleton to the joints to the controller
// update();                  - polls the avatar joints and then puts the correct data in

controller::Input::NamedVector PrioVRPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;
    if (availableInputs.size() == 0) {
        for (int i = 0; i < PrioVRJointIndex::Size; i++) {
            availableInputs.push_back(makePair(static_cast<controller::StandardPoseChannel>(priovrJointIndexToPoseIndex((PrioVRJointIndex)i)), controllerJointNames[i]));
        }
    };
    return availableInputs;
}

QString PrioVRPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/priovr.json";
    return MAPPING_JSON;
}

void PrioVRPlugin::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
                                       const std::vector<PrioVRPlugin::PrioVRJoint>& joints,
                                       const std::vector<PrioVRPlugin::PrioVRJoint>& prevJoints) {

    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    glm::quat controllerToAvatarRotation = glmExtractRotation(controllerToAvatar);

    
    vec3 yostHipPosition;
    if (joints.size() > YOST_SKELETON_BONE_HIPS) {
        yostHipPosition = joints[YOST_SKELETON_BONE_HIPS].position;
    }



    for (size_t i = 0; i < joints.size(); i++) {
        int poseIndex = priovrJointIndexToPoseIndex((PrioVRJointIndex)i);
        glm::vec3 linearVelocity, angularVelocity;

        // Adjust the position to be hip (avatar) relative, and rotated to match the avatar rotation
        const glm::vec3& position = controllerToAvatarRotation * (joints[i].position - yostHipPosition);

        // Note: we want our rotations presenting in the AVATAR frame, so we need to adjust that here.
        // glm::quat rotation = controllerToAvatarRotation * joints[i].rotation;

        //const glm::vec3& position = joints[i].position;
        
        const glm::quat& rotation = joints[i].rotation;

        if (Vectors::ZERO == position) {
            _poseStateMap[poseIndex] = controller::Pose();
            continue;
        }

        /*if (Vectors::ZERO == position && Quaternions::IDENTITY == rotation) {
            _poseStateMap[poseIndex] = controller::Pose();
            continue;
        }*/

        if (i < prevJoints.size()) {
            linearVelocity = (position - (prevJoints[i].position * METERS_PER_CENTIMETER)) / deltaTime;            
            glm::quat d = glm::log(rotation * glm::inverse(prevJoints[i].rotation));            
            angularVelocity = glm::vec3(d.x, d.y, d.z) / (0.5f * deltaTime); // radians/s
        }        

        _poseStateMap[poseIndex] = controller::Pose(position, rotation, linearVelocity, angularVelocity);
        // qDebug(inputplugins) << "Hose this pose: " << i << " " << poseIndex << _poseStateMap[poseIndex].translation << _poseStateMap[poseIndex].rotation;
    }
}
