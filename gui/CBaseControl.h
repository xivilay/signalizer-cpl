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

	file:CBaseControl.h
		
		CBaseControl - base class for all implementable controls.
		all methods prefixed with 'b' to avoid future nameclashing.
		This class provides a common interface for all 'controls' to support
		normalized value-, string, event system, serialization and position get/set.
		Also encapsulates listeners into a single one.
		Provides a cheap RAII mutex lock as well, as well as optional reference counting.
		Does not derive from the system's base class for graphics to avoid the diamond
		problem (classes deriving from this should set the base class in their constructor)

*************************************************************************************/

#ifndef CPL_CBASECONTROL_H
	#define CPL_CBASECONTROL_H

	#include "../CMutex.h"
	#include "../Common.h"
	#include <memory>
	#include "CToolTip.h"
	#include <vector>
	#include "../stdext.h"
	#include "../Utility.h"
	#include "DesignBase.h"
	#include "../CSerializer.h"

	namespace cpl
	{
		/// <summary>
		/// the internal value type of all controls
		/// </summary>
		typedef double iCtrlPrec_t;

		class CBaseControl;
		class CSerializer;
		class CCtrlEditSpace;
		class CBaseControl;
		class ResetListener;

		class DelegatedInternalDelete
		{
		public:
			void operator()(ResetListener *) const noexcept;
		};

		/// <summary>
		/// The base class of all controls supported in this SDK that are represented through a 
		///	graphic interface.
		/// </summary>
		class CBaseControl 
		: 
			public CToolTipClient,
			public Utility::DestructionServer<CBaseControl>,
			public CSerializer::Serializable
		{
		public:

			friend class CCtrlEditSpace;

			/// <summary>
			/// The same as a Listener, however it is NOT able to change the control's status, and
			/// it will be called AFTER the listener change.
			/// </summary>
			class Listener : virtual public Utility::DestructionServer<CBaseControl>::Client
			{
			public:
				virtual void valueChanged(const CBaseControl *) = 0;
				virtual ~Listener() {};
			};

			///	<summary>
			///	Most controls have an associated string-field that maps the internal value to a
			///	meaningful human-readable string. A control having the range 0-1 can represent -100 to 0 dB,
			///	for example. This can be manually set using bSetText(). Note that the control may
			///	independently change this (even inbetween paints).
			///	A return value of false will cause the next handler in the chain to handle the event.
			///	Otherwise, the event is considered handled, and no other handlers will be called.
			///	Add a valueformatter to this class (CBaseControl) to handle mapping of values to strings.
			///	There will be a default handler. The handlers are called from newest added to the first.
			///	</summary>
			class ValueFormatter : virtual public Utility::DestructionServer<CBaseControl>::Client
			{
			public:
				virtual bool stringToValue(const CBaseControl * ctrl, const std::string & buffer, iCtrlPrec_t & value) = 0;
				virtual bool valueToString(const CBaseControl * ctrl, std::string & buffer, iCtrlPrec_t value) = 0;
				virtual ~ValueFormatter() {};
			};

			CBaseControl(GraphicComponent * b) 
				: base(b), isEditSpacesAllowed(false), tipsEnabled(false)

			{
			
			}

			virtual ~CBaseControl();
			/// <summary>
			/// Returns the tooltop for this control (if they are set to be enabled)
			/// </summary>
			juce::String bGetToolTip() const override
			{
				return tipsEnabled ? (tooltip.size() ? tooltip : bGetTitle()) : "";
			}
			/// <summary>
			/// Sets the displayed tooltip. Remember to call enableTooltip(true) if you want to use this.
			/// </summary>
			void bSetDescription(const std::string & tip)
			{
				tooltip = tip;
			}

			void enableTooltip(bool toggle = true)
			{
				tipsEnabled = toggle;
			}
			/// <summary>
			/// Creates an edit space linked to this control. If toggleEditSpace() is set to false,
			/// it may return a nullptr instead.
			/// </summary>
			virtual std::unique_ptr<CCtrlEditSpace> bCreateEditSpace();

			virtual void bToggleEditSpaces(bool toggle)
			{
				isEditSpacesAllowed = toggle;
			}

			bool bGetEditSpacesAllowed() const noexcept
			{
				return isEditSpacesAllowed;
			}

			void bSetVisible(bool bVisibility)
			{
				if(base)
					base->setVisible(bVisibility);
			}
			/// <summary>
			/// Attach to a parent. DEPRECATED.
			/// </summary>
			virtual void addToParent(GraphicComponent * parent)
			{
				parent->addChildComponent(base);
			}
			/// <summary>
			/// DEPRECATED.
			/// </summary>
			virtual void removeFromParent(GraphicComponent * parent)
			{
				parent->removeChildComponent(base);
			}
			/// <summary>
			/// Gets the internal value of the control.
			///	Ranges from 0.0 to 1.0
			/// </summary>
			virtual iCtrlPrec_t bGetValue() const 
			{ 
				return iCtrlPrec_t(0.0);
			}
			/// <summary>
			/// Gets the internal value as a boolean toggle.
			/// </summary>
			bool bGetBoolState() const
			{ 
				return bGetValue () > 0.5;
			}
			/// <summary>
			/// Formats the value val, and returns true if a conversion was succesful.
			/// </summary>
			virtual bool bFormatValue(std::string & valueString, iCtrlPrec_t val) const
			{ 
				for (auto rit = formatters.rbegin(); rit != formatters.rend(); ++rit)
				{
					if ((*rit)->valueToString(this, valueString, val))
					{
						return true;
					}
				}
				if (bValueToString(valueString, val))
					return true;

				return false;
			}
			/// <summary>
			/// Sets the value of the control.
			/// Input must be between 0.0 and 1.0, inclusive
			/// </summary>
			/// <param name="synchronizedEvent">
			/// If set, the event will be handled immediately
			/// </param>
			virtual void bSetValue(iCtrlPrec_t val, bool synchronizedEvent = false)
			{

			}
			/// <summary>
			/// Sets the value of the control, interpretting the string using a formatter,
			/// and setting the value to the control using bSetValue.
			/// Returns true if the value was succesfully interpreted and set.
			/// </summary>
			/// <param name="setInternal">
			/// Whether this command should propagate an event
			/// </param>
			/// <param name="synchronizedEvent">
			/// Whether the propagated event is synchronized
			/// </param>
			virtual bool bInterpretAndSet(const std::string & valueString, bool setInternal = false, bool synchronizedEvent = false)
			{
				iCtrlPrec_t val(0);
				if (bInterpret(valueString, val))
				{
					if (setInternal)
						bSetInternal(val);
					else
						bSetValue(val, synchronizedEvent);
					return true;
				}
				return false;
			}
			/// <summary>
			/// Maps the string to a 0-1 range, if it was succesfully interpreted.
			/// </summary>
			virtual bool bInterpret(const std::string & valueString, iCtrlPrec_t & val) const
			{
				for (auto rit = formatters.rbegin(); rit != formatters.rend(); ++rit)
				{
					if ((*rit)->stringToValue(this, valueString, val))
					{
						return true;
					}
				}
				// try own formatter
				if (bStringToValue(valueString, val))
					return true;
				else
					return false;
			}
			/// <summary>
			/// Sets the internal value of the control.
			/// Input must be between 0.0f and 1.0f, inclusive.
			/// Guaranteed to have no sideeffects (ie.doesn't call listeners) or event propagation
			/// </summary>
			virtual void bSetInternal(iCtrlPrec_t val)
			{
			
			}
			/// <summary>
			/// Sets the display title of the control
			/// </summary>
			virtual void bSetTitle(const std::string & text)
			{
			
			}

			/// <summary>
			/// Returns the visible, exported name of this control, since
			/// the title may not uniquely identify external automation.
			/// </summary>
			virtual std::string bGetExportedName() { return{}; }

			/// <summary>
			/// Gets the display title of the control
			/// </summary>
			/// <returns></returns>
			virtual std::string bGetTitle() const
			{ 
				return ""; 
			}
			/// <summary>
			/// Sets the text value of the control. This may not be visible and/or included in the control.
			/// You probably want the formatter interface.
			/// </summary>
			virtual void bSetText(const std::string & text) 
			{
			
			}
			/// <summary>
			/// Returns the text value of the control.
			/// </summary>
			virtual std::string bGetText() const
			{ 
				return ""; 
			}
			/// <summary>
			/// Returns the size of this control.
			/// X and Y are relative to the parent coordinates.
			/// </summary>
			virtual CRect bGetSize() const
			{
				if (base)
					return base->getBounds();
				return CRect();
			}
			/// <summary>
			/// Returns the size of this control.
			///	X and Y are absolute coordinates(that is, relative to the top - level window).
			/// </summary>
			virtual CRect bGetAbsSize() const
			{
				if (base)
				{
					auto basicBounds = base->getBounds();
					GraphicComponent * parentPointer = base;
					
					while ((parentPointer = parentPointer->getParentComponent()))
					{
						// skip the top level, ie.
						// keep the returned position relative to top-most window
						if (!parentPointer->getParentComponent())
							break;
						basicBounds += parentPointer->getPosition();
					}

					return basicBounds;

				}
				return CRect();
			}
			/// <summary>
			/// Sets the x, y position of this control
			/// </summary>
			virtual void bSetPos(int x, int y)
			{
				if (base)
					base->setBounds(x, y, base->getWidth(), base->getHeight());
			}
			
			virtual void bSetSize(const CRect & size)
			{
				if (base)
					base->setBounds(size);
			}
			/// <summary>
			/// Issues a command to repaint the control
			/// </summary>
			virtual void bRedraw() 
			{ 
				base->repaint();
			}
			/// <summary>
			/// Get the view / component associated with this control
			/// </summary>
			GraphicComponent * bGetView() const
			{
				return base;
			}

			void bSetIsDefaultResettable(bool shouldBePossible);

			bool bIsDefaultResettable()
			{
				return serializedState.get() != nullptr;
			}

			bool bResetToDefaultState()
			{
				if (bIsDefaultResettable() && !serializedState->isEmpty() && queryResetOk())
				{
					onControlDeserialization(*serializedState.get(), serializedState->getMasterVersion());
					serializedState->rewindReader();
					return true;
				}

				return false;
			}

			/// <summary>
			/// Adds a passive listener (if not present) to be called back on value changes
			/// </summary>
			void bAddChangeListener(Listener * listener)
			{
				if (listener && !contains(passiveListeners, listener))
				{
					passiveListeners.push_back(listener);
					addClientDestructor(listener);
				}
			}
			/// <summary>
			/// Removes a change-listener (if present)
			/// </summary>
			void bRemoveChangeListener(Listener * listener)
			{
				auto it = std::find(passiveListeners.begin(), passiveListeners.end(), listener);
				if (it != passiveListeners.end())
				{
					passiveListeners.erase(it);
				}
			}

			/// <summary>
			/// Adds a formatter (if not present)
			/// </summary>
			void bAddFormatter(ValueFormatter * formatter)
			{
				if (!contains(formatters, formatter))
				{
					formatters.push_back(formatter);
					addClientDestructor(formatter);
					bRedraw();
				}
			}
			/// <summary>
			/// Removes a formatter (if not present)
			/// </summary>
			void bRemoveFormatter(ValueFormatter * formatter)
			{
				auto it = std::find(formatters.begin(), formatters.end(), formatter);
				if (it != formatters.end())
				{
					formatters.erase(it);
				}
			}
			/// <summary>
			/// Issues a valueChanged event
			/// </summary>
			virtual void bForceEvent()
			{
				postEvent();
			}

			/// <summary>
			/// public serialization of the control
			/// </summary>
			virtual void serialize(CSerializer::Archiver & ar, Version version) override final
			{
				onControlSerialization(ar, version);
			}
			/// <summary>
			/// public deserialization of the control
			/// </summary>
			virtual void deserialize(CSerializer::Builder & ar, Version version) override final
			{
				onControlDeserialization(ar, version);

				if (bIsDefaultResettable())
				{
					onControlSerialization(*serializedState.get(), cpl::programInfo.version);
				}
			}

			virtual bool bStringToValue(const std::string & stringInput, iCtrlPrec_t & val) const
			{
				return bMapStringToInternal(stringInput, val);
			}

			virtual bool bValueToString(std::string & stringBuf, iCtrlPrec_t  val) const
			{
				return bMapIntValueToString(stringBuf, val);
			}


		protected:

			/// <summary>
			/// This function is called just before resetting this control.
			/// The reset is only done if this function returns true.
			/// You can additionally use as a logic point for doing stuff before a reset.
			/// </summary>
			virtual bool queryResetOk() { return true; }

			/// <summary>
			/// The control's internal event callback. This is called whenever the controls value is changed.
			/// THis is responsible for calling listeners + updating info + redrawing.
			/// </summary>
			virtual void baseControlValueChanged() 
			{
				notifyListeners();
				bRedraw();
			}

			/// <summary>
			/// Internal serialization. If you override this, do not call the base.
			/// </summary>
			virtual void onControlSerialization(CSerializer::Archiver & ar, Version version) 
			{
				ar << bGetValue();
			}
			/// <summary>
			/// Internal serialization. If you override this, do not call the base.
			/// </summary>
			virtual void onControlDeserialization(CSerializer::Builder & ar, Version version) 
			{
				iCtrlPrec_t value(0);
				ar >> value;

				if(ar.getModifier(CSerializer::Modifiers::RestoreValue))
					bSetValue(value, true);
			}

			static bool bMapStringToInternal(const std::string & stringInput, iCtrlPrec_t & val)
			{
				char * ptr = nullptr;
				iCtrlPrec_t pVal(0);
				pVal = strtod(stringInput.c_str(), &ptr);
				if (ptr > stringInput.c_str())
				{
					val = cpl::Math::confineTo<iCtrlPrec_t>(pVal, 0.0, 1.0);
					return true;
				}
				return false;
			}

			static bool bMapIntValueToString(std::string & stringBuf, iCtrlPrec_t  val)
			{
				char buf[200];
				#ifdef CPL_MSVC
					sprintf_s(buf, "%.17g", cpl::Math::confineTo<iCtrlPrec_t>(val, 0.0, 1.0));
				#else
					snprintf(buf, sizeof(buf), "%.17g", cpl::Math::confineTo<iCtrlPrec_t>(val, 0.0, 1.0));
				#endif
				buf[sizeof(buf) - 1] = '\0';

				stringBuf = buf;
				return true;
			}

			void notifyListeners()
			{
				for (auto const & listener : passiveListeners)
					listener->valueChanged(this);
			}

		private:

			void postEvent()
			{
				baseControlValueChanged();
			}

			bool tipsEnabled;
			bool isEditSpacesAllowed;

			/// <summary>
			/// The implementation specific context of controls
			/// </summary>
			GraphicComponent * base;
			std::vector<Listener *> passiveListeners;
			std::vector<ValueFormatter *> formatters;
			std::string tooltip;
			std::unique_ptr<cpl::CSerializer> serializedState;
			std::unique_ptr<ResetListener, DelegatedInternalDelete> mouseResetter;
		};

		typedef CBaseControl::Listener CCtrlListener;

	};
#endif