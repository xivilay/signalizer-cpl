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
 
 file:SubpixelScanlineRenderer.h
 
	A templated class that rasterises outlines from juce::EdgeTables. It downscales the
	outlines by 3, so if they're already scaled by 3, it will effectively render outlines
	subpixel-aliased (including positioning).
 
	Bugs:
		Outlines that are beyond -150 pixels will fuck up rendering (because of division
		round up rules around zero). Either add a larger offset or fix the algorithm.

 *************************************************************************************/
#ifndef _CSUBPIXELSCANLINERENDERER_H
	#define _CSUBPIXELSCANLINERENDERER_H

	#include "../common.h"
	#include "SubpixelRendering.h"

	namespace cpl
	{
		namespace rendering
		{
			#ifdef _DEBUG
				#define __NOTHROW_IF_RELEASE
			#else
				#define __NOTHROW_IF_RELEASE noexcept
			#endif

			template<
				typename PixelType,  
				LCDMatrixOrientation matrixOrder = LCDMatrixOrientation::RGB, 
				class GammaLaw = LinearGammaScale<std::uint8_t>,
				typename CoordinateType = std::int_fast32_t
			>
			class CSubpixelScanlineRenderer
			{
				// adhering to std::is_standard_layout
			public:

				typedef PixelType Pixel;
				typedef CoordinateType IntType;

				static const int filterSize = 5;
				static const bool isBigEndian = PixelType::indexR == 2;
				typedef RGBToDisplayPixelMap<matrixOrder, !isBigEndian, filterSize> RGBToBMPMap;
				// can use this to modulo rgb indexes
				typedef RGBToDisplayPixelMap<LCDMatrixOrientation::RGB, true, filterSize> RGBToRGBMap;

				#ifdef __MSVC__
					static const std::uint8_t PixelMax = UCHAR_MAX;
				#else
					static constexpr std::uint8_t PixelMax = std::numeric_limits<std::uint8_t>::max();
				#endif

			
				CSubpixelScanlineRenderer
				(
					const juce::Image::BitmapData & data,
					juce::Colour colour,
					juce::Point<float> where,
					const juce::RectangleList<int> & rectangles,
					const GammaLaw & gammaFunctor = GammaLaw()
				)
				:
					data(data),
					colour(colour),
					y(0),
					origin(cpl::Math::floorToNInf(where.x), cpl::Math::round<int>(where.y)),
					alphaMap(),
					alphaPos(-1),
					subXOff(cpl::Math::round<IntType>(cpl::Math::frac(where.x) * 3)),
					lineIsBeingRendered(false),
					rectClip(rectangles),
					gamma(gammaFunctor)
				{
					colourSetup[0] = colour.getRed();
					colourSetup[1] = colour.getGreen();
					colourSetup[2] = colour.getBlue();
					colourSetup[3] = colour.getAlpha();
				}



				inline void incrementalMove(IntType diff)
				{
					const IntType addedMagicNumber = 150;
					const IntType magicNumberDiv3 = addedMagicNumber / 3;

					#ifdef _DEBUG
						// some glyphs thinks its funny to have outlines way out of their own space.
						// this kinda fucks up division around zero and negatives, calculating the wrong offsets.
						// the 'fix' so far is to add a large enough constant to divisions.
						if (alphaPos < -addedMagicNumber)
							throw std::runtime_error(
							"Article being drawn is too much out of bounds. Please fix the division here."
							);
					#endif

					std::div_t downScaledX;// = std::div(alphaPos + subXOff + addedMagicNumber, 3); <-- actually pretty slow?
					// the added magic number does nothing beyond hinting the compiler it can use a single cdq+idiv instruction
					// even a comment in between causes MSVC to generate two idiv instructions!!
					downScaledX.quot = ((alphaPos + subXOff + addedMagicNumber) / 3) - magicNumberDiv3;
					downScaledX.rem = (alphaPos + subXOff + addedMagicNumber) % 3;

					// saturate to 5
					for (IntType i = 0; i < diff; ++i)
					{
						auto subpixelIndex = downScaledX.rem + i - 2;
						// here we need the natural modulo around zero
						//auto rgbIndex = 0;
						auto rgbIndex = cpl::Math::mod(subpixelIndex, 3);

						// compile-time evaluation will get rid of these conditionals.
						// here we account for the endianness of rgb ordering of pixel bitmaps.
						IntType mappedSubpixel;
						if (PixelType::indexR == 2)
						{
							// usually windows little-endian
							if (matrixOrder == LCDMatrixOrientation::RGB)
							{
								mappedSubpixel = (subpixelIndex - rgbIndex) + (IntType(PixelType::indexR) - rgbIndex);
							}
							else if (matrixOrder == LCDMatrixOrientation::BGR)
							{
								mappedSubpixel = subpixelIndex + rgbIndex;
							}
						}
						// big endian bitmap types
						else
						{
							if (matrixOrder == LCDMatrixOrientation::RGB)
							{
								mappedSubpixel = PixelType::indexR + subpixelIndex + rgbIndex;
							}
							else if (matrixOrder == LCDMatrixOrientation::BGR)
							{
								mappedSubpixel = (subpixelIndex - rgbIndex) + (IntType(PixelType::indexR) - rgbIndex);
							}
						}

						// will break for filterkernels > 5 size
						//auto xWithoutOrigin = downScaledX.quot - (subpixelIndex < 0);
						auto xWithoutOrigin = (downScaledX.quot * 3 + subpixelIndex + 150) / 3;
						auto alsoX = downScaledX.quot - (subpixelIndex < 0);
						xWithoutOrigin -= 50;
						if (alsoX != xWithoutOrigin)
						{
							CPL_NOOP;
						}
						auto finalX = origin.x + xWithoutOrigin;
						auto finalY = origin.y + y;

						// unfortunately, we can only check collision at this point.
						if (!rectClip.containsPoint(finalX, finalY))
							continue;

						// get the pointer to the pixel - coordinates are verified at this point
						auto pixelPtr = data.getPixelPointer(finalX, finalY);

						// add the subpixel offset
						std::ptrdiff_t subpixelOffset = mappedSubpixel + (downScaledX.quot - xWithoutOrigin) * 3;
						pixelPtr += subpixelOffset;

						// colour for this alpha index
						auto colour = colourSetup[rgbIndex];
						// cumulated alpha levels for this colour
						auto alphaLevel = alphaMap[i];
						// alphablend with source
						*pixelPtr = cpl::Math::roundedMul(*pixelPtr, PixelMax - alphaLevel) + cpl::Math::roundedMul(alphaLevel, colour);

					}
					// shuffle buffer around
					for (IntType i = diff; i < filterSize; ++i)
					{
						alphaMap[i - diff] = alphaMap[i];
					}
					// clear unused alpha map entries
					for (IntType i = 0; i < diff; ++i)
					{
						alphaMap[(filterSize - i) - 1] = 0;
					}
				}

				inline void moveToPos(IntType x)
				{
					// haven't started render yet, move to pos.
					if (!lineIsBeingRendered)
					{
						lineIsBeingRendered = true;
						alphaPos = x;
						return;
					}
					// shouldn't happen; ignore
					if (x == alphaPos)
						return;

					#ifdef _DEBUG
						if (x < alphaPos)
							throw std::runtime_error("Subpixel renderer moved backwards. Uh oh.");
					#endif

					// see how much we should render - and saturate to 5
					IntType diff = cpl::Math::confineTo<IntType>(x - alphaPos, 0, filterSize);

					#ifdef _DEBUG
						if (!diff)
							throw std::runtime_error("Subpixel corrupt state.");
					#endif

					incrementalMove(diff);

					alphaPos = x;
				}
				inline void moveToStart() __NOTHROW_IF_RELEASE
				{
					alphaPos = 0;
					lineIsBeingRendered = false;
					alphaMap[0] = alphaMap[1] = alphaMap[2] = alphaMap[3] = alphaMap[4] = 0;
				}

					inline void rasterizeRestOfBuffer()
				{
					incrementalMove(filterSize);
					alphaPos += filterSize;
				}

				~CSubpixelScanlineRenderer()
				{
					rasterizeRestOfBuffer();
				}
				// 0 <= x <= 5
				inline void addToAlphaMap(IntType x, std::uint8_t val) __NOTHROW_IF_RELEASE
				{
					#ifdef _DEBUG
						static const std::uint8_t weightLut5[5] =
						{
							cpl::Math::round<std::uint8_t>(PixelMax * 1.0 / 9.0),
							cpl::Math::round<std::uint8_t>(PixelMax * 2.0 / 9.0),
							cpl::Math::round<std::uint8_t>(PixelMax * 3.0 / 9.0),
							cpl::Math::round<std::uint8_t>(PixelMax * 2.0 / 9.0),
							cpl::Math::round<std::uint8_t>(PixelMax * 1.0 / 9.0)
						};
						if (val > weightLut5[2])
							throw std::runtime_error("Subpixel renderer: Too large addition to alpha map"); // does not bode well
						if (static_cast<std::uint16_t>(alphaMap[x]) + val > UCHAR_MAX)
							throw std::runtime_error("Subpixel renderer: overflow in alpha cumulation");
					#endif
					alphaMap[x] += val;
				}

				inline void setPixel(IntType x, IntType y, std::uint8_t alpha = PixelMax)
				{

					static const std::uint8_t weightLut5[5] =
					{
						cpl::Math::round<std::uint8_t>(PixelMax * 1.0 / 9.0),
						cpl::Math::round<std::uint8_t>(PixelMax * 2.0 / 9.0),
						cpl::Math::round<std::uint8_t>(PixelMax * 3.0 / 9.0),
						cpl::Math::round<std::uint8_t>(PixelMax * 2.0 / 9.0),
						cpl::Math::round<std::uint8_t>(PixelMax * 1.0 / 9.0)
					};

					#ifdef _DEBUG
						if (this->y != y)
						{
							throw std::runtime_error("Subpixel renderer rendering on wrong scanline.");
						}
					#endif

					moveToPos(x);

					// 5 point lowpass filtering
					for (IntType i = 0; i < 5; ++i)
					{
						auto intensity = cpl::Math::roundedMul(gamma(alpha), colourSetup[3], weightLut5[i]);
						addToAlphaMap(i, intensity);

					}
				}

				static inline IntType mapMatrix(IntType z) __NOTHROW_IF_RELEASE
				{
					auto frac = cpl::Math::mod(z, 3);
					return (z - frac) + (juce::PixelRGB::indexR - frac);
				}

				// returns the subpixel around x,y ordered in RGB matrix
				// so getSubPixel(1, 1, -1) will return the the blue pixel to the left of the red pixel at (1, 1)
				inline uint8_t * getSubPixel(IntType xoff, IntType yoff, IntType n) const
				{

					auto frac = cpl::Math::mod(n, 3);
					auto mappedN = (n - frac) + (juce::PixelRGB::indexR - frac);


					IntType newX = (xoff * 3 + n) / 3;
					auto finalx = origin.x + newX;
					auto finaly = origin.y + yoff;

					if (this->rectClip.containsPoint(finalx, finaly))
						return nullptr;

					auto ptr = data.getPixelPointer(finalx, finaly);
					IntType offset = mappedN + (xoff - newX) * 3;
					return ptr + offset;

				}

				// renderer interface
				inline void setEdgeTableYPos(IntType y)
				{
					rasterizeRestOfBuffer();
					this->y = y;
					moveToStart();

				}
				inline void handleEdgeTablePixel(IntType x, std::uint8_t alphaLevel)
				{
					setPixel(x, y, alphaLevel);
				}
				inline void handleEdgeTablePixelFull(IntType x)
				{
					setPixel(x, y);
				}
				inline void handleEdgeTableLine(IntType x, IntType width, std::uint8_t alphaLevel)
				{
					for (IntType i = 0; i < width; ++i)
					{
						setPixel(x + i, y, alphaLevel);
					}
				}
				inline void handleEdgeTableLineFull(IntType x, IntType width)
				{
					for (IntType i = 0; i < width; ++i)
					{
						setPixel(x + i, y);
					}
				}

				// variables.
				const juce::Image::BitmapData & data;
				const juce::Colour colour;
				const juce::Point<int> origin;
				const juce::RectangleList<int> & rectClip;
				const IntType subXOff;
				const GammaLaw & gamma;
				std::uint_fast16_t gammaMul;
				std::uint8_t colourSetup[4];
				std::uint8_t alphaMap[5];
				IntType alphaPos;
				IntType y;
				bool lineIsBeingRendered;
				RGBToBMPMap bitmapMap;
				RGBToRGBMap rgbMap;
			};
		}; // {} rendering
	} // {} cpl
#endif