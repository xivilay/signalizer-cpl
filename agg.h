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

	file:CMutex.h
		
		Provides an interface for easily locking objects through RAII so long as
		they derive from CMutex::Lockable.
		Uses an special spinlock that yields the thread instead of busy-waiting.
		time-outs after a specified interval, and asks the user what to do, as well
		as providing debugger breakpoints in case of locks.

*************************************************************************************/

#ifndef _AGG_H
	#define _AGG_H
	#include "MacroConstants.h"

	#include "agg-2.5/include/agg_glyph_raster_bin.h"
	#include "agg-2.5/include/agg_rendering_buffer.h"
	#include "agg-2.5/include/agg_pixfmt_rgba.h"
	#include "agg-2.5/include/agg_span_solid.h"
	#include "agg-2.5/include/agg_span_allocator.h"
	#include "agg-2.5/include/agg_renderer_scanline.h"
	#include "agg-2.5/include/agg_renderer_raster_text.h"

	#include "agg-2.5/include/agg_basics.h"

	#include "agg-2.5/include/agg_scanline_u.h"
	#include "agg-2.5/include/agg_scanline_bin.h"
	#include "agg-2.5/include/agg_renderer_primitives.h"
	#include "agg-2.5/include/agg_rasterizer_scanline_aa.h"
	#include "agg-2.5/include/agg_conv_curve.h"
	#include "agg-2.5/include/agg_conv_contour.h"
	#include "agg-2.5/include/agg_conv_transform.h"
	#include "agg-2.5/include/agg_pixfmt_rgb.h"
	#include "agg-2.5/include/agg_pixfmt_rgb24_lcd.h"
	#include <cpl/FreeType/FreeTypeAmalgam.h>
	#include "agg-2.5/include/agg_font_freetype.h"
	//#include "agg-2.5/include/platform/agg_platform_support.h"
	#include "agg-2.5/include/agg_gamma_lut.h"

	#include "fonts/tahoma.h"

	#ifdef __WINDOWS__
		//#include "agg-2.5\include\platform\agg_platform_support.h"
	#endif
#endif