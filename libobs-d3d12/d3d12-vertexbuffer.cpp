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
#include <graphics/vec3.h>
#include "d3d12-subsystem.hpp"

static inline void PushBuffer(UINT *refNumBuffers, D3D12_VERTEX_BUFFER_VIEW *views,
			      const D3D12_VERTEX_BUFFER_VIEW &view, const char *name)
{
	const UINT numBuffers = *refNumBuffers;
	if (view.BufferLocation) {
		views[numBuffers].BufferLocation = view.BufferLocation;
		views[numBuffers].SizeInBytes = view.SizeInBytes;
		views[numBuffers].StrideInBytes = view.StrideInBytes;
		*refNumBuffers = numBuffers + 1;
	} else {
		blog(LOG_ERROR, "This vertex shader requires a %s buffer", name);
	}
}

void gs_vertex_buffer::FlushBuffer(ID3D12Resource *buffer, void *array, size_t elementSize)
{
	void *vtx_resource = NULL;
	D3D12_RANGE range;
	memset(&range, 0, sizeof(D3D12_RANGE));
	HRESULT hr;

	buffer->Map(0, &range, &vtx_resource);
	memcpy(vtx_resource, array, elementSize * vbd.data->num);
	buffer->Unmap(0, &range);
}

UINT gs_vertex_buffer::MakeBufferList(gs_vertex_shader *shader, D3D12_VERTEX_BUFFER_VIEW *views)
{
	UINT numBuffers = 0;
	PushBuffer(&numBuffers, views, vertexBufferView, "point");

	if (shader->hasNormals)
		PushBuffer(&numBuffers, views, normalBufferView, "normal");
	if (shader->hasColors)
		PushBuffer(&numBuffers, views, colorBufferView, "color");
	if (shader->hasTangents)
		PushBuffer(&numBuffers, views, tangentBufferView, "tangent");
	if (shader->nTexUnits <= uvBuffers.size()) {
		for (size_t i = 0; i < shader->nTexUnits; i++) {
			views[numBuffers] = uvBufferViews[i];
			++numBuffers;
		}
	} else {
		blog(LOG_ERROR,
		     "This vertex shader requires at least %u "
		     "texture buffers.",
		     (uint32_t)shader->nTexUnits);
	}

	return numBuffers;
}

void gs_vertex_buffer::InitBuffer(const size_t elementSize, const size_t numVerts, void *array, ID3D12Resource **buffer,
				  D3D12_VERTEX_BUFFER_VIEW *view)
{
	D3D12_HEAP_PROPERTIES vbufferHeapProps;
	D3D12_RESOURCE_DESC vbufferDesc;

	memset(&vbufferHeapProps, 0, sizeof(D3D12_HEAP_PROPERTIES));

	vbufferHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	vbufferHeapProps.CreationNodeMask = 1;
	vbufferHeapProps.VisibleNodeMask = 1;

	memset(&vbufferDesc, 0, sizeof(D3D12_RESOURCE_DESC));
	vbufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vbufferDesc.Alignment = 0;
	vbufferDesc.Width = numVerts * elementSize;
	vbufferDesc.Height = 1;
	vbufferDesc.DepthOrArraySize = 1;
	vbufferDesc.MipLevels = 1;
	vbufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	vbufferDesc.SampleDesc.Count = 1;
	vbufferDesc.SampleDesc.Quality = 0;
	vbufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vbufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr;

	hr = device->device->CreateCommittedResource(&vbufferHeapProps, D3D12_HEAP_FLAG_NONE, &vbufferDesc,
						     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(buffer));
	if (FAILED(hr))
		throw HRError("Failed to create buffer", hr);

	if (array) {
		UINT8* pVertexDataBegin;
		D3D12_RANGE readRange;
		memset(&readRange, 0, sizeof(D3D12_RANGE));
		(*buffer)->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		memcpy(pVertexDataBegin, array, numVerts * elementSize);
		(*buffer)->Unmap(0, nullptr);
	}

	view->BufferLocation = (*buffer)->GetGPUVirtualAddress();
	view->StrideInBytes = elementSize;
	view->SizeInBytes = numVerts * elementSize;
}

void gs_vertex_buffer::BuildBuffers()
{
	InitBuffer(sizeof(vec3), vbd.data->num, vbd.data->points, &vertexBuffer, &vertexBufferView);

	if (vbd.data->normals)
		InitBuffer(sizeof(vec3), vbd.data->num, vbd.data->normals, &normalBuffer, &normalBufferView);

	if (vbd.data->tangents)
		InitBuffer(sizeof(vec3), vbd.data->num, vbd.data->tangents, &tangentBuffer, &tangentBufferView);

	if (vbd.data->colors)
		InitBuffer(sizeof(uint32_t), vbd.data->num, vbd.data->colors, &colorBuffer, &colorBufferView);

	for (size_t i = 0; i < vbd.data->num_tex; i++) {
		struct gs_tvertarray *tverts = vbd.data->tvarray + i;

		if (tverts->width != 2 && tverts->width != 4)
			throw "Invalid texture vertex size specified";
		if (!tverts->array)
			throw "No texture vertices specified";

		ComPtr<ID3D12Resource> buffer;
		D3D12_VERTEX_BUFFER_VIEW uvView;
		InitBuffer(tverts->width * sizeof(float), vbd.data->num, tverts->array, &buffer, &uvView);

		uvBuffers.push_back(buffer);
		uvSizes.push_back(tverts->width * sizeof(float));
		uvBufferViews.push_back(uvView);
	}
}

gs_vertex_buffer::gs_vertex_buffer(gs_device_t *device, struct gs_vb_data *data, uint32_t flags)
	: gs_obj(device, gs_type::gs_vertex_buffer),
	  dynamic((flags & GS_DYNAMIC) != 0),
	  vbd(data),
	  numVerts(data->num)
{
	if (!data->num)
		throw "Cannot initialize vertex buffer with 0 vertices";
	if (!data->points)
		throw "No points specified for vertex buffer";
	vertexBufferData.clear();

	for (int32_t i = 0; i < data->num; ++i) {
		vertexBufferData.push_back(data->points[i]);
	}

	BuildBuffers();
}
