#pragma once

#include <string>

namespace Random
{
	std::string GetString(int length, const std::string& chars, const std::string& exclude)
	{
		std::string result;
		for (int i = 0; i < length; i++)
		{
			char c;
			auto generateSeed = [i]()->int
				{
					int randomSeed = rand() + (int)time(0);
					for (size_t j = 0; j < i; j++)
					{
						randomSeed += i << (rand() + time(0));
					}
					return randomSeed;
				};
			do
			{
				c = chars[generateSeed() % chars.length()];
			} while (exclude.find(c) != std::string::npos);
			result += c;
		}
		return result;
	}
	std::string GetString(int length, const std::string& chars)
	{
		return GetString(length, chars, "");
	}
	std::string GetString(int length)
	{
		return GetString(length, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
	}
}