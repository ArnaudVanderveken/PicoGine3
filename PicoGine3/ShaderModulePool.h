#ifndef SHADERMODULEPOOL_H
#define SHADERMODULEPOOL_H

#include <unordered_map>

class GfxDevice;

struct ShaderModule final
{
	VkShaderModule m_ShaderModule;
	uint32_t m_PushConstantSize;
};

class ShaderModulePool final
{
public:
	explicit ShaderModulePool(GfxDevice* pDevice);
	~ShaderModulePool();

	ShaderModulePool(const ShaderModulePool&) noexcept = delete;
	ShaderModulePool& operator=(const ShaderModulePool&) noexcept = delete;
	ShaderModulePool(ShaderModulePool&&) noexcept = delete;
	ShaderModulePool& operator=(ShaderModulePool&&) noexcept = delete;

	[[nodiscard]] const ShaderModule& GetShaderModule(const std::wstring& filename);

private:
	GfxDevice* m_pGfxDevice;
	std::unordered_map<std::wstring, ShaderModule> m_ShaderModules;

	static void ReadShaderFile(const std::wstring& filename, std::vector<char>& code);
	[[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
};

#endif //SHADERMODULEPOOL_H