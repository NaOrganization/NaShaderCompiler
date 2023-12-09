#pragma once

template<typename Period, typename ...FuncArg>
class CallbackManager
{
public:
	class Callback
	{
	public:
		using CallbackFunc = std::function<void(FuncArg...)>;
		Period period = Period::None;
		CallbackFunc callback = CallbackFunc();

		Callback() {}
		Callback(Period period, CallbackFunc callback)
		{
			this->period = period;
			this->callback = callback;
		}
	};

	std::vector<Callback> callbacks;

	void AddCallback(Period period, typename Callback::CallbackFunc callback)
	{
		callbacks.push_back(Callback(period, callback));
	}

	void RemoveCallback(Period period, typename Callback::CallbackFunc callback)
	{
		for (auto it = callbacks.begin(); it != callbacks.end(); ++it)
		{
			if (it->period == period && it->callback == callback)
			{
				callbacks.erase(it);
				break;
			}
		}
	}

	void InvokeCallbacks(Period period, FuncArg... arg)
	{
		for (auto& callback : callbacks)
		{
			if (callback.period == period)
			{
				callback.callback(arg...);
			}
		}
	}
};