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

	file:OpenGLRasterizers.h

		Some rasterizer primitives

 *************************************************************************************/

#ifndef CPL_OPENGLRASTERIZERS_H
#define CPL_OPENGLRASTERIZERS_H
#include "../Common.h"
#include "OpenGLRendering.h"
#include "Graphics.h"

namespace cpl
{
	namespace OpenGLRendering
	{

		class ImageDrawer
			:
			public COpenGLStack::Rasterizer
		{
		public:
			ImageDrawer(COpenGLStack & parentStack, const juce::OpenGLTexture & texture)
				: Rasterizer(parentStack)
			{
				texture.bind();
				glBegin(GL_QUADS);
				setColour(juce::Colours::white);
			}

			inline void drawAt(OpenGLRendering::Vertex x1, OpenGLRendering::Vertex y1, OpenGLRendering::Vertex x2, OpenGLRendering::Vertex y2)
			{
				glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);
				glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 1.0f, 0.0f);
				glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 0.0f);
				glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
			}

			inline void setColour(const juce::Colour & colour)
			{
				glColor4f(colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
			}

			inline void drawAt(juce::Rectangle<OpenGLRendering::Vertex> area)
			{
				glTexCoord2f(0, 0); glVertex3f(area.getX(), area.getY(), 0);
				glTexCoord2f(0, 1); glVertex3f(area.getX(), area.getY() + area.getHeight(), 0);
				glTexCoord2f(1, 1); glVertex3f(area.getX() + area.getWidth(), area.getY() + area.getHeight(), 0);
				glTexCoord2f(1, 0); glVertex3f(area.getX() + area.getWidth(), area.getY(), 0);
			}

			~ImageDrawer()
			{
				glEnd();
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		protected:

		};

		template<std::size_t VertexBufferSize = 1024>
		struct PrimitiveDrawer : public COpenGLStack::Rasterizer
		{
			typedef GraphicsND::UPixel<GraphicsND::ComponentOrder::OpenGL> Colour;

			PrimitiveDrawer(COpenGLStack& parentStack, GLFeatureType primitive)
				: Rasterizer(parentStack), numVertices(0), lastColour(0x7f, 0xFF, 0x7F, 0x00), feature(primitive)
			{
				switch (primitive)
				{
				case GL_POINTS: case GL_LINES: case GL_LINE_STRIP: break;
				default: CPL_RUNTIME_EXCEPTION("Unsupported batch primitive");
				}

				glInterleavedArrays(GL_C4UB_V3F, 0, vertexData);
			}

			~PrimitiveDrawer()
			{
				draw();
				CPL_DEBUGCHECKGL();
			}

			void addColour(ColourType r, ColourType g, ColourType b, ColourType a = (ColourType)1) noexcept
			{
				lastColour = Colour::rounded( r, g, b, a );
			}

			void addColour(Colour colour) noexcept
			{
				lastColour = colour;
			}

			void addVertex(const GLfloat x, const GLfloat y, const GLfloat z) noexcept
			{
				addVertex(x, y, z, lastColour);
			}

			void addVertex(const GLfloat x, const GLfloat y, const GLfloat z, const Colour colour) noexcept
			{
				VertexInfo* const v = vertexData + numVertices;
				v->colour = colour.pixel.p;
				v->x = x;
				v->y = y;
				v->z = z;

				numVertices++;

				if (numVertices > VertexBufferSize - 1)
					draw();
			}


		private:

			struct VertexInfo
			{
				GLuint colour;
				GLfloat x, y, z;
			};

			alignas(32) VertexInfo vertexData[VertexBufferSize];
			Colour lastColour;
			int numVertices;
			GLFeatureType feature;

			void draw() noexcept
			{
				if (numVertices <= 0)
					return;

				glDrawArrays(feature, 0, numVertices);

				switch (feature)
				{
				case GL_LINE_STRIP:
					vertexData[0] = vertexData[numVertices - 1];
					numVertices = 1;
					break;
				default:
					numVertices = 0;
				}
			}

		};
		template<>
		class PrimitiveDrawer<0> : public COpenGLStack::Rasterizer
		{

		public:

			PrimitiveDrawer(COpenGLStack & parentStack, GLFeatureType primitive)
				: Rasterizer(parentStack)
			{
				glBegin(primitive);
			}

			inline void addVertex(OpenGLRendering::Vertex x, OpenGLRendering::Vertex y, OpenGLRendering::Vertex z)
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

			template<cpl::GraphicsND::ComponentOrder order>
			inline void addColour(cpl::GraphicsND::UPixel<order> colour)
			{
				glColor4ub(colour.pixel.r, colour.pixel.g, colour.pixel.b, colour.pixel.a);
			}

			~PrimitiveDrawer()
			{
				rasterizeBuffer();
				glEnd();
			}

			void rasterizeBuffer()
			{


			}
		};


		template<std::size_t vertexBufferSize = 128>
		class RectangleDrawer2D
			:
			public COpenGLStack::Rasterizer,
			public juce::Rectangle<GLfloat>
		{
		public:

			RectangleDrawer2D(COpenGLStack & parentStack)
				: Rasterizer(parentStack)
			{
				glGetFloatv(GL_LINE_WIDTH, &oldLineSize);
			}


			~RectangleDrawer2D()
			{
				glLineWidth(oldLineSize);
			}
			inline void renderOutline(GLfloat outlineSize = 1.f)
			{
				glLineWidth(outlineSize);
				glBegin(GL_LINE_LOOP);
				applyColour();
				glVertex2f(getX(), getY());
				glVertex2f(getX() + getWidth(), getY());
				glVertex2f(getX() + getWidth(), getY() + getHeight());
				glVertex2f(getX(), getY() + getHeight());

				glEnd();
			}

			inline void fill()
			{
				glBegin(GL_POLYGON);
				applyColour();
				glVertex2f(getX(), getY());
				glVertex2f(getX() + getWidth(), getY());
				glVertex2f(getX() + getWidth(), getY() + getHeight());
				glVertex2f(getX(), getY() + getHeight());
				glEnd();
			}

			inline void setColour(const juce::Colour & c)
			{
				red = c.getFloatRed(); green = c.getFloatGreen(); blue = c.getFloatBlue(); alpha = c.getFloatAlpha();
			}
			inline void setColour(ColourType r, ColourType g, ColourType b, ColourType a = (ColourType)1)
			{
				red = r; green = g; blue = b; alpha = a;
			}
		private:
			inline void applyColour()
			{
				glColor4f(red, green, blue, alpha);
			}
			GLfloat oldLineSize;
			GLfloat red, green, blue, alpha;
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

			inline void addVertex(OpenGLRendering::Vertex x, OpenGLRendering::Vertex y, OpenGLRendering::Vertex z)
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
			//CPL_ALIGNAS(32) Vertex vertices[vertexBufferSize * dimensions];
		};


	}; // {} rendering
}; // {} cpl
#endif
