//
//  ViveControllerManager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DaydreamControllerManager.h"

#include <PerfStat.h>
#include <PathUtils.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <DeferredLightingEffect.h>
#include <NumericalConstants.h>
#include <ui-plugins/PluginContainer.h>
#include <UserActivityLogger.h>
#include <OffscreenUi.h>


#include <controllers/UserInputMapper.h>

#include <controllers/StandardControls.h>



const QString DaydreamControllerManager::NAME = "Daydream";
static const char* MENU_PATH = "Avatar" ">" "Daydream Controllers";

bool DaydreamControllerManager::isSupported() const {
  return true;
}

bool DaydreamControllerManager::activate() {
    _container->addMenu(MENU_PATH);
    InputPlugin::activate();
    //qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager::activate";

    // register with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();

    GvrState::init(__gvr_context);
    GvrState *gvrState = GvrState::getInstance();

    if (gvrState->_gvr_api) {
        //qDebug() << "[DAYDREAM-CONTROLLER] Initializing daydream controller"; 
        gvrState->_controller_api.reset(new gvr::ControllerApi);
        gvrState->_controller_api->Init(gvr::ControllerApi::DefaultOptions(), gvrState->_gvr_context);
        gvrState->_controller_api->Resume();
    }

    // TODO: retrieve state from daydream API
    unsigned int controllerConnected = true;


    _controller = std::make_shared<DaydreamControllerDevice>(*this);
    userInputMapper->registerDevice(_controller);
    _registeredWithInputMapper = true;
    return true;
}

void DaydreamControllerManager::deactivate() {
    //qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager::deactivate";

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    if (_controller) {
        userInputMapper->removeDevice(_controller->getDeviceID());
        _registeredWithInputMapper = false;
    }

}

void DaydreamControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    //qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager::pluginUpdate";

    // TODO: check state and deactivate if needed

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();

    // because update mutates the internal state we need to lock
    userInputMapper->withLock([&, this]() {
        _controller->update(deltaTime, inputCalibrationData);
    });

    /*if (!_registeredWithInputMapper && _inputDevice->_trackedControllers > 0) {
        userInputMapper->registerDevice(_inputDevice);
        _registeredWithInputMapper = true;
    }*/

}

void DaydreamControllerManager::pluginFocusOutEvent() {
  qDebug() << "pluginFocusOutEvent pluginFocusOutEvent pluginFocusOutEvent";
    if (_controller) {
        _controller->focusOutEvent();
    }
}


// An enum for buttons which do not exist in the StandardControls enum
enum DaydreamButtonChannel {
    APP_BUTTON = controller::StandardButtonChannel::NUM_STANDARD_BUTTONS,
    CLICK_BUTTON
};

QString DaydreamControllerManager::DaydreamControllerDevice::nvlButn(int w) {
    if (_buttonPressedMap.find(w) != _buttonPressedMap.end()) {
        return "YES";
    } else {
        return "null";
    }
}

QString DaydreamControllerManager::DaydreamControllerDevice::nvlAxis(int w) {
    if (_axisStateMap.find(w) != _axisStateMap.end()) {
        return QString::number(_axisStateMap[w]);
    } else {
        return "null";
    }
}

void DaydreamControllerManager::DaydreamControllerDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _poseStateMap.clear();
    _buttonPressedMap.clear();

    PerformanceTimer perfTimer("DaydreamControllerManager::update");

    GvrState *gvrState = GvrState::getInstance();

    // Read current controller state. This must be done once per frame
    gvrState->_controller_state.Update(*gvrState->_controller_api);

    int32_t currentApiStatus = gvrState->_controller_state.GetApiStatus();
    int32_t currentConnectionState = gvrState->_controller_state.GetConnectionState();

    // Print new API status and connection state, if they changed.
    if (currentApiStatus != gvrState->_last_controller_api_status ||
          currentConnectionState != gvrState->_last_controller_connection_state) {
            //qDebug() << "[DAYDREAM-CONTROLLER]: Controller API status: " <<
            //gvr_controller_api_status_to_string(currentApiStatus) << ", connection state: " <<
            //gvr_controller_connection_state_to_string(currentConnectionState);

            gvrState->_last_controller_api_status = currentApiStatus;
            gvrState->_last_controller_connection_state = currentConnectionState;
    }

    handleController(gvrState, deltaTime, inputCalibrationData);

    // handle haptics
    /*
    {
        Locker locker(_lock);
        if (_leftHapticDuration > 0.0f) {
            hapticsHelper(deltaTime, true);
        }
        if (_rightHapticDuration > 0.0f) {
            hapticsHelper(deltaTime, false);
        }
    }
    */
    /*using namespace controller;
    qDebug() << "[DAYDREAM-CONTROLLER2] _buttonPressedMap and _axisStateMap FINAL STATUS: ";
    qDebug() << "[DAYDREAM-CONTROLLER2] RX:" << nvlAxis(RX) << " RY:" << nvlAxis(RY)  << " RS:" << nvlButn(RS)
                                            << " RS_TOUCH:" << nvlButn(RS_TOUCH) << " RT_CLICK:" << nvlButn(RT_CLICK)
                                            << " RS_CENTER:" << nvlButn(RS_CENTER)
                                            << " RS_X:" << nvlButn(RS_X) << " RS_Y:" << nvlButn(RS_Y);
    qDebug() << "[DAYDREAM-CONTROLLER2] _buttonPressedMap and _axisStateMap CLEANING...: ";
    _buttonPressedMap.erase(RS);
    _buttonPressedMap.erase(RS_TOUCH);
    qDebug() << "[DAYDREAM-CONTROLLER2] RX:" << nvlAxis(RX) << " RY:" << nvlAxis(RY)  << " RS:" << nvlButn(RS)
                                            << " RS_TOUCH:" << nvlButn(RS_TOUCH) << " RT_CLICK:" << nvlButn(RT_CLICK)
                                            << " RS_CENTER:" << nvlButn(RS_CENTER)
                                            << " RS_X:" << nvlButn(RS_X) << " RS_Y:" << nvlButn(RS_Y);*/
}

void DaydreamControllerManager::DaydreamControllerDevice::handleController(GvrState *gvrState, float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

      gvr::ControllerQuat orientation = gvrState->_controller_state.GetOrientation();
      //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat orientation: " << orientation.qx << "," << orientation.qy << "," << orientation.qz << "," << orientation.qw;
      handlePoseEvent(deltaTime, inputCalibrationData, orientation);

      bool trackpadClicked = false;

      if (gvrState->_last_controller_api_status == gvr_controller_api_status::GVR_CONTROLLER_API_OK && 
          gvrState->_last_controller_connection_state == gvr_controller_connection_state::GVR_CONTROLLER_CONNECTED) {
        for (int k = gvr_controller_button::GVR_CONTROLLER_BUTTON_NONE; k < gvr_controller_button::GVR_CONTROLLER_BUTTON_COUNT ;k++) {
          bool pressed = gvrState->_controller_state.GetButtonDown(static_cast<gvr::ControllerButton>(k)); // Returns whether the given button was just pressed (transient).
          bool pressing = gvrState->_controller_state.GetButtonState(static_cast<gvr::ControllerButton>(k)); // Returns whether the given button is currently pressed.
          bool touched = gvrState->_controller_state.GetButtonUp(static_cast<gvr::ControllerButton>(k)); // Returns whether the given button was just released (transient).
          //if ((pressed || touched || pressing) || rand() % 100 > 98)
              //qDebug() << "[DAYDREAM-CONTROLLER]: call handleButtonEvent(deltaTime: " << deltaTime << ", k: " << k <<
              //        ", pressed: " << pressed << ", touched: " << touched << ",  pressing: " <<  pressing;

          trackpadClicked = trackpadClicked || ( gvr_controller_button::GVR_CONTROLLER_BUTTON_CLICK && (pressed || touched || pressing) );
          handleButtonEvent(deltaTime, k, pressed, touched, pressing);

          if (pressed) {
            qDebug() << "[DAYDREAM-CONTROLLER]: " << k << " button has just been pressed";
          }
          if (pressing) {
            if (rand() % 100 > 98) {
                qDebug() << "[DAYDREAM-CONTROLLER]: " << k << " button is being pressed";
            }
          }

          if (touched) {
            qDebug() << "[DAYDREAM-CONTROLLER]: " << k << " button has just been released";
          }
        }
      }

    if (trackpadClicked) {
        bool isTouching = gvrState->_controller_state.IsTouching();
        gvr_vec2f touchPos = gvrState->_controller_state.GetTouchPos();
        handleAxisEvent(deltaTime, isTouching, touchPos);
        partitionTouchpad(controller::RS, controller::RX, controller::RY, controller::RT_CLICK, controller::RS_X, controller::RS_Y);
    }

}

void DaydreamControllerManager::DaydreamControllerDevice::handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, gvr::ControllerQuat gvrOrientation) {
    glm::quat orientation = toGlm(gvrOrientation);
    //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat GLM orientation: " << orientation.x << "," << orientation.y << "," << orientation.z << "," << orientation.w;
    auto pose = daydreamControllerPoseToHandPose(false, orientation);
    vec3 tranz = pose.getTranslation();
    quat rotz = pose.getRotation();
    //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat handPose tranz : " << tranz.x << "," << tranz.y << "," << tranz.z;
    //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat handPose rotz : " << rotz.x << "," << rotz.y << "," << rotz.z << "," << rotz.w;
    //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat pose : " << orientation.x << "," << orientation.y "," << orientation.z "," << orientation.w;
    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    auto pose2 = pose.transform(controllerToAvatar);
    vec3 tranz2 = pose2.getTranslation();
    quat rotz2 = pose2.getRotation();
    //_poseStateMap[controller::RIGHT_HAND] = pose.transform(controllerToAvatar);
    _poseStateMap[controller::RIGHT_HAND] = pose2;
    //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat handPose tranz2: " << tranz2.x << "," << tranz2.y << "," << tranz2.z;
    //qDebug() << "[DAYDREAM-CONTROLLER]: gvr::ControllerQuat handPose rotz2: " << rotz.x << "," << rotz2.y << "," << rotz2.z << "," << rotz2.w;

    pose = daydreamControllerPoseToHandPose(true, orientation);    
    _poseStateMap[controller::LEFT_HAND] = pose.transform(controllerToAvatar);
}

// These functions do translation from the Steam IDs to the standard controller IDs
void DaydreamControllerManager::DaydreamControllerDevice::handleButtonEvent(float deltaTime, uint32_t button, bool pressed, bool touched, bool pressing) {

    using namespace controller;
    // gvr_controller_button::GVR_CONTROLLER_BUTTON_CLICK = 1,  ///< Touchpad Click.
    // gvr_controller_button::GVR_CONTROLLER_BUTTON_HOME = 2,
    // gvr_controller_button::GVR_CONTROLLER_BUTTON_APP = 3,
    // gvr_controller_button::GVR_CONTROLLER_BUTTON_VOLUME_UP = 4,
    // gvr_controller_button::GVR_CONTROLLER_BUTTON_VOLUME_DOWN = 5,

    if (pressed) {
        if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_CLICK) {
            //qDebug() << "[DAYDREAM-CONTROLLER]: RT_CLICK inserted";
            //_buttonPressedMap.insert(RT_CLICK);
            //_buttonPressedMap.insert(RT);
            _buttonPressedMap.insert(RS);
            _buttonPressedMap.insert(RS_TOUCH);
        } else if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_APP) {
            _buttonPressedMap.insert(RIGHT_PRIMARY_THUMB);
          //_buttonPressedMap.insert(LS);
        } else if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_HOME) {
            // TODO: we must not use this home button, check the desired mapping
            //_axisStateMap[LEFT_GRIP] = 1.0f;
        }
    } else {
        if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_HOME) {
            //_axisStateMap[LEFT_GRIP] = 0.0f;
        }
    }

    if (pressing) {
        if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_CLICK) {
            //qDebug() << "[DAYDREAM-CONTROLLER]: RT_CLICK inserted (continues)";
            //_buttonPressedMap.insert(RT_CLICK);
            //_buttonPressedMap.insert(RT);
            _buttonPressedMap.insert(RS);
            _buttonPressedMap.insert(RS_TOUCH);
        } else if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_APP) {
            _buttonPressedMap.insert(RIGHT_PRIMARY_THUMB);
          //_buttonPressedMap.insert(LS);
        }
    }

    if (touched) {
          // TODO: this is also duplicated. Perhaps we discard some feature later
         if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_CLICK) {
          // TODO: this is also duplicated. Perhaps we discard some feature later
            //_buttonPressedMap.insert(LS_TOUCH);
            //qDebug() << "[DAYDREAM-CONTROLLER]: RT_CLICK inserted";
            //_buttonPressedMap.insert(RT_CLICK);
        } else if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_APP) {
            _buttonPressedMap.insert(RIGHT_PRIMARY_THUMB);
          //_buttonPressedMap.insert(LS);
        }
    }
}

// These functions do translation from the Steam IDs to the standard controller IDs
void DaydreamControllerManager::DaydreamControllerDevice::handleAxisEvent(float deltaTime, bool isTouching, gvr_vec2f touchPos) {
    using namespace controller;
    if (isTouching) {
        glm::vec2 stick(2*(touchPos.x-0.5f), 2*(touchPos.y-0.5f));
        stick = _filteredRightStick.process(deltaTime, stick);
        //qDebug() << "[DAYDREAM-CONTROLLER]: Touching x:" << stick.x << " y:" << stick.y;
        _axisStateMap[RX] = stick.x;// * 10000.0f;
        _axisStateMap[RY] = stick.y;// * 10000.0f;
        qDebug() << "[DAYDREAM-CONTROLLER2] stick.x " << stick.x << " stick.y " << stick.y;

    } else {
      _axisStateMap.clear();
    }
}

void DaydreamControllerManager::DaydreamControllerDevice::partitionTouchpad(int sButton, int xAxis, int yAxis, int centerPseudoButton, int xPseudoButton, int yPseudoButton) {
    // Populate the L/RS_CENTER/OUTER pseudo buttons, corresponding to a partition of the L/RS space based on the X/Y values.
    const float CENTER_DEADBAND = 0.6f;
    const float DIAGONAL_DIVIDE_IN_RADIANS = PI / 4.0f;
    if (_buttonPressedMap.find(sButton) != _buttonPressedMap.end()) {
        float absX = fabs(_axisStateMap[controller::RX]);
        float absY = fabs(_axisStateMap[controller::RY]);
        glm::vec2 cartesianQuadrantI(absX, absY);
        float angle = glm::atan(cartesianQuadrantI.y / cartesianQuadrantI.x);
        float radius = glm::length(cartesianQuadrantI);
        bool isCenter = radius < CENTER_DEADBAND;
        qDebug() << "[DAYDREAM-CONTROLLER2] absX " << fabs(_axisStateMap[controller::RX]) << ", absY " << fabs(_axisStateMap[controller::RY]) <<  " radius " << radius << " angle " << angle << " button found " << (isCenter ? "CENTER" : ((angle < DIAGONAL_DIVIDE_IN_RADIANS) ? " X " : " Y "));
        //partitionTouchpad(controller::RS, controller::RX, controller::RY, controller::RT_CLICK, controller::RS_X, controller::RS_Y);
        qDebug() << "[DAYDREAM-CONTROLLER2] RS:" << controller::RS << " RX:" << controller::RX << " RY:" << controller::RY << " RT_CLICK:" << controller::RT_CLICK << " RS_X:" << controller::RS_X << " RS_Y:" << controller::RS_Y;
        int toInsert = isCenter ? centerPseudoButton : ((angle < DIAGONAL_DIVIDE_IN_RADIANS) ? xPseudoButton :yPseudoButton);
        //_buttonPressedMap.insert(isCenter ? centerPseudoButton : ((angle < DIAGONAL_DIVIDE_IN_RADIANS) ? xPseudoButton :yPseudoButton));
        qDebug() << "[DAYDREAM-CONTROLLER2]insert: " << toInsert;
        _buttonPressedMap.insert(toInsert);
        if (isCenter) {
            // extra RT
            _buttonPressedMap.insert(controller::RT);
            _axisStateMap[controller::RT] = 1;
        }
    }
}

void DaydreamControllerManager::DaydreamControllerDevice::focusOutEvent() {
  qDebug() << "DaydreamControllerDevice::focusOutEvent DaydreamControllerDevice::focusOutEvent DaydreamControllerDevice::focusOutEvent ";
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

controller::Input::NamedVector DaydreamControllerManager::DaydreamControllerDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{

        // touch pad press
        makePair(RS, "RS"),

        makePair(RX, "RX"),
        makePair(RY, "RY"),
        
        makePair(RS_TOUCH, "RSTouch"),

        // triggers
        makePair(RT, "RT"),

        // Trigger clicks
        makePair(RT_CLICK, "RTClick"),

        // Differentiate where we are in the touch pad click
        makePair(RS_CENTER, "RSCenter"),
        makePair(RS_X, "RSX"),
        makePair(RS_Y, "RSY"),

        makePair(RIGHT_PRIMARY_THUMB, "RightPrimaryThumb"),

        // 3d location of controller
        makePair(RIGHT_HAND, "RightHand"),

    };

    return availableInputs;
}



QString DaydreamControllerManager::DaydreamControllerDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/daydream.json";
    //qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager daydream.json = " << MAPPING_JSON;
    return MAPPING_JSON;
}



