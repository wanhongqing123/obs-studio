#pragma

#include <obs-module.h>
#include <obs.h>
#include <util/dstr.hpp>
#include <util/platform.h>

#include <mfapi.h>
#include <Mfidl.h>

#include <string>
#include <vector>

struct MFCameraDeviceInfo {
	std::string id;
	std::string name;
};

class MediaFoundationCameraSource {
public:
	MediaFoundationCameraSource(obs_source_t* source);
	~MediaFoundationCameraSource();

	void Activate();
	void Deactivate();
	void Update(obs_data_t* setting);
	bool IsActive();
	void RequestSample();

	static DWORD WINAPI CaptureThread(void *data);

	static void EnumMFCameraDevices(std::vector<MFCameraDeviceInfo>& devices);

	int32_t SelectBestStream(IMFPresentationDescriptor* desc);
	std::string id_;
	bool active_ = false;
	HANDLE hThread_ = nullptr;
	int32_t tick_count_ = 0;
	obs_source_t *source_ptr;
};
