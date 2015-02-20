/*************************************************************************************
 
	 Audio Programming Environment - Audio Plugin - v. 0.3.0.
	 
	 Copyright (C) 2014 Janus Lynggaard Thorborg [LightBridge Studios]
	 
	 This program is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.
	 
	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.
	 
	 You should have received a copy of the GNU General Public License
	 along with this program.  If not, see <http://www.gnu.org/licenses/>.
	 
	 See \licenses\ for additional details on licenses associated with this program.
 
 **************************************************************************************

	file:stdext.h
	
		Adds extensions to the std namespace.

*************************************************************************************/

#ifndef _STDEXT_H
	#define _STDEXT_H

	#include <cmath>
	#include <functional>
	#include <algorithm>

	namespace std
	{

		const char newl = '\n';
		const char tab = '\t';

		template <> 
			struct modulus < float >
			{
				typedef float T;
				T operator() (const T& x, const T& y) const { return std::fmod(x, y); }
				typedef T first_argument_type;
				typedef T second_argument_type;
				typedef T result_type;
			};

		template <>
			struct modulus < double >
			{
				typedef double T;
				T operator() (const T& x, const T& y) const { return std::fmod(x, y); }
				typedef T first_argument_type;
				typedef T second_argument_type;
				typedef T result_type;
			};


		/*
			fine piece of code, originally posted here:
				http://codereview.stackexchange.com/a/59999/37465
		*/

		template<class C, class T>
			inline auto _contains_impl(const C& c, const T& x, int)
				-> decltype(c.find(x), true)
			{
				return end(c) != c.find(x);
			}

		template<class C, class T>
			inline bool _contains_impl(const C& v, const T& x, long)
			{
				return end(v) != std::find(begin(v), end(v), x);
			}

		template<class C, class T>
			inline auto contains(const C& c, const T& x)
				-> decltype(end(c), true)
			{
				return _contains_impl(c, x, 0);
			}

		#if defined(__LLVM__) && !defined(__CPP14__)
			// http://isocpp.org/files/papers/N3656.txt
			
			template<class T> struct _Unique_if {
				typedef unique_ptr<T> _Single_object;
			};
			
			template<class T> struct _Unique_if<T[]> {
				typedef unique_ptr<T[]> _Unknown_bound;
			};
			
			template<class T, size_t N> struct _Unique_if<T[N]> {
				typedef void _Known_bound;
			};
			
			template<class T, class... Args>
			typename _Unique_if<T>::_Single_object
			make_unique(Args&&... args) {
				return unique_ptr<T>(new T(std::forward<Args>(args)...));
			}
			
			template<class T>
			typename _Unique_if<T>::_Unknown_bound
			make_unique(size_t n) {
				typedef typename remove_extent<T>::type U;
				return unique_ptr<T>(new U[n]());
			}
			
			template<class T, class... Args>
			typename _Unique_if<T>::_Known_bound
			make_unique(Args&&...) = delete;
		#endif
		
		template<typename T>
			std::string to_string(T * ptr_type)
			{
				char buf[100];
				sprintf_s(buf, "0x%p", ptr_type);
				return buf;
			}
		
		template<class C, typename T>
			std::pair<std::size_t, bool> index_of(const C & c, const T & t)
			{
				auto it = begin(c);
				std::size_t count = 0;
				for (; it != end(c); it++)
				{
					count++;
				}
				return it == end(c) ? std::make_pair(0ul, false) : std::make_pair(count, true);
			}

	}; // std
#endif