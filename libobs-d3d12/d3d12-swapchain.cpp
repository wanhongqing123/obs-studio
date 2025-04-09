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

#include "d3d12-swapchain.hpp"
#include "d3d12-subsystem.hpp"
#include "d3d12-graphics-context.hpp"

enum gs_color_space get_next_space(gs_device_t* device, HWND hwnd, DXGI_SWAP_EFFECT effect)
{
	enum gs_color_space next_space = GS_CS_SRGB;
	if (effect == DXGI_SWAP_EFFECT_FLIP_DISCARD) {
		const HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		if (hMonitor) {
			const gs_monitor_color_info info = device->GetMonitorColorInfo(hMonitor);
			if (info.hdr)
				next_space = GS_CS_709_SCRGB;
			else if (info.bits_per_color > 8)
				next_space = GS_CS_SRGB_16F;
		}
	}

	return next_space;
}

enum gs_color_format get_swap_format_from_space(gs_color_space space, gs_color_format sdr_format)
{
	gs_color_format format = sdr_format;
	switch (space) {
	case GS_CS_SRGB_16F:
	case GS_CS_709_SCRGB:
		format = GS_RGBA16F;
	}

	return format;
}

enum gs_color_space make_swap_desc(gs_device* device, DXGI_SWAP_CHAIN_DESC& desc,
	const gs_init_data* data, DXGI_SWAP_EFFECT effect, UINT flags)
{
	const HWND hwnd = (HWND)data->window.hwnd;
	const enum gs_color_space space = get_next_space(device, hwnd, effect);
	const gs_color_format format = get_swap_format_from_space(space, data->format);

	memset(&desc, 0, sizeof(desc));
	desc.BufferDesc.Width = data->cx;
	desc.BufferDesc.Height = data->cy;
	desc.BufferDesc.Format = ConvertGSTextureFormatView(format);
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = data->num_backbuffers;
	desc.OutputWindow = hwnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = effect;
	desc.Flags = flags;

	return space;
}

void gs_swap_chain::InitTarget(uint32_t cx, uint32_t cy)
{
	HRESULT hr;

	for (int32_t i = 0; i < initData.num_backbuffers; ++i) {
		hr = swap->GetBuffer(i, __uuidof(ID3D12Resource), (void**)target[i].texture.Assign());
		if (FAILED(hr))
			throw HRError("Failed to get swap buffer texture", hr);

		target[i].width = cx;
		target[i].height = cy;
		D3D12_RENDER_TARGET_VIEW_DESC rtv;
		memset(&rtv, 0, sizeof(rtv));
		rtv.Format = target[i].dxgiFormatView;
		rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv.Texture2D.MipSlice = 0;
		device->AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, &target[i].renderTargetDescriptor[0]);
		device->device->CreateRenderTargetView(target[i].texture, &rtv,
			target[i].renderTargetDescriptor[0]->cpuHandle);
		if (target[i].dxgiFormatView == target[i].dxgiFormatViewLinear) {
			target[i].renderTargetLinearDescriptor[0] = target[i].renderTargetDescriptor[0];
		}
		else {
			rtv.Format = target[i].dxgiFormatViewLinear;
			device->AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
				&target[i].renderTargetLinearDescriptor[0]);
			device->device->CreateRenderTargetView(target[i].texture, &rtv,
				target[i].renderTargetLinearDescriptor[0]->cpuHandle);
		}
	}
}

void gs_swap_chain::InitZStencilBuffer(uint32_t cx, uint32_t cy)
{
	zs.width = cx;
	zs.height = cy;

	if (zs.format != GS_ZS_NONE && cx != 0 && cy != 0) {
		zs.InitBuffer();
	}
	else {
		zs.Clear();
	}
}

void gs_swap_chain::Resize(uint32_t cx, uint32_t cy, gs_color_format format)
{
	RECT clientRect;
	HRESULT hr;
	for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i)
		target[i].Release();
	zs.Clear();

	initData.cx = cx;
	initData.cy = cy;

	if (cx == 0 || cy == 0) {
		GetClientRect(hwnd, &clientRect);
		if (cx == 0)
			cx = clientRect.right;
		if (cy == 0)
			cy = clientRect.bottom;
	}

	const DXGI_FORMAT dxgi_format = ConvertGSTextureFormatView(format);
	hr = swap->ResizeBuffers(swapDesc.BufferCount, cx, cy, dxgi_format, swapDesc.Flags);
	if (FAILED(hr))
		throw HRError("Failed to resize swap buffers", hr);

	const DXGI_COLOR_SPACE_TYPE dxgi_space = (format == GS_RGBA16F) ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709
		: DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
	hr = swap->SetColorSpace1(dxgi_space);
	if (FAILED(hr))
		throw HRError("Failed to set color space", hr);

	for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i) {
		target[i].dxgiFormatResource = ConvertGSTextureFormatResource(format);
		target[i].dxgiFormatView = dxgi_format;
		target[i].dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);
	}

	InitTarget(cx, cy);
	InitZStencilBuffer(cx, cy);
	currentBackBufferIndex = swap->GetCurrentBackBufferIndex();
}

void gs_swap_chain::Init()
{
	const gs_color_format format =
		get_swap_format_from_space(get_next_space(device, hwnd, swapDesc.SwapEffect), initData.format);
	for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i) {
		target[i].device = device;
		target[i].isRenderTarget = true;
		target[i].format = initData.format;
		target[i].dxgiFormatResource = ConvertGSTextureFormatResource(format);
		target[i].dxgiFormatView = ConvertGSTextureFormatView(format);
		target[i].dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);
	}

	InitTarget(initData.cx, initData.cy);

	zs.device = device;
	zs.format = initData.zsformat;
	zs.dxgiFormat = ConvertGSZStencilFormat(initData.zsformat);
	InitZStencilBuffer(initData.cx, initData.cy);
	currentBackBufferIndex = swap->GetCurrentBackBufferIndex();
}

void gs_swap_chain::Release()
{

	for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i)
		target[i].Release();
	zs.Release();
	swap.Clear();
}

gs_swap_chain::gs_swap_chain(gs_device* device, const gs_init_data* data)
	: gs_obj(device, gs_type::gs_swap_chain),
	hwnd((HWND)data->window.hwnd),
	initData(*data),
	space(GS_CS_SRGB)
{
	DXGI_SWAP_EFFECT effect = DXGI_SWAP_EFFECT_DISCARD;
	UINT flags = 0;

	ComQIPtr<IDXGIFactory5> factory5 = device->factory;
	if (factory5) {
		initData.num_backbuffers = data->num_backbuffers > 2 ? data->num_backbuffers : 2;

		effect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	}

	space = make_swap_desc(device, swapDesc, &initData, effect, flags);

	ComPtr<IDXGISwapChain> swap1;
	HRESULT hr = device->factory->CreateSwapChain(device->commandQueue->commandQueue, &swapDesc, swap1.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create swap chain", hr);

	swap = ComQIPtr<IDXGISwapChain3>(swap1);
	if (!swap)
		throw HRError("Failed to create swap chain3", hr);

	/* Ignore Alt+Enter */
	device->factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	Init();
}

gs_swap_chain::~gs_swap_chain() {}
