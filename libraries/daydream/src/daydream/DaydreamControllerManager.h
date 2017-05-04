//
//  ViveControllerManager.h
//  daydream/src/daydream
//
//  Created by Gabriel Calero & Cristian Duarte on 11/17/2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__DaydreamControllerManager
#define hifi__DaydreamControllerManager

#include <QObject>
#include <unordered_set>
#include <controllers/InputDevice.h>
#include <plugins/InputPlugin.h>
#include <controllers/StandardControls.h>
#include <RenderArgs.h>
#include <render/Scene.h>
#include "../DaydreamPlugin.h"
#include "DaydreamHelpers.h"


class DaydreamControllerManager : public InputPlugin {
    Q_OBJECT
public:
    // Plugin functions
    bool isSupported() const override;
    const QString getName() const override { return NAME; }

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override;
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    void setRenderControllers(bool renderControllers) { /*_renderControllers = renderControllers; */}
    bool isHandController() const override { return true; }

private:
    class DaydreamControllerDevice : public controller::InputDevice {
    public:
        DaydreamControllerDevice(DaydreamControllerManager& parent) : controller::InputDevice("Daydream"), _parent(parent) { }
        
        using Pointer = std::shared_ptr<DaydreamControllerDevice>;
        controller::Input::NamedVector getAvailableInputs() const override;
        QString getDefaultMappingConfig() const override;
        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        void handleController(GvrState *gvrState, float deltaTime, const controller::InputCalibrationData& inputCalibrationData);
        void handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, gvr::ControllerQuat orientation);
        void handleButtonEvent(float deltaTime, uint32_t button, bool pressed, bool touched, bool pressing);
        void handleAxisEvent(float deltaTime, bool isTouching, gvr_vec2f touchPos);
        void focusOutEvent() override;
        void partitionTouchpad(int sButton, int xAxis, int yAxis, int centerPsuedoButton, int xPseudoButton, int yPseudoButton);
        
        QString nvlButn(int w);
        QString nvlAxis(int w);

        class FilteredStick {
        public:
            glm::vec2 process(float deltaTime, const glm::vec2& stick) {
                // Use a timer to prevent the stick going to back to zero.
                // This to work around the noisy touch pad that will flash back to zero breifly
                const float ZERO_HYSTERESIS_PERIOD = 0.2f;  // 200 ms
                if (glm::length(stick) == 0.0f) {
                    if (_timer <= 0.0f) {
                        return glm::vec2(0.0f, 0.0f);
                    } else {
                        _timer -= deltaTime;
                        return _stick;
                    }
                } else {
                    _timer = ZERO_HYSTERESIS_PERIOD;
                    _stick = stick;
                    return stick;
                }
            }
        protected:
            float _timer { 0.0f };
            glm::vec2 _stick { 0.0f, 0.0f };
        };

        FilteredStick _filteredRightStick;

        DaydreamControllerManager& _parent;
        friend class DaydreamControllerManager;
    };

    DaydreamControllerDevice::Pointer _controller;
    bool _registeredWithInputMapper { false };
    static const QString NAME;


};
#endif // hifi__DaydreamControllerManager