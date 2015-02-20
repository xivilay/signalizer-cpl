/*************************************************************************************

	Audio Programming Environment VST. 
		
		VST is a trademark of Steinberg Media Technologies GmbH.

    Copyright (C) 2013 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:CPresetManager.cpp
		
		Implementation of CPresetManager

*************************************************************************************/

#include "CPresetManager.h"
#include "Misc.h"
#include <vector>

namespace cpl
{

	auto presetDirectory = []() { return cpl::Misc::DirectoryPath() + "/presets/"; };
	

	CPresetManager & CPresetManager::instance()
	{
		static CPresetManager ins;
		return ins;
	}

	bool CPresetManager::savePresetAs(const CSerializer::Archiver & archive, juce::File & location)
	{
		// should we really save empty files?
		if (archive.isEmpty())
			return false;

		juce::FileChooser fileChooser(programInfo.name + ": Save preset to a file...", 
			juce::File(presetDirectory()), "*." + programInfo.programAbbr);

		if (fileChooser.browseForFileToSave(true))
		{
			auto result = fileChooser.getResult();

			std::string path = result.withFileExtension(programInfo.programAbbr.c_str()).getFullPathName().toStdString();

			if (!savePreset(path, archive, location))
			{
				auto userAnswer = Misc::MsgBox("Error opening file:\n" + path + "\nTry another location?",
					programInfo.name + "Error saving preset to file...", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning);
				if (userAnswer == Misc::MsgButton::bYes)
				{
					return savePresetAs(archive, location);
				}
				else
				{
					return false;
				}
			}

		}

		return true;
	}
	bool CPresetManager::loadPresetAs(CSerializer::Builder & builder, juce::File & location)
	{

		juce::FileChooser fileChooser(programInfo.name + ": Save preset to a file...",
			juce::File(presetDirectory()), "*." + programInfo.programAbbr);

		if (fileChooser.browseForFileToOpen())
		{
			auto result = fileChooser.getResult();
			// the file chooser can only choose file names with the correct extension,
			// no need to check.
			std::string path = result.getFullPathName().toStdString();

			if (!result.existsAsFile() || !loadPreset(path, builder, location))
			{
				auto userAnswer = Misc::MsgBox("Error opening file:\n" + path + "\nTry another location?",
					programInfo.name + "Error loading preset from file...", Misc::MsgStyle::sYesNoCancel | Misc::MsgIcon::iWarning);
				if (userAnswer == Misc::MsgButton::bYes)
				{
					return loadPresetAs(builder, location);
				}
				else
				{
					return false;
				}
			}
			else
			{
				return true;
			}

		}

		return false;
	}

		// these functions saves/loads directly
	bool CPresetManager::savePreset(const std::string & path, const CSerializer::Archiver & archive, juce::File & location)
	{
		CExclusiveFile file;
		
		if (!file.open(path, file.writeMode))
			return false;

		// clear existing file..
		file.remove();

		if (!file.open(path))
			return false;

		auto content = archive.compile();

		if (file.write(content.getBlock(), content.getSize()))
		{
			location = path;
			return true;
		}

		return false;
	}
	bool CPresetManager::loadPreset(const std::string & path, CSerializer::Builder & builder, juce::File & location)
	{
		try
		{
			CExclusiveFile file;

			if (!file.open(path, file.readMode))
				return false;

			std::vector<std::uint8_t> data;
			std::size_t size = file.getFileSize();
			data.resize(size);

			if (!file.read(data.data(), size))
				return false;

			builder.clear();
			if (builder.build(CSerializer::WeakContentWrapper(data.data(), size)))
			{
				location = path;
				return true;
			}
		}
		catch (const std::exception & e)
		{
			Misc::MsgBox("Exception loading preset at " + path + ":\n" + e.what(), programInfo.name, Misc::MsgIcon::iStop);
		}
		return false;
	}
	const std::vector<juce::File> & CPresetManager::getPresets()
	{
		currentPresets.clear();
		juce::DirectoryIterator iter(File(presetDirectory()), false, "*." + programInfo.programAbbr);
		while (iter.next())
		{
			currentPresets.push_back(iter.getFile());
			
		}

		return currentPresets;
	}
	bool CPresetManager::saveDefaultPreset(const CSerializer::Archiver & archive, juce::File & location)
	{
		return savePreset(presetDirectory() + "default." + programInfo.programAbbr, archive, location);
	}

	bool CPresetManager::loadDefaultPreset(CSerializer::Builder & builder, juce::File & location)
	{
		auto path = presetDirectory() + "default." + programInfo.programAbbr;
		if (!loadPreset(path, builder, location))
		{
			auto answer = cpl::Misc::MsgBox(
				"Error loading default preset at:\n" + path + "\n" + Misc::GetLastOSErrorMessage() +
				"\nLoad a different preset?",
				programInfo.name + ": Error loading preset...",
				Misc::MsgIcon::iQuestion | Misc::MsgStyle::sYesNoCancel);
			if (answer == Misc::MsgButton::bYes)
			{
				return loadPresetAs(builder, location);
			}
			else
			{
				return false;
			}
			
		}
		return true;
	}

	CPresetManager::CPresetManager() {}

	CPresetManager::~CPresetManager() {}

};