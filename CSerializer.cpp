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

	file:CSerializer.cpp
 
		Implementation of CSerializer.h

*************************************************************************************/


#include "CSerializer.h"
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <exception>
#include <stdexcept>
#include "stdext.h"
#include "PlatformSpecific.h"
#include "Misc.h"
#include "ProgramVersion.h"
#include "lib/md5/md5.h"


namespace cpl
{

	bool CSerializer::build(const WeakContentWrapper & cr)
	{
		if (!cr.getBlock() || !cr.getSize())
			return false;
		const StdHeader * start = (const StdHeader *)cr.getBlock();

		// a child system is a key that identifies the child followed by 
		// a child entry. Childs without key are invalid.
		bool childHasKey = false;
		// there should only be one data entry.
		bool hasDataBeenFound = false;
		// avoid stupid zero-is-valid-pointer-but-other-numbers-are-not
		// C-leftover
		Key currentKey(1);
		// our iterator
		const StdHeader * current = start;

		// test if we are at top, or the parent of all nodes.
		if (start->type == HeaderType::Start)
		{
			const MasterHeader * master = (const MasterHeader*)start;
			version = master->info.versionID;
			// this can be used to see if things fucked up
			auto totalDataSize = master->info.totalSize;
			// corrupt header.
			if (totalDataSize != cr.getSize())
				throw std::runtime_error("Corrupt header; mismatch between stored size and buffer size at "
					+ std::to_string(cr.getBlock())
					);
			// skip the master header, and go to next entry
			current = current->next();
			// loop over all entries
			for (;;)
			{
				// we must get an end entry before we run out of memory.
				if (current >= (const StdHeader *)(cr.getBlock() + cr.getSize()))
					throw std::runtime_error("No end entry found before end of block at "
						+ std::to_string(cr.getBlock()) + " + "
						+ std::to_string((char*)current - cr.getBlock())
						+ " bytes."
						);
				// interpret data
				switch (current->type)
				{
				case HeaderType::Child:
				{
					auto child = (const StdHeader*)current;
					auto childDataSize = child->dataSize;
					if (!childHasKey)
						throw std::runtime_error("No identifying key for child at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((const char*)current - cr.getBlock())
							+ " bytes."
							);
					auto & cs = getContent(currentKey);
					// build child hierachy.
					cs.build(WeakContentWrapper(child->getData<char>(), childDataSize));

					// remember to reset key:
					childHasKey = false;
					break;
				}
				case HeaderType::Data:
				{
					if (hasDataBeenFound)
						throw std::runtime_error("Multiple data entries found at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((char*)current - cr.getBlock())
							+ " bytes."
							);

					// copy data into our buffer
					data.appendBytes(current->getData<char>(), current->dataSize);

					hasDataBeenFound = true;
					break;
				}
				case HeaderType::End:
				{
					if (childHasKey)
						throw std::runtime_error("No child for previous key at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((char*)current - cr.getBlock())
							+ " bytes."
							);
					if ((const char*)current + current->headerSize != (cr.getBlock() + cr.getSize()))
						throw std::runtime_error("Corrupt header; End entry found at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((char*)current - cr.getBlock())
							+ " bytes, but should be at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string(cr.getSize() - sizeof(StdHeader))
							);
					else
						// memory block parsed successfully!
						return true;
				}
				case HeaderType::Key:
				{
					if (childHasKey)
						throw std::runtime_error("Multiple key-pairs found at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((char*)current - cr.getBlock())
							+ " bytes."
							);
					const KeyHeader * kh = (const KeyHeader *)current;

					// see if we can build the key
					if (!currentKey.build(kh))
						return false;

					childHasKey = true;
					break;
				}
				case HeaderType::Start:
				{
					throw std::runtime_error("Multiple start entries found at "
						+ std::to_string(cr.getBlock()) + " + "
						+ std::to_string((char*)current - cr.getBlock())
						+ " bytes."
						);
				}
				default:
					throw std::runtime_error("Unrecognized HeaderType ("
						+ std::to_string(static_cast<int>(current->type)) + ") at "
						+ std::to_string(cr.getBlock()) + " + "
						+ std::to_string((char*)current - cr.getBlock())
						+ " bytes."
						);
				}

				// iterate to next entry
				current = current->next();
			}
		}
		else // we are a child entry
		{
			// loop over all entries
			for (;;)
			{

				// did we parse all data?
				if (current >= (const StdHeader *)(cr.getBlock() + cr.getSize()))
					return true;
				// interpret data
				switch (current->type)
				{
				case HeaderType::Child:
				{
					auto child = (const StdHeader*)current;
					auto childDataSize = child->dataSize;
					if (!childHasKey)
						throw std::runtime_error("No identifying key for child at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((const char*)current - cr.getBlock())
							+ " bytes."
							);
					auto & cs = getContent(currentKey);
					// build child hierachy.
					cs.build(WeakContentWrapper(child->getData<char>(), childDataSize));

					// remember to reset key:
					childHasKey = false;
					break;
				}
				case HeaderType::Data:
				{
					if (hasDataBeenFound)
						throw std::runtime_error("Multiple data entries found at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((char*)current - cr.getBlock())
							+ " bytes."
							);

					// copy data into our buffer
					data.appendBytes(current->getData<char>(), current->dataSize);

					hasDataBeenFound = true;
					break;
				}
				case HeaderType::End:
				{
					throw std::runtime_error("Corrupt arguments; End entry found as child at "
						+ std::to_string(cr.getBlock()) + " + "
						+ std::to_string((char*)current - cr.getBlock())
						+ " bytes"
						);
				}
				case HeaderType::Key:
				{
					if (childHasKey)
						throw std::runtime_error("Multiple key-pairs found at "
							+ std::to_string(cr.getBlock()) + " + "
							+ std::to_string((char*)current - cr.getBlock())
							+ " bytes."
							);
					const KeyHeader * kh = (const KeyHeader *)current;

					// see if we can build the key
					if (!currentKey.build(kh))
						return false;

					childHasKey = true;
					break;
				}
				case HeaderType::Start:
				{
					throw std::runtime_error("Multiple start entries found at "
						+ std::to_string(cr.getBlock()) + " + "
						+ std::to_string((char*)current - cr.getBlock())
						+ " bytes."
						);
				}
				default:
					throw std::runtime_error("Unrecoginized HeaderType ("
						+ std::to_string(static_cast<int>(current->type)) + ") at "
						+ std::to_string(cr.getBlock()) + " + "
						+ std::to_string((char*)current - cr.getBlock())
						+ " bytes."
						);
				}


				// iterate to next entry
				current = current->next();
			}
		}

	}

	ContentWrapper CSerializer::compile(bool addMasterHeader) const
	{
		BinaryBuilder dataOut;
		// write the master header.
		if (addMasterHeader)
		{
			MasterHeader header;
			header.type = HeaderType::Start;
			header.info.versionID = version.compiled;
			header.dataSize = 0;
			dataOut.appendBytes(&header, sizeof(header));
		}
		// write data (if we have some)
		if (data.getSize())
		{
			// write our header and data
			StdHeader ownData;
			ownData.dataSize = data.getSize();
			ownData.type = HeaderType::Data;
			dataOut.appendBytes(&ownData, sizeof(ownData));
			dataOut.appendBytes(data.getPointer(), data.getSize());
		}
		// write all children.
		for (auto & child : content)
		{
			BinaryBuilder childData;
			StdHeader childHeader;
			ContentWrapper output(child.second.compile());


			childHeader.type = HeaderType::Child;
			childHeader.dataSize = output.getSize();
			childData.appendBytes(&childHeader, sizeof(childHeader));
			childData.appendBytes(output.getBlock(), childHeader.dataSize);


			// generate key, and append it
			auto keyData = child.first.compile();
			dataOut.appendBytes(keyData.getBlock(), keyData.getSize());
			// append the actual child header and it's data
			dataOut.appendBytes(childData.getPointer(), childData.getSize());
		}
		// write the ending point of the data
		if (addMasterHeader)
		{
			StdHeader endData;
			endData.type = HeaderType::End;
			endData.dataSize = 0;
			dataOut.appendBytes(&endData, sizeof(endData));
			// hack in the total size:
			((MasterHeader*)dataOut.getPointer())->info.totalSize = dataOut.getSize();
		}
		return ContentWrapper(dataOut);
	}



	ContentWrapper CCheckedSerializer::compile(bool addMasterHeader) const
	{
		BinaryBuilder b;

		const CSerializer * contentEntry = internalSerializer.findForKey("Content");

		if (contentEntry)
		{
			auto cw = contentEntry->compile(addMasterHeader);

			auto md5 = bzf::md5(cw.getBlock(), cw.getSize());

			CSerializer::MD5CheckedHeader header;

			std::memcpy(header.info, md5.result, md5.size);
			header.dataSize = nameReference.size() + 1;
			header.type = CSerializer::HeaderType::CheckedHeader;

			b.appendBytes(&header, sizeof(header));
			b.appendBytes(nameReference.c_str(), nameReference.size() + 1);
			b.appendBytes(cw.getBlock(), cw.getSize());

			auto totalSize = b.getSize();

			return{ b.acquirePointer(), totalSize };

		}

		CPL_RUNTIME_EXCEPTION("Checked header compilation failed since no \'Content\' entry was found.");
	}

	bool CCheckedSerializer::build(const WeakContentWrapper & cr)
	{
		const std::uint64_t md5SizeInBytes = 16;

		std::uint64_t nameSize = nameReference.size() + 1;

		const CSerializer::MD5CheckedHeader * startHeader
			= reinterpret_cast<const CSerializer::MD5CheckedHeader*>(cr.getBlock());

		if (startHeader->dataSize != nameSize)
			CPL_RUNTIME_EXCEPTION("Checked header's name size is different from this (" + nameReference + ")");

		if (startHeader->type != CSerializer::HeaderType::CheckedHeader)
			CPL_RUNTIME_EXCEPTION("Header was expected to contain MD5 checksum, but is unrecognizable "
				"(expected type " + std::to_string((std::uint16_t)CSerializer::HeaderType::CheckedHeader) + ", was " + std::to_string((std::uint16_t)startHeader->type) + ".");

		if (startHeader->headerSize != CSerializer::MD5CheckedHeader().headerSize)
			CPL_RUNTIME_EXCEPTION("Checked header has invalid size.");

		if (std::memcmp(startHeader->getData<char>(), nameReference.c_str(), static_cast<std::size_t>(nameSize)) != 0)
			CPL_RUNTIME_EXCEPTION(
				std::string("Checked header's name \'") + startHeader->getData<char>() + "\' is different from expected \'" + nameReference + "\'."
				);

		const void * dataBlock = startHeader->next();
		std::size_t dataSize = static_cast<std::size_t>(cr.getSize() - ((const char *)startHeader->next() - (const char *)startHeader));

		auto md5 = bzf::md5(dataBlock, dataSize);

		if (startHeader->info != md5)
			CPL_RUNTIME_EXCEPTION("Checked header for " + nameReference + "'s MD5 checksum is wrong!");

		// build self
		auto ret = internalSerializer.getContent("Content").build({ dataBlock, dataSize });

		// must contain 'content'
		return ret && internalSerializer.findForKey("Content");

	}
};