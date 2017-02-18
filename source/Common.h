#pragma once

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
