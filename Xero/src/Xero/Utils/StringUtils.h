#pragma once

namespace Xero::Utils {

	namespace String
	{
		bool EqualsIgnoreCase(const std::string& a, const std::string& b);
		std::string& ToLower(std::string& string);
		std::string ToLowerCopy(const std::string& string);
	}

	std::string BytesToString(uint64_t);

}