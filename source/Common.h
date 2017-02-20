#pragma once

#include <cstdio>
#include <stdarg.h>

#define EX_OK           0       /* successful termination */
#define EX_USAGE        64      /* command line usage error */
#define EX_IOERR        74      /* input/output error */

template<typename T>
class not_null
{
public:
	not_null(T* ptr)
		: m_Ptr(ptr)
	{
		assert(ptr != nullptr);
	}
	not_null(const not_null<T>& other)
	{
		m_Ptr = other.m_Ptr;
	}
	T& operator*() const
	{
		return *m_Ptr;
	}
	T* operator->() const
	{
		return m_Ptr;
	}
	operator T*() const
	{
		return m_Ptr;	
	}
private:
	T* m_Ptr;
};

enum LogLevel
{
	kError,
	kOutput,
	kDebug,
};

extern LogLevel g_LogLevel;
inline void SetLogLevel(const LogLevel level)
{
	g_LogLevel = level;
}

inline void Log(const LogLevel level, const char* format, ...)
{
	if (level <= g_LogLevel)
	{
		va_list args;
		va_start(args, format);
		if (level == kOutput)
			vfprintf(stdout, format, args);
		else
			vfprintf(stderr, format, args);
	}
}

inline void LogLine(const LogLevel level, const char* format, ...)
{
	if (level <= g_LogLevel)
	{
		va_list args;
		va_start(args, format);
		if (level == kOutput)
		{
			vfprintf(stdout, format, args);
			fprintf(stdout, "\n");
		}
		else
		{
			if (level != kOutput)
				fprintf(stderr, "drwap: ");
			vfprintf(stderr, format, args);
			fprintf(stderr, "\n");
		}

	}
}

