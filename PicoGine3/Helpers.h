#ifndef HELPERS_H
#define HELPERS_H

namespace StrUtils
{
	inline std::wstring cstr2stdwstr(const char* str)
	{
		return std::wstring(str, str + strlen(str));
	}
}

inline void HandleHr(HRESULT hr)
{
	if (FAILED(hr))
	{
		Logger::Get().LogError(hr);
	}
}

inline void HandleNonHrWin32(BOOL result)
{
	if (!result)
		HandleHr(::HRESULT_FROM_WIN32(::GetLastError()));
}

#if defined(_VK)
#pragma warning(push)
#pragma warning(disable:28251)
#include <volk.h>
#pragma warning(pop)
#include <vulkan/vk_enum_string_helper.h>
inline void HandleVkResult(VkResult result)
{
	if ((result) != VK_SUCCESS)
	{
		const char* str = string_VkResult(result);
		Logger::Get().LogError(StrUtils::cstr2stdwstr(str));
	}
}

#endif //defined(_VK)

#endif //HELPERS_H
