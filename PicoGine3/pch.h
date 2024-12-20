#ifndef PCH_H
#define PCH_H

/* --- VLD --- */
#if _DEBUG
#include <vld.h>
#endif

/* --- WINDOWS --- */
#include "CleanedWindows.h"

/* --- STD --- */
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

/* --- DirectXMath --- */
#include <DirectXMath.h>
#include <DirectXColors.h>
using namespace DirectX;

/* --- XInput --- */
#include <Xinput.h>
#pragma comment(lib, "Xinput.lib")

/* --- 3RD PARTY --- */


/* --- PROJECT FILES --- */
#include "Logger.h"

#include "Helpers.h"
#include "Singleton.h"

#endif //PCH_H