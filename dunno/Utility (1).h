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

	file:Utility.h
	
		Utility classes

*************************************************************************************/

#ifndef _UTILITY_H
	#define _UTILITY_H

	#include "MacroConstants.h"
	#include <cstdint>
	#include <xmmintrin.h>
	#include <immintrin.h>

	namespace cpl
	{
		namespace Types
		{
			// the fastest possible integer of at least 32 bits
			typedef std::uint_fast32_t fint_t;
			// the fastest possible signed integer of at least 32 bits 
			typedef std::int_fast32_t fsint_t;
			// the fastest possible unsigned integer of at least 32 bits
			typedef std::uint_fast32_t fuint_t;
			// sse-vector of 4 floats
			typedef __m128 v4sf;
			// sse-vector of 8 floats
			typedef __m256 v8sf;
			// sse-vector of 2 doubles
			typedef __m128d v2sd;
			// avx-vector of 4 doubles
			typedef __m256d v4sd;
		};
		namespace Utility 
		{
			/*
				Represents a set of bounding coordinates
			*/
			template<typename Scalar>
				struct Bounds
				{
					union
					{
						Scalar left;
						Scalar top;
					};
					union
					{
						Scalar right;
						Scalar bottom;
					};

					Scalar dist() const { return std::abs(left - bottom); }
				};


			class CNoncopyable
			{

			protected:
				CNoncopyable() {}
				~CNoncopyable() {}
				/*
					move constructor - c++11 delete ?
				*/
			private:
				CNoncopyable(const CNoncopyable & other);
				CNoncopyable & operator=(const CNoncopyable & other);
			};

		}; // Utility
	}; // cpl
#endif