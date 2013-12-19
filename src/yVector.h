#pragma once

#include <Exceptions.h>

template<typename T>
struct is_pointer { static const bool value = false; };

template<typename T>
struct is_pointer<T*> { static const bool value = true; };

template <typename T> class VECTOR
{
public:
	VECTOR<T>() :
	  _uSize(0),
		  _uAllocatedSize(0),
		  _uSizeIncrementStep(200),
		  _aVector(NULL)
	  {
	  }
	  ~VECTOR<T>()
	  {
		  deallocate();
	  }

	  void clear()
	  {
		  for(unsigned int i=0; i<_uSize; i++)
		  {
			  T* pElement = ((T*)_aVector) + i;
			  pElement->~T();
		  }

		  _uSize = 0;
	  }

	  void deallocate()
	  {
		  clear();

#ifdef __APPLE__
		  free(_aVector);
#else
#ifdef DEBUG_MEM
		  _aligned_free_dbg(_aVector);
#else
		  _aligned_free(_aVector);
#endif
#endif
		  _aVector = NULL;
		  _uAllocatedSize = 0;
	  }

	  void reserve(unsigned long int size)
	  {
		  if(_aVector)
		  {
			  if(_uAllocatedSize < size)
			  {
#ifdef __APPLE__
				  unsigned char* aNewVector = (unsigned char*)malloc(sizeof(T) * size);
				  TAssert((((unsigned long int)aNewVector) % __alignof(T)) == 0);
#else
#ifdef DEBUG_MEM
				  unsigned char* aNewVector = (unsigned char*)_aligned_malloc_dbg(sizeof(T) * size, __alignof(T), __FILE__, __LINE__);
#else
				  unsigned char* aNewVector = (unsigned char*)_aligned_malloc(sizeof(T) * size, __alignof(T));
#endif
#endif

				  memcpy(aNewVector, _aVector, sizeof(T) * _uSize);
#ifdef __APPLE__
				  free(_aVector);
#else
#ifdef DEBUG_MEM
				  _aligned_free_dbg(_aVector);
#else
				  _aligned_free(_aVector);
#endif
#endif
				  _aVector = aNewVector;
				  _uAllocatedSize = size;
			  }
		  }
		  else
		  {
#ifdef __APPLE__
			  _aVector = (unsigned char*)malloc(sizeof(T) * size);
			  TAssert((((unsigned long int)_aVector) % __alignof(T)) == 0);
#else
#ifdef DEBUG_MEM
			  _aVector = (unsigned char*)_aligned_malloc_dbg(sizeof(T) * size, __alignof(T), __FILE__, __LINE__);
#else
			  _aVector = (unsigned char*)_aligned_malloc(sizeof(T) * size, __alignof(T));
#endif
#endif
			  _uAllocatedSize = size;
		  }
	  }

	  T* allocate(unsigned long int size)
	  {
		  unsigned long int oldSize = _uSize;

		  reserve(_uSize + size);
		  _uSize += size;

		  for(unsigned long int i=0; i<size; i++)
		  {
			  T* pNewElement = ((T*)_aVector) + oldSize + i;
			  pNewElement = new (pNewElement) T;
		  }

		  return ((T*)_aVector) + oldSize;
	  }

	  unsigned long int getSize() const
	  {
		  return _uSize;
	  }

	  T& getElement(unsigned long int idx) const
	  {
		  TAssert(idx >= 0);
		  TAssert(idx < _uSize);

		  return ((T*)_aVector)[idx];
	  }

	  T* getElementPtr(unsigned long int idx)
	  {
		  TAssert(idx >= 0);
		  TAssert(idx < _uSize);

		  return &(((T*)_aVector)[idx]);
	  }


	  T* getNextElement()
	  {
		  if(_uSize == _uAllocatedSize)
		  {
			  reserve(_uAllocatedSize + _uSizeIncrementStep);
		  }

		  return &(((T*)_aVector)[_uSize++]);
	  }

	  unsigned long int push(T* pElementToPush)
	  {
		  if(_uSize == _uAllocatedSize)
		  {
			  reserve(_uAllocatedSize + _uSizeIncrementStep);
		  }

		  ((T*)_aVector)[_uSize] = *pElementToPush;
		  return _uSize++;
	  }

	  T& operator[] (unsigned int idx) const
	  {
		  TAssert(idx < _uSize);
		  return ((T*)_aVector)[idx]; 
	  }

	  VECTOR<T>& operator= (const VECTOR<T>& x)
	  {
		  this->allocate(x.getSize());

		  for(unsigned int i=0; i<x.getSize(); i++)
		  {
			  (*this)[i] = x[i];
		  }

		  return *this;
	  }

private:
	unsigned long int _uSize;
	unsigned long int _uAllocatedSize;
	unsigned long int _uSizeIncrementStep;

	union
	{
		unsigned char* _aVector;
		T* _aVectorDebug;
	};
};
