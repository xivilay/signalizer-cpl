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

#ifndef _CPLISTEDITOR_H
	#define _CPLISTEDITOR_H

	#include "Common.h"

	namespace cpl
	{
		namespace PList
		{
			class Property
			{
			protected:
				void setElement(XmlElement * key) { element = key; }
				Property() : element(nullptr) {};
				XmlElement * element;
			public:
				Property(XmlElement * key) : element(key) {}

				virtual ~Property() {}
				bool exists() { return element != nullptr; }

				Property getKey(const std::string & key)
				{
					if(element)
					{
						auto & parent = *element;
						forEachXmlChildElement(parent, childKey)
						{
							//std::cout << childKey->getNamespace() << " == " << key << childKey->getNextElement()->getAllSubText() <<  std::endl;
							if(childKey->getNamespace() == key ||childKey->getAllSubText() == key)
							{
								return childKey;
							}
						}
					}
					return Property();
				}
				bool setValue(const std::string & valueString)
				{
					if(element)
					{
						auto value = element->getNextElement();
						if(value)
						{
							auto child = value->getFirstChildElement();
							if(child && child->isTextElement())
							{
								child->setText(valueString);
								return true;
							}
						}
					}
					return false;
					
				}
				
				Property getValue()
				{
					if(element)
					{
						auto value = element->getNextElement();
						if(value)
						{
							auto child = value->getFirstChildElement();
							if(child && child->isTextElement())
								return child;
							else
								return value;
						}
					}
					return Property();
				}
				std::string toString()
				{
					if(element)
					{
						if(element->isTextElement())
							return element->getText().toStdString();
						else
							return element->getNamespace().toStdString();
						
						
					}
					return "";
				}
			};
		};
		
		class CPListEditor : public PList::Property
		{
		private:
			juce::File plist;
			ScopedPointer<XmlElement> xml;
		public:
			
			
			
			CPListEditor(juce::File list)
				: plist(list)
			{
			}
			virtual ~CPListEditor() {}
			bool parse()
			{
				xml = element = XmlDocument::parse(plist);
				return exists();
			}
			
			bool editKey(const std::string key, const std::string value)
			{
				if(xml)
				{
					auto & parent = *xml->getFirstChildElement();
					forEachXmlChildElement(parent, childKey)
					{
						if(childKey->getAllSubText() == key)
						{
							auto valueEntry = childKey->getNextElement();
							if(valueEntry)
							{
								auto child = valueEntry->getFirstChildElement();
								if(child && child->isTextElement())
								{
									child->setText(value);
									return true;
								}
							}
							
						}
						
						
					}
				}
				return false;
			}
			
			bool save()
			{
				if (xml) {
					return xml->writeToFile(plist, getplistDTD());
				}
				else
					return false;
			}
			bool saveAs(juce::File location)
			{
				if (xml) {
					return xml->writeToFile(location, getplistDTD());
				}
				else
					return false;
				
			}
			const char * getplistDTD()
			{
				return "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">";
			}
		};
	}
#endif