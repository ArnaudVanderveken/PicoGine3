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

#endif //MACROS_H