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

#ifndef _OPENGLRASTERIZERS_H
	#define _OPENGLRASTERIZERS_H
	#include "OpenGLEngine.h"

	namespace cpl
	{
		namespace OpenGLEngine
		{
			template<std::size_t vertexBufferSize = 1024>			
				class PrimitiveDrawer
				:
					public COpenGLStack::Rasterizer
				{

				public:

					PrimitiveDrawer(COpenGLStack & parentStack, GLFeatureType primitive)
						: Rasterizer(parentStack), vertexPointer(0)
					{
						glBegin(primitive);
					}

					inline void addVertex(OpenGLEngine::Vertex x, OpenGLEngine::Vertex y, OpenGLEngine::Vertex z)
					{
						glVertex3f(x, y, z);
					}

					inline void addColour(ColourType r, ColourType g, ColourType b, ColourType a = (ColourType)1)
					{
						glColor4f(r, g, b, a);
					}

					inline void addColour(const juce::Colour & c)
					{
						glColor4f(c.getFloatRed(), c.getFloatGreen(), c.getFloatBlue(), c.getFloatAlpha());
					}

					~PrimitiveDrawer()
					{
						rasterizeBuffer();
						glEnd();
					}

					void rasterizeBuffer()
					{


					}

				protected:
					std::size_t vertexPointer;
					//__alignas(32) Vertex vertices[vertexBufferSize * dimensions];
				};


			template<std::size_t vertexBufferSize = 1024>			
				class ConnectedLineDrawer
				:
					public COpenGLStack::Rasterizer
				{

				public:

					ConnectedLineDrawer(COpenGLStack & parentStack)
						: Rasterizer(parentStack), vertexPointer(0)
					{
						glBegin(GL_LINE_STRIP);
					}

					inline void addVertex(OpenGLEngine::Vertex x, OpenGLEngine::Vertex y, OpenGLEngine::Vertex z)
					{
						glVertex3f(x, y, z);
					}

					~ConnectedLineDrawer()
					{
						rasterizeBuffer();
						glEnd();
					}

					void rasterizeBuffer()
					{


					}

				protected:
					std::size_t vertexPointer;
					//__alignas(32) Vertex vertices[vertexBufferSize * dimensions];
				};


		}; // {} rendering
	}; // {} cpl
#endif