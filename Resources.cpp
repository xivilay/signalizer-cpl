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

	file:Resources.cpp
 
		Implementation of Resources.h

*************************************************************************************/

#include "Resources.h"
namespace cpl
{
	std::mutex CResourceManager::loadMutex;
	std::atomic<CResourceManager *> CResourceManager::internalInstance { nullptr };

	/*********************************************************************************************
	 
	 CImage
	 
	 *********************************************************************************************/
	CImage::CImage(const std::string & inPath)
	: path(inPath)
	{
		
	}
	
	CImage::CImage()
	{
	}
	
	void CImage::setPath(const std::string & inPath)
	{
		path = inPath;
		
	}
	
	bool CImage::load()
	{
		juce::File f(path);
		
		// handle scalable vector graphics
		if (f.getFileExtension() == ".svg")
		{
			internalImage = juce::Image::null;
			ScopedPointer<juce::XmlElement> element = juce::XmlDocument::parse(f);
			if (element.get())
			{
				drawableImage = juce::Drawable::createFromSVG(*element);
				return true;
			}
			else
			{
				auto drawable = new juce::DrawableImage();
				drawable->setImage(juce::Image::null);
				drawableImage = drawable;
			}
		}
		else
		{
			internalImage = juce::ImageFileFormat::loadFrom(f);

			if (internalImage.isValid()) {
				auto drawable = new juce::DrawableImage();
				drawable->setImage(internalImage);
				drawableImage = drawable;
				return true;
			}
			else
			{
				// set a default image?

				auto drawable = new juce::DrawableImage();
				drawable->setImage(juce::Image::null);
				drawableImage = drawable;
			}
		}
		return false;
	}
	
	juce::Image & CImage::getImage()
	{
		return internalImage;
		
	}
	
	juce::Drawable * CImage::getDrawable()
	{
		return drawableImage.get();
	}
	
	CImage::~CImage()
	{
	}
	/*********************************************************************************************
	 
	 CResourceManager
	 
	 *********************************************************************************************/
	
	CImage * CResourceManager::loadResource(const std::string & name)
	{
		auto it = resources.find(name);
		if (it != resources.end())
		{
			return &it->second;
		}


		std::string dir = Misc::DirectoryPath() + "/resources/";

		auto & image = resources[name];
		std::string path = (dir + name);
		image.setPath(path);
		if (!image.load())
		{
			Misc::MsgBox("Error loading resource " + path + ":" + newl + Misc::GetLastOSErrorMessage(), programInfo.name + " error!", Misc::MsgIcon::iStop);
			CPL_BREAKIFDEBUGGED();
			return nullptr;
		}

		return &image;
	}
	
	CResourceManager::CResourceManager()
	{
		defaultImage.load();
	};
	
	CResourceManager::~CResourceManager()
	{
		internalInstance = nullptr;
	}

	std::unique_ptr<juce::Drawable> CResourceManager::createDrawable(const std::string & name)
	{
		std::lock_guard<std::mutex> lock(loadMutex);
		CImage * resource = loadResource(name);

		if (!resource)
		{
			#ifdef CPL_THROW_ON_NO_RESOURCE
				CPL_RUNTIME_EXCEPTION("Resource " + name + " was not found. Compile without CPL_THROW_ON_NO_RESOURCE to remove this exception.");
			#endif
			resource = &defaultImage;
		}

		return std::unique_ptr<juce::Drawable>(resource->getDrawable()->createCopy());
	}

	juce::Image CResourceManager::getImage(const std::string & name)
	{
		std::lock_guard<std::mutex> lock(loadMutex);
		CImage * resource = loadResource(name);
		if (!resource)
		{
			#ifdef CPL_THROW_ON_NO_RESOURCE
				CPL_RUNTIME_EXCEPTION("Resource " + name + " was not found. Compile without CPL_THROW_ON_NO_RESOURCE to remove this exception.");
			#endif
			resource = &defaultImage;
		}

		return resource->getImage();
		
	}
	
	CResourceManager & CResourceManager::instance()
	{
		auto instance = internalInstance.load(std::memory_order_acquire);

		if (instance == nullptr)
		{
			std::lock_guard<std::mutex> lock(loadMutex);

			instance = internalInstance.load(std::memory_order_acquire);
			if (instance == nullptr)
			{
				internalInstance.store(new CResourceManager(), std::memory_order_release);
			}
		}

		return * (instance ? instance : internalInstance.load(std::memory_order_acquire));
	}

};