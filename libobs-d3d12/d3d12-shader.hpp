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
#include <graphics/shader-parser.h>

struct gs_staging_descriptor;

struct gs_sampler_state : gs_obj {
	gs_sampler_info info;
	gs_staging_descriptor* samplerDescriptor;

	inline void Release()
	{
		if (samplerDescriptor)
			bfree(samplerDescriptor);

		samplerDescriptor = NULL;
	}

	gs_sampler_state(gs_device_t* device, const gs_sampler_info* info);
};

struct gs_shader_param {
	std::string name;
	gs_shader_param_type type;

	uint32_t textureID;
	struct gs_sampler_state* nextSampler = nullptr;

	int arrayCount;

	size_t pos;

	std::vector<uint8_t> curValue;
	std::vector<uint8_t> defaultValue;
	bool changed;

	gs_shader_param(shader_var& var, uint32_t& texCounter);
};

struct ShaderError {
	ComPtr<ID3D10Blob> errors;
	HRESULT hr;

	inline ShaderError(const ComPtr<ID3D10Blob>& errors, HRESULT hr) : errors(errors), hr(hr) {}
};

struct gs_shader : gs_obj {
	gs_shader_type type;
	std::vector<gs_shader_param> params;

	size_t samplerCount = 0;
	size_t textureCount = 0;
	size_t uniform32BitBufferCount = 0; // const buffer
	size_t constantSize;

	std::vector<uint8_t> data;
	std::string actuallyShaderString;

	inline void UpdateParam(std::vector<uint8_t>& constData, gs_shader_param& param, bool& upload);
	void UploadParams();

	void BuildConstantBuffer();
	void Compile(const char* shaderStr, const char* file, const char* target, ID3D10Blob** shader);

	inline gs_shader(gs_device_t* device, gs_type obj_type, gs_shader_type type)
		: gs_obj(device, obj_type),
		type(type),
		constantSize(0)
	{
	}

	virtual ~gs_shader() {}
};

struct ShaderSampler {
	std::string name;
	gs_sampler_state sampler;

	inline ShaderSampler(const char* name, gs_device_t* device, gs_sampler_info* info)
		: name(name),
		sampler(device, info)
	{
	}
};

struct gs_vertex_shader : gs_shader {
	/*ComPtr<ID3D11VertexShader> shader;
	ComPtr<ID3D11InputLayout> layout;*/

	gs_shader_param* world, * viewProj;

	std::vector<D3D12_INPUT_ELEMENT_DESC> layoutData;

	bool hasNormals;
	bool hasColors;
	bool hasTangents;
	uint32_t nTexUnits;

	void Rebuild(ID3D12Device* dev);

	inline void Release()
	{
		/*shader.Release();
		layout.Release();
		constants.Release();*/
	}

	inline uint32_t NumBuffersExpected() const
	{
		uint32_t count = nTexUnits + 1;
		if (hasNormals)
			count++;
		if (hasColors)
			count++;
		if (hasTangents)
			count++;

		return count;
	}

	void GetBuffersExpected(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputs);

	gs_vertex_shader(gs_device_t* device, const char* file, const char* shaderString);
};

struct gs_pixel_shader : gs_shader {
	std::vector<std::unique_ptr<ShaderSampler>> samplers;

	void Rebuild(ID3D12Device* dev);

	inline void Release()
	{
	}

	inline void GetSamplerDescriptor(gs_samplerstate_t** descriptor)
	{
		size_t i;
		for (i = 0; i < samplers.size(); i++)
			descriptor[i] = &samplers[i]->sampler;
		for (; i < GS_MAX_TEXTURES; i++)
			descriptor[i] = NULL;
	}

	gs_pixel_shader(gs_device_t* device, const char* file, const char* shaderString);
};

struct gs_graphics_rootsignature {
	ComPtr<ID3D12RootSignature> rootSignature;

	int32_t vertexUniform32BitBufferRootIndex = -1;

	int32_t pixelSamplerRootIndex = -1;
	int32_t pixelTextureRootIndex = -1;
	int32_t pixelUniform32BitBufferRootIndex = -1;

	inline bool operator==(const gs_graphics_rootsignature& other) const
	{
		return vertexUniform32BitBufferRootIndex == other.vertexUniform32BitBufferRootIndex &&
			pixelSamplerRootIndex == other.pixelSamplerRootIndex &&
			pixelTextureRootIndex == other.pixelTextureRootIndex &&
			pixelUniform32BitBufferRootIndex == other.pixelUniform32BitBufferRootIndex;
	}

	inline bool operator!=(const gs_graphics_rootsignature& other) const { return !(*this == other); }

	inline ~gs_graphics_rootsignature()
	{
		rootSignature.Clear();
		vertexUniform32BitBufferRootIndex = -1;
		pixelSamplerRootIndex = -1;
		pixelTextureRootIndex = -1;
		pixelUniform32BitBufferRootIndex = -1;
	}
	inline gs_graphics_rootsignature() {}
	gs_graphics_rootsignature(ID3D12Device* device, gs_vertex_shader* vertexShader, gs_pixel_shader* pixelShader);
};

struct gs_graphics_pipeline {
	ComPtr<ID3D12PipelineState> pipeline_state = nullptr;

	BlendState blendState;
	RasterState rasterState;
	ZStencilState zstencilState;

	gs_vertex_shader* vertexShader = nullptr;
	gs_pixel_shader* pixelShader = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	DXGI_FORMAT zsformat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT rtvformat = DXGI_FORMAT_UNKNOWN;

	gs_graphics_rootsignature curRootSignature;

	inline bool operator==(const gs_graphics_pipeline& other) const
	{
		return pipeline_state == other.pipeline_state && blendState == other.blendState &&
			rasterState == other.rasterState && zstencilState == other.zstencilState &&
			vertexShader == other.vertexShader && pixelShader == other.pixelShader &&
			topologyType == other.topologyType && zsformat == other.zsformat &&
			rtvformat == other.rtvformat && curRootSignature == other.curRootSignature;
	}

	inline bool operator!=(const gs_graphics_pipeline& other) const { return !(*this == other); }
	inline ~gs_graphics_pipeline() {
		pipeline_state.Release();
		vertexShader = nullptr;
		pixelShader = nullptr;
	}

	inline gs_graphics_pipeline() {}

	inline gs_graphics_pipeline(ID3D12Device* device, const BlendState& blend, const RasterState& raster,
		const ZStencilState& zs, gs_vertex_shader* vertexShader_,
		gs_pixel_shader* pixelShader_, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_,
		DXGI_FORMAT zsformat_, DXGI_FORMAT format)
		: blendState(blend),
		rasterState(raster),
		zstencilState(zs),
		vertexShader(vertexShader_),
		pixelShader(pixelShader_),
		topologyType(topology_),
		zsformat(zsformat_),
		rtvformat(format),
		curRootSignature(device, vertexShader_, pixelShader_)
	{
	}
};
