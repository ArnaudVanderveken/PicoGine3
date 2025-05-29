#include "pch.h"
#include "ShaderModulePool.h"

#include <format>
#include <spirv_reflect.h>

#include "GfxDevice.h"
#include "GraphicsAPI.h"

ShaderModulePool::ShaderModulePool(GfxDevice* pDevice) :
	m_pGfxDevice{ pDevice }
{
}

ShaderModulePool::~ShaderModulePool()
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	for (const auto& val : m_ShaderModules | std::views::values)
	{
		vkDestroyShaderModule(device, val.m_ShaderModule, nullptr);
	}
}

const ShaderModule& ShaderModulePool::GetShaderModule(const std::wstring& filename)
{
	if (!m_ShaderModules.contains(filename))
	{
		auto& logger{ Logger::Get() };

		std::vector<char> code;
		ReadShaderFile(filename, code);

		const auto shaderModule{ CreateShaderModule(code) };

		SpvReflectShaderModule reflectModule;
		if (spvReflectCreateShaderModule(code.size(), code.data(), &reflectModule) != SPV_REFLECT_RESULT_SUCCESS)
			logger.LogError(std::format(L"Unable to create reflection for shader %s", filename));

		uint32_t pushConstantsSize{};

		for (uint32_t i{}; i < reflectModule.push_constant_block_count; ++i)
		{
			const SpvReflectBlockVariable& block = reflectModule.push_constant_blocks[i];
			pushConstantsSize = std::max(pushConstantsSize, block.offset + block.size);
		}

		spvReflectDestroyShaderModule(&reflectModule);

		m_ShaderModules[filename] = { shaderModule, pushConstantsSize };
	}

	return m_ShaderModules[filename];
}

void ShaderModulePool::ReadShaderFile(const std::wstring& filename, std::vector<char>& code)
{
	std::ifstream file{ filename, std::ios::ate | std::ios::binary };

	if (!file.is_open())
		Logger::Get().LogError(L"Unable to open " + filename);

	const size_t fileSize{ static_cast<size_t>(file.tellg()) };
	std::vector<char> buffer(fileSize);
	code.resize(fileSize);

	file.seekg(0);
	file.read(code.data(), fileSize);

	file.close();
}

VkShaderModule ShaderModulePool::CreateShaderModule(const std::vector<char>& code) const
{
	const auto& device{ m_pGfxDevice->GetDevice() };

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	HandleVkResult(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

	return shaderModule;
}
