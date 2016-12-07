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
    return true; //openVrSupported();
}

bool DaydreamControllerManager::activate() {
    _container->addMenu(MENU_PATH);
    InputPlugin::activate();
    qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager::activate";

    // register with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();

    GvrState::init(__gvr_context);
    GvrState *gvrState = GvrState::getInstance();

    if (gvrState->_gvr_api) {
        qDebug() << "[DAYDREAM-CONTROLLER] Initializing daydream controller"; 
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
    qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager::deactivate";

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

// An enum for buttons which do not exist in the StandardControls enum
enum DaydreamButtonChannel {
    APP_BUTTON = controller::StandardButtonChannel::NUM_STANDARD_BUTTONS,
    CLICK_BUTTON
};

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
            qDebug() << "[DAYDREAM-CONTROLLER]: Controller API status: " <<
            gvr_controller_api_status_to_string(currentApiStatus) << ", connection state: " <<
            gvr_controller_connection_state_to_string(currentConnectionState);

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
}

void DaydreamControllerManager::DaydreamControllerDevice::handleController(GvrState *gvrState, float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

      gvr::ControllerQuat orientation = gvrState->_controller_state.GetOrientation();
      handlePoseEvent(deltaTime, inputCalibrationData, orientation);

      if (gvrState->_last_controller_api_status == gvr_controller_api_status::GVR_CONTROLLER_API_OK && 
          gvrState->_last_controller_connection_state == gvr_controller_connection_state::GVR_CONTROLLER_CONNECTED) {
        for (int k = gvr_controller_button::GVR_CONTROLLER_BUTTON_NONE; k < gvr_controller_button::GVR_CONTROLLER_BUTTON_COUNT ;k++) {
          bool pressed = gvrState->_controller_state.GetButtonDown(static_cast<gvr::ControllerButton>(k)); // Returns whether the given button was just pressed (transient).
          bool pressing = gvrState->_controller_state.GetButtonState(static_cast<gvr::ControllerButton>(k)); // Returns whether the given button is currently pressed.
          bool touched = gvrState->_controller_state.GetButtonUp(static_cast<gvr::ControllerButton>(k)); // Returns whether the given button was just released (transient).
          if ((pressed || touched || pressing) || rand() % 100 > 98)
              qDebug() << "[DAYDREAM-CONTROLLER]: call handleButtonEvent(deltaTime: " << deltaTime << ", k: " << k <<
                      ", pressed: " << pressed << ", touched: " << touched << ",  pressing: " <<  pressing;
          handleButtonEvent(deltaTime, k, pressed, touched, pressing);

          if (pressed) {
            qDebug() << "[DAYDREAM-CONTROLLER]: " << k << " button has just been pressed";
          }
          if (pressing) {
            qDebug() << "[DAYDREAM-CONTROLLER]: " << k << " button is being pressed";
          }

          if (touched) {
            qDebug() << "[DAYDREAM-CONTROLLER]: " << k << " button has just been released";
          }
        }
      }


      bool isTouching = gvrState->_controller_state.IsTouching();

      if (isTouching) {
          gvr_vec2f touchPos = gvrState->_controller_state.GetTouchPos();
          qDebug() << "[DAYDREAM-CONTROLLER]: Touching x:" << touchPos.x << " y:" << touchPos.y;
          handleAxisEvent(deltaTime, touchPos.x, touchPos.y);
      }
    
}

void DaydreamControllerManager::DaydreamControllerDevice::handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, gvr::ControllerQuat gvrOrientation) {
    glm::quat orientation = toGlm(gvrOrientation);
    auto pose = daydreamControllerPoseToHandPose(false, orientation);
    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    
    _poseStateMap[controller::RIGHT_HAND] = pose.transform(controllerToAvatar);
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
          qDebug() << "[DAYDREAM-CONTROLLER]: inserting into _buttonPressedMap LT_CLICK";
          _buttonPressedMap.insert(LT_CLICK);
        } else if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_APP) {
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

/*    if (touched) {
          // TODO: this is also duplicated. Perhaps we discard some feature later
         if (button == gvr_controller_button::GVR_CONTROLLER_BUTTON_CLICK) {
          // TODO: this is also duplicated. Perhaps we discard some feature later
             _buttonPressedMap.insert(LS_TOUCH);
        }
    }
    */
}

// These functions do translation from the Steam IDs to the standard controller IDs
void DaydreamControllerManager::DaydreamControllerDevice::handleAxisEvent(float deltaTime, float x, float y) {
    //FIX ME? It enters here every frame: probably we want to enter only if an event occurs
    //axis += vr::k_EButton_Axis0;
    using namespace controller;

    //if (axis == vr::k_EButton_SteamVR_Touchpad) {
/*        glm::vec2 stick(x, y);
        stick = _filteredLeftStick.process(deltaTime, stick);
        _axisStateMap[LX] = stick.x;
        _axisStateMap[LY] = stick.y;
        */
    /*} else if (axis == vr::k_EButton_SteamVR_Trigger) {
        _axisStateMap[isLeftHand ? LT : RT] = x;
        // The click feeling on the Vive controller trigger represents a value of *precisely* 1.0, 
        // so we can expose that as an additional button
        if (x >= 1.0f) {
            _buttonPressedMap.insert(isLeftHand ? LT_CLICK : RT_CLICK);
        }
    }*/
}

void DaydreamControllerManager::DaydreamControllerDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

controller::Input::NamedVector DaydreamControllerManager::DaydreamControllerDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
        // Trackpad analogs
        makePair(LX, "LX"), // left X
        makePair(LY, "LY"), // left Y
//        makePair(RX, "RX"),
//        makePair(RY, "RY"),

        // capacitive touch on the touch pad
        //makePair(LS_TOUCH, "LSTouch"),
//        makePair(RS_TOUCH, "RSTouch"),

        // touch pad press
        makePair(LS, "LS"),
//        makePair(RS, "RS"),
        // Differentiate where we are in the touch pad click
        //makePair(LS_CENTER, "LSCenter"),
        //makePair(LS_X, "LSX"),
        //makePair(LS_Y, "LSY"),
//        makePair(RS_CENTER, "RSCenter"),
//        makePair(RS_X, "RSX"),
//        makePair(RS_Y, "RSY"),


        // triggers
        //makePair(LT, "LT"), // APP button
//        makePair(RT, "RT"),

        // Trigger clicks
        makePair(LT_CLICK, "LTClick"),
//        makePair(RT_CLICK, "RTClick"),

        // low profile side grip button.
        //makePair(LEFT_GRIP, "LeftGrip"),
//        makePair(RIGHT_GRIP, "RightGrip"),

        // 3d location of controller
        makePair(LEFT_HAND, "LeftHand"),
//        makePair(RIGHT_HAND, "RightHand"),

        // app button above trackpad.
        //Input::NamedPair(Input(_deviceID, LEFT_APP_MENU, ChannelType::BUTTON), "LeftApplicationMenu"),
//        Input::NamedPair(Input(_deviceID, RIGHT_APP_MENU, ChannelType::BUTTON), "RightApplicationMenu"),
    };

    return availableInputs;
}



QString DaydreamControllerManager::DaydreamControllerDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/daydream.json";
    qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager daydream.json = " << MAPPING_JSON;
    return MAPPING_JSON;
}



