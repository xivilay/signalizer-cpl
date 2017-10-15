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

	file:SerializerModifiers.h

		Provides stream insertion modifiers that alter the behaviour of serialization.

*************************************************************************************/

#ifndef CPL_SERIALIZERMODIFIERS_H
#define CPL_SERIALIZERMODIFIERS_H

#include "CSerializer.h"

namespace cpl
{
	namespace Serialization
	{
		/// <summary>
		/// Scope of serializer must out-live this modifier.
		/// Reverses any changes done. Must only be inserted once.
		/// </summary>
		class ScopedModifier
		{
		public:
			explicit ScopedModifier(CSerializer::Modifiers m, bool doSet = true)
				: mod(m), stream(nullptr), set(doSet) {}

			void modifyStream(CSerializer & s)
			{
				bool reset = s.getModifier(mod);
				s.modify(mod, set);
				set = reset;
				stream = &s;
			}

			~ScopedModifier()
			{
				if (stream)
					stream->modify(mod, set);
			}

		private:
			CSerializer * stream;
			CSerializer::Modifiers mod;
			bool set;
		};

		class Reserve
		{
		public:
			explicit Reserve(std::size_t sizeBytes) : bytes(sizeBytes) {}
			std::size_t getBytes() const noexcept { return bytes; }
		private:
			std::size_t bytes;
		};

		typedef Reserve Consume;


		inline CSerializer & operator << (CSerializer & s, Reserve b)
		{
			s.fill(b.getBytes());
			return s;
		}

		inline CSerializer & operator << (CSerializer & s, ScopedModifier && m)
		{
			m.modifyStream(s);
			return s;
		}

		inline CSerializer & operator << (CSerializer & s, ScopedModifier & m)
		{
			m.modifyStream(s);
			return s;
		}

		inline CSerializer & operator >> (CSerializer & s, Consume b)
		{
			s.discard(b.getBytes());
			return s;
		}
	};
};
#endif