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

	file:SafeSerializableObject.h
 
		Implementation of SafeSerializableObject.h

*************************************************************************************/

#include "SafeSerializableObject.h"
#include "Protected.h"
#include "CViews.h"
#include "CBaseControl.h"

namespace cpl
{

	bool SafeSerializableObject::serializeObject(CSerializer::Archiver & ar, long long int version)
	{
		bool serializedCorrectly = false;

		const char * options = "Do you want to propagate the error, "
			"potentially crashing the program (YES), ignore the error and keep the changes as is - no guarantees about object behaviour - (NO) "
			"or null out the stored settings for this object (CANCEL)?";

		try
		{
			// this should really not throw, but oh well!
			CProtected::instance().runProtectedCode(
				[&]() 
				{
					serialize(ar, version);
				}
			);

			serializedCorrectly = true;
		}
		catch (const CProtected::CSystemException & e)
		{
			auto const exceptionString =
				"System exception while trying to serialize " + tryComposeIdentifiableName() + ": " +
				CProtected::formatExceptionMessage(e);

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe exception is unknown from an unknown place, and is very dangerous.\n\n" + options,
				programInfo.name + ": Error saving data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					e.reraise();
					break;
				case Misc::bCancel:
					ar.clear();
			}
		}
		catch (const Misc::CPLRuntimeException & e)
		{
			auto const exceptionString =
				"Exception while trying to serialize " + tryComposeIdentifiableName() + ": " + e.what();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe exception is intrinsic to this program, however it can still potentially be dangerous.\n\n" + options,
				programInfo.name + ": Error saving data", Misc::MsgStyle::sYesNo | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
					ar.clear();
			}
		}
		catch (const std::exception & e)
		{
			auto const exceptionString =
				"Exception while trying to serialize " + tryComposeIdentifiableName() + ": " + e.what();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe exception is unknown from an unknown place, and is probably dangerous.\n\n" + options,
				programInfo.name + ": Error saving data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
					ar.clear();
			}
		}
		catch (...)
		{
			auto const exceptionString =
				"Unknown exception while trying to serialize " + tryComposeIdentifiableName();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString + ", continuing is most likely very dangerous.\n\n" + options,
				programInfo.name + ": Error saving data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
					ar.clear();
			}
		}

		return serializedCorrectly;
	}

	bool SafeSerializableObject::deserializeObject(CSerializer::Builder & ar, long long int version)
	{
		using namespace std;
		bool deserializedCorrectly = false;
		CSerializer::Builder safeState;
		serializeObject(safeState, version);

		const std::string versionComparison = "This software is version " + std::to_string(programInfo.versionInteger) 
			+ ", while the serialized data is from version " + std::to_string(version) + ".\n";

		const std::string options = versionComparison + "Do you want to propagate the error, "
			"potentially crashing the program (YES), ignore the error and keep the changes as is - no guarantees about object behaviour - (NO) "
			"or revert the changes to the last known, safe state (CANCEL)?";

		try
		{
			CProtected::instance().runProtectedCode(
				[&] () {
					deserialize(ar, version);
				}
			);


			deserializedCorrectly = true;
		}
		catch (const CProtected::CSystemException & e)
		{
			auto const exceptionString =
				"System exception while trying to deserialize " + tryComposeIdentifiableName() + ": " +
				CProtected::formatExceptionMessage(e);

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe exception is unknown from an unknown place, and is very dangerous.\n\n" + options,
				programInfo.name + ": Error loading data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					e.reraise();
					break;
				case Misc::bCancel:
				{
					deserialize(safeState, version);
				}
			}
		}
		catch (const CSerializer::ExhaustedException & e)
		{
			auto const exceptionString =
				"Exception while trying to deserialize " + tryComposeIdentifiableName() + ": " + e.what();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe serialized data was exhausted before compled serialization. "
				"This is probably not a serious error.\n\n" + options,
				programInfo.name + ": Error loading data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
				{
					deserialize(safeState, version);
				}
			}
		}
		catch (const Misc::CPLRuntimeException & e)
		{
			auto const exceptionString =
				"Exception while trying to deserialize " + tryComposeIdentifiableName() + ": " + e.what();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe exception is intrinsic to this program, however it can still potentially be dangerous.\n\n" + options,
				programInfo.name + ": Error loading data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
				{
					deserialize(safeState, version);
				}
			}
		}
		catch (const std::exception & e)
		{
			auto const exceptionString =
				"Exception while trying to deserialize " + tryComposeIdentifiableName() + ": " + e.what();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString +
				"\nThe exception is unknown from an unknown place, and is probably dangerous.\n\n" + options,
				programInfo.name + ": Error loading data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
				{
					deserialize(safeState, version);
				}
			}
		}
		catch (...)
		{
			auto const exceptionString =
				"Unidentifiable exception while trying to deserialize " + tryComposeIdentifiableName();

			Misc::LogException(exceptionString);

			int userAnswer = Misc::MsgBox(
				exceptionString + ", continuing is most likely very dangerous.\n\n" + options,
				programInfo.name + ": Error loading data", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning
			);

			switch (userAnswer)
			{
				case Misc::bYes:
					throw;
					break;
				case Misc::bCancel:
				{
					deserialize(safeState, version);
				}
			}
		}

		return deserializedCorrectly;
	}

	std::string SafeSerializableObject::tryComposeIdentifiableName()
	{
		auto typeName = Misc::DemangledTypeName(*this);

		if (auto control = dynamic_cast<CBaseControl *>(this))
		{
			return "(" + typeName + "*) UI control \"" + control->bGetTitle() + "\"";
		}
		else if (auto view = dynamic_cast<CView *>(this))
		{
			return "(" + typeName + "*) view \"" + view->getName() + "\"";
		}
		else
		{
			return "(" + typeName + "*) object";
		}
	}

};
