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

	file:Graphics.h
	
		Some rendering routines - move to /rendering
 
*************************************************************************************/

#ifndef _GRAPHICS_H
	#define _GRAPHICS_H

	#include <cstdint>
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
	namespace cpl
	{
		// it should be called Graphics, but has n-dimensions appended to avoid 
		// name clashes.
		namespace GraphicsND
		{

			template<typename T>
				union Transform3D
				{
					Transform3D(T defaultValue)
					{
						for (auto & c : data)
								c = defaultValue;
					}
					struct component
					{
						T x, y, z;
					};

					T data[9];
					struct
					{
						component position;
						component rotation;
						component scale;
					};

					T & element(int x, int y)
					{
						return data[x * 3 + y];
					}

					void applyToOpenGL()
					{
						glPushMatrix();
						// move first - bad
						glTranslatef(static_cast<GLfloat>(position.x), static_cast<GLfloat>(position.y), static_cast<GLfloat>(position.z));
						// to avoid clipping. this is probably not how it is done.
						glScalef(1, 1, 0.1);
						glRotatef(static_cast<GLfloat>(rotation.x), 1.0f, 0.0f, 0.0f);
						glRotatef(static_cast<GLfloat>(rotation.y), 0.0f, 1.0f, 0.0f);
						glRotatef(static_cast<GLfloat>(rotation.z), 0.0f, 0.0f, 1.0f);
						// do the actual scaling
						glScalef(static_cast<GLfloat>(scale.x), static_cast<GLfloat>(scale.y), static_cast<GLfloat>(scale.z));

					}
					static void revert()
					{
						glPopMatrix();
					}
				};


			/*
			http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
			plots lines fast, but unaliased
			*/
			template <typename Ty, class plot>
			void inline bDrawLine(Ty x0, Ty y0, Ty x1, Ty y1, plot f)
			{
				Ty dx = fastabs(x1 - x0);
				Ty dy = fastabs(y1 - y0);
				Ty sx, sy, err, e2;
				if (x0 < x1)
					sx = 1;
				else
					sx = -1;
				if (y0 < y1)
					sy = 1;
				else
					sy = -1;
				err = dx - dy;

				while (true)
				{
					f(x0, y0);
					if (x0 == x1 && y0 == y1)
						break;
					e2 = err * 2;
					if (e2 > -dy)
					{
						err = err - dy;
						x0 = x0 + sx;
					}
					if (e2 < dx)
					{
						err = err + dx;
						y0 = y0 + sy;
					}
				}


			}


			#define ipart(x) ((int)x)

			#define round(x) ipart(x + 0.5)
			#define fpart(x) (x - ipart(x))
			#define rfpart(x) (1 - fpart(x))


			template <typename Ty, class plotFunction>
			void inline wuDrawLine(Ty x0, Ty y0, Ty x1, Ty y1, plotFunction plot)
			{
				bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
				if (steep)
				{
					std::swap(x0, y0);
					std::swap(x1, y1);
					if (x0 > x1)
					{
						std::swap(x0, x1);
						std::swap(y0, y1);
					}

					auto dx = x1 - x0;
					auto dy = y1 - y0;
					auto gradient = dy / dx; // float?

					// handle first endpoint
					auto xend = round(x0);
					auto yend = y0 + gradient * (xend - x0);
					auto xgap = rfpart(x0 + 0.5);

					int xpxl1 = xend; //this will be used in the main loop
					int ypxl1 = ipart(yend);

					plot(ypxl1, xpxl1, rfpart(yend) * xgap);
					plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap);

					auto intery = yend + gradient; // first y-intersection for the main loop

					// handle second endpoint

					xend = round(x1);
					yend = y1 + gradient * (xend - x1);
					xgap = fpart(x1 + 0.5);
					auto xpxl2 = xend; //this will be used in the main loop
					auto ypxl2 = ipart(yend);

					plot(ypxl2, xpxl2, rfpart(yend) * xgap);
					plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap);

					// main loop

					for (auto x = xpxl1 + 1; x < xpxl2; ++x)
					{
						plot(ipart(intery), x, rfpart(intery));
						plot(ipart(intery) + 1, x, fpart(intery));
						intery = intery + gradient;

					}
				}
				else
				{ // not steep.
					if (x0 > x1)
					{
						std::swap(x0, x1);
						std::swap(y0, y1);
					}

					auto dx = x1 - x0;
					auto dy = y1 - y0;
					auto gradient = dy / dx; // float?

					// handle first endpoint
					auto xend = round(x0);
					auto yend = y0 + gradient * (xend - x0);
					auto xgap = rfpart(x0 + 0.5);

					int xpxl1 = xend; //this will be used in the main loop
					int ypxl1 = ipart(yend);

					plot(xpxl1, ypxl1, rfpart(yend) * xgap);
					plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
					auto intery = yend + gradient; // first y-intersection for the main loop

					// handle second endpoint

					xend = round(x1);
					yend = y1 + gradient * (xend - x1);
					xgap = fpart(x1 + 0.5);
					auto xpxl2 = xend; //this will be used in the main loop
					auto ypxl2 = ipart(yend);
					plot(xpxl2, ypxl2, rfpart(yend) * xgap);
					plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);

					// main loop

					for (auto x = xpxl1 + 1; x < xpxl2; ++x)
					{

						plot(x, ipart(intery), rfpart(intery));
						plot(x, ipart(intery) + 1, fpart(intery));
						intery = intery + gradient;

					}
				}
			}
			#undef ipart
			#undef round
			#undef fpart
			#undef rfpart

			struct RGBPixel
			{
				union
				{
					struct
					{
						uint8_t r, g, b;
					};
					uint8_t data[3];
				};
				inline void setColour(RGBPixel other)
				{
					*this = other;
				}
				inline void setColour(uint8_t r, uint8_t g, uint8_t b)
				{
					this->r = r; this->g = g; this->b = b;
				}
				inline void setColour(uint32_t colour)
				{
					r = (colour & 0x00FF0000) >> 16;
					g = (colour & 0x0000FF00) >> 8;
					b = (colour & 0x000000FF);
				}
				inline void setColour(float colour)
				{

					setColour(std::uint32_t((colour * 0xFFFFFF)) | 0xFF000000);
				}
				inline void blendOtherChannels(float intensity, uint8_t channel)
				{
					auto p1 = data + channel + 1 % 2;
					auto p2 = data + channel -1 % 2;
					*p1 = static_cast<uint8_t>(*p1 + ((0xFF - *p1) >> 1)) * intensity;
					*p2 = static_cast<uint8_t>(*p2 + ((0xFF - *p2) >> 1)) * intensity;
				}
				inline void blend(float intensity, uint8_t channel)
				{
					auto p1 = data + channel;
					*p1 = static_cast<uint8_t>(*p1 + ((0xFF - *p1) >> 1)) * intensity;
				}
				inline void blendOtherChannels(uint8_t channel)
				{
					auto p1 = data + channel + 1 % 2;
					auto p2 = data + channel -1 % 2;
					*p1 = *p1 + ((0xFF - *p1) >> 1);
					*p2 = *p2 + ((0xFF - *p2) >> 1);
				}
				inline void blend(uint8_t channel)
				{
					auto p1 = data + channel;
					*p1 = *p1 + ((0xFF - *p1) >> 1);
				}
			};

			struct UPixel
			{
				union
				{
					struct
					{
						uint8_t a, r, g, b;
					};
					uint32_t p;
					uint8_t data[4];
				};

				UPixel(unsigned int pixel)
					: p(pixel)
				{

				}

				UPixel operator + (const UPixel & other)
				{

					auto r = _mm_adds_epu8(_mm_set1_epi32(p), _mm_set1_epi32(other.p));
#ifdef __MSVC__
					return r.m128i_u32[0];
#else
					return r[0];
#endif
				}


				UPixel & operator += (const UPixel & other)
				{

					auto r = _mm_adds_epu8(_mm_set1_epi32(p), _mm_set1_epi32(other.p));

#ifdef __MSVC__
					p = r.m128i_u32[0];
#else
					p = r[0];
#endif
					return *this;
				}

				UPixel operator * (float scale)
				{

					UPixel ret(*this);

					ret.a *= scale;
					ret.r *= scale;
					ret.g *= scale;
					ret.b *= scale;

					return ret;
				}


			};

		}; // Graphics
	}; // cpl
#endif