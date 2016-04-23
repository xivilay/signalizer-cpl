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

	file:CBaseControl.cpp

		Implementation of some stuff, avoiding cyclic dependencies

*************************************************************************************/
#include "CBaseControl.h"
#include "CCtrlEditSpace.h"

namespace cpl
{

	class ResetListener : public juce::MouseListener, juce::ComponentListener
	{
	public:

		ResetListener(CBaseControl & controlToRecieveNotifications)
			: parent(controlToRecieveNotifications), doUnregister(true)
		{
			parent.bGetView()->addMouseListener(this, true);
			parent.bGetView()->addComponentListener(this);
		}

		virtual ~ResetListener()
		{
			if (doUnregister)
			{
				parent.bGetView()->removeMouseListener(this);
				parent.bGetView()->removeComponentListener(this);
			}
		}
	private:



		virtual void componentBeingDeleted(Component& component) override
		{
			if (&component == parent.bGetView()) doUnregister = false;
		}
		virtual void mouseDown(const juce::MouseEvent & e) override
		{
			if (e.mods.testFlags(ModifierKeys::altModifier))
			{
				if (e.eventComponent == parent.bGetView())
				{
					parent.bResetToDefaultState();
					return;
				}

				// whatever was clicked is nested arbitrarily down in a cbasecontrl
				// walk upwards in component hierachies until we find a cbasecontrol
				for (auto componentParent = e.eventComponent; componentParent; componentParent = componentParent->getParentComponent())
				{
					if (auto other = dynamic_cast<CBaseControl *>(componentParent))
					{
						// we could loop further or reset the whole widget
						// but it makes more sense to ignore the reset event
						// if the target component doesn't want to be reset.
						if (!other->bIsDefaultResettable())
							return;

						// - if it isn't us, it will either 
						// a) have handled the event itself
						// b) ignored it cause of reasons we will respect
						// this should really have been written in a system
						// that allows to handle events so they don't propagate
						// all over the place without state
						if (other == &parent)
							other->bResetToDefaultState();

						return;
					}
				}

				CPL_RUNTIME_ASSERTION(false && "Mouse reset listener attached to some CBaseControl was notified of event, "
					"but no relevant parent existed in component hierachy.");
				parent.bResetToDefaultState();

			}
		}
		bool doUnregister;
		CBaseControl & parent;
	};


	CBaseControl::~CBaseControl()
	{
	
	}

	void DelegatedInternalDelete::operator()(ResetListener *p) const noexcept
	{
		delete p;
	}

	void CBaseControl::bSetIsDefaultResettable(bool shouldBePossible)
	{
		if (shouldBePossible)
		{
			if (!bIsDefaultResettable())
			{
				serializedState.reset(new CSerializer());
				mouseResetter.reset(new ResetListener(*this));
			}
		}
		else
		{
			serializedState.reset(nullptr);
		}
	}


	std::unique_ptr<CCtrlEditSpace> CBaseControl::bCreateEditSpace()
	{
		if (isEditSpacesAllowed)
			return std::unique_ptr<CCtrlEditSpace>(new CCtrlEditSpace(this));
		else
			return nullptr;
	}
};