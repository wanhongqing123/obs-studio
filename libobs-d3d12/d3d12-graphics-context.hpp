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

#define STAGING_HEAP_DESCRIPTOR_COUNT 1024

struct gs_staging_descriptor_heap;
struct gs_staging_descriptor;
struct gs_staging_descriptor_pool;
struct gs_gpu_descriptor_heap;
struct gs_command_context;
struct gs_command_queue;

struct gs_staging_descriptor_heap {
	ID3D12DescriptorHeap* handle = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapCPUStart = { 0 };
	size_t descriptorSize = 0;
};

struct gs_staging_descriptor {
	gs_staging_descriptor_pool* pool = NULL;
	gs_staging_descriptor_heap* heap = NULL;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = { 0 };
	int32_t cpuHandleIndex = 0;
};

struct gs_staging_descriptor_pool {
	int32_t heapCount = 0;
	gs_staging_descriptor_heap** heaps = NULL;
	size_t descriptorCapacity = 0;
	size_t freeDescriptorCount = 0;
	gs_staging_descriptor* freeDescriptors = NULL;
};

struct gs_gpu_descriptor_heap {
	ID3D12DescriptorHeap* handle = NULL;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorHeapGPUStart = { 0 };
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapCPUStart = { 0 };
	size_t maxDescriptors = 0;
	size_t descriptorSize = 0;
	int32_t currentDescriptorIndex = 0;
};

gs_staging_descriptor_pool *gs_staging_descriptor_pool_create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type);
void gs_expand_staging_descriptor_pool(ID3D12Device *device, gs_staging_descriptor_pool *pool);
void gs_staging_descriptor_pool_destroy(gs_staging_descriptor_pool *pool);
void gs_staging_descriptor_release(gs_staging_descriptor *cpuDescriptor);
gs_gpu_descriptor_heap *gs_gpu_descriptor_heap_create(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type,
						      size_t descriptorCount);
void gs_gpu_descriptor_heap_destroy(gs_gpu_descriptor_heap *heap);
void gs_gpu_descriptor_heap_reset(gs_gpu_descriptor_heap *heap);

struct gs_command_queue {
	ID3D12Device* device = nullptr;
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_LIST_TYPE type;
	ID3D12Fence* fence = nullptr;
	uint64_t nextFenceValue = 0;
	uint64_t lastCompletedFenceValue = 0;
	HANDLE fenceEventHandle = 0;

	std::vector<ID3D12CommandAllocator*> allocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> readyAllocators;

	gs_command_queue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

	inline ~gs_command_queue() {
		Release();
	}

private:
	friend struct gs_command_context;
	uint64_t IncrementFence(void);
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle(void);

	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* list);
	ID3D12CommandAllocator* RequestAllocator(void);
	void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);
	inline void Release() {
	}
};

struct gs_command_context {
	gs_device* device = nullptr;
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12CommandAllocator* currentAllocator = nullptr;

	gs_gpu_descriptor_heap* gpu_descriptor_heap[2] = { nullptr };

	D3D12_RESOURCE_BARRIER resourceBarrierBuffer[32] = {};
	UINT numBarriersToFlush = 0;

	inline ID3D12GraphicsCommandList* CommandList() {
		return commandList;
	}

	gs_command_context(gs_device* device);
	void Reset();
	uint64_t Flush(bool waitForCompletion = false);
	uint64_t Finish(bool waitForCompletion = false);
	void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
};

