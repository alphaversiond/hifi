//
//  PrioVRPlugin.h
//  input-plugins/src/input-plugins
//
//  Created by William Brown and Thijs Wenker on 2016/10/29
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PrioVRPlugin_h
#define hifi_PrioVRPlugin_h

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>
#include <plugins/InputPlugin.h>

#include <prio_api_export.h>
#include <yost_skeleton_api.h>

struct _BvhDataHeaderEx;
void FrameDataReceivedCallback(void* context, void* sender, _BvhDataHeaderEx* header, float* data);

// Handles interaction with the PrioVR SDK
class PrioVRPlugin : public InputPlugin {
    Q_OBJECT
public:
    friend void FrameDataReceivedCallback(void* context, void* sender, _BvhDataHeaderEx* header, float* data);

    bool isHandController() const override { return false; }

    // Plugin functions
    virtual bool isSupported() const override;
    virtual const QString getName() const override { return NAME; }
    const QString getID() const override { return PRIOVR_ID_STRING; }

    virtual bool activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

protected:

    struct PrioVRJoint {
        glm::vec3 position;
        glm::quat rotation;
    };

    class InputDevice : public controller::InputDevice {
    public:
        friend class PrioVRPlugin;

        InputDevice() : controller::InputDevice("PrioVR") {}

        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override {};
        virtual void focusOutEvent() override {};

        void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
            const std::vector<PrioVRPlugin::PrioVRJoint>& joints, const std::vector<PrioVRPlugin::PrioVRJoint>& prevJoints);
    };

    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    static const char* NAME;
    static const char* PRIOVR_ID_STRING;

    prio_device_id _prioDeviceID;
    yost_skeleton_id _skeletonID;


    // wb: OLD [9][32]
    // wb: OLD [17][32]

    // wb: NEW [59][32]
    // wb: Reference - */Skeleton_API/test/main.c line 108 see also list of sensors screenshot.
    char _jointNames[59][32];

    // copy of data directly from the PrioVRDataReader SDK
    std::vector<PrioVRJoint> _joints;

    // one frame old copy of _joints, used to caluclate angular and linear velocity.
    std::vector<PrioVRJoint> _prevJoints;
};

#endif // hifi_PrioVRPlugin_h
