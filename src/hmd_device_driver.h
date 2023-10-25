#pragma once

#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

#include "UDPsocket.h"
#include "openvr_driver.h"

struct TOpenTrack {
    double X;
    double Y;
    double Z;
    double Yaw;
    double Pitch;
    double Roll;
};


struct OpenTrackDriverConfiguration
{
	int32_t window_x;
	int32_t window_y;

	int32_t window_width;
	int32_t window_height;

	int32_t render_width;
	int32_t render_height;
};

class OpenTrackDisplayComponent : public vr::IVRDisplayComponent {
public:
    OpenTrackDisplayComponent(OpenTrackDriverConfiguration config);

    virtual bool IsDisplayOnDesktop() override;
    virtual bool IsDisplayRealDisplay() override;
    virtual void GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight) override;
    virtual void GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override;
    virtual void GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom) override;
    virtual vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eEye, float fU, float fV) override;
    virtual void GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) override;

private:
    OpenTrackDriverConfiguration config_;
};

class OpenTrackDeviceDriver : public vr::ITrackedDeviceServerDriver {
public:
    OpenTrackDeviceDriver();

    const std::string &GetSerialNumber();

    void RunFrame();

    virtual vr::EVRInitError Activate(uint32_t unObjectId) override;
    virtual void Deactivate() override;
    virtual void EnterStandby() override;
    virtual void* GetComponent(const char* pchComponentNameAndVersion) override;
    virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
    virtual vr::DriverPose_t GetPose() override;

private:
    vr::TrackedDeviceIndex_t device_id_;

    std::unique_ptr<OpenTrackDisplayComponent> display_component_;

    std::string serial_number_;
    std::string model_number_;

    UDPsocket socket_;

    std::thread socket_thread_;

    double Yaw_, Pitch_, Roll_;
    double pX_, pY_, pZ_;

    void SocketReadLoop();
};
