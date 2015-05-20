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

#ifndef _COPENGLIMAGE_H
	#define _COPENGLIMAGE_H
	#include "../common.h"
	#include "OpenGLEngine.h"
	#include "../Mathext.h"

	namespace cpl
	{
		namespace OpenGLEngine
		{
			class COpenGLImage
			{

			public:

				class OpenGLImageDrawer;
				friend class OpenGLImageDrawer;

				class OpenGLImageDrawer : public COpenGLStack::Rasterizer
				{
					
				public:

					OpenGLImageDrawer(COpenGLImage & img, COpenGLStack & s)
						: Rasterizer(s), image(img)
					{
						image.text.bind();
						glBegin(GL_QUADS);
						glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					}

					inline void setColour(const juce::Colour & colour)
					{
						glColor4f(colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
					}
					/// <summary>
					/// Draws the image pixel-perfect at x and y, with x-offset being a value between 0.
					/// Values over 0 wraps x around, virtually using the image as a circular buffer.
					/// </summary>
					inline void drawCircular(float xoffset)
					{

						glTexCoord2f(xoffset, 0.0f); glVertex2f(-1.0f, -1.0f);
						glTexCoord2f(xoffset, 1.0f); glVertex2f(-1.0f, 1.0f);
						glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f - xoffset * 2, 1.0f);
						glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f - xoffset * 2, -1.0f);


						glTexCoord2f(0, 0.0f); glVertex2f(1.0f - xoffset * 2, -1.0f);
						glTexCoord2f(0, 1.0f); glVertex2f(1.0f - xoffset * 2, 1.0f);
						glTexCoord2f(xoffset, 1.0f); glVertex2f(1.0f, 1.0f);
						glTexCoord2f(xoffset, 0.0f); glVertex2f(1.0f, -1.0f);

					}

					~OpenGLImageDrawer()
					{
						glEnd();
						image.text.unbind();
					}

				private:

					COpenGLImage & image;
				};

				COpenGLImage()
					: preserveAcrossContexts(false), height(), width(), actualHeight(), actualWidth()
				{

				}

				COpenGLImage(std::size_t w, std::size_t h)
					: preserveAcrossContexts(false)
				{
					resize(w, h, false);
				}


				bool resize(std::size_t newWidth, std::size_t newHeight, bool copyOldContents = true)
				{
					std::size_t newActualWidth(newWidth), newActualHeight(newHeight);

					if (newWidth == 0 || newHeight == 0)
						return false;

					if (!text.isValidSize(newWidth, newHeight))
					{
						newActualWidth = Math::nextPow2(newWidth);
						newActualHeight = Math::nextPow2(newWidth);
					}

					// maybe too large?
					if (!text.isValidSize(newActualWidth, newActualHeight))
						return false;

					juce::Image newContents(juce::Image::ARGB, newActualWidth, newActualHeight, true);

					if (copyOldContents && text.getTextureID() != 0)
					{
						text.bind();

						// copy texture into memory
						juce::Image oldContents(juce::Image::PixelFormat::ARGB, actualWidth, actualHeight, false);
						{
							juce::Image::BitmapData data(oldContents, juce::Image::BitmapData::writeOnly);

							glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

							glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data);

							if (glGetError() != GL_NO_ERROR)
								return false;
						}

						// paint old image onto new
						{
							juce::Graphics g(newContents);

							// copy and rescale the subsection onto the new image
							g.drawImage(oldContents, 0, 0, newWidth, newHeight, 0, 0, width, height);
						}
					}

					text.loadImage(newContents);

					width = newWidth; height = newHeight; actualWidth = newActualWidth; actualHeight = newActualHeight;

					return glGetError() == GL_NO_ERROR;
				}

				bool updateSingleRow();

				GLint getTextureID()
				{
					return text.getTextureID();
				}

			private:
				// the size of the image
				std::size_t width, height;
				// the actual size, may be larger
				std::size_t actualWidth, actualHeight;
				bool preserveAcrossContexts;
				juce::OpenGLTexture text;
			};
		};
	}; // {} cpl
#endif