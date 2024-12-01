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
#include <iostream>
#include <memory>
#include <string>

/* --- DirectXMath --- */
#include <DirectXMath.h>
#include <DirectXColors.h>
using namespace DirectX;

/* --- 3RD PARTY --- */


/* --- PROJECT FILES --- */
#include "Logger.h"
#include "Macros.h"
#include "Singleton.h"

#endif //PCH_H