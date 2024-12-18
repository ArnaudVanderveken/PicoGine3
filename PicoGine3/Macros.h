#ifndef MACROS_H
#define MACROS_H

#ifdef _DEBUG
#define HALT() DebugBreak();
#define HALT_HR(hr) DebugBreak();
#else
#define HALT() exit(-1);
#define HALT_HR(hr) exit(hr);
#endif //_DEBUG

#define HANDLE_ERROR(hr)\
	if (FAILED(hr))\
	{\
		Logger::Get().LogError(hr);\
		HALT_HR(hr)\
	}

#if defined(_VK)
#include <vulkan/vk_enum_string_helper.h>
#define CHECK_VK_ERROR(result)\
	if ((result) != VK_SUCCESS)\
	{\
		const char* str = string_VkResult(result);\
		Logger::Get().LogError(std::wstring(str, str + strlen(str)));\
		HALT()\
	}
#endif

#endif //MACROS_H