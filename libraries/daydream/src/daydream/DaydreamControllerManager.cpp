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



const QString DaydreamControllerManager::NAME = "OpenVR";

bool DaydreamControllerManager::isSupported() const {
    return true; //openVrSupported();
}

bool DaydreamControllerManager::activate() {
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

    if (controllerConnected) {
        _controller = std::make_shared<DaydreamControllerDevice>(*this);
        userInputMapper->registerDevice(_controller);
    }

    return true;
}

void DaydreamControllerManager::deactivate() {
    qDebug() << "[DAYDREAM-CONTROLLER] DaydreamControllerManager::deactivate";

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    if (_controller) {
        userInputMapper->removeDevice(_controller->getDeviceID());
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
}

// An enum for buttons which do not exist in the StandardControls enum
enum DaydreamButtonChannel {
    APP_BUTTON = controller::StandardButtonChannel::NUM_STANDARD_BUTTONS,
    CLICK_BUTTON
};

controller::Input::NamedVector DaydreamControllerManager::DaydreamControllerDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs {
//        makePair(DaydreamButtonChannel::APP_BUTTON, "APP"),
//        makePair(DaydreamButtonChannel::CLICK_BUTTON, "CLICK"),
    };
    return availableInputs;
}

void DaydreamControllerManager::DaydreamControllerDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _buttonPressedMap.clear();

    GvrState *gvrState = GvrState::getInstance();
    int32_t currentApiStatus = gvrState->_controller_state.GetApiStatus();
    int32_t currentConnectionState = gvrState->_controller_state.GetConnectionState();


    // Read current controller state. This must be done once per frame
    gvrState->_controller_state.Update(*gvrState->_controller_api);

      // Print new API status and connection state, if they changed.
      if (currentApiStatus != gvrState->_last_controller_api_status ||
          currentConnectionState != gvrState->_last_controller_connection_state) {
            qDebug() << "[DAYDREAM-CONTROLLER]: Controller API status: " <<
            gvr_controller_api_status_to_string(gvrState->_controller_state.GetApiStatus()) << ", connection state: " <<
            gvr_controller_connection_state_to_string(gvrState->_controller_state.GetConnectionState());

            gvrState->_last_controller_api_status = currentApiStatus;
            gvrState->_last_controller_connection_state = currentConnectionState;
      }
    // Read current controller state. This must be done once per frame
      /*gvrState->_controller_state.Update(*gvrState->_controller_api);
      if (gvrState->_controller_state.GetRecentered()) {
        qDebug() << "[DAYDREAM-CONTROLLER] Recenter";
       // resetEyeProjections();
      }
      */
      bool isTouching = gvrState->_controller_state.IsTouching();

      if (isTouching) {
          gvr_vec2f touchPos = gvrState->_controller_state.GetTouchPos();
          qDebug() << "[DAYDREAM-CONTROLLER]: Touching x:" << touchPos.x << " y:" << touchPos.y;

      }

        bool appbutton = gvrState->_controller_state.GetButtonUp(gvr::kControllerButtonApp);
      if (appbutton) {
            qDebug() << "[DAYDREAM-CONTROLLER]: App button pressed";
      }

      gvr::ControllerQuat orientation = gvrState->_controller_state.GetOrientation();
      //qDebug() << "[DAYDREAM-CONTROLLER]: Orientation: " << orientation.qx << "," << orientation.qy << "," << orientation.qz << "," << orientation.qw;

    /*const auto& inputState = _parent._inputState;
    for (const auto& pair : BUTTON_MAP) {
        if (inputState.Buttons & pair.first) {
            _buttonPressedMap.insert(pair.second);
        }
    }*/
}

QString DaydreamControllerManager::DaydreamControllerDevice::getDefaultMappingConfig() const {
    //static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/oculus_remote.json";
    return "{}";
}



