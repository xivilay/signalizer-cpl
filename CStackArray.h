#ifndef CSTACKARRAY_H
	#define CSTACKARRAY_H
	#include <cstdlib>
	#include <string>
	#include <exception>

	namespace cpl
	{
		/*
			Never use this code, ever. It should be logical why.
			Currently exists because at some point a technology might remedy the design.
		*/
		template<typename T>
			class CStackArrayPimpl
			{
				typedef T Type;
				typedef Type * iterator;
				typedef const Type * const_iterator;

			public:

				CStackArrayPimpl(std::size_t size)
					: beginptr(nullptr), endptr(nullptr)
				{
					resize(size);
				}

				CStackArrayPimpl() : beginptr(nullptr), endptr(nullptr) {};

				void resize(std::size_t size)
				{
					if (size > size())
					{
						release();
						#ifdef __MSVC__
							auto p = _malloca(size * sizeof(Type));

						#endif
					}

				}

				Type & operator [] (std::size_t idx) 
				{ 
					#ifdef _DEBUG
						if (idx >= size())
							throw std::runtime_error
							(
								"Index out of bounds for CStackArray<" + typeid(Type).name() + "> at 0x" + 
								std::to_string((void*)this) + " + " + std::to_string(idx)
							);
					#endif
					return *(beginptr + idx); 
				}
				Type & at(std::size_t idx) 
				{ 
					if (idx >= size())
						throw std::runtime_error
						(
							"Index out of bounds for CStackArray<" + typeid(Type).name() + "> at 0x" + 
							std::to_string((void*)this) + " + " + std::to_string(idx)
						);

					return *(beginptr + idx); 
				}
				iterator begin() { return beginptr; }
				iterator end() { return endptr; }
				std::size_t size() { return endptr - beginptr; }
				void release() 
				{ 
					#ifdef __MSVC__
						if (size())
							_freea(begin());

					#else
					#endif

					beginptr = endptr = nullptr;
				}
				~CStackArrayPimpl()
				{
					release();
				}

			private:
				iterator beginptr;
				iterator endptr;
			};

		//template<typename T>
		//	using StackVector = std::vector<T, StackAllocator<T>>;
	}

#endif