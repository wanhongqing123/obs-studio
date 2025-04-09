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

struct gs_texture_2d;
struct gs_sampler_state;
struct gs_vertex_buffer;
struct gs_index_buffer;
struct gs_vertex_shader;
struct gs_pixel_shader;
struct gs_command_queue;
struct gs_command_context;
struct gs_graphics_pipeline;
struct gs_staging_descriptor_pool;
struct gs_staging_descriptor;
struct gs_gpu_descriptor_heap;

struct mat4float {
	float mat[16];
};

struct gs_device {
	ComPtr<IDXGIFactory6> factory;
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIAdapter> adapter;

	uint32_t adpIdx = 0;
	bool nv12Supported = false;
	bool p010Supported = false;
	bool fastClearSupported = false;

	gs_texture_2d *curRenderTarget = nullptr;
	gs_zstencil_buffer *curZStencilBuffer = nullptr;
	int curRenderSide = 0;
	enum gs_color_space curColorSpace = GS_CS_SRGB;
	bool curFramebufferSrgb = false;
	bool curFramebufferInvalidate = false;

	gs_texture *curTextures[GS_MAX_TEXTURES];
	gs_sampler_state *curSamplers[GS_MAX_TEXTURES];
	gs_vertex_buffer *curVertexBuffer = nullptr;
	gs_index_buffer *curIndexBuffer = nullptr;
	gs_vertex_shader *curVertexShader = nullptr;
	gs_pixel_shader *curPixelShader = nullptr;
	gs_swap_chain *curSwapChain = nullptr;

	ZStencilState curZstencilState;
	RasterState curRasterState;
	BlendState curBlendState;

	gs_staging_descriptor_pool *stagingDescriptorPools[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D12_PRIMITIVE_TOPOLOGY curToplogy;
	gs_graphics_pipeline* curPipeline;

	std::vector<gs_graphics_pipeline*> graphicsPipelines;
	gs_command_queue* commandQueue;
	gs_command_context* currentCommandContext;

	gs_rect viewport;

	std::vector<mat4float> projStack;

	matrix4 curProjMatrix;
	matrix4 curViewMatrix;
	matrix4 curViewProjMatrix;

	std::vector<gs_device_loss> loss_callbacks;
	gs_obj *first_obj = nullptr;
	std::vector<std::pair<HMONITOR, gs_monitor_color_info>> monitor_to_hdr;

	void InitFactory();
	void InitAdapter(uint32_t adapterIdx);
	void InitDevice(uint32_t adapterIdx);

	void AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType, gs_staging_descriptor **cpuDescripotr);
	void WriteGPUDescriptor(gs_gpu_descriptor_heap *gpuHeap, D3D12_CPU_DESCRIPTOR_HANDLE *cpuHandle, int32_t count,
				D3D12_GPU_DESCRIPTOR_HANDLE *gpuBaseDescriptor);

	void ConvertZStencilState(D3D12_DEPTH_STENCIL_DESC &desc, const ZStencilState &zs);
	void ConvertRasterState(D3D12_RASTERIZER_DESC &desc, const RasterState &rs);
	void ConvertBlendState(D3D12_BLEND_DESC &desc, const BlendState &bs);

	void GeneratePipelineState(gs_graphics_pipeline *pipeline);

	void LoadGraphicsPipeline(gs_graphics_pipeline** new_pipeline);
	void LoadVertexBufferData();
	void LoadSamplerDescriptors();
	void LoadTextureDescriptors();

	std::vector<gs_command_context*> contextPool;
	std::queue<gs_command_context*>  availableContexts;

	gs_command_context* AllocateContext();
	void FreeContext(gs_command_context* context);

	void CopyTex(ID3D12Resource *dst, uint32_t dst_x, uint32_t dst_y, gs_texture_t *src, uint32_t src_x,
		     uint32_t src_y, uint32_t src_w, uint32_t src_h);
	void UpdateViewProjMatrix();

	void FlushOutputViews();

	void RebuildDevice();

	bool HasBadNV12Output();
	gs_monitor_color_info GetMonitorColorInfo(HMONITOR hMonitor);

	gs_device(uint32_t adapterIdx);
	~gs_device();
};

extern "C" EXPORT int device_texture_acquire_sync(gs_texture_t *tex, uint64_t key, uint32_t ms);
