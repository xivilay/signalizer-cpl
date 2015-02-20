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
 
	file:CSubpixelSoftwareGraphics.h
 
		Implementation of CSubpixelSoftwareGraphics.h
 
		Bugs:
			Outlines that are beyond -150 pixels will fuck up rendering (because of division
			round up rules around zero). Either add a larger offset or fix the algorithm.

 *************************************************************************************/

#include "CSubpixelSoftwareGraphics.h"
#include "SubpixelRendering.h"
#include "CSubpixelScanlineRenderer.h"
#include "../stdext.h"

namespace cpl
{
	namespace rendering
	{

		struct DisplayData
		{
			bool systemUsesSubpixels;
			double fontGamma;
			LCDMatrixOrientation displayMatrixOrder;
			LutGammaScale gammaScale;
		};

		struct DisplaySetup
		{
			std::vector<DisplayData> displays;

		};


		double CSubpixelSoftwareGraphics::gamma = 1;

		const int RGBToDisplayPixelMap<LCDMatrixOrientation::RGB, false>::map[3] =
		{
			0,
			1,
			2
		};

		const int RGBToDisplayPixelMap<LCDMatrixOrientation::RGB, true>::map[3] =
		{
			2,
			1,
			0
		};

		const int RGBToDisplayPixelMap<LCDMatrixOrientation::BGR, false>::map[3] =
		{
			2,
			1,
			0
		};

		const int RGBToDisplayPixelMap<LCDMatrixOrientation::BGR, true>::map[3] =
		{
			0,
			1,
			2
		};

		const int RGBToDisplayPixelMap<LCDMatrixOrientation::GBR, false>::map[3] =
		{
			2,
			0,
			1
		};

		const int RGBToDisplayPixelMap<LCDMatrixOrientation::GBR, true>::map[3] =
		{
			1,
			0,
			2
		};

		CSubpixelSoftwareGraphics::CSubpixelSoftwareGraphics
		(
			const juce::Image & imageToRenderOn, 
			const juce::Point<int> & origin,
			const juce::RectangleList<int> & initialClip,
			bool allowAlphaDrawing
		)
		: 
			buffer(imageToRenderOn), 
			origin(origin), 
			startingClip(initialClip),
			defaultFontGamma(1200),
			LowLevelGraphicsSoftwareRenderer(imageToRenderOn, origin, initialClip),
			displayInfo(new DisplaySetup)
		{
			// throw if we're given an alpha image.
			if (!allowAlphaDrawing && imageToRenderOn.getFormat() != imageToRenderOn.RGB)
			{
				throw std::runtime_error(
					"CSubpixelSoftwareGraphics(): Image to render on was not RGB!"
				);
			}
			collectSystemInfo();

		}

		void CSubpixelSoftwareGraphics::collectSystemInfo()
		{

			displayInfo->displays.clear();

			#ifdef __WINDOWS__
				BOOL systemIsSmoothing = FALSE;
				// antialiased text set?
				if (SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &systemIsSmoothing, 0) 
					&& systemIsSmoothing)
				{
					UINT smoothingType = 0;

					// clear type enabled?
					if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &smoothingType, 0) 
						&& smoothingType == FE_FONTSMOOTHINGCLEARTYPE)
					{
						UINT systemGamma = 0; // 1000 - 2200
						double finalGamma = defaultFontGamma;

						// collect gamma correction
						if (SystemParametersInfo(SPI_GETFONTSMOOTHINGCONTRAST, 0, &systemGamma, 0))
						{
							systemGamma = cpl::Math::confineTo<std::uint16_t>(systemGamma, 1000, 2200);
							finalGamma = systemGamma / 1000.0;
						}
							
						UINT systemMatrixOrder = 0;

						// retrieve the orientation of pixels on the monitor.
						// change this to be monitor dependant some day.
						if (SystemParametersInfo(SPI_GETFONTSMOOTHINGORIENTATION, 0, &systemMatrixOrder, 0))
						{
							switch (systemMatrixOrder)
							{
								case FE_FONTSMOOTHINGORIENTATIONBGR:
								{
									for (auto & display : juce::Desktop::getInstance().getDisplays().displays)
									{
										displayInfo->displays.push_back
										(
											{
												true, 
												finalGamma, 
												LCDMatrixOrientation::BGR, 
												0.4 + finalGamma
											}
										);
									}
									break;
								}
								case FE_FONTSMOOTHINGORIENTATIONRGB:
								{
									for (auto & display : juce::Desktop::getInstance().getDisplays().displays)
									{
										displayInfo->displays.push_back
										(
											{ 
												true,
												finalGamma,
												LCDMatrixOrientation::RGB,
												0.4 + finalGamma 
											}
										);
									}
									break;
								}

							}

						}

					}
				}
			#else
				// fill in stuff for other operating systems
			#endif

			// set up text gamma

		}

		void CSubpixelSoftwareGraphics::drawGlyph(int glyphNumber, const AffineTransform & z)
		{
			if (!tryToDrawGlyph(glyphNumber, z))
				return juce::LowLevelGraphicsSoftwareRenderer::drawGlyph(glyphNumber, z);
		}


		bool CSubpixelSoftwareGraphics::tryToDrawGlyph(int glyphNumber, const AffineTransform & z)
		{

			using namespace juce::RenderingHelpers;

			typedef CachedGlyphEdgeTable<SoftwareRendererSavedState> GlyphType;
			typedef ClipRegions<SoftwareRendererSavedState>::EdgeTableRegion EdgeTableRegionType;
			typedef GlyphCache<GlyphType, SoftwareRendererSavedState> GlyphCacheType;

			// only colours supported now
			if (!stack->fillType.isColour())
				return false;

			// obtain transform
			auto & transform = stack->transform;
			// starting point
			Point<float> pos(z.getTranslationX(), z.getTranslationY());

			// find what display our glyph resides on.
			auto & displays = juce::Desktop::getInstance().getDisplays();
			auto currentMonitor = displays.getDisplayContaining(pos.toInt());
			// check this perhaps
			std::size_t currentMonitorIndex = std::index_of(displays.displays, currentMonitor).first;

			// check this as well.
			bool glyphSpansMultipleMonitors = false;


			if (glyphSpansMultipleMonitors || !displayInfo->displays[currentMonitorIndex].systemUsesSubpixels)
				return false;

			// get the current font and scale it 3 times horizontally.
			auto font = getFont();
			font.setHorizontalScale(font.getHorizontalScale() * 3);

			// get the glyph outlines
			std::unique_ptr<const juce::EdgeTable, cpl::Utility::maybe_delete<const juce::EdgeTable>> outlines;

			if (z.isOnlyTranslation() && !transform.isRotated)
			{
				// get the glyph from the cache
				GlyphCacheType& cache = GlyphCacheType::getInstance();

				// straight drawing
				if (transform.isOnlyTranslated)
				{
					if (juce::ReferenceCountedObjectPtr<GlyphType> glyphRef = cache.findOrCreateGlyph(font, glyphNumber))
					{
						// increase cache locality
						glyphRef->lastAccessCount++;

						outlines.reset(glyphRef->edgeTable);
						// dont delete this instance
						outlines.get_deleter().shared = true;
						// add transform offset (if any)
						pos += transform.offset.toFloat();
					}
					else
					{
						// maybe create a new? but this indicates something is wrong..
						return false;
					}
				}
				else
				{
					// horizontal/vertical scaling - or dpi
					// scale pos.
					pos = transform.transformed(pos);
					//pos += transform.offset.toFloat();
					// create a scaled version of the font.
					Font scaledFont(font);
					scaledFont.setHeight(font.getHeight() * transform.complexTransform.mat11);

					const float xScale = transform.complexTransform.mat00 / transform.complexTransform.mat11;
					if (std::abs(xScale - 1.0f) > 0.01f)
						scaledFont.setHorizontalScale(scaledFont.getHorizontalScale() * xScale);

					if (juce::ReferenceCountedObjectPtr<GlyphType> glyphRef = cache.findOrCreateGlyph(scaledFont, glyphNumber))
					{
						// increase cache locality
						glyphRef->lastAccessCount++;

						outlines.reset(glyphRef->edgeTable);
						// dont delete this instance
						outlines.get_deleter().shared = true;
						// add transform offset (if any)
						//pos += transform.offset.toFloat();
					}
					else
					{
						// maybe create a new? but this indicates something is wrong..
						return false;
					}


				}
				//transform.getTransform().transformPoint(pos.x, pos.y);


			}
			else // here we have to create a new edgetable
			{

				return false;

				// fix this code. looks funny.
				const float fontHeight = font.getHeight();

				juce::AffineTransform t(transform.getTransformWith(
					juce::AffineTransform::scale(fontHeight * font.getHorizontalScale(), fontHeight)
					.followedBy(z)));

				outlines.reset(font.getTypeface()->getEdgeTableForGlyph(glyphNumber, t, fontHeight));

				// delete it later.
				outlines.get_deleter().shared = false;
			}

			// hmm?
			if (!outlines.get())
				return false;

			// the colour to draw with
			auto & colour = stack->fillType.colour;

			// obtain bitmap buffer.
			Image::BitmapData destData(buffer, Image::BitmapData::readWrite);

			// create the correct renderer
			switch (displayInfo->displays[currentMonitorIndex].displayMatrixOrder)
			{
				case LCDMatrixOrientation::RGB:
				{
					// create renderer, 
					switch (buffer.getFormat())
					{
						case juce::Image::RGB:
						{

							CSubpixelScanlineRenderer<PixelRGB, LCDMatrixOrientation::RGB, WeightMap<5, 9>, LutGammaScale>
								subpixelRenderer(destData, colour, pos, startingClip, 
									displayInfo->displays[currentMonitorIndex].gammaScale);

							outlines->iterate(subpixelRenderer);
							break;
						}
						case juce::Image::ARGB:
						{

							CSubpixelScanlineRenderer<PixelARGB, LCDMatrixOrientation::RGB, WeightMap<5, 9>, LutGammaScale>
								subpixelRenderer(destData, colour, pos, startingClip,
									displayInfo->displays[currentMonitorIndex].gammaScale);

							outlines->iterate(subpixelRenderer);
							break;
						}
						// unknown image type
						default:
							return false;
					}
					break;

				}
				case LCDMatrixOrientation::BGR:
				{
					// create renderer, 
					switch (buffer.getFormat())
					{
						case juce::Image::RGB:
						{

							CSubpixelScanlineRenderer<PixelRGB, LCDMatrixOrientation::BGR, WeightMap<5, 9>, LutGammaScale>
								subpixelRenderer(destData, colour, pos, startingClip,
									displayInfo->displays[currentMonitorIndex].gammaScale);

							outlines->iterate(subpixelRenderer);
							break;
						}
						case juce::Image::ARGB:
						{

							CSubpixelScanlineRenderer<PixelARGB, LCDMatrixOrientation::BGR, WeightMap<5, 9>, LutGammaScale>
								subpixelRenderer(destData, colour, pos, startingClip,
									displayInfo->displays[currentMonitorIndex].gammaScale);

							outlines->iterate(subpixelRenderer);
							break;
						}
						// unknown image type
						default:
							return false;
					}
					break;
				}
				// unknown pixel matrix of screen
				default:
					return false;
			}
			// succesfully rendered something.
			return true;
		}

	}; // {} rendering
}; // {} cpl
