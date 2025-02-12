/******************************************************************************
    Copyright (C) 2025 by hongqingwan <hongqingwan@obsproject.com>

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

static inline DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat) {
	switch (defaultFormat) {
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_D16_UNORM;

	default:
		return defaultFormat;
	}
}

static inline DXGI_FORMAT GetDepthFormat(DXGI_FORMAT defaultFormat) {
	switch (defaultFormat) {
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

		// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

static inline DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat) {
	switch (defaultFormat) {
		// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

		// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}


void gs_zstencil_buffer::InitBuffer()
{
	HRESULT hr;

	memset(&td, 0, sizeof(td));
	td.Width = width;
	td.Height = height;

	td.Alignment = 0;
	td.DepthOrArraySize = 1;
	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	td.Flags = D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	td.Format = dxgiFormat;
	td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	td.MipLevels = 1;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;

	clearValue.Format = dxgiFormat;

	memset(&headProp, 0, sizeof(headProp));

	headProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	headProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	headProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	headProp.CreationNodeMask = 1;
	headProp.VisibleNodeMask = 1;

	hr = device->device->CreateCommittedResource(&headProp, D3D12_HEAP_FLAG_NONE,
		&td, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&texture));
	if (FAILED(hr))
		throw HRError("Failed to create depth stencil texture", hr);

	CreateDerivedViews(device->device, dxgiFormat);
}

void gs_zstencil_buffer::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format)
{
	ID3D12Resource* resource = texture.Get();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(format);
	if (resource->GetDesc().SampleDesc.Count == 1) {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	} else {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}


	dsvDescHeap[0] = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	hDSV[0] = dsvDescHeap[0]->GetCPUDescriptorHandleForHeapStart();

	dsvDescHeap[1] = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	hDSV[1] = dsvDescHeap[1]->GetCPUDescriptorHandleForHeapStart();

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	device->CreateDepthStencilView(resource, &dsvDesc, hDSV[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	device->CreateDepthStencilView(resource, &dsvDesc, hDSV[1]);

	DXGI_FORMAT stencilReadFormat = GetStencilFormat(format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		dsvDescHeap[2] = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		hDSV[2] = dsvDescHeap[2]->GetCPUDescriptorHandleForHeapStart();

		dsvDescHeap[3] = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		hDSV[3] = dsvDescHeap[3]->GetCPUDescriptorHandleForHeapStart();

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		device->CreateDepthStencilView(resource, &dsvDesc, hDSV[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		device->CreateDepthStencilView(resource, &dsvDesc, hDSV[3]);
	} else {
		hDSV[2] = hDSV[0];
		hDSV[3] = hDSV[1];
	}


	depthSRVHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	hDepthSRV = depthSRVHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = GetDepthFormat(format);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(resource, &srvDesc, hDepthSRV);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		stencilSRVHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		hStencilSRV = stencilSRVHeap->GetCPUDescriptorHandleForHeapStart();

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = 1;

		device->CreateShaderResourceView(resource, &srvDesc, hStencilSRV);
	}
}


gs_zstencil_buffer::gs_zstencil_buffer(gs_device_t* device, uint32_t width, uint32_t height, gs_zstencil_format format)
	: gs_obj(device, gs_type::gs_zstencil_buffer),
	width(width),
	height(height),
	format(format),
	dxgiFormat(ConvertGSZStencilFormat(format))
{
	InitBuffer();
}
