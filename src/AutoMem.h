#ifndef AUTO_MEM_H
#define AUTO_MEM_H

template<typename TYPE, size_t SIZE>
class TAutoMem
{
private:
	typedef TAutoMem<TYPE, SIZE> self_type;

public:
	explicit TAutoMem(size_t len)
	{
		if (len > SIZE)
		{
			m_mem = (TYPE*)malloc(sizeof(TYPE) * len);
			m_size = len;
		}
		else
		{
			m_mem = m_stack;
			m_size = SIZE;
		}
	}

	~TAutoMem()
	{
		if (m_mem != m_stack)
		{
			free(m_mem);
		}
	}

	TYPE* GetBuffer()
	{
		return m_mem;
	}

	size_t GetLength()
	{
		return m_size;
	}

private:
	TAutoMem();
	TAutoMem(const self_type&);
	self_type& operator=(const self_type&);

private:
	TYPE* m_mem;
	size_t m_size;
	TYPE m_stack[SIZE];
};
#endif // AUTO_MEM_H
