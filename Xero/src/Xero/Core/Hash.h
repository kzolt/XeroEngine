#pragma once

#include <string>

namespace Xero {

	class Hash {
	public:
		static uint32_t GenerateFNVHash(const char* str);
		static uint32_t GenerateFNVHash(const std::string& string);
	};

}