#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <sstream>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Xero/Core/Core.h"
#include "Xero/Core/Application.h"
#include "Xero/Core/Ref.h"
#include "Xero/Core/Timestep.h"

#ifdef XO_PLATFORM_WINDOWS
	#include <Windows.h>
#endif