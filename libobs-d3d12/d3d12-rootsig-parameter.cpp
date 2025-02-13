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


void gs_rootsig_parameter::InitAsConstants(uint32_t register_index, uint32_t numDwords,
					   D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
{
	rootSignatureParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootSignatureParam.ShaderVisibility = visibility;
	rootSignatureParam.Constants.Num32BitValues = numDwords;
	rootSignatureParam.Constants.ShaderRegister = register_index;
	rootSignatureParam.Constants.RegisterSpace = 0;
}

void gs_rootsig_parameter::InitAsConstantBuffer(uint32_t register_index, D3D12_SHADER_VISIBILITY visibility)
{
	rootSignatureParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootSignatureParam.ShaderVisibility = visibility;
	rootSignatureParam.Descriptor.ShaderRegister = register_index;
	rootSignatureParam.Descriptor.RegisterSpace = 0;
}
void gs_rootsig_parameter::InitAsBufferSRV(uint32_t register_index, D3D12_SHADER_VISIBILITY visibility)
{
	rootSignatureParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootSignatureParam.ShaderVisibility = visibility;
	rootSignatureParam.Descriptor.ShaderRegister = register_index;
	rootSignatureParam.Descriptor.RegisterSpace = 0;
}
void gs_rootsig_parameter::InitAsBufferUAV(uint32_t register_index, D3D12_SHADER_VISIBILITY visibility)
{
	rootSignatureParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootSignatureParam.ShaderVisibility = visibility;
	rootSignatureParam.Descriptor.ShaderRegister = register_index;
	rootSignatureParam.Descriptor.RegisterSpace = 0;
}
void gs_rootsig_parameter::InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t rangeIndex,
						 uint32_t register_index, uint32_t count,
						 D3D12_SHADER_VISIBILITY visibility)
{
	rootSignatureParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootSignatureParam.ShaderVisibility = visibility;

	desc_ranges.resize(count);
	rootSignatureParam.DescriptorTable.pDescriptorRanges = desc_ranges.data();
	rootSignatureParam.DescriptorTable.NumDescriptorRanges = count;

	if (desc_ranges.size() > 0) {
		D3D12_DESCRIPTOR_RANGE &desc_range = desc_ranges[rangeIndex];
		desc_range.RangeType = type;
		desc_range.NumDescriptors = count;
		desc_range.BaseShaderRegister = register_index;
		desc_range.RegisterSpace = 0;
		desc_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
}
