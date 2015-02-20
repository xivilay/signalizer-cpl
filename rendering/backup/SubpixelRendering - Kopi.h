/*************************************************************************************
 
	cpl - cross-platform library - v. 0.1.0.
 
	Copyright (C) 2015 Janus Lynggaard Thorborg [LightBridge Studios]
 
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
 
	file:SubpixelRendering.h
 
		Utilities and types needed for rendering subpixel graphics.

 *************************************************************************************/

#ifndef _SUBPIXELRENDERING_H
	#define _SUBPIXELRENDERING_H
	#include <cstdint>
	#include "../Types.h"

	namespace cpl
	{
		namespace rendering
		{

			enum class LCDMatrixOrientation
			{
				RGB,
				BGR,
				GBR
			};

			struct DisplayData
			{
				bool systemUsesSubpixels;
				std::uint16_t fontGamma;
				LCDMatrixOrientation displayMatrixOrder;
			};

			/*
				if you want to access a certain sub pixel on a display
				with a certain layout using a kernel of a certain size,
				through a software bitmap with a certain endianness, 
				the map found in this struct remaps an offset around zero to
				the correct offset (with modulo).
			
			*/
			template<LCDMatrixOrientation layout, bool littleEndian>
				struct RGBToDisplayPixelMap;


			template<> struct RGBToDisplayPixelMap < LCDMatrixOrientation::RGB, false>
			{
				/* substitute to constexpr when compiler support is added
					{
						0,
						1,
						2
					};
				*/
				static const int map[3];
			};

			template<> struct RGBToDisplayPixelMap < LCDMatrixOrientation::RGB, true>
			{
				/* substitute to constexpr when compiler support is added
					{
						2,
						1,
						0
					};
				*/
				static const int map[3];
			};

			template<> struct RGBToDisplayPixelMap < LCDMatrixOrientation::BGR, false>
			{
				/* substitute to constexpr when compiler support is added
					{
						2,
						1,
						0
					};	
				*/
				static const int map[3];
			};

			template<> struct RGBToDisplayPixelMap < LCDMatrixOrientation::BGR, true>
			{
				/* substitute to constexpr when compiler support is added
					{
						0,
						1,
						2
					};	
				*/
				static const int map[3];
			};

			template<> struct RGBToDisplayPixelMap < LCDMatrixOrientation::GBR, false>
			{
				/* substitute to constexpr when compiler support is added
					{
						2,
						0,
						1
					};
				*/
				static const int map[3];
			};

			template<> struct RGBToDisplayPixelMap < LCDMatrixOrientation::GBR, true>
			{
				/* substitute to constexpr when compiler support is added
					{
						1,
						0,
						2
					};
				*/
				static const int map[3];
			};
			/*
				This struct must have the following typedefs:
					Pixel -> the type of pixel (often unsigned char)
					IntType -> the general integer type

				The following members:
					static centerIndex -> alphaMap[centerIndex] is the center of the map
					static size -> the size of the alphaMap
					Pixel alphaMap[size] -> the actual alpha map.

				the following methods:
					inline void addIntensityToMap(std::uint_fast16_t alphaLevel) noexcept
						-> where alphaLevel is a 16-bit level of intensity.

					inline void clearAndShuffle(IntType numSteps) noexcept
						-> clears a certain number of steps in the map.

					inline void clear() noexcept
						-> clears the map.

				Note: all members must be public. Must have standard_layout type.
				
			*/
			template<int size, int denominator>
				struct WeightMap;

			template<> struct WeightMap<5, 16>
			{
				typedef std::uint8_t Pixel;
				typedef std::int_fast32_t IntType;
				std::uint8_t alphaMap[5];
				// make constexpr some day.
				//static const std::uint8_t lut[5];
				static const std::size_t centerIndex = 5 >> 1;
				static const std::size_t size = 5;

				inline void addIntensityToMap(std::uint_fast16_t alphaLevel) noexcept
				{
					auto const low = static_cast<Pixel>((alphaLevel + 0x800) >> 12); // 1/16
					auto const medium = static_cast<Pixel>((alphaLevel + 0x200) >> 10); // 4/16
					auto const high = static_cast<Pixel>((alphaLevel * 96 + 0x8000) >> 16); // 6/16

					alphaMap[0] += low;
					alphaMap[1] += medium;
					alphaMap[2] += high;
					alphaMap[3] += medium;
					alphaMap[4] += low;
				}

				inline void clearAndShuffle(IntType numSteps) noexcept
				{
					// shuffle buffer around
					for (IntType i = numSteps; i < size; ++i)
					{
						alphaMap[i - numSteps] = alphaMap[i];
					}
					// clear unused alpha map entries
					for (IntType i = 0; i < numSteps; ++i)
					{
						alphaMap[(size - i) - 1] = 0;
					}

				}

				inline void clear() noexcept
				{
					std::memset(alphaMap, 0, sizeof(alphaMap));
				}
			};

			template<> struct WeightMap<5, 9>
			{
				typedef std::uint8_t Pixel;
				typedef std::int_fast32_t IntType;
				Pixel alphaMap[5];
				// make constexpr some day.
				//static const std::uint8_t lut[5];
				static const std::size_t centerIndex = 5 >> 1;
				static const std::size_t size = 5;

				inline void addIntensityToMap(std::uint_fast16_t alphaLevel) noexcept
				{
					/*  this is actually calculated at runtime, even on release settings.
						kept here for reference.

						static const std::uint8_t weightLut5[3] =
						{
							cpl::Math::round<Pixel>(std::numeric_limits<Pixel>::max() * 1.0 / 9.0),
							cpl::Math::round<Pixel>(std::numeric_limits<Pixel>::max() * 2.0 / 9.0),
							cpl::Math::round<Pixel>(std::numeric_limits<Pixel>::max() * 3.0 / 9.0)
						};
						auto const low = cpl::Math::roundedMul(alphaLevel, weightLut5[0]);
						auto const medium = cpl::Math::roundedMul(alphaLevel, weightLut5[1]);
						auto const high = cpl::Math::roundedMul(alphaLevel, weightLut5[2]);
					*/
					
					auto const low = static_cast<Pixel>((static_cast<std::uint_fast32_t>(alphaLevel) * 28 + 0x8000) >> 16);
					auto const medium = static_cast<Pixel>((static_cast<std::uint_fast32_t>(alphaLevel) * 57 + 0x8000) >> 16);
					auto const high = static_cast<Pixel>((static_cast<std::uint_fast32_t>(alphaLevel) * 85 + 0x8000) >> 16);

					alphaMap[0] += low;
					alphaMap[1] += medium;
					alphaMap[2] += high;
					alphaMap[3] += medium;
					alphaMap[4] += low;
				}

				inline void clearAndShuffle(IntType numSteps) noexcept
				{
					// shuffle buffer around
					for (IntType i = numSteps; i < size; ++i)
					{
						alphaMap[i - numSteps] = alphaMap[i];
					}
					// clear unused alpha map entries
					for (IntType i = 0; i < numSteps; ++i)
					{
						alphaMap[(size - i) - 1] = 0;
					}

				}

				inline void clear() noexcept
				{
					std::memset(alphaMap, 0, sizeof(alphaMap));
				}
			};

			struct LutGammaScale
			{
				typedef std::uint8_t Pixel;
				// intermediate type for multiplications
				typedef std::uint16_t IntPixel;
				static const std::size_t PixelMax = UCHAR_MAX;
				static const std::size_t size = PixelMax >> 4;
				static const Pixel entryMax = size;
				// one more to ease calculations
				IntPixel lut[size + 2];

				LutGammaScale(double gamma)
				{

					double correction = 1.0 / gamma;
					double advance = double(PixelMax) / size;
					for (std::size_t i = 0; i < (size + 1); ++i)
					{
						auto a = i / double(size );
						auto expo = std::pow(a, correction);
						lut[i] = static_cast<Pixel>(PixelMax * expo + 0.49);
					}
					lut[size + 1] = PixelMax;

				}

				template<typename T>
					inline typename std::enable_if<std::is_same<T, Pixel>::value, Pixel>::type
						operator ()(T input)
				{
					auto indexFloor = input >> 4;
					auto indexCeil = indexFloor + 1;
					auto fraction = input & 0xF;

					auto lowerPartLarge = static_cast<IntPixel>(lut[indexFloor]) * (size - fraction);
					//lowerPartLarge += 0x8; // round
					auto lowerPartLittle = static_cast<Pixel>(lowerPartLarge >> 4);

					auto higherPartLarge = static_cast<IntPixel>(lut[indexCeil]) * (fraction);
					//higherPartLarge += 0x8; // round
					auto higherPartLittle = static_cast<Pixel>(higherPartLarge >> 4);

					auto lerpedAmount = lowerPartLittle + higherPartLittle;

					return lerpedAmount;
					//return (lerpedAmount * input + 0x80) >> 8;


				}
			};

			/*
				This is the most correct gamma scale, adheres to standard correction values.
				Usual gamma correction values are in between 1.2 and 2.2
			*/
			template<typename T>
				struct PowerGammaScale
				{
					typedef T ValueType;

					typedef typename cpl::Types::mul_promotion<ValueType>::type IntermediateType;
					const ValueType PixelMax = std::numeric_limits<ValueType>::max();
					const double power;

					PowerGammaScale(double gammaCorrection)
						: power(1.0 / gammaCorrection)
					{

					}

					inline ValueType operator()(ValueType input) const noexcept
					{
						return static_cast<ValueType>
						(
							PixelMax * std::pow(static_cast<double>(input) / PixelMax, power) + 0.499
						);
					}

				};

			/*
				This gamma scale mimics the standard one found in juce,
				if setGammaCorrection(colour.getBrightness(), 1.6f) is set
			*/
			template<typename T>
				struct LinearGammaScale
				{
					typedef T ValueType;

					typedef typename cpl::Types::mul_promotion<ValueType>::type IntermediateType;

					const ValueType PixelMax = std::numeric_limits<ValueType>::max();

					LinearGammaScale(IntermediateType correction)
						: gammaCorrection(correction)
					{

					}

					LinearGammaScale()
						: gammaCorrection(PixelMax)
					{

					}

					inline void setGammaCorrection(float brightness, float gammaScale)
					{
						gammaCorrection = PixelMax;
						brightness -= 0.5f;
						if (brightness > 0)
						{
							gammaCorrection += static_cast<IntermediateType>(gammaScale * brightness * PixelMax);
						}
					}

					inline ValueType operator()(ValueType input) const noexcept
					{
						return static_cast<ValueType>
						(
							std::min<IntermediateType>(PixelMax, (gammaCorrection * input) >> 8)
						);
					}

					IntermediateType gammaCorrection;
				};

		}; // {} rendering
	}; // {} cpl
#endif