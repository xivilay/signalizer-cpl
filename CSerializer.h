#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <exception>
#include <stdexcept>
#include <cpl/stdext.h>

namespace cpl
{

	class BinaryBuilder
	{
	public:
		typedef char basic_element;
		typedef basic_element byte;

		template<typename org_type>
			struct custom_deleter
			{
				template<typename new_type>
					void operator()(new_type * obj)
					{
						auto casted_ptr = reinterpret_cast<org_type*>(obj);
						delete[] casted_ptr;
					}
			};

		using deleter = custom_deleter < basic_element > ;

		template<typename T>
			using owned_ptr = std::unique_ptr <T , deleter > ;

		BinaryBuilder()
			: memory(nullptr), bytesUsed(0), bytesAllocated(0), readPtr(0)
		{

		}
		// the 'big' five
		BinaryBuilder(const BinaryBuilder & other)
			: memory(nullptr), bytesUsed(0), bytesAllocated(0), readPtr(0)
		{
			appendBytes(other.getPointer(), other.getSize());
			readPtr = other.readPtr;
		}

		BinaryBuilder(BinaryBuilder && other) noexcept
			: memory(nullptr), bytesUsed(0), bytesAllocated(0), readPtr(0)
		{
			bytesAllocated = other.bytesAllocated;
			bytesUsed = other.bytesUsed;
			readPtr = other.readPtr;
			memory = std::move(other.memory);
		}

		BinaryBuilder & operator = (const BinaryBuilder & other)
		{
			reset();
			appendBytes(other.getPointer(), other.getSize());
			readPtr = other.readPtr;
			return *this;
		}

		BinaryBuilder & operator = (BinaryBuilder && other) noexcept
		{
			reset();
			bytesAllocated = other.bytesAllocated;
			bytesUsed = other.bytesUsed;
			readPtr = other.readPtr;
			memory = std::move(other.memory);
			return *this;
		}

		void appendBytes(const void * content, std::size_t bytes)
		{
			if (!bytes)
				return;
			ensureExtraBytes(bytes);
			std::memcpy(getCurrentPointer(), content, bytes);
			bytesUsed += bytes;
			// apply this for FIFO
			//readPtr = bytesUsed;
		}

		std::string getString()
		{
			std::size_t size = 0;
			if(!bytesUsed || !getPointer())
				return "";
			
			const char * pointer = reinterpret_cast<const char * >(memory.get());
			pointer += readPtr;
			for (;;)
			{
				// no nulltermination, assume something is corrupt and bail out
				if(size > bytesUsed)
					return "";
				if(pointer[size] == '\0')
				   break; 
				size++;
				
			}
			// add one more byte to include the nullterminator
			readPtr += size + 1;
			if (size > 1)
			{
				// notice it reads size bytes from pointer, not 
				// [pointer .. pointer + size] which would be 
				// one more byte, thus including the zero terminator.
				return std::string(pointer, size); 
			}
			else
				return "";
		}
		
		bool readBytes(void * content, std::size_t bytes)
		{
			// bad params
			if (!bytes || !content)
				return false;
			// reading more than what's available
			if (bytes + readPtr > bytesUsed)
				return false;
			// nothing to read - or reading more than what's available
			if (!getPointer())
				return false;

			std::memcpy(content, getPointer() + readPtr, bytes);
			readPtr += bytes;
			return true;
		}

		void ensureExtraBytes(std::size_t ebytes)
		{
			return ensureBytes(ebytes + bytesUsed);
		}

		void ensureBytes(std::size_t bytes)
		{
			if (bytes > bytesAllocated)
			{
				std::size_t newSize = static_cast<std::size_t>(bytes * 1.5);
				auto newPtr = std::make_unique<char[]>(newSize);
				std::memcpy(newPtr.get(), memory.get(), bytesUsed);
				bytesAllocated = newSize;
				memory.reset(newPtr.release());
			}
		}
		char * getCurrentPointer() const { return memory.get() + bytesUsed; }
		char * getPointer() const { return memory.get(); }
		std::size_t getSize() const { return bytesUsed; }
		void reset() { acquirePointer(); }
		void rewindRead() { readPtr = 0; }
		void rewindWrite() { bytesUsed = 0; }
		owned_ptr<byte[]> acquirePointer() { readPtr = bytesAllocated = bytesUsed = 0; return owned_ptr<byte[]>(memory.release()); }
	private:

		std::size_t bytesAllocated, bytesUsed;
		std::size_t readPtr;
		owned_ptr<byte[]> memory;
	};


	class CSerializer
	{
	public:

		typedef CSerializer Archiver;
		typedef CSerializer Builder;

		class Serializable
		{
		public:
			virtual void save(Archiver & ar, long long int version) {};
			virtual void load(Builder & ar, long long int version) {};
		};

		class ContentWrapper
		{
		public:
			//ContentWrapper(std::string filePath);
			
			ContentWrapper(BinaryBuilder & b)
			{
				contentSize = b.getSize();
				contents.reset(b.acquirePointer().release());
			}
			
			ContentWrapper(BinaryBuilder::owned_ptr<BinaryBuilder::byte[]> memory, std::size_t size)
				: contents(memory.release()), contentSize(size)
			{

			}

			const char * getBlock() const { return static_cast<const char*>(contents.get()); }
			std::size_t getSize() const { return contentSize; }
			ContentWrapper(ContentWrapper && cw)
				: contents(cw.contents.release()), contentSize(cw.contentSize)
			{

			}
		
		private:
			std::unique_ptr<char[]> contents;
			std::size_t contentSize;
		};

		class WeakContentWrapper
		{
		public:
			WeakContentWrapper(const BinaryBuilder::byte * c, std::size_t size) : contents(c), contentSize(size) {}
			WeakContentWrapper(const void * c, std::size_t size) 
				: contents(static_cast<const BinaryBuilder::byte *>(c)), contentSize(size) {}
			WeakContentWrapper(const ContentWrapper & cw) : contents(cw.getBlock()), contentSize(cw.getSize()) {}

			const char * getBlock() const { return contents; }
			std::size_t getSize() const { return contentSize; }

		private:
			const BinaryBuilder::byte * contents;
			std::size_t contentSize;
		};

		enum class HeaderType : short
		{
			Start = 0x10, // some sentinel value makes debugging easier
			Key, 
			Data,
			Child,
			End,
			Invalid
		};
		template<typename ExtraHeader>
			struct __alignas(8) BinaryHeader
			{
				std::size_t headerSize;
				std::size_t dataSize;
				HeaderType type;
				ExtraHeader info;

				BinaryHeader()
					: headerSize(sizeof(*this)), dataSize(0), type(HeaderType::Invalid), info()
				{}
				const BinaryHeader<int> * next() const
				{
					if (type == HeaderType::End)
						return nullptr;
					else
						return (BinaryHeader<int>*)(((char*)this) + headerSize + dataSize);
				}
				template<typename T>
					const T * getData() const
					{
						return reinterpret_cast<const T *>(((const char *)this) + sizeof(*this));
					}
			};

		struct MasterHeaderInfo
		{
			std::size_t totalSize;
			long long versionID;
		};

		struct KeyHeaderInfo
		{
			bool isString;
			long long ID;
		};

		struct DataHeaderInfo
		{

		};

		using MasterHeader = BinaryHeader < MasterHeaderInfo > ;
		using KeyHeader = BinaryHeader < KeyHeaderInfo > ;
		using StdHeader = BinaryHeader < int > ;
		class Key
		{
		public:
			Key(const KeyHeader * kh);
			Key(const std::string & s) : isString(true), intKey(0), stringKey(s) {};
			Key(const char * s) : isString(true), intKey(0), stringKey(s) {};
			Key(long long ID) : isString(false), intKey(ID) {};

			bool operator < (const Key & other) const
			{
				// true return == other > this

				// strings compare greater than IDs
				if (other.isString)
				{
					if (!isString)
						return true;
					else
						return stringKey < other.stringKey;
				}
				else
				{
					if (isString)
						return false;
					else
						return intKey < other.intKey;
				}
				return false;
			}

			bool operator == (const Key & other) const
			{
				if (other.isString != isString)
					return false;

				if (other.isString)
				{
					if (other.stringKey == stringKey)
						return true;
				}
				else
				{
					if (other.intKey == intKey)
						return true;
				}
				return false;
			}

			// compiles this state down to a block of bytes.
			// build does the reverse action.
			// the data inside can be cast to a const KeyHeader *
			ContentWrapper compile() const
			{
				BinaryBuilder data;
				KeyHeader header;
				// we dont carry the additional zero-terminating null, since the size is already given.
				header.dataSize = isString ? stringKey.length() : 0;
				header.type = HeaderType::Key;
				header.info.isString = isString;
				header.info.ID = intKey;

				data.appendBytes(&header, sizeof(header));
				if(isString)
					data.appendBytes(stringKey.c_str(), stringKey.size());

				return ContentWrapper(data);
			}

			// restores this object to the state found in kh.
			bool build(const KeyHeader * kh)
			{
				if (kh->headerSize >= sizeof(KeyHeader))
				{
					isString = kh->info.isString;
					intKey = kh->info.ID;
					if (isString && kh->dataSize > 0)
					{
						stringKey = std::string(kh->getData<char>(), kh->getData<char>() + kh->dataSize);
					}
					return true;
				}
				else
					return false;
			}

		private:
			bool isString;
			std::string stringKey;
			long long intKey;
		};


		CSerializer(Key k)
			: key(k), version(0)
		{

		}

		CSerializer()
			: key(1), version(0)
		{

		}
		~CSerializer()
		{
			clear();
		}
		void clear()
		{
			content.clear();
			data.reset();
		}
		bool isEmpty() const noexcept
		{
			return !content.size() && !data.getSize();
		}
		void clearDataOnly()
		{
			for (auto & child : content)
			{
				child.second.clearDataOnly();
			}
			data.reset();
		}

		long long int getMasterVersion() { return version; } const
		bool build(const WeakContentWrapper & cr)
		{
			const StdHeader * start = (const StdHeader *) cr.getBlock();

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
							auto & cs = getKey(currentKey);
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
			else
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
						auto & cs = getKey(currentKey);
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

		ContentWrapper compile(bool addMasterHeader = false, long long version = 0) const
		{
			BinaryBuilder dataOut;
			// write the master header.
			if (addMasterHeader)
			{
				MasterHeader header;
				header.type = HeaderType::Start;
				header.info.versionID = version;
				header.dataSize = 0;
				dataOut.appendBytes(&header, sizeof(header));
			}
			// write data (if we have some)
			if(data.getSize())
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

		void append(const CSerializer & se);
		void append(const Key & k);

		template<typename T>
			typename std::enable_if<std::is_pod<T>::value, void>::type 
				append(T * object, std::size_t size = SIZE_MAX)
		{
			if (size == SIZE_MAX)
			{
				size = sizeof(T);
			}

			data.appendBytes(object, size);
		}

		template<typename T>
			typename std::enable_if<std::is_standard_layout<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value, CSerializer &>::type
				operator << (const T & object)
			{

				data.appendBytes(&object, sizeof(T));

				return *this;
			}

		void rewindReader()
		{
			data.rewindRead();
		}
		void rewindWriter()
		{
			data.rewindWrite();
		}
		//template<typename T>
		//	CSerializer & operator << (const T * object);

		template<typename T>
			typename std::enable_if<std::is_standard_layout<T>::value && !std::is_pointer<T>::value && !std::is_array<T>::value, CSerializer &>::type
				operator >> ( T & object)
			{

				data.readBytes(&object, sizeof(T));

				return *this;
			}

		inline CSerializer & operator >> (Serializable * object)
		{
			object->load(*this, version);
			return *this;
		}

		inline CSerializer & operator << (Serializable * object)
		{
			object->save(*this, version);
			return *this;
		}
			
		inline CSerializer & operator >> (Serializable & object)
		{
			object.load(*this, version);
			return *this;
		}

		inline CSerializer & operator << (Serializable & object)
		{
			object.save(*this, version);
			return *this;
		}

		CSerializer & operator << (const std::string & str)
		{
			data.appendBytes(str.c_str(), str.size() + 1);
			return *this;
		}
		CSerializer & operator >> ( std::string & str)
		{
			
			str = data.getString();
			
			return *this;
		}
						
		CSerializer & getKey(const Key & k)
		{
			/**/
			//auto count = content.count(k);
			auto it = content.find(k);

			// vector::find code
			/*auto it = content.begin();
			for (; it != content.end(); it++)
			{
				if (it->first == k)
					return it->second;
			}*/

			if (it == content.end())
			{
				return content.emplace(k, k).first->second;
				
				// vector:
				//content.emplace_back(std::pair<Key, CSerializer>(k, k));
			}
			return it->second;
			// possible to get recursion here if emplace_back fails, but in that case
			// we are screwed either way....
			//return getKey(k);
		}



	private:
		
		BinaryBuilder data;
		//std::vector<std::pair<Key, CSerializer>> content;
		std::map<Key, CSerializer> content;
		Key key;

		long long version;

		// iterator interface. must be placed after declaration in MSVC
	public:
		auto begin() const noexcept -> decltype(content.begin())
		{
			return content.begin();
		}

		auto end() const noexcept -> decltype(content.end())
		{
			return content.end();
		}
	};


};