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



class DaydreamControllerManager : public InputPlugin {
    Q_OBJECT
public:
    // Plugin functions
    bool isSupported() const override;
    const QString& getName() const override { return NAME; }

    bool activate() override;
    void deactivate() override;

    void pluginFocusOutEvent() override { /*_inputDevice->focusOutEvent(); */ }
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    void setRenderControllers(bool renderControllers) { /*_renderControllers = renderControllers; */}
private:
    class DaydreamControllerDevice : public controller::InputDevice {
    public:
        DaydreamControllerDevice(DaydreamControllerManager& parent) : controller::InputDevice("DaydreamController"), _parent(parent) { }
        
        using Pointer = std::shared_ptr<DaydreamControllerDevice>;
        controller::Input::NamedVector getAvailableInputs() const override;
        QString getDefaultMappingConfig() const override;
        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        //void focusOutEvent() override;

        DaydreamControllerManager& _parent;
        friend class DaydreamControllerManager;
    };

    DaydreamControllerDevice::Pointer _controller;
    static const QString NAME;


};
#endif // hifi__DaydreamControllerManager