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

#include <util/windows/win-version.h>

#include <vector>
#include <string>
#include <memory>
#include <queue>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <util/base.h>
#include <graphics/matrix4.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/HRError.hpp>
#include <graphics/shader-parser.h>

#include "d3dx12.h"

#define MAX_UNIFORM_BUFFERS_PER_STAGE 16

enum class gs_type {
	gs_vertex_buffer,
	gs_index_buffer,
	gs_upload_buffer,
	gs_gpu_buffer,
	gs_uniform_buffer,
	gs_download_buffer,
	gs_texture_2d,
	gs_zstencil_buffer,
	gs_stage_surface,
	gs_sampler_state,
	gs_vertex_shader,
	gs_pixel_shader,
	gs_duplicator,
	gs_swap_chain,
	gs_timer,
	gs_timer_range,
	gs_texture_3d,
};

static inline uint32_t GetWinVer()
{
	struct win_version_info ver;
	get_win_ver(&ver);

	return (ver.major << 8) | ver.minor;
}

static inline DXGI_FORMAT ConvertGSTextureFormatResource(gs_color_format format)
{
	switch (format) {
	case GS_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
	case GS_A8:
		return DXGI_FORMAT_A8_UNORM;
	case GS_R8:
		return DXGI_FORMAT_R8_UNORM;
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	case GS_R10G10B10A2:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case GS_RGBA16:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case GS_R16:
		return DXGI_FORMAT_R16_UNORM;
	case GS_RGBA16F:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case GS_RGBA32F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case GS_RG16F:
		return DXGI_FORMAT_R16G16_FLOAT;
	case GS_RG32F:
		return DXGI_FORMAT_R32G32_FLOAT;
	case GS_R16F:
		return DXGI_FORMAT_R16_FLOAT;
	case GS_R32F:
		return DXGI_FORMAT_R32_FLOAT;
	case GS_DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case GS_DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case GS_DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case GS_R8G8:
		return DXGI_FORMAT_R8G8_UNORM;
	case GS_RGBA_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case GS_RG16:
		return DXGI_FORMAT_R16G16_UNORM;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSTextureFormatView(gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline DXGI_FORMAT ConvertGSTextureFormatViewLinear(gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline gs_color_format ConvertDXGITextureFormat(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_A8_UNORM:
		return GS_A8;
	case DXGI_FORMAT_R8_UNORM:
		return GS_R8;
	case DXGI_FORMAT_R8G8_UNORM:
		return GS_R8G8;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return GS_RGBA;
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		return GS_BGRX;
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return GS_BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return GS_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return GS_RGBA16;
	case DXGI_FORMAT_R16_UNORM:
		return GS_R16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return GS_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return GS_RGBA32F;
	case DXGI_FORMAT_R16G16_FLOAT:
		return GS_RG16F;
	case DXGI_FORMAT_R32G32_FLOAT:
		return GS_RG32F;
	case DXGI_FORMAT_R16_FLOAT:
		return GS_R16F;
	case DXGI_FORMAT_R32_FLOAT:
		return GS_R32F;
	case DXGI_FORMAT_BC1_UNORM:
		return GS_DXT1;
	case DXGI_FORMAT_BC2_UNORM:
		return GS_DXT3;
	case DXGI_FORMAT_BC3_UNORM:
		return GS_DXT5;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return GS_RGBA_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return GS_BGRX_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return GS_BGRA_UNORM;
	case DXGI_FORMAT_R16G16_UNORM:
		return GS_RG16;
	}

	return GS_UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSZStencilFormat(gs_zstencil_format format)
{
	switch (format) {
	case GS_ZS_NONE:
		return DXGI_FORMAT_UNKNOWN;
	case GS_Z16:
		return DXGI_FORMAT_D16_UNORM;
	case GS_Z24_S8:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case GS_Z32F:
		return DXGI_FORMAT_D32_FLOAT;
	case GS_Z32F_S8X24:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static inline D3D12_COMPARISON_FUNC ConvertGSDepthTest(gs_depth_test test)
{
	switch (test) {
	case GS_NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case GS_LESS:
		return D3D12_COMPARISON_FUNC_LESS;
	case GS_LEQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case GS_EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case GS_GEQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case GS_GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case GS_NOTEQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case GS_ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}

	return D3D12_COMPARISON_FUNC_NEVER;
}

static inline D3D12_STENCIL_OP ConvertGSStencilOp(gs_stencil_op_type op)
{
	switch (op) {
	case GS_KEEP:
		return D3D12_STENCIL_OP_KEEP;
	case GS_ZERO:
		return D3D12_STENCIL_OP_ZERO;
	case GS_REPLACE:
		return D3D12_STENCIL_OP_REPLACE;
	case GS_INCR:
		return D3D12_STENCIL_OP_INCR;
	case GS_DECR:
		return D3D12_STENCIL_OP_DECR;
	case GS_INVERT:
		return D3D12_STENCIL_OP_INVERT;
	}

	return D3D12_STENCIL_OP_KEEP;
}

static inline D3D12_BLEND ConvertGSBlendType(gs_blend_type type)
{
	switch (type) {
	case GS_BLEND_ZERO:
		return D3D12_BLEND_ZERO;
	case GS_BLEND_ONE:
		return D3D12_BLEND_ONE;
	case GS_BLEND_SRCCOLOR:
		return D3D12_BLEND_SRC_COLOR;
	case GS_BLEND_INVSRCCOLOR:
		return D3D12_BLEND_INV_SRC_COLOR;
	case GS_BLEND_SRCALPHA:
		return D3D12_BLEND_SRC_ALPHA;
	case GS_BLEND_INVSRCALPHA:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case GS_BLEND_DSTCOLOR:
		return D3D12_BLEND_DEST_COLOR;
	case GS_BLEND_INVDSTCOLOR:
		return D3D12_BLEND_INV_DEST_COLOR;
	case GS_BLEND_DSTALPHA:
		return D3D12_BLEND_DEST_ALPHA;
	case GS_BLEND_INVDSTALPHA:
		return D3D12_BLEND_INV_DEST_ALPHA;
	case GS_BLEND_SRCALPHASAT:
		return D3D12_BLEND_SRC_ALPHA_SAT;
	}

	return D3D12_BLEND_ONE;
}

static inline D3D12_BLEND_OP ConvertGSBlendOpType(gs_blend_op_type type)
{
	switch (type) {
	case GS_BLEND_OP_ADD:
		return D3D12_BLEND_OP_ADD;
	case GS_BLEND_OP_SUBTRACT:
		return D3D12_BLEND_OP_SUBTRACT;
	case GS_BLEND_OP_REVERSE_SUBTRACT:
		return D3D12_BLEND_OP_REV_SUBTRACT;
	case GS_BLEND_OP_MIN:
		return D3D12_BLEND_OP_MIN;
	case GS_BLEND_OP_MAX:
		return D3D12_BLEND_OP_MAX;
	}

	return D3D12_BLEND_OP_ADD;
}

static inline D3D12_CULL_MODE ConvertGSCullMode(gs_cull_mode mode)
{
	switch (mode) {
	case GS_BACK:
		return D3D12_CULL_MODE_BACK;
	case GS_FRONT:
		return D3D12_CULL_MODE_FRONT;
	case GS_NEITHER:
		return D3D12_CULL_MODE_NONE;
	}

	return D3D12_CULL_MODE_BACK;
}

static inline D3D12_PRIMITIVE_TOPOLOGY ConvertGSTopology(gs_draw_mode mode)
{
	switch (mode) {
	case GS_POINTS:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case GS_LINES:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case GS_LINESTRIP:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case GS_TRIS:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case GS_TRISTRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}

	return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
}

static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertD3D12Topology(D3D12_PRIMITIVE_TOPOLOGY tology)
{
	switch (tology) {
	case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

static inline D3D12_TEXTURE_ADDRESS_MODE ConvertGSAddressMode(gs_address_mode mode)
{
	switch (mode) {
	case GS_ADDRESS_WRAP:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case GS_ADDRESS_CLAMP:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case GS_ADDRESS_MIRROR:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case GS_ADDRESS_BORDER:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case GS_ADDRESS_MIRRORONCE:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}

	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

static inline D3D12_FILTER ConvertGSFilter(gs_sample_filter filter)
{
	switch (filter) {
	case GS_FILTER_POINT:
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	case GS_FILTER_LINEAR:
		return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	case GS_FILTER_MIN_MAG_POINT_MIP_LINEAR:
		return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	case GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
		return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	case GS_FILTER_MIN_POINT_MAG_MIP_LINEAR:
		return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	case GS_FILTER_MIN_LINEAR_MAG_MIP_POINT:
		return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	case GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	case GS_FILTER_MIN_MAG_LINEAR_MIP_POINT:
		return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	case GS_FILTER_ANISOTROPIC:
		return D3D12_FILTER_ANISOTROPIC;
	}

	return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

static inline void PushBuffer(UINT* refNumBuffers, D3D12_VERTEX_BUFFER_VIEW* views, ID3D12Resource* buffer,
	size_t elementSize, const char* name)
{
	const UINT numBuffers = *refNumBuffers;
	if (buffer) {
		views[numBuffers].BufferLocation = buffer->GetGPUVirtualAddress();
		views[numBuffers].StrideInBytes = elementSize;
		views[numBuffers].SizeInBytes = buffer->GetDesc().Width;
		*refNumBuffers = numBuffers + 1;
	}
	else {
		blog(LOG_ERROR, "This vertex shader requires a %s buffer", name);
	}
}

static constexpr double DoubleTriangleArea(double ax, double ay, double bx, double by, double cx, double cy)
{
	return ax * (by - cy) + bx * (cy - ay) + cx * (ay - by);
}

static inline double to_GiB(size_t bytes)
{
	return static_cast<double>(bytes) / (1 << 30);
}

struct BlendState {
	bool blendEnabled;
	gs_blend_type srcFactorC;
	gs_blend_type destFactorC;
	gs_blend_type srcFactorA;
	gs_blend_type destFactorA;
	gs_blend_op_type op;

	bool redEnabled;
	bool greenEnabled;
	bool blueEnabled;
	bool alphaEnabled;

	inline bool operator==(const BlendState& other) const
	{
		return blendEnabled == other.blendEnabled && srcFactorC == other.srcFactorC &&
			destFactorC == other.destFactorC && srcFactorA == other.srcFactorA && op == other.op &&
			redEnabled == other.redEnabled && greenEnabled == other.greenEnabled &&
			blueEnabled == other.blueEnabled && alphaEnabled == other.alphaEnabled;
	}

	inline BlendState()
		: blendEnabled(true),
		srcFactorC(GS_BLEND_SRCALPHA),
		destFactorC(GS_BLEND_INVSRCALPHA),
		srcFactorA(GS_BLEND_ONE),
		destFactorA(GS_BLEND_INVSRCALPHA),
		op(GS_BLEND_OP_ADD),
		redEnabled(true),
		greenEnabled(true),
		blueEnabled(true),
		alphaEnabled(true)
	{
	}

	inline BlendState(const BlendState& state) { memcpy(this, &state, sizeof(BlendState)); }
};

struct StencilSide {
	gs_depth_test test;
	gs_stencil_op_type fail;
	gs_stencil_op_type zfail;
	gs_stencil_op_type zpass;

	inline bool operator==(const StencilSide& other) const
	{
		return test == other.test && fail == other.fail && zfail == other.zfail && zpass == other.zpass;
	}

	inline bool operator!=(const StencilSide& other) const { return !(*this == other); }

	inline StencilSide() : test(GS_ALWAYS), fail(GS_KEEP), zfail(GS_KEEP), zpass(GS_KEEP) {}
};

struct ZStencilState {
	bool depthEnabled;
	bool depthWriteEnabled;
	gs_depth_test depthFunc;

	bool stencilEnabled;
	bool stencilWriteEnabled;
	StencilSide stencilFront;
	StencilSide stencilBack;

	inline ZStencilState()
		: depthEnabled(true),
		depthWriteEnabled(true),
		depthFunc(GS_LESS),
		stencilEnabled(false),
		stencilWriteEnabled(true)
	{
	}

	inline bool operator==(const ZStencilState& other) const
	{
		return depthEnabled == other.depthEnabled && depthWriteEnabled == other.depthWriteEnabled &&
			depthFunc == other.depthFunc && stencilEnabled == other.stencilEnabled &&
			stencilWriteEnabled == other.stencilWriteEnabled && stencilFront == other.stencilFront &&
			stencilBack == other.stencilBack;
	}

	inline bool operator!=(const ZStencilState& other) const { return !(*this == other); }

	inline ZStencilState(const ZStencilState& state) { memcpy(this, &state, sizeof(ZStencilState)); }
};

struct RasterState {
	gs_cull_mode cullMode;

	inline bool operator==(const RasterState& other) const { return cullMode == other.cullMode; }

	inline bool operator!=(const RasterState& other) const { return !(*this == other); }

	inline RasterState() : cullMode(GS_BACK) {}

	inline RasterState(const RasterState& state) { memcpy(this, &state, sizeof(RasterState)); }
};

struct gs_monitor_color_info {
	bool hdr;
	UINT bits_per_color;
	ULONG sdr_white_nits;

	gs_monitor_color_info(bool hdr, int bits_per_color, ULONG sdr_white_nits)
		: hdr(hdr),
		bits_per_color(bits_per_color),
		sdr_white_nits(sdr_white_nits)
	{
	}
};
