#pragma once

template<typename T, bool zeroNewMemory = false, bool skipConstructor = false>
class FastVector
{
public:
	FastVector()
	{
		data = &one;
		if(zeroNewMemory)
			memset(data, 0, sizeof(T));
		max = 1;
		count = 0;
	}
	explicit FastVector(unsigned int reserved)
	{
		if(!skipConstructor)
			data = NULLC::construct<T>(reserved);
		else
			data = (T*)NULLC::alloc(sizeof(T) * reserved);
		if(zeroNewMemory)
			memset(data, 0, reserved * sizeof(T));
		max = reserved;
		count = 0;
	}
	~FastVector()
	{
		if(data != &one)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::dealloc(data);
		}
	}
	void		reset()
	{
		if(data != &one)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::dealloc(data);
		}
		data = &one;
		if(zeroNewMemory)
			memset(data, 0, sizeof(T));
		max = 1;
		count = 0;
	}

	__forceinline T*		push_back(){ count++; if(count==max) grow(count); return &data[count - 1]; };
	__forceinline void		push_back(const T& val){ assert(data); data[count++] = val; if(count==max) grow(count); };
	__forceinline void		push_back(const T* valptr, unsigned int count)
	{
		if(count+count>=max) grow(count+count);
		for(unsigned int i = 0; i < count; i++) data[count++] = valptr[i];
	};
	__forceinline T&		back(){ return data[count-1]; }
	__forceinline unsigned int		size(){ return count; }
	__forceinline void		pop_back(){ count--; }
	__forceinline void		clear(){ count = 0; }
	__forceinline T&		operator[](unsigned int index){ return data[index]; }
	__forceinline void		resize(unsigned int newsize){ if(newsize >= max) grow(newsize); count = newsize; }
	__forceinline void		shrink(unsigned int newSize){ count = newSize; }
	__forceinline void		reserve(unsigned int ressize){ if(ressize >= max) grow(ressize); }

	__inline void	grow(unsigned int newSize)
	{
		if(max+(max>>1) > newSize)
			newSize = max+(max>>1);
		else
			newSize += 32;
		T* ndata;
		if(!skipConstructor)
			ndata = NULLC::construct<T>(newSize);
		else
			ndata = (T*)NULLC::alloc(sizeof(T) * newSize);
		if(zeroNewMemory)
			memset(ndata, 0, newSize * sizeof(T));
		memcpy(ndata, data, max * sizeof(T));
		if(data != &one)
		{
			if(!skipConstructor)
				NULLC::destruct(data, max);
			else
				NULLC::dealloc(data);
		}
		data=ndata;
		max=newSize;
	}
	T	*data;
	T	one;
	unsigned int	max, count;
};
