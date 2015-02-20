
/****************************************************************************\
* DustFFT - (c) Copyright Teemu Voipio 2014         version-2014-07-07-1     *
*----------------------------------------------------------------------------*
* You can use and/or redistribute this for whatever purpose, free of charge, *
* provided that the above copyright notice and this permission notice appear *
* in all copies of the software or it's associated documentation.            *
*                                                                            *
* THIS SOFTWARE IS PROVIDED "AS-IS" WITHOUT ANY WARRANTY. USE AT YOUR OWN    *
* RISK. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE HELD LIABLE FOR ANYTHING.  *
*----------------------------------------------------------------------------*
*                                                                            *
* This file is generated by a script, direct editing is a terrible idea.     *
\****************************************************************************/
#ifndef DUSTFFT_H
#define DUSTFFT_H

namespace cpl
{
	namespace signaldust
	{
		/* Main interface:
		Use SSE2 packed processing for 16-byte aligned buffers (faster).
		For unaligned buffers, falls back to slower scalar code.
		*/
		void DustFFT_fwdD(double * buf, unsigned n);
		void DustFFT_revD(double * buf, unsigned n);

		/* These assume 16 bytes alignment without checking anything.
		Use these if you want to crash (for debug/testing) on unaligned buffers.
		*/
		void DustFFT_fwdDa(double * buf, unsigned n);
		void DustFFT_revDa(double * buf, unsigned n);
	};
};
#endif /* DUSTFFT_H */