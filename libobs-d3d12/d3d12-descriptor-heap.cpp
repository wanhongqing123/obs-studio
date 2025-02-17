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

#define STAGING_HEAP_DESCRIPTOR_COUNT 1024

static gs_staging_descriptor_heap *staging_descriptor_heap_create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type,
							   int32_t descriptorCount)
{
	gs_staging_descriptor_heap *heap;
	ID3D12DescriptorHeap *handle;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	HRESULT hr;

	heap = (gs_staging_descriptor_heap *)bmalloc(1 * sizeof(gs_staging_descriptor_heap));
	if (!heap) {
		return NULL;
	}

	heapDesc.NumDescriptors = descriptorCount;
	heapDesc.Type = type;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;

	hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&handle));
	if (FAILED(hr))
		throw HRError("Failed to create staging desc heap", hr);

	heap->handle = handle;
	heap->heapType = type;
	heap->maxDescriptors = descriptorCount;
	heap->descriptorSize = device->GetDescriptorHandleIncrementSize(type);
	heap->descriptorHeapCPUStart = handle->GetCPUDescriptorHandleForHeapStart();

	return heap;
}

static gs_gpu_descriptor_heap *gpu_descriptor_heap_create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type,
						   int32_t descriptorCount)
{
	gs_gpu_descriptor_heap *heap;
	ID3D12DescriptorHeap *handle;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	HRESULT hr;

	heap = (gs_gpu_descriptor_heap *)bmalloc(1 * sizeof(gs_gpu_descriptor_heap));
	if (!heap) {
		return NULL;
	}

	heapDesc.NumDescriptors = descriptorCount;
	heapDesc.Type = type;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&handle));
	if (FAILED(hr))
		throw HRError("Failed to create gpu desc heap", hr);

	heap->handle = handle;
	heap->heapType = type;
	heap->maxDescriptors = descriptorCount;
	heap->descriptorSize = device->GetDescriptorHandleIncrementSize(type);
	heap->descriptorHeapCPUStart = handle->GetCPUDescriptorHandleForHeapStart();
	heap->descriptorHeapGPUStart = handle->GetGPUDescriptorHandleForHeapStart();

	return heap;
}

gs_staging_descriptor_pool *staging_descriptor_pool_create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	gs_staging_descriptor_heap *heap = staging_descriptor_heap_create(device, type, STAGING_HEAP_DESCRIPTOR_COUNT);

	gs_staging_descriptor_pool *pool = (gs_staging_descriptor_pool *)bmalloc(sizeof(gs_staging_descriptor_pool));

	pool->heapCount = 1;
	pool->heaps = (gs_staging_descriptor_heap **)bmalloc(sizeof(gs_staging_descriptor_heap *));
	pool->heaps[0] = heap;

	pool->descriptorCapacity = STAGING_HEAP_DESCRIPTOR_COUNT;
	pool->freeDescriptorCount = STAGING_HEAP_DESCRIPTOR_COUNT;
	pool->freeDescriptors =
		(gs_staging_descriptor *)bmalloc(STAGING_HEAP_DESCRIPTOR_COUNT * sizeof(gs_staging_descriptor));

	for (int32_t i = 0; i < STAGING_HEAP_DESCRIPTOR_COUNT; ++i) {
		pool->freeDescriptors[i].pool = pool;
		pool->freeDescriptors[i].heap = heap;
		pool->freeDescriptors[i].cpuHandleIndex = i;
		pool->freeDescriptors[i].cpuHandle.ptr = heap->descriptorHeapCPUStart.ptr + (i * heap->descriptorSize);
	}

	return pool;
}

static void gs_staging_descriptor_heap_destroy(gs_staging_descriptor_heap* heap) {
	if (!heap)
		return;

	if (heap->handle)
		heap->handle->Release();

	bfree(heap);
}

void gs_staging_descriptor_pool_destroy(gs_staging_descriptor_pool* pool) {
	if (!pool)
		return;

	if (pool->heaps) {
		for (int32_t i = 0; i < pool->heapCount; ++i) {
			gs_staging_descriptor_heap_destroy(pool->heaps[i]);
		}

		bfree(pool->heaps);
	}

	bfree(pool->freeDescriptors);
	bfree(pool);
}

void gs_expand_staging_descriptor_pool(ID3D12Device *device, gs_staging_descriptor_pool *pool)
{
	gs_staging_descriptor_heap *heap =
		staging_descriptor_heap_create(device, pool->heaps[0]->heapType, STAGING_HEAP_DESCRIPTOR_COUNT);

	pool->heapCount += 1;
	pool->heaps = (gs_staging_descriptor_heap **)brealloc(pool->heaps,
							      pool->heapCount * sizeof(gs_staging_descriptor_heap *));
	pool->heaps[pool->heapCount - 1] = heap;

	pool->descriptorCapacity += STAGING_HEAP_DESCRIPTOR_COUNT;
	pool->freeDescriptorCount += STAGING_HEAP_DESCRIPTOR_COUNT;
	pool->freeDescriptors = (gs_staging_descriptor *)brealloc(
		pool->freeDescriptors, pool->descriptorCapacity * sizeof(gs_staging_descriptor));

	for (int32_t i = 0; i < STAGING_HEAP_DESCRIPTOR_COUNT; i += 1) {
		pool->freeDescriptors[i].pool = pool;
		pool->freeDescriptors[i].heap = heap;
		pool->freeDescriptors[i].cpuHandleIndex = i;
		pool->freeDescriptors[i].cpuHandle.ptr = heap->descriptorHeapCPUStart.ptr + (i * heap->descriptorSize);
	}
}

void gs_release_staging_descriptor(gs_staging_descriptor* cpuDescriptor) {
	gs_staging_descriptor_pool* pool = cpuDescriptor->pool;

	if (!pool)
		return;

	memcpy(&pool->freeDescriptors[pool->freeDescriptorCount], cpuDescriptor, sizeof(gs_staging_descriptor));
	pool->freeDescriptorCount += 1;
}
