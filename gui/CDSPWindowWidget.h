/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

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

	file:CColourControl.h

		A widget that can display a colour and allow the user to choose a new.
		Note the explicit use of juce::PixelARGB - avoid representing colours
		through integers (platforms has binary inconsistensies).
 
*************************************************************************************/

#ifndef CDSPWINDOWWIDGET_H
	#define CDSPWINDOWWIDGET_H

	#include "../common.h"
	#include "ControlBase.h"
	#include "CButton.h"
	#include "CComboBox.h"
	#include <atomic>
	#include "../dsp/DSPWindows.h"

	namespace cpl
	{

		class CDSPWindowWidget
		:
			public juce::Component,
			public CBaseControl,
			protected CBaseControl::PassiveListener,
			protected CBaseControl::ValueFormatter
		{
		public:

			enum Setup
			{

			};

			/// <summary>
			/// 
			/// </summary>
			CDSPWindowWidget();

			/// <summary>
			/// Generates the window according to the user-specified settings in the GUI.
			/// Safe, deterministic and wait-free to call from any thread.
			/// OutVector is a random-access indexable container of T, sized N.
			/// 
			/// Returns the time-domain scaling coefficient for the window.
			/// </summary>
			template<typename T, class OutVector>
				T generateWindow(OutVector & w, std::size_t N) const
				{
					auto ltype = p.wType.load(std::memory_order_acquire);
					auto lsym = p.wSymmetry.load(std::memory_order_acquire);
					auto lalpha = (T)p.wAlpha.load(std::memory_order_acquire);
					auto lbeta = (T)p.wBeta.load(std::memory_order_acquire);

					dsp::windowFunction(ltype, w, N, lsym, lalpha, lbeta);

					return dsp::windowScale(ltype, w, N, lsym, lalpha, lbeta);
				}

			virtual void serialize(CSerializer::Archiver & ar, long long int version) override;
			virtual void deserialize(CSerializer::Builder & ar, long long int version) override;
			
			struct Params
			{
				//
				std::atomic<dsp::WindowTypes> wType;
				std::atomic<dsp::Windows::Shape> wSymmetry;
				std::atomic<double> wAlpha, wBeta;
			};

			virtual const Params & getParams() const noexcept;

			CComboBox & getWindowList() noexcept { return kwindowList; }

		protected:

			class WindowAnalyzer : public juce::Component
			{
			public:

				WindowAnalyzer(const CDSPWindowWidget & parent);
				void paint(juce::Graphics & g) override;

			private:
				const CDSPWindowWidget & p;
			};

			// overrides
			virtual void valueChanged(const CBaseControl * c) override;
			virtual void onObjectDestruction(const ObjectProxy & object) override;

			virtual void resized() override;

			virtual bool stringToValue(const CBaseControl * ctrl, const std::string & buffer, iCtrlPrec_t & value) override;
			virtual bool valueToString(const CBaseControl * ctrl, std::string & buffer, iCtrlPrec_t value) override;

			void initControls();

			// data
			CComboBox kwindowList;
			CComboBox ksymmetryList;
			CKnobSlider kalpha, kbeta;
			MatrixSection layout;
			Setup layoutSetup;
			WindowAnalyzer analyzer;

			Params p;

		};

	};

#endif