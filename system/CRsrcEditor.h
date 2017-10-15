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

	file:CRsrcEditor.h
		An "interface" for editing AudioUnit binary resource forks on OSX.

*************************************************************************************/

#ifndef CPL_CRSRCEDITOR_H
#define CPL_CRSRCEDITOR_H

#include "Common.h"
#include <cstdint>
#include <sstream>

#ifndef CPL_MAC
#warning "cpl: osx .rsrc resource editor included for non-mac targets"
#endif

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1
#endif
namespace cpl
{

	class CRsrcEditor
	{
		/*
		 Audio Unit .rsrc file (+ indicates resource header (ElementResource) before resource):

		 rsrcHeaderStart;
		 rsrcpadding;
		 resource part 1:
			 + brandAndName : pstr;
			 + description : pstr;
			 + view1 : cstr;
			 + view2 : cstr;
			 + auEntry : cstr;
			 + auView : cstr;
			 + audioUnitDescription : thng;
			 + audioUnitDescription2 : thng;
		 resource part 2:
			rsrcHeaderStart;
			rsrcpart2;
			brandAndName2 : pstr; // notice missing resource header!!
			view3 : pstr; // notice missing resource header!!



		*/
	public:
		enum AudioUnitResource
		{
			brandAndName = 0,
			description



		};
	private:
		#pragma pack(push, 1)
		struct alignas(1) RsrcHeaderStart
		{
			union
			{
				struct
				{
					uint32_t absoluteOffsetToResouceStart;
					uint32_t absoluteOffsetToResourceEnd;
					uint32_t part1Size;
					uint32_t part2Size;
				};
				uint8_t data[16];
			};
		};
		struct alignas(1) Rsrcpadding
		{
			uint8_t _[240]; // seems to be zero
		};
		/*
			Header for a resource. The resource follows immediately after.
			Describes the total size in bytes of the resource.
		*/
		struct alignas(1) ResourceElement
		{
			uint32_t resourceSize;
		};

		/*
			A pascal string resource. Only used for strings < 0xFF size, obviously.
		*/
		struct alignas(1) RsrcPStr
		{
			uint8_t stringLength;
			char data[1];

		};
		/*
			A C-string resource with a terminating NULL character.
		*/
		struct alignas(1) RsrcCStr
		{
			char data[1];
		};
		/*
			The resource part 1 that describes the audio unit
		*/
		struct alignas(1) Rsrcthng1 // sizeof 70 -- NOTE THIS IS KEPT IN MEMORY AS BIGENDIAN
		{
			uint32_t auMaintype;
			uint32_t auSubtype;
			uint32_t manufacturerCode;
			const uint8_t k68Compatible[14]; // always zero
			const uint8_t stringID1[3]; // 'str'
			const uint8_t always32_1;
			const uint16_t resid;
			const uint8_t stringID2[3]; // 'str'
			const uint8_t always32_2;
			const uint16_t residPlusOne;
			const uint32_t icon[2];
			uint16_t version;
			uint8_t _[22];
		};
		/*
			Same as first part, except the 'aumainpart' field seems to always be 'auvw'
		*/
		struct alignas(1) Rsrcthng2 // sizeof 70 -- NOTE THIS IS KEPT IN MEMORY AS BIGENDIAN
		{
			const int32_t alwaysauvw;
			int32_t auSubtype;
			int32_t manufacturerCode;
			uint8_t _1[34];
			uint16_t version;


			uint8_t _2[22];
		};
		/*
			header/type descriptions of these previous structs (i think)
		*/
		struct alignas(1) Rsrcpart2 // sizeof 134
		{
			union
			{
				struct
				{
					union
					{
						struct
						{
							uint32_t zero1; // 0x0
							uint32_t elementCount; // au: 0x09000000
							uint32_t _1; // 0x001C0096
							uint16_t elDe; // 0x02
							uint8_t _str[3];
							uint16_t mresID; // 0x2000
						};
						uint8_t staticpart1[38];
					};
					struct {
						uint16_t resID; // 2000 and 2001 decimal
						uint16_t allbitsset; // usually 0xFFFF | 0x0000
						uint16_t maybeZero; // 0x2000 | 0x0000
						uint16_t offset; // where the resource is found (resourceheader.start + offset = this resource)
						uint32_t defiZero;
					} elements[8];
				};


				// raw bytes
				uint8_t _[134];

			};


		};
		#pragma pack(pop)
		struct
		{
			// --- part 1
			RsrcHeaderStart headerStart;
			Rsrcpadding pad;
			ResourceElement header;
			RsrcPStr * brandAndName, *view1, *view2, *description;
			RsrcCStr  * auEntry, *auViewEntry;
			Rsrcthng1 * thng1;
			Rsrcthng2 * thng2;

			// --- part 2
			Rsrcpart2 * part2;
			RsrcPStr * brandAndName2, *view3;
		} contents;

		std::size_t totalSize;
	protected:
		juce::File filePath;
		bool loaded;
	public:
		CRsrcEditor()
			: contents(), loaded(false)
		{

		}

		~CRsrcEditor()
		{
			if (contents.brandAndName)
				std::free(contents.brandAndName);
			if (contents.brandAndName2)
				std::free(contents.brandAndName2);
			if (contents.view1)
				std::free(contents.view1);
			if (contents.view2)
				std::free(contents.view2);

			if (contents.view3)
				std::free(contents.view3);

			if (contents.auViewEntry)
				std::free(contents.auViewEntry);
			if (contents.auEntry)
				std::free(contents.auEntry);
			if (contents.thng1)
				std::free(contents.thng1);

			if (contents.thng2)
				std::free(contents.thng2);

			if (contents.part2)
				std::free(contents.part2);



		}

		std::string getContents()
		{
			std::string ret;

			if (loaded)
			{
				using namespace std;
				stringstream ss;

				ss << "Contents of: " << filePath.getFullPathName() << "\n";
				ss << "\t";
				ss << "Size = " << contents.headerStart.absoluteOffsetToResouceStart + contents.headerStart.part1Size + contents.headerStart.part2Size;
				ss << "\n";
				ss << "\tCalculated size is: " << calculateSize() << "\n";



				return ss.str();

			}
			return ret;
		}


		class CByteStreamReader
		{

			std::size_t offset;
			char * pointer;
			bool freeOnDestruction;

		public:

			std::uint8_t readByte()
			{
				return *reinterpret_cast<std::uint8_t*>(pointer + offset);
			}

			std::uint16_t readBigShort()
			{
				return juce::ByteOrder::swap(*reinterpret_cast<std::uint16_t*>(pointer + offset));
				offset += sizeof(std::uint16_t);
			}
			std::uint32_t readBigInt()
			{
				return juce::ByteOrder::swap(*reinterpret_cast<std::uint16_t*>(pointer + offset));
				offset += sizeof(std::uint32_t);
			}

			template<typename T>
			void readInteger(T & value)
			{
				value = juce::ByteOrder::swap(*reinterpret_cast<T*>(pointer + offset));
				offset += sizeof(T);

			}

			std::uint8_t peekByte()
			{
				return *reinterpret_cast<uint8_t*>(pointer + offset);
			}

			void readBytes(void * buf, std::size_t size)
			{
				std::memcpy(buf, pointer + offset, size);
				offset += size;
			}

			std::size_t getOffset() { return offset; }
			void skip(std::size_t size) { offset += size; }
			CByteStreamReader(char * buf, bool assumeOwnership = false) : offset(0), pointer(buf), freeOnDestruction(assumeOwnership) {}
			~CByteStreamReader()
			{
				if (freeOnDestruction)
					std::free(pointer);
			}


		};

		class CByteStreamWriter
		{

			std::size_t offset;
			char * pointer;
			bool freeOnDestruction;

		public:

			void writeByte(std::uint8_t byte)
			{
				*reinterpret_cast<std::uint8_t*>(pointer + offset) = byte;
			}


			template<typename T>
			void writeInteger(T value)
			{
				*reinterpret_cast<T*>(pointer + offset) = juce::ByteOrder::swap(value);
				offset += sizeof(T);

			}

			void writeBytes(const void * buf, std::size_t size)
			{
				std::memcpy(pointer + offset, buf, size);
				offset += size;
			}

			std::size_t getOffset() { return offset; }
			void skip(std::size_t size) { offset += size; }
			CByteStreamWriter(char * buf, bool assumeOwnership = false) : offset(0), pointer(buf), freeOnDestruction(assumeOwnership) {}
			~CByteStreamWriter()
			{
				if (freeOnDestruction)
					std::free(pointer);
			}


		};

		std::size_t part1Size, part2Size;

		void setStringDetails(const std::string & name, const std::string & manu, const std::string & description)
		{
			if (!loaded)
				return;

			std::free(contents.brandAndName);
			std::free(contents.brandAndName2);
			std::free(contents.description);
			std::string newContents = manu + ": " + name;
			std::size_t size = newContents.size(), deSize = description.size();

			contents.brandAndName = reinterpret_cast<RsrcPStr*>(std::malloc(size + 1));
			contents.brandAndName2 = reinterpret_cast<RsrcPStr*>(std::malloc(size + 1));
			contents.description = reinterpret_cast<RsrcPStr*>(std::malloc(deSize + 1));
			std::memcpy(contents.brandAndName->data, newContents.data(), size);
			std::memcpy(contents.brandAndName2->data, newContents.data(), size);
			std::memcpy(contents.description->data, description.data(), deSize);
			contents.brandAndName->stringLength = contents.brandAndName2->stringLength = size;
			contents.description->stringLength = deSize;
		}
		#warning change to a type that supports string and int initialization
			void setAUDetails(std::uint32_t mainType, std::uint32_t subType, std::uint32_t manu)
		{
			if (!loaded)
				return;

			contents.thng1->auMaintype = juce::ByteOrder::swapIfLittleEndian(mainType);
			contents.thng1->auSubtype = juce::ByteOrder::swapIfLittleEndian(subType);
			contents.thng1->manufacturerCode = juce::ByteOrder::swapIfLittleEndian(manu);

			contents.thng2->auSubtype = juce::ByteOrder::swapIfLittleEndian(subType);
			contents.thng2->manufacturerCode = juce::ByteOrder::swapIfLittleEndian(manu);
		}

		std::size_t calculateSize()
		{
			if (!loaded)
				return 0;

			// this should be equal to headerStart.absoluteOffsetToResourceStart
			std::size_t currentSize = sizeof(RsrcHeaderStart) + sizeof(Rsrcpadding);
			const std::size_t headerSize = sizeof(ResourceElement);
			std::uint16_t offset = 0, counter = 0, nameLength; // offset from resource start to current resource


			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset = headerSize + contents.brandAndName->stringLength + 1; // note the extra byte ( = size) (pascal strings)
			nameLength = offset - headerSize;
			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset += headerSize + contents.description->stringLength + 1;

			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset += headerSize + contents.view1->stringLength + 1;

			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset += headerSize + contents.view2->stringLength + 1;

			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset += headerSize + std::strlen(contents.auEntry->data) + 1; // extra byte = null termination

			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset += headerSize + std::strlen(contents.auViewEntry->data) + 1;

			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			offset += headerSize + sizeof(Rsrcthng1);

			contents.part2->elements[counter++].offset = juce::ByteOrder::swapIfLittleEndian(offset);
			// i dont know the general reason this is needed... it refers to the second 'thng' resource
			contents.part2->elements[7].allbitsset = juce::ByteOrder::swapIfLittleEndian(nameLength);
			offset += headerSize + sizeof(Rsrcthng2);

			part1Size = offset;

			part2Size = sizeof(RsrcHeaderStart);
			part2Size += sizeof(Rsrcpart2);
			part2Size += contents.brandAndName2->stringLength + 1;
			part2Size += contents.view3->stringLength + 1;

			/*

			 writing resource header locations

			*/
			return totalSize = currentSize + part1Size + part2Size;
		}

		bool saveAs(std::string & error, juce::File resourceFile = juce::File::nonexistent)
		{
			juce::File saveFile = resourceFile == juce::File::nonexistent ? filePath : resourceFile;
			if (saveFile.exists() && saveFile.isDirectory())
			{
				error = "Input file is directory";
				return false;
			}

			// rsize = current resource element size
			juce::uint32 rsize = 0, size = calculateSize();
			char * buf = reinterpret_cast<char*>(std::malloc(size));
			std::memset(buf, 0, size);
			if (!buf)
			{
				error = "Error allocating memory";
				return false;
			}

			CByteStreamWriter writer(buf, true);
			// calculate sizes
			calculateSize();

			// set header data sizes
			contents.headerStart.absoluteOffsetToResouceStart = sizeof(RsrcHeaderStart) + sizeof(Rsrcpadding);
			contents.headerStart.part1Size = part1Size;
			contents.headerStart.part2Size = part2Size;
			contents.headerStart.absoluteOffsetToResourceEnd = contents.headerStart.absoluteOffsetToResouceStart + contents.headerStart.part1Size;

			// header
			writer.writeInteger(contents.headerStart.absoluteOffsetToResouceStart);
			writer.writeInteger(contents.headerStart.absoluteOffsetToResourceEnd);
			writer.writeInteger(contents.headerStart.part1Size);
			writer.writeInteger(contents.headerStart.part2Size);
			writer.writeBytes(&contents.pad, sizeof(Rsrcpadding));

			// part 1
			rsize = contents.brandAndName->stringLength + 1;
			writer.writeInteger(rsize);
			writer.writeBytes(contents.brandAndName, rsize);

			rsize = contents.description->stringLength + 1;
			writer.writeInteger(rsize);
			writer.writeBytes(contents.description, rsize);

			rsize = contents.view1->stringLength + 1;
			writer.writeInteger(rsize);
			writer.writeBytes(contents.view1, rsize);

			rsize = contents.view2->stringLength + 1;
			writer.writeInteger(rsize);
			writer.writeBytes(contents.view2, rsize);

			rsize = strlen(contents.auEntry->data) + 1;
			writer.writeInteger(rsize);
			writer.writeBytes(contents.auEntry, rsize);

			rsize = strlen(contents.auViewEntry->data) + 1;
			writer.writeInteger(rsize);
			writer.writeBytes(contents.auViewEntry, rsize);

			rsize = sizeof(Rsrcthng1);
			writer.writeInteger(rsize);
			writer.writeBytes(contents.thng1, rsize);

			rsize = sizeof(Rsrcthng2);
			writer.writeInteger(rsize);
			writer.writeBytes(contents.thng2, rsize);

			// part 2 - header

			writer.writeInteger(contents.headerStart.absoluteOffsetToResouceStart);
			writer.writeInteger(contents.headerStart.absoluteOffsetToResourceEnd);
			writer.writeInteger(contents.headerStart.part1Size);
			writer.writeInteger(contents.headerStart.part2Size);

			writer.writeBytes(contents.part2, sizeof(Rsrcpart2));
			writer.writeBytes(contents.brandAndName2, contents.brandAndName2->stringLength + 1);
			writer.writeBytes(contents.view3, contents.view3->stringLength + 1);


			if (writer.getOffset() != size)
			{
				error = "Error compiling data, data compiled: " + std::to_string(writer.getOffset()) + ", expected " + std::to_string(size);

			}

			if (saveFile.existsAsFile())
				saveFile.deleteFile();

			if (juce::ScopedPointer<juce::FileOutputStream> s = saveFile.createOutputStream())
			{
				if (!s->write(buf, size))
				{
					error = "Incorrect amount of bytes written to file";
					return false;
				}

			}
			else
			{
				error = "Error opening output file";
				return false;

			}
			return true;
		}

		bool load(juce::File resourceFile, std::string & error)
		{
			if (!resourceFile.existsAsFile() || resourceFile.getFileExtension() != ".rsrc")
			{
				error = "Invalid .rsrc file";
				return false;
			}

			filePath = resourceFile;
			char * buffer = nullptr;
			long long size = 0;
			std::size_t offset = 0;
			if (juce::ScopedPointer<juce::FileInputStream> s = filePath.createInputStream())
			{
				size = s->getTotalLength();
				if (!size)
				{
					error = "Invalid file size";
					return false;
				}
				buffer = reinterpret_cast<char*>(std::malloc(size));
				if (!buffer)
				{
					error = "Error allocating memory for reading file";
					return false;
				}
				if (size != s->read(buffer, size))
				{
					std::free(buffer);
					error = "Error reading whole file";
					return false;
				}
			}

			CByteStreamReader reader(buffer, true);


			// header start
			reader.readInteger(contents.headerStart.absoluteOffsetToResouceStart);
			reader.readInteger(contents.headerStart.absoluteOffsetToResourceEnd);
			reader.readInteger(contents.headerStart.part1Size);
			reader.readInteger(contents.headerStart.part2Size);

			reader.readBytes(&contents.pad, sizeof(Rsrcpadding));

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at headerstart - offset to resources: " + std::to_string(offset) + ", actual size of file: " + std::to_string(size);
				return false;
			}

			// resource part 1 - brandAndName
			reader.readInteger(contents.header.resourceSize);
			contents.brandAndName = reinterpret_cast<RsrcPStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.brandAndName, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at brandAndName, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 1 - description
			reader.readInteger(contents.header.resourceSize);
			contents.description = reinterpret_cast<RsrcPStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.description, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at description, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 1 - view1
			reader.readInteger(contents.header.resourceSize);
			contents.view1 = reinterpret_cast<RsrcPStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.view1, contents.header.resourceSize);


			if (reader.getOffset() > size)
			{
				error = "Corrupt data at view1, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 1 - view2
			reader.readInteger(contents.header.resourceSize);
			contents.view2 = reinterpret_cast<RsrcPStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.view2, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at view2, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 1 - auEntry
			reader.readInteger(contents.header.resourceSize);
			contents.auEntry = reinterpret_cast<RsrcCStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.auEntry, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at auEntry, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 1 - auViewEntry
			reader.readInteger(contents.header.resourceSize);
			contents.auViewEntry = reinterpret_cast<RsrcCStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.auViewEntry, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at auViewEntry, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}


			// resource part 1 - audioUnitDescription1
			reader.readInteger(contents.header.resourceSize);
			contents.thng1 = reinterpret_cast<Rsrcthng1 *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.thng1, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at thng1, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}


			// resource part 1 - audioUnitDescription2
			reader.readInteger(contents.header.resourceSize);
			contents.thng2 = reinterpret_cast<Rsrcthng2 *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.thng2, contents.header.resourceSize);


			if (offset > size)
			{
				error = "Corrupt data at thng2, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 2 - rsrcheaderStart2
			// -- no need to copy this: it is identical to the first one (and we only keep one copy)
			reader.skip(sizeof(RsrcHeaderStart));

			// resource part 2 - rsrcpart2 (static length):
			contents.header.resourceSize = sizeof(Rsrcpart2);
			contents.part2 = reinterpret_cast<Rsrcpart2 *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.part2, contents.header.resourceSize);

			if (reader.getOffset() > size)
			{
				error = "Corrupt data at rsrcpart2, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}


			// resource part 2 - brandAndName2
			contents.header.resourceSize = reader.peekByte(); // NOTE THIS: these fields contain no header! the size is given as the first byte, and must be included in the copied data - so we wont increase the offset here!
			contents.brandAndName2 = reinterpret_cast<RsrcPStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.brandAndName2, contents.header.resourceSize + 1); // NOTE THIS: we add one because the size of a PStr is pstr.size + 1 (the storage size of the 1 byte that keeps the size)


			if (reader.getOffset() > size)
			{
				error = "Corrupt data at brandAndName2, header declares " + std::to_string(contents.header.resourceSize) + " bytes";
				return false;
			}

			// resource part 2 - view3
			contents.header.resourceSize = reader.peekByte(); // NOTE THIS: these fields contain no header! the size is given as the first byte, and must be included in the copied data - so we wont increase the offset here!
			contents.view3 = reinterpret_cast<RsrcPStr *>(std::malloc(contents.header.resourceSize));
			reader.readBytes(contents.view3, contents.header.resourceSize + 1);

			if (reader.getOffset() != size)
			{
				error = "Corrupt file, expected " + std::to_string(size) + " bytes, read " + std::to_string(reader.getOffset()) + " bytes";
				return false;

			}
			size = reader.getOffset();
			return loaded = true;
		}

	};

};



#endif
