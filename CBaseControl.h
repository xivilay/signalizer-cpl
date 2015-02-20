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

	file:CBaseControl.h
		
		CBaseControl - base class for all implementable controls.
		all methods prefixed with 'b' to avoid future nameclashing.
		This class provides a common interface for all 'controls' to support
		normalized value-, string and position get/set.
		Also encapsulates listeners into a single one.
		Provides a cheap RAII mutex lock as well, as well as optional reference counting.
		Does not derive from the system's base class for graphics to avoid the diamond
		problem (classes deriving from this should set the base class in their constructor)

*************************************************************************************/

#ifndef _CBASECONTROL_H
	#define _CBASECONTROL_H

	#include "CMutex.h"
	#include "Common.h"
	#include <memory>
	#include "CToolTip.h"
	#include <vector>
	#include "stdext.h"
	#include "Utility.h"
	#include "gui/DesignBase.h"
	#include "CSerializer.h"

	namespace cpl
	{
		// the internal precision of all controls
		typedef double iCtrlPrec_t;

		/*
			The base class of all controls represented through
			a graphical interface.
		*/
		class CBaseControl;
		class CSerializer;
		class CCtrlEditSpace;
		class CBaseControl;

		//-----------------------------------------------------------------------------
		// CReferenceCounter Declaration (Reference Counting)
		// This class is from VSTGUI 3.6 (removed in 4+)
		//-----------------------------------------------------------------------------
		class CReferenceCounter2
		{
		public:
			CReferenceCounter2() : nbReference(1) {}
			virtual ~CReferenceCounter2() {}

			virtual void forget() { nbReference--; if (nbReference == 0) delete this; }
			virtual void remember() { nbReference++; }
			long getNbReference() const { return nbReference; }
	
		private:
			long nbReference;
		};

		// reference counter used for all controls
		typedef CReferenceCounter2 refCounter;

		/*********************************************************************************************

			The base class of all controls supported in this SDK that are represented through a 
			graphic interface.

		*********************************************************************************************/
		class CBaseControl 
		: 
			public refCounter,
			public CMutex::Lockable,
			public juce::Slider::Listener,
			public juce::Button::Listener,
			public juce::ScrollBar::Listener,
			public juce::ComboBox::Listener,
			public CToolTipClient,
			public Utility::DestructionServer<CBaseControl>,
			public CSerializer::Serializable
		{
		public:

			//friend CSerializer::Archiver & operator << (CSerializer::Archiver & left, CBaseControl * right);
			//friend CSerializer::Builder & operator >> (CSerializer::Builder & left, CBaseControl * right);

			/*********************************************************************************************

				A CBaseControl can call a listener back on events.
				A listener can override the controls own event handler by returning true from valueChanged
				A return value of false will cause the next handler in the chain to handle the event.
				Otherwise, the event is considered handled, and no other handlers will be called.

				There will be a default handler. The handlers are called from newest added to the first.

			*********************************************************************************************/
			class Listener : virtual public Utility::DestructionServer<CBaseControl>::Client
			{
			public:
				virtual bool valueChanged(CBaseControl *) = 0;
				virtual ~Listener() {};
			};
			/*********************************************************************************************

				The same as a Listener, however it is NOT able to change the control's status, and
				it will be called AFTER the listener change.

			*********************************************************************************************/
			class PassiveListener : virtual public Utility::DestructionServer<CBaseControl>::Client
			{
			public:
				virtual void valueChanged(const CBaseControl *) = 0;
				virtual ~PassiveListener() {};
			};

			/*********************************************************************************************

				Most controls have an associated string-field that maps the internal value to a
				meaningful human-readable string. A control having the range 0-1 can represent -100 to 0 dB,
				for example. This can be manually set using bSetText(). Note that the control may
				independently change this (even inbetween paints).
				A return value of false will cause the next handler in the chain to handle the event.
				Otherwise, the event is considered handled, and no other handlers will be called.
				Add a valueformatter to this class to handle mapping of values to strings.

				There will be a default handler. The handlers are called from newest added to the first.

			*********************************************************************************************/
			class ValueFormatter : virtual public Utility::DestructionServer<CBaseControl>::Client
			{
			public:
				virtual bool stringToValue(const CBaseControl * ctrl, const std::string & buffer, iCtrlPrec_t & value) = 0;
				virtual bool valueToString(const CBaseControl * ctrl, std::string & buffer, iCtrlPrec_t value) = 0;
				virtual ~ValueFormatter() {};
			};

		protected:
			/*
				All controls have a unique associated tag.
			*/
			long tag;
			/*
				Whether the control is attached to anything
			*/
			bool isAttached, tipsEnabled;
			/*
				The implementation specific context of controls
			*/
			GraphicComponent * base;
			/*
				The callback listeners
			*/
			std::vector<Listener *> listeners;
			/*
				The passive callback listeners
			*/
			std::vector<PassiveListener *> passiveListeners;
			/*
				The value formatters
			*/
			std::vector<ValueFormatter *> formatters;
			/*
				The description of this control to show in a tooltip
			*/
			std::string tooltip;
			/*
				Whether this control allows to spawn edit spaces
			*/
			bool isEditSpacesAllowed;
		public:
			/*********************************************************************************************

				Returns whether the control is attached to anything - deprecated

			*********************************************************************************************/
			bool bIsAttached() const
			{ 
				return isAttached; 
			}
			/*********************************************************************************************

				Constructor

			*********************************************************************************************/
			CBaseControl(GraphicComponent * b) 
				: tag(0), isAttached(false), base(b), isEditSpacesAllowed(false), tipsEnabled(false)

			{
			
			}
			/*********************************************************************************************

				Constructor

			*********************************************************************************************/
			CBaseControl(GraphicComponent * b,  int tag, bool bIsAttached = false) 
				: tag(tag), isAttached(bIsAttached), tipsEnabled(true), base(b), isEditSpacesAllowed(false)
			{
			
			}
			/*********************************************************************************************

				Virtual destructor, of course

			*********************************************************************************************/
			virtual ~CBaseControl()
			{
			}
			/*********************************************************************************************
			 
				Tooltip interface
			 
			 *********************************************************************************************/
			juce::String bGetToolTip() const override
			{
				return tipsEnabled ? (tooltip.size() ? tooltip : bGetTitle()) : "";
			}
			/*********************************************************************************************

				Sets the displayed tooltip. Remember to call enableTooltip(true) if you want to use this.

			*********************************************************************************************/
			void bSetDescription(const std::string & tip)
			{
				tooltip = tip;
			}
			void enableTooltip(bool toggle = true)
			{
				tipsEnabled = toggle;
			}
			/*********************************************************************************************

				Creates an edit space linked to this control. If toggleEditSpace() is set to false,
				it may return a nullptr instead.

			*********************************************************************************************/
			virtual std::unique_ptr<CCtrlEditSpace> bCreateEditSpace();
			/*********************************************************************************************

				Creates an edit space linked to this control. If toggleEditSpace() is set to false,
				it may return a nullptr instead.

			*********************************************************************************************/
			virtual void bToggleEditSpaces(bool toggle)
			{
				isEditSpacesAllowed = toggle;
			}
			/*********************************************************************************************

				Set visibility of control

			*********************************************************************************************/
			void bSetVisible(bool bVisibility)
			{
				if(base)
					base->setVisible(bVisibility);
			}
			/*********************************************************************************************

				Attach to a parent 

			*********************************************************************************************/
			virtual void addToParent(GraphicComponent * parent)
			{
				parent->addChildComponent(base);
			}
			/*********************************************************************************************

				Remove from a parent

			*********************************************************************************************/
			virtual void removeFromParent(GraphicComponent * parent)
			{
				parent->removeChildComponent(base);
			}
			/*********************************************************************************************

				Gets the internal value of the control.
				Ranges from 0.0 to 1.0

			*********************************************************************************************/
			virtual iCtrlPrec_t bGetValue() const 
			{ 
				return iCtrlPrec_t(0.0);
			}
			/*********************************************************************************************

				Formats the value val, and returns true if a conversion was succesful.

			*********************************************************************************************/
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
			/*********************************************************************************************

				Sets the value of the control.
				Input must be between 0.0f and 1.0f, inclusive

			*********************************************************************************************/
			virtual void bSetValue(iCtrlPrec_t val)
			{

			}
			/*********************************************************************************************

				Sets the value of the control, interpretting the string using a formatter,
				and setting the value to the control using bSetValue.
				Returns true if the value was succesfully interpreted and set.

			*********************************************************************************************/
			virtual bool bInterpretAndSet(const std::string & valueString, bool setInternal = false)
			{
				iCtrlPrec_t val(0);
				if (bInterpret(valueString, val))
				{
					if (setInternal)
						bSetInternal(val);
					else
						bSetValue(val);
					return true;
				}
				return false;
			}
			/*********************************************************************************************

				Maps the string to a 0-1 range, if it was succesfully interpreted.

			*********************************************************************************************/
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
			/*********************************************************************************************

				Sets the internal value of the control.
				Input must be between 0.0f and 1.0f, inclusive.
				Guaranteed to have no sideeffects (ie. doesn't call listeners)

			*********************************************************************************************/
			virtual void bSetInternal(iCtrlPrec_t val)
			{
			
			}
			/*********************************************************************************************

				Sets the title of a control.

			*********************************************************************************************/
			virtual void bSetTitle(const std::string & text)
			{
			
			}
			/*********************************************************************************************

				Gets the title of a control

			*********************************************************************************************/
			virtual std::string bGetTitle() const
			{ 
				return ""; 
			}
			/*********************************************************************************************

				Sets the text of the control

			*********************************************************************************************/
			virtual void bSetText(const std::string & text) 
			{
			
			}
			/*********************************************************************************************

				Returns the text of a control

			*********************************************************************************************/
			virtual std::string bGetText() const
			{ 
				return ""; 
			}
			/*********************************************************************************************

				Returns the associated tag with this control

			*********************************************************************************************/
			virtual long bGetTag() const
			{ 
				return tag; 
			}
			/*********************************************************************************************

				Sets the tag of this control

			*********************************************************************************************/
			virtual void bSetTag(long newTag) 
			{
				tag = newTag;
			}
			/*********************************************************************************************

				Returns the size of this control.
				X and Y are relative to the parent coordinates.

			*********************************************************************************************/
			virtual CRect bGetSize() const
			{
				if (base)
					return base->getBounds();
				return CRect();
			}
			/*********************************************************************************************

				Returns the size of this control.
				X and Y are absolute coordinates (that is, relative to the top-level window).

			*********************************************************************************************/
			virtual CRect bGetAbsSize() const
			{
				if (base)
				{
					auto basicBounds = base->getBounds();
					GraphicComponent * parentPointer = base;
					
					while (parentPointer = parentPointer->getParentComponent())
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
			/*********************************************************************************************

				Sets the x, y position of this control

			*********************************************************************************************/
			virtual void bSetPos(int x, int y)
			{
				if (base)
					base->setBounds(x, y, base->getWidth(), base->getHeight());
			}
			/*********************************************************************************************

				Sets the size of this control

			*********************************************************************************************/
			virtual void bSetSize(const CRect & size)
			{
				if (base)
					base->setBounds(size);
				// set pos?
			}
			/*********************************************************************************************

				Called before any repainting is done.
				If a control wishes to repaint itself, it should call the relevant repaint command
				(usually base->repaint())

			*********************************************************************************************/
			virtual void bRedraw() 
			{ 
			
			}
			/*********************************************************************************************
			 
				Get the view / component associated with this control
			 
			 *********************************************************************************************/
			GraphicComponent * bGetView() const
			{
				return base;
			}
			/*********************************************************************************************

				Internal use

			*********************************************************************************************/
			void setDirty()
			{
				bRedraw();
			}
			/*********************************************************************************************

				Adds a listener (if not present) to be called back on value changes

			*********************************************************************************************/
			void bAddChangeListener(Listener * listener)
			{
				if (listener && !std::contains(listeners, listener))
				{
					listeners.push_back(listener);
					addClientDestructor(listener);
				}
			}
			/*********************************************************************************************

				Removes a change-listener (if present)

			*********************************************************************************************/
			void bRemoveChangeListener(Listener * listener)
			{
				auto it = std::find(listeners.begin(), listeners.end(), listener);
				if (it != listeners.end())
				{
					listeners.erase(it);
				}
			}

			/*********************************************************************************************

			Adds a passive listener (if not present) to be called back on value changes

			*********************************************************************************************/
			void bAddPassiveChangeListener(PassiveListener * listener)
			{
				if (listener && !std::contains(passiveListeners, listener))
				{
					passiveListeners.push_back(listener);
					addClientDestructor(listener);
				}
			}
			/*********************************************************************************************

			Removes a change-listener (if present)

			*********************************************************************************************/
			void bRemovePassiveChangeListener(PassiveListener * listener)
			{
				auto it = std::find(passiveListeners.begin(), passiveListeners.end(), listener);
				if (it != passiveListeners.end())
				{
					passiveListeners.erase(it);
				}
			}

			/*********************************************************************************************

				Adds a formatter (if not present)
				
			*********************************************************************************************/
			void bAddFormatter(ValueFormatter * formatter)
			{
				if (!std::contains(formatters, formatter))
				{
					formatters.push_back(formatter);
					addClientDestructor(formatter);
				}
			}
			/*********************************************************************************************

				Removes a formatter (if not present)

			*********************************************************************************************/
			void bRemoveFormatter(ValueFormatter * formatter)
			{
				auto it = std::find(formatters.begin(), formatters.end(), formatter);
				if (it != formatters.end())
				{
					formatters.erase(it);
				}
			}
			/*********************************************************************************************

				Force a valueChanged event, and a callback

			*********************************************************************************************/
			virtual void bForceEvent()
			{
				postEvent();
			}
			/*********************************************************************************************

				The control's internal event callback. This is called whenever the controls value is changed.

			*********************************************************************************************/
			virtual void onValueChange() 
			{

			}

			/*********************************************************************************************

				Serialization

			*********************************************************************************************/
			virtual void save(CSerializer::Archiver & ar, long long int version) override
			{
				ar << bGetValue();
			}
			virtual void load(CSerializer::Builder & ar, long long int version) override
			{
				iCtrlPrec_t value(0);

				ar >> value;
				bSetValue(value);
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
				#ifdef __MSVC__
					sprintf_s(buf, "%.17g", cpl::Math::confineTo<iCtrlPrec_t>(val, 0.0, 1.0));
				#else
					snprintf(buf, sizeof(buf), "%.17g", cpl::Math::confineTo<iCtrlPrec_t>(val, 0.0, 1.0));
				#endif
				buf[sizeof(buf) - 1] = '\0';

				stringBuf = buf;
				return true;
			}

			protected:
				/*
					some layers of indirection is needed here, since juce doesn't
					share the same class for listeners.
					the base object works as a proxy that turns all the events
					into cbasecontrol listener callbacks.
				*/

				virtual void buttonClicked(juce::Button * c)
				{
					postEvent();
				}
				virtual void sliderValueChanged(juce::Slider * c)
				{
					postEvent();
				}
				virtual void scrollBarMoved(juce::ScrollBar * c, double newRange)
				{
					postEvent();
				}
				virtual void comboBoxChanged(juce::ComboBox * c)
				{
					postEvent();
				}

				/*
					Basic formatters.
				*/
				virtual bool bStringToValue(const std::string & stringInput, iCtrlPrec_t & val) const
				{
					return bMapStringToInternal(stringInput, val);
				}
				virtual bool bValueToString(std::string & stringBuf, iCtrlPrec_t  val) const
				{
					return bMapIntValueToString(stringBuf, val);
				}

			private:

				void postEvent()
				{
					if (!listenerChangeHandling())
						onValueChange();
					notifyPassiveListeners();
				}

				void notifyPassiveListeners()
				{
					for (auto const & listener : passiveListeners)
						listener->valueChanged(this);
				}

				bool listenerChangeHandling()
				{
					for (auto rit = listeners.rbegin(); rit != listeners.rend(); ++rit)
					{
						if ((*rit)->valueChanged(this))
							return true;
					}
					return false;
				}
		};

		typedef CBaseControl::Listener CCtrlListener;

		/*
			Global operators
		*/

		//CSerializer::Archiver & operator << (CSerializer::Archiver & left, CBaseControl * right);
		//CSerializer::Builder & operator >> (CSerializer::Builder & left, CBaseControl * right);
	};
#endif