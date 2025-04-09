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

#include "d3d12-util.hpp"
#include "d3d12-obj.hpp"

#define GS_GPU_BUFFERUSAGE_VERTEX (1u << 0)                /**< Buffer is a vertex buffer. */
#define GS_GPU_BUFFERUSAGE_INDEX (1u << 1)                 /**< Buffer is an index buffer. */
#define GS_GPU_BUFFERUSAGE_INDIRECT (1u << 2)              /**< Buffer is an indirect buffer. */
#define GS_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ (1u << 3) /**< Buffer supports storage reads in graphics stages. */
#define GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ (1u << 4)  /**< Buffer supports storage reads in the compute stage. */
#define GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE (1u << 5) /**< Buffer supports storage writes in the compute stage. */

struct gs_vertex_shader;
struct gs_pixel_shader;
struct gs_staging_descriptor;

struct VBDataPtr {
	gs_vb_data* data;

	inline VBDataPtr(gs_vb_data* data) : data(data) {}
	inline ~VBDataPtr() { gs_vbdata_destroy(data); }
};

struct DataPtr {
	void* data;

	inline DataPtr(void* data) : data(data) {}
	inline ~DataPtr() { bfree(data); }
};

struct gs_buffer : gs_obj {
	ComPtr<ID3D12Resource> resource;
	gs_staging_descriptor* uavDescriptor = nullptr;
	gs_staging_descriptor* srvDescriptor = nullptr;
	gs_staging_descriptor* cbvDescriptor = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;

	bool transitioned = false;
	uint32_t usageFlags = 0;

	int32_t size = 0;
	gs_buffer(gs_device* device, int32_t size, gs_type type, uint32_t flags);
	inline ~gs_buffer() {}

	void UploadToBuffer(gs_buffer* source, uint32_t source_offset, gs_buffer* dest, uint32_t dest_offset);
	void UploadToBuffer(uint8_t* data, size_t size, gs_buffer* dest, uint32_t dest_offset);

	void CpoyBufferToBuffer(gs_buffer* source, uint32_t source_offset, gs_buffer* dest, uint32_t dest_offset);
	void DownloadFromBuffer(gs_buffer* source, uint32_t source_offset, gs_buffer* dest, uint32_t dest_offset);
};


struct gs_vertex_buffer : gs_obj {
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> normalBuffer;
	ComPtr<ID3D12Resource> colorBuffer;
	ComPtr<ID3D12Resource> tangentBuffer;
	D3D12_VERTEX_BUFFER_VIEW tangentBufferView;

	std::vector<ComPtr<ID3D12Resource>> uvBuffers;

	bool dynamic;
	VBDataPtr vbd;
	size_t numVerts;
	std::vector<size_t> uvSizes;

	void FlushBuffer(ID3D12Resource* buffer, void* array, size_t elementSize);

	UINT MakeBufferList(gs_vertex_shader* shader, D3D12_VERTEX_BUFFER_VIEW* views);

	void InitBuffer(const size_t elementSize, const size_t numVerts, void* array, ID3D12Resource** buffer);

	void BuildBuffers();

	inline void Release()
	{
		vertexBuffer.Release();
		normalBuffer.Release();
		colorBuffer.Release();
		tangentBuffer.Release();
		uvBuffers.clear();
	}

	gs_vertex_buffer(gs_device_t* device, struct gs_vb_data* data, uint32_t flags);
};

struct gs_index_buffer : gs_obj {
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW view;
	bool dynamic;
	gs_index_type type;
	size_t indexSize;
	size_t num;
	DataPtr indices;

	void InitBuffer();

	inline ~gs_index_buffer() {
		indexBuffer.Release();
	}

	inline void Release() { indexBuffer.Release(); }

	gs_index_buffer(gs_device_t* device, enum gs_index_type type, void* indices, size_t num, uint32_t flags);
};


struct gs_zstencil_buffer : gs_obj {
	ComPtr<ID3D12Resource> texture;
	gs_staging_descriptor* textureDescriptor = nullptr;

	uint32_t width, height;
	gs_zstencil_format format;
	DXGI_FORMAT dxgiFormat;

	void InitBuffer();

	void Clear();
	void Release();


	inline gs_zstencil_buffer() {}

	gs_zstencil_buffer(gs_device_t* device, uint32_t width, uint32_t height, gs_zstencil_format format);
};

struct gs_stage_surface : gs_obj {
	ComPtr<ID3D12Resource> texture;

	uint32_t width, height;
	gs_color_format format;
	DXGI_FORMAT dxgiFormat;

	inline void Release() { texture.Release(); }

	gs_stage_surface(gs_device_t* device, uint32_t width, uint32_t height, gs_color_format colorFormat);
	gs_stage_surface(gs_device_t* device, uint32_t width, uint32_t height, bool p010);
};
