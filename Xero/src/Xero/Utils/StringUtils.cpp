#include "xopch.h"
#include "StringUtils.h"

namespace Xero::Utils {

	namespace String {

		bool EqualsIgnoreCase(const std::string& a, const std::string& b)
		{
			if (a.size() != b.size())
				return false;

			return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b)
				{
					return std::tolower(a) == std::tolower(b);
				});
		}

		std::string& ToLower(std::string& string)
		{
			std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) { return std::tolower(c); });
			return string;
		}

		std::string ToLowerCopy(const std::string& string)
		{
			std::string result = string;
			ToLower(result);
			return result;
		}

	}

	std::string BytesToString(uint64_t bytes)
	{
		constexpr uint64_t GB = 1024 * 1024 * 1024;
		constexpr uint64_t MB = 1024 * 1024;
		constexpr uint64_t KB = 1024;

		char buffer[16];

		if (bytes > GB)
			sprintf_s(buffer, "%.2f GB", (float)bytes / (float)GB);
		else if (bytes > MB)
			sprintf_s(buffer, "%.2f MB", (float)bytes / (float)MB);
		else if (bytes > KB)
			sprintf_s(buffer, "%.2f KB", (float)bytes / (float)KB);
		else
			sprintf_s(buffer, "%.2f bytes", (float)bytes);

		return std::string(buffer);
	}


}