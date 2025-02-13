/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <util/base.h>
#include "d3d12-subsystem.hpp"

void gs_texture_3d::InitSRD(std::vector<D3D12_SUBRESOURCE_DATA> &srd)
{
	uint32_t rowSizeBits = width * gs_get_format_bpp(format);
	uint32_t sliceSizeBytes = height * rowSizeBits / 8;
	uint32_t actual_levels = levels;

	if (!actual_levels)
		actual_levels = gs_get_total_levels(width, height, depth);

	uint32_t newRowSize = rowSizeBits / 8;
	uint32_t newSlizeSize = sliceSizeBytes;

	for (uint32_t level = 0; level < actual_levels; ++level) {
		D3D12_SUBRESOURCE_DATA newSRD;
		newSRD.pData = data[level].data();
		newSRD.RowPitch = newRowSize;
		newSRD.SlicePitch = newSlizeSize;
		srd.push_back(newSRD);

		newRowSize /= 2;
		newSlizeSize /= 4;
	}
}

void gs_texture_3d::BackupTexture(const uint8_t *const *data)
{
	this->data.resize(levels);

	uint32_t w = width;
	uint32_t h = height;
	uint32_t d = depth;
	const uint32_t bbp = gs_get_format_bpp(format);

	for (uint32_t i = 0; i < levels; i++) {
		if (!data[i])
			break;

		const uint32_t texSize = bbp * w * h * d / 8;
		this->data[i].resize(texSize);

		std::vector<uint8_t> &subData = this->data[i];
		memcpy(&subData[0], data[i], texSize);

		if (w > 1)
			w /= 2;
		if (h > 1)
			h /= 2;
		if (d > 1)
			d /= 2;
	}
}

void gs_texture_3d::GetSharedHandle(IDXGIResource *dxgi_res)
{
	HANDLE handle;
	HRESULT hr;

	hr = dxgi_res->GetSharedHandle(&handle);
	if (FAILED(hr)) {
		blog(LOG_WARNING,
		     "GetSharedHandle: Failed to "
		     "get shared handle: %08lX",
		     hr);
	} else {
		sharedHandle = (uint32_t)(uintptr_t)handle;
	}
}

void gs_texture_3d::InitTexture(const uint8_t *const *data) {}

void gs_texture_3d::InitResourceView() {}

#define SHARED_FLAGS (GS_SHARED_TEX | GS_SHARED_KM_TEX)

gs_texture_3d::gs_texture_3d(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth,
			     gs_color_format colorFormat, uint32_t levels, const uint8_t *const *data, uint32_t flags_)
	: gs_texture(device, gs_type::gs_texture_3d, GS_TEXTURE_3D, levels, colorFormat),
	  width(width),
	  height(height),
	  depth(depth),
	  flags(flags_),
	  dxgiFormatResource(ConvertGSTextureFormatResource(format)),
	  dxgiFormatView(ConvertGSTextureFormatView(format)),
	  dxgiFormatViewLinear(ConvertGSTextureFormatViewLinear(format)),
	  isDynamic((flags_ & GS_DYNAMIC) != 0),
	  isShared((flags_ & SHARED_FLAGS) != 0),
	  genMipmaps((flags_ & GS_BUILD_MIPMAPS) != 0),
	  sharedHandle(GS_INVALID_HANDLE)
{
	InitTexture(data);
	InitResourceView();
}

gs_texture_3d::gs_texture_3d(gs_device_t *device, uint32_t handle)
	: gs_texture(device, gs_type::gs_texture_3d, GS_TEXTURE_3D),
	  isShared(true),
	  sharedHandle(handle)
{
	HRESULT hr;
	hr = device->device->OpenSharedHandle((HANDLE)(uintptr_t)handle, IID_PPV_ARGS(texture.Assign()));
	if (FAILED(hr))
		throw HRError("Failed to open shared 3D texture", hr);

	td = texture->GetDesc();

	const gs_color_format format = ConvertDXGITextureFormat(td.Format);

	this->type = GS_TEXTURE_3D;
	this->format = format;
	this->levels = 1;
	this->device = device;

	this->width = td.Width;
	this->height = td.Height;
	this->depth = td.DepthOrArraySize;
	this->dxgiFormatResource = ConvertGSTextureFormatResource(format);
	this->dxgiFormatView = ConvertGSTextureFormatView(format);
	this->dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);

	InitResourceView();
}
