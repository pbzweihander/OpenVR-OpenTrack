#include "hmd_device_driver.h"

static const char *OPENTRACK_MAIN_SETTINGS_SECTION = "driver_opentrack";

inline vr::HmdQuaternion_t NewHmdQuaternion(double w, double x, double y, double z) {
    vr::HmdQuaternion_t quat;
    quat.w = w;
    quat.x = x;
    quat.y = y;
    quat.z = z;
    return quat;
}

inline vr::HmdQuaternion_t EulerAngleToQuaternion(double Yaw, double Pitch, double Roll) {
    vr::HmdQuaternion_t q;
    double cy = std::cos(Yaw * 0.5);
    double sy = std::sin(Yaw * 0.5);
    double cp = std::cos(Pitch * 0.5);
    double sp = std::sin(Pitch * 0.5);
    double cr = std::cos(Roll * 0.5);
    double sr = std::sin(Roll * 0.5);

    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    return q;
}

double DegToRad(double f) {
	return f * (3.14159265358979323846 / 180);
}

OpenTrackDisplayComponent::OpenTrackDisplayComponent(OpenTrackDriverConfiguration config) {
    config_ = config;
};

bool OpenTrackDisplayComponent::IsDisplayOnDesktop() { 
    return false;
}

bool OpenTrackDisplayComponent::IsDisplayRealDisplay() { 
    return false;
}

void OpenTrackDisplayComponent::GetRecommendedRenderTargetSize(uint32_t *pnWidth, uint32_t *pnHeight) {
    *pnWidth = config_.render_width;
    *pnHeight = config_.render_height;
}

void OpenTrackDisplayComponent::GetEyeOutputViewport(vr::EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) {
    *pnY = 0;

    // Each eye will have half the window
    *pnWidth = config_.window_width / 2;

    // Each eye will have the full height
    *pnHeight = config_.window_height;

    if (eEye == vr::Eye_Left) {
        // Left eye viewport on the left half of the window
        *pnX = 0;
    } else {
        // Right eye viewport on the right half of the window
        *pnX = config_.window_width / 2;
    }
}

void OpenTrackDisplayComponent::GetProjectionRaw(vr::EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom) {
	*pfLeft = -1.0;
	*pfRight = 1.0;
	*pfTop = -1.0;
	*pfBottom = 1.0;
}

vr::DistortionCoordinates_t OpenTrackDisplayComponent::ComputeDistortion(vr::EVREye eEye, float fU, float fV) {
    vr::DistortionCoordinates_t coordinates{};
    coordinates.rfBlue[0] = fU;
    coordinates.rfBlue[1] = fV;
    coordinates.rfGreen[0] = fU;
    coordinates.rfGreen[1] = fV;
    coordinates.rfRed[0] = fU;
    coordinates.rfRed[1] = fV;
    return coordinates;
}

void OpenTrackDisplayComponent::GetWindowBounds(int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight) {
    *pnX = config_.window_x;
    *pnY = config_.window_y;
    *pnWidth = config_.window_width;
    *pnHeight = config_.window_height;
}

OpenTrackDeviceDriver::OpenTrackDeviceDriver() {
    char serial_number[1024];
    vr::VRSettings()->GetString(OPENTRACK_MAIN_SETTINGS_SECTION, "serial_number", serial_number, sizeof(serial_number));
    serial_number_ = serial_number;

    char model_number[1024];
    vr::VRSettings()->GetString(OPENTRACK_MAIN_SETTINGS_SECTION, "model_number", model_number, sizeof(model_number));
    model_number_ = model_number;

    OpenTrackDriverConfiguration config{};

    config.window_x = vr::VRSettings()->GetInt32(OPENTRACK_MAIN_SETTINGS_SECTION, "window_x");
    config.window_y = vr::VRSettings()->GetInt32(OPENTRACK_MAIN_SETTINGS_SECTION, "window_y");

    config.window_width = vr::VRSettings()->GetInt32(OPENTRACK_MAIN_SETTINGS_SECTION, "window_width");
    config.window_height = vr::VRSettings()->GetInt32(OPENTRACK_MAIN_SETTINGS_SECTION, "window_height");

    config.render_width = vr::VRSettings()->GetInt32(OPENTRACK_MAIN_SETTINGS_SECTION, "render_width");
    config.render_height = vr::VRSettings()->GetInt32(OPENTRACK_MAIN_SETTINGS_SECTION, "render_height");

    display_component_ = std::make_unique<OpenTrackDisplayComponent>(config);
}

const std::string &OpenTrackDeviceDriver::GetSerialNumber() {
    return serial_number_;
}

void OpenTrackDeviceDriver::RunFrame() {
    if (device_id_ != vr::k_unTrackedDeviceIndexInvalid) {
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(device_id_, GetPose(), sizeof(vr::DriverPose_t));
    }
}

vr::EVRInitError OpenTrackDeviceDriver::Activate(uint32_t unObjectId) {
    device_id_ = unObjectId;

    vr::PropertyContainerHandle_t container = vr::VRProperties()->TrackedDeviceToPropertyContainer(device_id_);

    vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, model_number_.c_str());

    const float ipd = vr::VRSettings()->GetFloat(vr::k_pch_SteamVR_Section, vr::k_pch_SteamVR_IPD_Float);
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserIpdMeters_Float, ipd);

    vr::VRProperties()->SetFloatProperty(container, vr::Prop_UserHeadToEyeDepthMeters_Float, 0.f);
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.11f);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_IsOnDesktop_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_DisplayDebugMode_Bool, true); // TODO: Remove

    if (socket_.open() < 0) {
        vr::VRDriverLog()->Log("failed to open UDP socket");
        return vr::VRInitError_Driver_Failed;
    }
    if (socket_.bind(4242) < 0) {
        vr::VRDriverLog()->Log("failed to bind UDP socket");
        return vr::VRInitError_Driver_Failed;
    }

    socket_thread_ = std::thread(&OpenTrackDeviceDriver::SocketReadLoop, this);

    return vr::VRInitError_None;
}

void OpenTrackDeviceDriver::Deactivate() {
    socket_.close();
    socket_thread_.join();

    device_id_ = vr::k_unTrackedDeviceIndexInvalid;
}

void OpenTrackDeviceDriver::EnterStandby() {
    // Do nothing.
}

void* OpenTrackDeviceDriver::GetComponent(const char* pchComponentNameAndVersion) {
    if (std::strcmp(pchComponentNameAndVersion, vr::IVRDisplayComponent_Version) == 0) {
        return display_component_.get();
    }

    return nullptr;
}

void OpenTrackDeviceDriver::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    if(unResponseBufferSize >= 1)
        pchResponseBuffer[0] = 0;
}

vr::DriverPose_t OpenTrackDeviceDriver::GetPose() {
    vr::DriverPose_t pose = { 0 };

    if (!socket_.is_closed()) {
        pose.poseIsValid = true;
        pose.result = vr::TrackingResult_Running_OK;
        pose.deviceIsConnected = true;
    } else {
        pose.poseIsValid = false;
        pose.result = vr::TrackingResult_Uninitialized;
        pose.deviceIsConnected = false;
    }

    pose.qWorldFromDriverRotation = NewHmdQuaternion(1, 0, 0, 0);
    pose.qDriverFromHeadRotation = NewHmdQuaternion(1, 0, 0, 0);
    pose.shouldApplyHeadModel = true;
    pose.poseTimeOffset = 0;
    pose.willDriftInYaw = false;

    pose.qRotation = EulerAngleToQuaternion(Roll_, -Yaw_, Pitch_);

    pose.vecPosition[0] = pX_ * 0.01;
    pose.vecPosition[1] = pZ_ * 0.01;
    pose.vecPosition[2] = pY_ * 0.01;

    return pose;
}

void OpenTrackDeviceDriver::SocketReadLoop() {
    while (!socket_.is_closed()) {
        UDPsocket::IPv4 addr;
        std::string msg;
        if (socket_.recv(msg, addr) >= 0 && msg.length() >= sizeof(TOpenTrack)) {
            TOpenTrack data;
            std::memcpy(&data, msg.c_str(), sizeof(TOpenTrack));
            Yaw_ = DegToRad(data.Yaw);
            Pitch_ = DegToRad(data.Pitch);
            Roll_ = DegToRad(data.Roll);
            pX_ = data.X;
            pY_ = data.Y;
            pZ_ = data.Z;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}
