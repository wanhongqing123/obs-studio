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

#include "d3d12-subsystem.hpp"

gs_stage_surface::gs_stage_surface(gs_device_t* device, uint32_t width, uint32_t height, gs_color_format colorFormat)
	: gs_obj(device, gs_type::gs_stage_surface),
	width(width),
	height(height),
	format(colorFormat),
	dxgiFormat(ConvertGSTextureFormatView(colorFormat))
{
	HRESULT hr;

	memset(&td, 0, sizeof(td));
	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	td.Width = width;
	td.Height = height;
	td.DepthOrArraySize = 1;
	td.MipLevels = 1;
	td.Format = dxgiFormat;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	td.Flags = D3D12_RESOURCE_FLAG_NONE;

	memset(&heapProp, 0, sizeof(heapProp));
	heapProp.Type = D3D12_HEAP_TYPE_READBACK;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	hr = device->device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &td,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
	if (FAILED(hr))
		throw HRError("Failed to create staging surface", hr);
}

gs_stage_surface::gs_stage_surface(gs_device_t* device, uint32_t width, uint32_t height, bool p010)
	: gs_obj(device, gs_type::gs_stage_surface),
	width(width),
	height(height),
	format(GS_UNKNOWN),
	dxgiFormat(p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12)
{
	HRESULT hr;

	memset(&td, 0, sizeof(td));
	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	td.Width = width;
	td.Height = height;
	td.DepthOrArraySize = 1;
	td.MipLevels = 1;
	td.Format = dxgiFormat;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	td.Flags = D3D12_RESOURCE_FLAG_NONE;

	memset(&heapProp, 0, sizeof(heapProp));
	heapProp.Type = D3D12_HEAP_TYPE_READBACK;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	hr = device->device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &td,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
	if (FAILED(hr))
		throw HRError("Failed to create staging surface", hr);
}
