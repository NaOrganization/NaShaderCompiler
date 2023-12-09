#pragma once

#define FORMAT(level) va_list args;\
va_start(args, format);\
char buffer[1024];\
vsprintf_s(buffer, format.c_str(), args);\
va_end(args);\
LogMessage(line, file, buffer, Log::Level::##level);

#define TIME(format) char buffer[80];\
time_t now = time(0);\
tm ltm = {};\
localtime_s(&ltm,&now);\
strftime(buffer, 80, format, &ltm);\
std::string bufferString = buffer;

class Logger
{
public:
	class Log
	{
	public:
		enum class Level
		{
			Error,
			Warning,
			Info,
			Succes,
			Debug,
			None
		};
		const std::string message = "";
		std::string timestamp = "";
		const Level level = Level::None;
		const std::string file = "";
		const int line = 0;

		Log(std::string message, Level level, int line, std::string file) : message(message), level(level), line(line) , file(file)
		{
			TIME("%d-%m-%Y %H:%M:%S");
			timestamp = buffer;
		}

		std::string ToString() const
		{
			return "[" + timestamp + "] " + LevelToString() + ": " + message;
		}

		std::string LevelToString() const
		{
			std::string levelString = "";
			switch (level)
			{
			case Level::Error:
				levelString = "Error";
				break;
			case Level::Warning:
				levelString = "Warning";
				break;
			case Level::Info:
				levelString = "Info";
				break;
			case Level::Succes:
				levelString = "Succes";
				break;
			case Level::Debug:
				levelString = "Debug";
				break;
			case Level::None:
				levelString = "None";
				break;
			}
			return levelString;
		}
	};
public:
	std::vector <Log> logs = std::vector<Log>();
	fs::path filePath = "";
	std::ofstream fileStream;

	Logger()
	{
		TIME("%d-%m-%Y %H-%M-%S");
		filePath = fs::current_path() / "logs";
		if (!fs::exists(filePath))
		{
			fs::create_directory(filePath);
		}
		filePath /= bufferString + ".log";
#ifndef _DEBUG
		fileStream = std::ofstream(filePath);
#endif
		}

	void LogMessage(int line, std::string file, std::string message, Log::Level level)
	{
		Log log = Log(message, level, line, file);
		logs.push_back(log);
		fileStream << log.ToString() << std::endl;
	}

	void AddError(int line, std::string file, std::string format, ...)
	{
		FORMAT(Error);
	}

	void AddWarning(int line, std::string file, std::string format, ...)
	{
		FORMAT(Warning);
	}

	void AddInfo(int line, std::string file, std::string format, ...)
	{
		FORMAT(Info);
	}

	void AddDebug(int line, std::string file, std::string format, ...)
	{
		FORMAT(Debug);
	}

	const std::vector<Log>& GetLogs()
	{
		return logs;
	}
};

#undef FORMAT

#define FORMAT_LOG(level, format, ...) SingleInstance<Logger>::Get()->Add##level(__LINE__, __FILE__, format, __VA_ARGS__);