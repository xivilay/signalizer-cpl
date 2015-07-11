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

					/*
										inline void drawCircular(float xoffset)
					{
						float width = 1;
						float height = 1;

						//float width = float(image.width) / image.textureWidth;
						//float height = float(image.height) / image.textureHeight;

						glTexCoord2f(xoffset * width, 0.0f); glVertex2f(-1.0f, -1.0f);
						glTexCoord2f(xoffset * width, height); glVertex2f(-1.0f, 1.0f);
						glTexCoord2f(width, height); glVertex2f(1.0f - xoffset * 2, 1.0f);
						glTexCoord2f(width, 0.0f); glVertex2f(1.0f - xoffset * 2, -1.0f);


						glTexCoord2f(0, 0.0f); glVertex2f(1.0f - xoffset * 2, -1.0f);
						glTexCoord2f(0, height); glVertex2f(1.0f - xoffset * 2, 1.0f);
						glTexCoord2f(xoffset * width, width); glVertex2f(1.0f, 1.0f);
						glTexCoord2f(xoffset * width, 0.0f); glVertex2f(1.0f, -1.0f);

					}
					*/

					~OpenGLImageDrawer()
					{
						glEnd();
						image.text.unbind();
					}

				private:

					COpenGLImage & image;
				};

				COpenGLImage()
					: preserveAcrossContexts(false), height(), width(), textureHeight(), textureWidth()
				{

				}

				COpenGLImage(std::size_t w, std::size_t h)
					: preserveAcrossContexts(false)
				{
					resize(w, h, false);
				}


				bool loadImage(const juce::Image & oldContents)
				{
					if (oldContents.isNull())
						return false;

					/*if (!internalResize(oldContents.getWidth(), oldContents.getHeight()))
						return false;*/

					const bool flip = true;
					const bool offset = true;
					juce::Image newContents(juce::Image::ARGB, textureWidth, textureHeight, false);
					{
						juce::Graphics g(newContents);
						g.setOpacity(1.0f);
						g.fillAll(Colours::blue);
						// copy and rescale the subsection onto the new image
						
						g.drawImage(oldContents, 0, offset ? textureHeight - height : 0, width, height, 0, 0, oldContents.getWidth(), oldContents.getHeight());
					}

					text.loadImage(newContents);

					return true;
				}

				void createEmptyImage()
				{
					juce::Image newContents(juce::Image::ARGB, textureWidth, textureHeight, false);
					text.loadImage(newContents);

				}

				/// <summary>
				/// Loads the stored image onto the context, and deletes the memory-mapped image.
				/// Context needs to be active.
				/// </summary>
				void load()
				{
					text.loadImage(currentContents);
					currentContents = juce::Image::null;
				}

				/// <summary>
				/// Offloads the texture into a memory-stored image, and deletes it.
				/// Context needs to be active.
				/// </summary>
				bool offload()
				{
					text.bind();

					// copy texture into memory
					
					juce::Image offloaded(juce::Image::PixelFormat::RGB, textureWidth, textureHeight, false);
					{
						juce::Image::BitmapData data(offloaded, juce::Image::BitmapData::readWrite);

						glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data.data);

						
						
						if (glGetError() != GL_NO_ERROR)
							return false;
					}

					// chop off irrelvant sections.
					// can use a memory copy operation here instead, but this is fail safe.
					// can also directly assign if width == actualWidth and height == actualHeight
					currentContents = juce::Image(juce::Image::PixelFormat::ARGB, width, height, false);
					{
						juce::Graphics g(currentContents);
						g.setOpacity(1.0f);
						g.fillAll(Colours::green);
						g.drawImage(offloaded, 0, 0, width, height, 0, 0, width, height, false);
					}
					// release the texture.
					text.release();
					return true;
				}

				/// <summary>
				/// Deletes all resources and cleans up textures.
				/// Context needs to be active.
				/// </summary>
				void release()
				{
					currentContents = juce::Image::null;
					text.release();
				}

				void loadImageInternal(const juce::Image & oldContents)
				{
					
				}


				bool internalResize(std::size_t newWidth, std::size_t newHeight)
				{
					std::size_t newActualWidth(newWidth), newActualHeight(newHeight);

					if (newWidth == 0 || newHeight == 0)
						return false;

					if (!text.isValidSize(newWidth, newHeight))
					{
						newActualWidth = Math::nextPow2(newWidth);
						newActualHeight = Math::nextPow2(newWidth);
					}

					// changes sizes, so the new load will rescale it.
					width = newWidth; height = newHeight; textureWidth = newActualWidth; textureHeight = newActualHeight;
				}
				
				/// <summary>
				/// 'Zooms' in/out vertically. Needs the active context.
				/// </summary>
				bool scaleTextureVertically(float amount)
				{
					offload();

					
					float upscale = std::min(1.0f, amount);
					float downscale = std::max(1.0f, amount);
					float destHeight = height * upscale;
					float sourceHeight = height * (2.0f - downscale);
					float sourceY = (height - sourceHeight) * 0.5f;
					float destY = (height - destHeight) * 0.5f;

					juce::Image upload(juce::Image::PixelFormat::RGB, width, height, true);
					{
						juce::Graphics g(upload);
						g.fillAll(Colours::blue);
						g.drawImage(currentContents, 0, destY, width, destHeight, 0, sourceY, width, sourceHeight, false);
					}

					loadImage(upload);

					return true;
				}

				bool resize(std::size_t newWidth, std::size_t newHeight, bool copyOldContents = true)
				{
					std::size_t newActualWidth(newWidth), newActualHeight(newHeight);

					if (newWidth == width && newHeight == height)
						return true;

					if (newWidth == 0 || newHeight == 0)
						return false;

					if (!text.isValidSize(newWidth, newHeight))
					{
						newActualWidth = Math::nextPow2Inc(newWidth);
						newActualHeight = Math::nextPow2Inc(newHeight);
					}

					// maybe too large?
					if (!text.isValidSize(newActualWidth, newActualHeight))
						return false;

					if (copyOldContents && text.getTextureID() != 0)
					{
						// copy the contents to memory.
						offload();
					}

					// changes sizes, so the new load will rescale it.
					width = newWidth; height = newHeight; textureWidth = newActualWidth; textureHeight = newActualHeight;

					// copy new image
					if (currentContents.isNull())
						createEmptyImage();
					else
						loadImage(currentContents);

					return glGetError() == GL_NO_ERROR;
				}

				/// <summary>
				/// Copies the array of RGB pixels as unsigned bytes into the x-specified column of the texture.
				/// The input vector shall support []-operator with contigous access, and must have a length of 
				/// height() * 3 unsigned chars.
				/// </summary>
				template<typename ArgbPixelVector, bool bind = true>
					inline void updateSingleColumn(int x, ArgbPixelVector & pixels)
					{
						if(bind)
							text.bind();
						glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0,
							1, height, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);
						if(bind)
							text.unbind();
					}

				GLint getTextureID()
				{
					return text.getTextureID();
				}

			private:
				// the size of the image
				std::size_t width, height;
				// the actual size, may be larger than width and height (next power of two)
				std::size_t textureWidth, textureHeight;
				bool preserveAcrossContexts;
				juce::OpenGLTexture text;
				juce::Image currentContents;
			};
		};
	}; // {} cpl
#endif