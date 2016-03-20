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

	file:CPLSource.cpp
	
		All the source code needed to compile the cpl lib. Add this file to your project.

*************************************************************************************/

#include "MacroConstants.h"
#include "LibraryOptions.h"

#include "Common.h"
#include "Misc.cpp"

#ifdef CPL_JUCE

	#include "GraphicComponents.cpp"
	#include "Resources.cpp"
	#include "CBaseControl.cpp"
	#include "CToolTip.cpp"
	#include "CCtrlEditSpace.cpp"
	
	//#include "fonts/tahoma.cpp"

	// gui elements
	#include "gui/DesignBase.cpp"
	#include "gui/Controls.h"
	#include "gui/CColourControl.cpp"
	#include "gui/CKnobSlider.cpp"
	#include "gui/CKnobSliderEditor.cpp"
	#include "gui/CValueControl.cpp"
	#include "gui/CComboBox.cpp"
	#include "gui/CButton.cpp"
	#include "gui/CTransformWidget.cpp"
	#include "gui/CPresetWidget.cpp"
	#include "gui/CInputControl.cpp"
	#include "gui/CDSPWindowWidget.cpp"
	// rendering
	#include "rendering/CSubpixelSoftwareGraphics.cpp"
	#include "rendering/CDisplaySetup.cpp"
	
	// io and stuff

	#include "CPresetManager.cpp"


#endif	
#include "CExclusiveFile.cpp"
#include "ffts/dustfft.cpp"

#ifdef CPL_INC_KISS
	#include "ffts/kiss_fft/tools/kiss_fft.c"
	#include "ffts/kiss_fft/tools/kiss_fftr.c"
#endif

#include "InstructionSet.cpp"
#include "CTimer.cpp"
#include "simd/simd_consts.cpp"
#include "SigMathImp.cpp"
#include "octave/octave_all.cpp"
#include "Protected.cpp"


#if defined(CPL_HINT_FONT)
	#include "vf_lib/vf_gui/vf_FreeTypeFaces.cpp"
	#include "FreeType/FreeTypeAmalgam.h"
	#include "FreeType/FreeTypeAmalgam.cpp"
#endif

#if !defined(CPL_LEAN)
	#include "CPLTests.cpp"
#endif