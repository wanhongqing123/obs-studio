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
#include "d3d12-texture.hpp"
#include "d3d12-buffer.hpp"

enum gs_color_space get_next_space(gs_device_t* device, HWND hwnd, DXGI_SWAP_EFFECT effect);

enum gs_color_format get_swap_format_from_space(gs_color_space space, gs_color_format sdr_format);

inline enum gs_color_space make_swap_desc(gs_device* device, DXGI_SWAP_CHAIN_DESC& desc,
	const gs_init_data* data, DXGI_SWAP_EFFECT effect, UINT flags);

struct gs_swap_chain : gs_obj {
	HWND hwnd;
	gs_init_data initData;
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	gs_color_space space;

	gs_texture_2d target[GS_MAX_TEXTURES];
	gs_zstencil_buffer zs;
	ComPtr<IDXGISwapChain3> swap;
	int32_t currentBackBufferIndex = 0;

	void InitTarget(uint32_t cx, uint32_t cy);
	void InitZStencilBuffer(uint32_t cx, uint32_t cy);
	void Resize(uint32_t cx, uint32_t cy, gs_color_format format);
	void Init();
	void Release();

	gs_swap_chain(gs_device* device, const gs_init_data* data);
	virtual ~gs_swap_chain();
};
