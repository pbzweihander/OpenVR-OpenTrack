#pragma once

#include <memory>

#include "openvr_driver.h"
#include "UDPsocket.h"

#include "hmd_device_driver.h"

class OpenTrackDeviceProvider : public vr::IServerTrackedDeviceProvider {
    public:
        // Inherited via IServerTrackedDeviceProvider
        vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext) override;
        void Cleanup() override;
        const char * const *GetInterfaceVersions() override;
        void RunFrame() override;
        bool ShouldBlockStandbyMode() override;
        void EnterStandby() override;
        void LeaveStandby() override;

    private:
        std::unique_ptr<OpenTrackDeviceDriver> opentrack_device_;
};
