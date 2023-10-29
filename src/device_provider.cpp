#include "device_provider.h"

vr::EVRInitError
OpenTrackDeviceProvider::Init(vr::IVRDriverContext *pDriverContext) {
  VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);

  opentrack_device_ = std::make_unique<OpenTrackDeviceDriver>();

  if (!vr::VRServerDriverHost()->TrackedDeviceAdded(
          opentrack_device_->GetSerialNumber().c_str(),
          vr::TrackedDeviceClass_HMD, opentrack_device_.get())) {
    vr::VRDriverLog()->Log("failed to create OpenTrack HMD device.");
    return vr::VRInitError_Driver_Unknown;
  }

  return vr::VRInitError_None;
}

void OpenTrackDeviceProvider::Cleanup() {
  opentrack_device_ = nullptr;

  VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

const char *const *OpenTrackDeviceProvider::GetInterfaceVersions() {
  return vr::k_InterfaceVersions;
}

void OpenTrackDeviceProvider::RunFrame() {
  if (opentrack_device_ != nullptr) {
    opentrack_device_->RunFrame();
  }

  vr::VREvent_t vrevent{};
  while (vr::VRServerDriverHost()->PollNextEvent(&vrevent,
                                                 sizeof(vr::VREvent_t))) {
    if (opentrack_device_ != nullptr) {
      // TODO
    }
  }
}

bool OpenTrackDeviceProvider::ShouldBlockStandbyMode() {
  // Deprecated. Do nothing.
  return false;
}

void OpenTrackDeviceProvider::EnterStandby() {
  // Do nothing.
}

void OpenTrackDeviceProvider::LeaveStandby() {
  // Do nothing.
}
