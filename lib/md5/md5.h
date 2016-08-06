/* MD5
converted to C++ class by Frank Thilo (thilo@unix-ag.org)
for bzflag (http://www.bzflag.org)

based on:

md5.h and md5.c
reference implementation of RFC 1321

Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

/* 
	CPL modifcation 2016 by Janus Thorborg: ability to get/set results using raw bytes.
	Added helper struct MD5Result, comparable.
	Changed to utilize std::uint*_t instead of relying on specific compilers.
*/

#ifndef BZF_MD5_H
#define BZF_MD5_H

#include <cstring>
#include <iostream>

namespace bzf
{

	struct MD5Result
	{
		MD5Result()
		{
			std::memset(result, 0, size);
		}

		MD5Result(const std::uint8_t * data)
		{
			std::memcpy(result, data, size);
		}

		static_assert(CHAR_BIT == 8, "Only platforms for bitsof(char) == 8 is supported");
		static const std::size_t size = 16;
		std::uint8_t result[size];

	};

	inline bool operator == (const MD5Result & a, const MD5Result & b) noexcept { return std::memcmp(a.result, b.result, MD5Result::size) == 0; }
	inline bool operator != (const MD5Result & a, const MD5Result & b) noexcept { return !(a == b); }
	inline bool operator < (const MD5Result & a, const MD5Result & b) noexcept { return std::memcmp(a.result, b.result, MD5Result::size) < 0; }
	inline bool operator > (const MD5Result & a, const MD5Result & b) noexcept { return std::memcmp(a.result, b.result, MD5Result::size) > 0; }

	// a small class for calculating MD5 hashes of strings or byte arrays
	// it is not meant to be fast or secure
	//
	// usage: 1) feed it blocks of uchars with update()
	//      2) finalize()
	//      3) get hexdigest() string
	//      or
	//      MD5(std::string).hexdigest()
	class MD5
	{
	public:
		typedef std::uint32_t size_type; // must be 32bit

		MD5();
		MD5(const std::string& text);
		void update(const unsigned char *buf, size_type length);
		void update(const char *buf, size_type length);
		MD5& finalize();
		std::string hexdigest() const;
		MD5Result rawdigest() const;
		friend std::ostream& operator<<(std::ostream&, MD5 md5);

	private:
		void init();

		enum { blocksize = 64 }; // VC6 won't eat a const static int here

		void transform(const std::uint8_t block[blocksize]);
		static void decode(std::uint32_t output[], const std::uint8_t input[], size_type len);
		static void encode(std::uint8_t output[], const std::uint32_t input[], size_type len);

		bool finalized;
		std::uint8_t buffer[blocksize]; // bytes that didn't fit in last 64 byte chunk
		std::uint32_t count[2];   // 64bit counter for number of bits (lo, hi)
		std::uint32_t state[4];   // digest so far
		std::uint8_t digest[16]; // the result

						  // low level logic operations
		static inline std::uint32_t F(std::uint32_t x, std::uint32_t y, std::uint32_t z);
		static inline std::uint32_t G(std::uint32_t x, std::uint32_t y, std::uint32_t z);
		static inline std::uint32_t H(std::uint32_t x, std::uint32_t y, std::uint32_t z);
		static inline std::uint32_t I(std::uint32_t x, std::uint32_t y, std::uint32_t z);
		static inline std::uint32_t rotate_left(std::uint32_t x, int n);
		static inline void FF(std::uint32_t &a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac);
		static inline void GG(std::uint32_t &a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac);
		static inline void HH(std::uint32_t &a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac);
		static inline void II(std::uint32_t &a, std::uint32_t b, std::uint32_t c, std::uint32_t d, std::uint32_t x, std::uint32_t s, std::uint32_t ac);
	};

	std::string md5(const std::string str);
	MD5Result md5(const void * data, std::size_t bytes);
};
#endif