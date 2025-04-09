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

#pragma once

#include "d3d12-obj.hpp"
#include "d3d12-util.hpp"

struct gs_buffer;
struct gs_staging_descriptor;

struct gs_texture : gs_obj {
	gs_texture_type type = GS_TEXTURE_2D;
	uint32_t layerCountOrDepth = 1; // layer count for 2d, depth for 3d
	uint32_t levels = 0;
	int32_t sampleCount = 1;
	gs_color_format format = GS_BGRA;

	bool needUpdate = false;
	inline gs_texture(gs_texture_type type, uint32_t levels, gs_color_format format)
		: type(type),
		levels(levels),
		format(format)
	{
	}

	inline gs_texture(gs_device* device, gs_type obj_type, gs_texture_type type)
		: gs_obj(device, obj_type),
		type(type)
	{
	}

	inline gs_texture(gs_device* device, gs_type obj_type, gs_texture_type type, uint32_t levels,
		gs_color_format format)
		: gs_obj(device, obj_type),
		type(type),
		levels(levels),
		format(format)
	{
	}
};

struct gs_texture_2d : gs_texture {
	gs_buffer* upload_buffer = nullptr;

	gs_staging_descriptor* textureDescriptor = nullptr;
	ComPtr<ID3D12Resource> texture;
	D3D12_RESOURCE_STATES resourceState = (D3D12_RESOURCE_STATES)0;

	gs_staging_descriptor* renderTargetDescriptor[6] = { 0 };
	gs_staging_descriptor* renderTargetLinearDescriptor[6] = { 0 };

	uint32_t width = 0, height = 0;
	uint32_t flags = 0;
	DXGI_FORMAT dxgiFormatResource = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatView = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatViewLinear = DXGI_FORMAT_UNKNOWN;

	bool isRenderTarget = false;
	bool isDynamic = false;
	bool genMipmaps = false;

	gs_texture_2d* pairedTexture = nullptr;
	bool twoPlane = false;
	bool chroma = false;
	bool acquired = false;

	std::vector<std::vector<uint8_t>> data;
	std::vector<D3D12_SUBRESOURCE_DATA> srd;
	D3D12_RESOURCE_DESC td;
	D3D12_HEAP_PROPERTIES heapProp;

	void InitSRD(std::vector<D3D12_SUBRESOURCE_DATA>& srd);
	void InitTexture(const uint8_t* const* data);
	void InitResourceView();
	void InitRenderTargets();
	void BackupTexture(const uint8_t* const* data);
	void GetSharedHandle(IDXGIResource* dxgi_res);
	void UpdateSubresources();

	bool Map(int32_t subresourceIndex, D3D12_MEMCPY_DEST* map);
	void Unmap(int32_t subresourceIndex);

	void Release();

	inline gs_texture_2d() : gs_texture(GS_TEXTURE_2D, 0, GS_UNKNOWN) {}

	gs_texture_2d(gs_device_t* device, uint32_t width, uint32_t height, gs_color_format colorFormat,
		uint32_t levels, const uint8_t* const* data, uint32_t flags, gs_texture_type type,
		bool gdiCompatible, bool twoPlane = false);

	gs_texture_2d(gs_device_t* device, ID3D12Resource* nv12, uint32_t flags);
	gs_texture_2d(gs_device_t* device, uint32_t handle, bool ntHandle = false);
	gs_texture_2d(gs_device_t* device, ID3D12Resource* obj);
};

struct gs_texture_3d : gs_texture {
	ComPtr<ID3D12Resource> texture;
	D3D12_RESOURCE_DESC td;

	uint32_t width = 0, height = 0, depth = 0;
	uint32_t flags = 0;
	DXGI_FORMAT dxgiFormatResource = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatView = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatViewLinear = DXGI_FORMAT_UNKNOWN;
	bool isDynamic = false;
	bool isShared = false;
	bool genMipmaps = false;
	uint32_t sharedHandle = GS_INVALID_HANDLE;

	bool chroma = false;
	bool acquired = false;

	std::vector<std::vector<uint8_t>> data;
	std::vector<D3D12_SUBRESOURCE_DATA> srd;

	void InitSRD(std::vector<D3D12_SUBRESOURCE_DATA>& srd);
	void InitTexture(const uint8_t* const* data);
	void InitResourceView();
	void BackupTexture(const uint8_t* const* data);
	void GetSharedHandle(IDXGIResource* dxgi_res);

	inline void Release() {}

	inline gs_texture_3d() : gs_texture(GS_TEXTURE_3D, 0, GS_UNKNOWN) {}

	gs_texture_3d(gs_device_t* device, uint32_t width, uint32_t height, uint32_t depth, gs_color_format colorFormat,
		uint32_t levels, const uint8_t* const* data, uint32_t flags);

	gs_texture_3d(gs_device_t* device, uint32_t handle);
};
