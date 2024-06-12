#include "mf-camera.hpp"
#include <memory>
#include <mfreadwrite.h>

#define OPT_DEVICE_ID "device_id"

MediaFoundationCameraSource::MediaFoundationCameraSource(obs_source_t *source)
	: source_ptr(source)
{
}

MediaFoundationCameraSource::~MediaFoundationCameraSource() {}

void MediaFoundationCameraSource::Activate() {
	if (id_ == "") {
		return;
	}
	active_ = true;
	hThread_ = CreateThread(0, 0, &MediaFoundationCameraSource::CaptureThread, this, 0, 0);
}

void MediaFoundationCameraSource::Deactivate() {
	active_ = false;
	WaitForSingleObject(hThread_, INFINITE);
}

void MediaFoundationCameraSource::Update(obs_data_t* settings) {
	const char* device_id = obs_data_get_string(settings, OPT_DEVICE_ID);
	id_ = device_id;
}

bool MediaFoundationCameraSource::IsActive() {
	return active_;
}

void MediaFoundationCameraSource::RequestSample() {
	tick_count_++;
}

static void QueryActivationObjectString(DStr& str, IMFActivate* activation, const GUID& pguid)
{
	LPWSTR wstr = nullptr;
	UINT32 wlen = 0;
	HRESULT ret = activation->GetAllocatedString(pguid, &wstr, &wlen);
	if (FAILED(ret)) {
		return;
	}

	dstr_from_wcs(str, wstr);
	CoTaskMemFree(wstr);
}

void MediaFoundationCameraSource::EnumMFCameraDevices(std::vector<MFCameraDeviceInfo>& devices) {
	HRESULT hr;

	IMFAttributes* attrs = nullptr;
	if (FAILED(MFCreateAttributes(&attrs, 1))) {
		 return;
	}

	hr = attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr)) {
		attrs->Release();
		return;
	}

	IMFActivate** activations = nullptr;
	UINT32 total = 0;
	hr = MFEnumDeviceSources(attrs, &activations, &total);
	attrs->Release();
	if (FAILED(hr)) {
		return;  // oh well, no cameras for you.
	}

	for (UINT32 i = 0; i < total; i++) {
		DStr symbol_link;
		QueryActivationObjectString(symbol_link, activations[i], MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK);
		DStr name;
		QueryActivationObjectString(name, activations[i], MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME);
		if (symbol_link->array && name->array) {
			MFCameraDeviceInfo info;
			info.id = symbol_link;
			info.name = name;
			devices.emplace_back(info);
		}
		activations[i]->Release();
	}
}

int32_t MediaFoundationCameraSource::SelectBestStream(IMFPresentationDescriptor* desc) {
	return 0;
}

DWORD WINAPI MediaFoundationCameraSource::CaptureThread(void *data)
{
	IMFAttributes *attrs = nullptr;
	IMFMediaSource *source = nullptr;
	IMFMediaType *mediatype = nullptr;
	IMFMediaStream *stream = nullptr;
	IMFSourceReader *sourcereader = NULL;
	HRESULT hr;

	MediaFoundationCameraSource *obj =
		static_cast<MediaFoundationCameraSource *>(data);

	hr = MFCreateAttributes(&attrs, 1);
	auto attrs_auto_release = [attrs]() {
		if (attrs)
			attrs->Release();
	};
	if (FAILED(hr)) {
		return 0;
	}

	hr = attrs->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
			    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr)) {
		return 0;
	}

	wchar_t *w_id;
	os_utf8_to_wcs_ptr(obj->id_.c_str(), obj->id_.size(), &w_id);
	if (!w_id) {
		return 0;
	}

	hr = attrs->SetString(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, w_id);
	bfree(w_id);
	if (FAILED(hr)) {
		return 0;
	}

	hr = MFCreateDeviceSource(attrs, &source);
	auto source_auto_release = [source]() {
		if (source)
			source->Release();
	};
	if (FAILED(hr)) {
		return 0;
	}

	IMFPresentationDescriptor *presentationdesc = nullptr;
	hr = source->CreatePresentationDescriptor(&presentationdesc);

	auto presentationdesc_auto_release = [presentationdesc]() {
		if (presentationdesc)
			presentationdesc->Release();
	};

	if (FAILED(hr)) {
		return 0;
	}

	DWORD streamcount = 0;
	presentationdesc->GetStreamDescriptorCount(&streamcount);
	PROPVARIANT var;
	PropVariantInit(&var);
	var.vt = VT_EMPTY;
	hr = source->Start(presentationdesc, NULL, &var);
	PropVariantClear(&var);

	if (FAILED(hr)) {
		return 0;
	}

	auto source_reader_auto_release = [sourcereader]() {
		if (sourcereader) {
			sourcereader->Release();
		}
	};
        hr = MFCreateSourceReaderFromMediaSource(source, NULL, &sourcereader);
	if (FAILED(hr)) {
		return 0;
	}

        hr = MFCreateMediaType(&mediatype);
	auto mediatype_auto_release = [mediatype]() {
		if (mediatype)
			mediatype->Release();
	};

	if (FAILED(hr)) {
		return 0;
	}

	hr = mediatype->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (FAILED(hr)) {
		return 0;
	}

	hr = mediatype->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	if (FAILED(hr)) {
		return 0;
	}
	UINT64 frame_size = 0;
	UINT64 width = 640;
	UINT64 height = 480;
	frame_size = (width << 32) | height;
	hr = mediatype->SetUINT64(MF_MT_FRAME_SIZE, frame_size);
	if (FAILED(hr)) {
		return 0;
	}

	hr = sourcereader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL,
		mediatype);
	if (FAILED(hr)) {
		return 0;
	}

	while (obj->active_) {
		if (sourcereader && stream && obj->tick_count_ > 0) {
			obj->tick_count_--;
			DWORD stream_flags = 0;
			IMFSample *sample = nullptr;
			sourcereader->ReadSample(
				(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
				NULL, &stream_flags, NULL, &sample);
			if (!sample) {
				continue;
			}

			// sample->Get
			obs_source_frame2 frame;
			// frame.timestamp = sample->GetSampleTime();
			frame.width = 640;
			frame.height = 360;
			frame.format = VIDEO_FORMAT_RGBA;
			IMFMediaBuffer *media_buffer = nullptr;
			sample->ConvertToContiguousBuffer(&media_buffer);
			if (media_buffer) {
				IMF2DBuffer *buffer2d = nullptr;
				media_buffer->QueryInterface(IID_IMF2DBuffer, (void **)(&buffer2d));
				if (buffer2d) {
					BYTE *pixels = nullptr;
					LONG pitch = 0;
					buffer2d->Lock2D(&pixels, &pitch);
					if (pixels) {
						frame.data[0] = pixels;
						frame.linesize[0] = pitch;
						obs_source_output_video2(
							obj->source_ptr, &frame);
					}
					buffer2d->Unlock2D();
				}
			}
		}
	}
	hr = source->Stop();
	return 0;
}



static const char* GetMediaFoundationInputName(void*)
{
	return obs_module_text("MediaFoundationVideoCaptureDevice");
}

static void* CreateMediaFoundationDeviceInput(obs_data_t* settings,
	obs_source_t* source)
{
	MFStartup(MF_VERSION, 0);
	return new MediaFoundationCameraSource(source);
}

static void DestroyMediaFoundationSource(void* obj)
{
	delete static_cast<MediaFoundationCameraSource*>(obj);
	MFShutdown();
}

static void GetMediaFoundationDefaultsInput(obs_data_t* settings)
{
	
}

static void ActivateMediaFoundationSource(void* obj)
{
	static_cast<MediaFoundationCameraSource*>(obj)->Activate();
}

static void DeactivateMediaFoundationSource(void* obj)
{
	static_cast<MediaFoundationCameraSource*>(obj)->Deactivate();
}

static void UpdateMediaFoundationSource(void* obj, obs_data_t* settings)
{
	static_cast<MediaFoundationCameraSource*>(obj)->Update(settings);
}

static bool ActivateClicked(obs_properties_t*, obs_property_t* p, void* data)
{
	if (static_cast<MediaFoundationCameraSource*>(data)->IsActive()) {
		static_cast<MediaFoundationCameraSource*>(data)->Deactivate();
		obs_property_set_description(p, obs_module_text("Activate"));
	}
	else {
		static_cast<MediaFoundationCameraSource*>(data)->Activate();
		obs_property_set_description(p, obs_module_text("Deactivate"));
	}

	return true;
}

static void VideoTickMediaFoundationSource(void *obj, float seconds) {
	static_cast<MediaFoundationCameraSource *>(obj)->RequestSample();
}

static obs_properties_t* GetMediaFoundationPropertiesInput(void* obj)
{
	obs_properties_t* props = obs_properties_create();
	std::vector<MFCameraDeviceInfo> devices;

	obs_property_t* device_prop = obs_properties_add_list(
		props, OPT_DEVICE_ID, obs_module_text("Device"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	MediaFoundationCameraSource::EnumMFCameraDevices(devices);

	for (size_t i = 0; i < devices.size(); i++) {
		MFCameraDeviceInfo& device = devices[i];
		obs_property_list_add_string(device_prop, device.name.c_str(),
			device.id.c_str());
	}

	const char* activateText = obs_module_text("Activate");
	if (!static_cast<MediaFoundationCameraSource*>(obj)->IsActive()) {
		activateText = obs_module_text("Deactivate");
	}

	obs_properties_add_button(props, "activate", activateText,
				  ActivateClicked);


	return props;
}


void RegisterMediaFoundationInput()
{
	obs_source_info info = {};
	info.id = "media_foundation_camea_capture";
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO  |
		OBS_SOURCE_ASYNC | OBS_SOURCE_DO_NOT_DUPLICATE;
	info.get_name = GetMediaFoundationInputName;
	info.create = CreateMediaFoundationDeviceInput;
	info.video_tick = VideoTickMediaFoundationSource;
	info.destroy = DestroyMediaFoundationSource;
	info.update = UpdateMediaFoundationSource;
	info.activate = ActivateMediaFoundationSource;
	info.deactivate = DeactivateMediaFoundationSource;
	info.get_defaults = GetMediaFoundationDefaultsInput;
	info.get_properties = GetMediaFoundationPropertiesInput;

	info.icon_type = OBS_ICON_TYPE_CAMERA;
	obs_register_source(&info);
}
