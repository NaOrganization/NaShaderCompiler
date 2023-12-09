#pragma once

// Single Instance
template <typename T>
class SingleInstance
{
private:
	inline static T* instance = NULL;
public:
	template <typename... Args>
	inline static T* Get(Args... args)
	{
		if (instance == NULL)
		{
			instance = new T(args...);
		}
		return instance;
	}

	inline static void DestroyInstance()
	{
		if (instance != NULL)
		{
			delete instance;
			instance = NULL;
		}
	}

	inline static bool IsCreated()
	{
		return instance != NULL;
	}

	inline static bool IsDestroyed()
	{
		return instance == NULL;
	}
};