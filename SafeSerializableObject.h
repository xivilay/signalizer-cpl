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
 
		Defines an interface wrapped around standard serializable objects,
		that guards the loading / saving code, and asks the user what to do in case 
		of errors. Intended for top-level objects.

*************************************************************************************/

#ifndef CPL_SAFESERIALIZABLEOBJECT_H
	#define CPL_SAFESERIALIZABLEOBJECT_H

	#include "CSerializer.h"

	namespace cpl
	{

		class SafeSerializableObject : private CSerializer::Serializable
		{
		public:

			/// <summary>
			/// The end-user is responsible for whatever is serialized, so you have to trust that person.
			/// The return code indicates if there were errors, though.
			/// </summary>
			bool serializeObject(CSerializer::Archiver & ar, Version version);

			/// <summary>
			/// The end-user is responsible for whatever is serialized, so you have to trust that person.
			/// The return code indicates if there were errors, though.
			/// </summary>
			bool deserializeObject(CSerializer::Builder & ar, Version version);

		private:

			std::string tryComposeIdentifiableName();
		};

		inline CSerializer::Archiver & operator << (CSerializer::Archiver & left, SafeSerializableObject & right)
		{
			right.serializeObject(left, left.getMasterVersion());
			return left;
		}

		inline CSerializer::Builder & operator >> (CSerializer::Builder & left, SafeSerializableObject & right)
		{
			right.deserializeObject(left, left.getMasterVersion());
			return left;
		}

		inline CSerializer::Archiver & operator << (CSerializer::Archiver & left, SafeSerializableObject * right)
		{
			right->serializeObject(left, left.getMasterVersion());
			return left;
		}

		inline CSerializer::Builder & operator >> (CSerializer::Builder & left, SafeSerializableObject * right)
		{
			right->deserializeObject(left, left.getMasterVersion());
			return left;
		}

	};
#endif