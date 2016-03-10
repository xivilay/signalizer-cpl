/*************************************************************************************
 
	cpl - cross-platform library - v. 0.1.0.
 
	Copyright (C) 2016 Janus Lynggaard Thorborg (www.jthorborg.com)
 
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
 
	file:CCircularOpenGLImage.h
 
		An class that wraps an openGL texture, and supports drawing it pixel-perfectly.
		What can this be used for? Spectrograms, pretty much.

 *************************************************************************************/

#ifndef _CCIRCULAROPENGLIMAGE_H
	#define _CCIRCULAROPENGLIMAGE_H

	#include "COpenGLImage.h"

	namespace cpl
	{
		namespace OpenGLEngine
		{
			/// <summary>
			/// Circular, procedural pixel-perfect openGL image.
			/// </summary>
			class CCircularPPPOImage : public COpenGLImage
			{

			public:

				class CircularOpenGLImageDrawer;
				friend class CircularOpenGLImageDrawer;

				class OpenGLImageDrawer 
				: 
					public COpenGLStack::Rasterizer, 
					public MatrixModification, 
					public MatrixModeModification
				{
					
				public:

					CircularOpenGLImageDrawer(CCircularOpenGLImage & img, COpenGLStack & s)
						: Rasterizer(s), image(img)
					{
						img.bind();
						glMatrixMode(GL_PROJECTION);
						loadIdentityMatrix();
						glOrtho(0.0f, img.width, img.height, 0.0f, 0.0f, 1.0f);
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
					inline void drawWrapped()
					{
						//float width = 1;
						//float height = 1;
						float width = float(image.width) / image.textureWidth;
						float height = float(image.height) / image.textureHeight;

						glTexCoord2f(xoffset * width, 0.0f); glVertex2f(-1.0f, -1.0f);
						glTexCoord2f(xoffset * width, height); glVertex2f(-1.0f, 1.0f);
						glTexCoord2f(width, height); glVertex2f(1.0f - xoffset * 2, 1.0f);
						glTexCoord2f(width, 0.0f); glVertex2f(1.0f - xoffset * 2, -1.0f);


						glTexCoord2f(0, 0.0f); glVertex2f(1.0f - xoffset * 2, -1.0f);
						glTexCoord2f(0, height); glVertex2f(1.0f - xoffset * 2, 1.0f);
						glTexCoord2f(xoffset * width, height); glVertex2f(1.0f, 1.0f);
						glTexCoord2f(xoffset * width, 0.0f); glVertex2f(1.0f, -1.0f);

					}

					inline void drawFlat()
					{

					}


					~CircularOpenGLImageDrawer()
					{
						glEnd();
						image.unbind();
					}

				private:

					COpenGLImage & image;
				};

				CCircularOpenGLImage() {}

				CCircularOpenGLImage(std::size_t w, std::size_t h)
					: COpenGLImage(w, h)
				{
				}




			private:


				bool transferToMemory() override
				{
					// copy texture into memory

					bind();

					juce::Image offloaded(juce::Image::PixelFormat::RGB, textureWidth, textureHeight, false);
					{
						juce::Image::BitmapData data(offloaded, juce::Image::BitmapData::readWrite);

						
						Texture::Copy2DTextureToMemory(textureID, data.data, textureWidth * textureHeight, GL_RGB, GL_UNSIGNED_BYTE);


						if (glGetError() != GL_NO_ERROR)
							return false;
					}

					// chop off irrelvant sections.
					// can use a memory copy operation here instead, but this is fail safe.
					// can also directly assign if width == actualWidth and height == actualHeight
					currentContents = juce::Image(juce::Image::PixelFormat::RGB, width, height, false);
					{
#pragma message cwarn("Needs to take the circular position into account, such that images doesn't wrap around. This requires this class to know about the circular position, though.")
						juce::Graphics g(currentContents);
						g.setOpacity(1.0f);
						g.fillAll(fillColour);
						g.drawImage(offloaded,
							0, 0, width, height,
							0, 0, width, height,
							false);
					}
					return true;
				}

				std::size_t indexPointer;

			};
		};
	}; // {} cpl
#endif