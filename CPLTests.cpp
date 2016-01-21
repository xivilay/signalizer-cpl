/*************************************************************************************

	cpl - cross-platform library - v. 0.1.0.

	Copyright (C) 2015 Janus Lynggaard Thorborg [LightBridge Studios]

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

	file:CPLSource.cpp
	
		All the source code needed to compile the cpl lib. Add this file to your project.

*************************************************************************************/

#include "CPLTests.h"
#include "CAudioStream.h"
#include <stdio.h>
#include "lib/AlignedAllocator.h"
#include "dsp.h"
#include <cstdarg>
#include <vector>
#include <numeric>
#include <iostream>
#include <map>
namespace cpl
{
	const auto warn = DiagnosticLevel::Warnings;
	const auto verb = DiagnosticLevel::All;
	const auto info = DiagnosticLevel::Info;


	void func()
	{
		std::atomic_bool quit = false;

		std::vector<float> data(100);
		std::mutex m;

		std::thread one([&]() 
			{
				while (!quit.load(std::memory_order_relaxed))
				{
					std::unique_lock<std::mutex> lock(m, std::try_to_lock);
					
					if (lock.owns_lock())
					{
						for (std::size_t i = 0; i < data.size(); ++i)
						{
							data[i] = (float)std::rand();
						}

						std::atomic_thread_fence(std::memory_order::memory_order_release);
					}
				}
			}
		);

		std::thread two([&]() 
			{
				while (!quit.load(std::memory_order_relaxed))
				{
					std::unique_lock<std::mutex> lock(m, std::try_to_lock);

					if (lock.owns_lock())
					{
						// guaranteed that any changes from thread one to 'data' is seen after this fence?
						std::atomic_thread_fence(std::memory_order::memory_order_acquire);

						auto res = std::accumulate(data.begin(), data.end(), 0.0f);
						std::cout << "Accumulated result is: " << res << std::endl;
					}
				}
			}
		);

		fgetc(stdin);

		quit.store(true);
		one.join();  two.join();
	}

	void dout(DiagnosticLevel req, DiagnosticLevel  lvl, const char * msg, ...)
	{
		if (lvl >= req)
		{
			va_list args;
			va_start(args, msg);
			vprintf(msg, args);
			va_end(args);
		}
	}
	bool CAudioStreamTest(std::size_t emulatedBufferSize, double sampleRate, DiagnosticLevel lvl)
	{
		typedef float ftype;



		class LList : public cpl::CAudioStream<ftype, 128>::Listener
		{
		public:
			virtual bool onRTAudio(const Stream & source, ftype ** buffer, std::size_t numChannels, std::size_t numSamples)  override
			{
				dout(verb, lvl, "AT: recieved %d realtime samples\n", numSamples);
				rtc += numSamples;
				return false;
			};
			/// <summary>
			/// Called from a non real-time thread.
			/// </summary>
			virtual bool onAsyncAudio(const Stream & source, ftype ** buffer, std::size_t numChannels, std::size_t numSamples)  override
			{
				dout(verb, lvl, "AST: recieved %d async samples\n", numSamples);
				asc += numSamples;
				return false;
			};

			std::size_t getACount()
			{
				return asc;
			}
			std::size_t getRTCount()
			{
				return rtc;
			}
			std::size_t rtc = 0, asc = 0;

			DiagnosticLevel lvl = DiagnosticLevel::None;
		};

		std::map<std::size_t, std::pair<LList, bool>> listeners;
		LList permListener;
		std::size_t count = 0;
		std::size_t drops = 0;
		std::size_t listenerTests = 300;

		double msPerRender = emulatedBufferSize / sampleRate;

		{
			cpl::CAudioStream<ftype, 128> stream(16, true, 10, 10000);
			std::atomic_bool quit(false);
			dout(info, lvl, "Press any key to quit - starting in 1000ms\n");
			Sleep(1000);

			permListener.listenToSource(stream);
			//permListener.lvl = warn;

			std::thread audioThread
			(
				[&]()
				{

					cpl::CAudioStream<ftype, 128>::AudioStreamInfo sinfo{};
					sinfo.anticipatedChannels = 2;
					sinfo.anticipatedSize = emulatedBufferSize;
					sinfo.callAsyncListeners = true;
					sinfo.callRTListeners = true;
					sinfo.sampleRate = sampleRate;
					sinfo.storeAudioHistory = true;
					stream.initializeInfo(sinfo);

					cpl::aligned_vector<ftype, 16> audioData[2];
					ftype * buffers[2];
					std::uint64_t prevDroppedFrames = 0;
					while (!quit)
					{
						auto size = sinfo.anticipatedSize + (std::rand() % 10) - 5;
						for (auto & ch : audioData)
						{
							ch.resize(size);
							cpl::dsp::fillWithRand(ch, ch.size());
						}

						buffers[0] = audioData[0].data();
						buffers[1] = audioData[1].data();

						stream.processIncomingRTAudio(buffers, 2, size);
						auto newDrops = stream.getPerfMeasures().droppedAudioFrames - prevDroppedFrames;
						const auto fsizef = stream.getASyncBufferSize();
						dout(newDrops > 0 ? warn : verb, lvl,
							"AT: Sent " CPL_FMT_SZT " realtime samples - dropped %llu frames." 
							"Fifo size: " CPL_FMT_SZT " (" CPL_FMT_SZT "  bytes)\n",
							size, newDrops, 
							fsizef, fsizef * stream.packetSize);
						prevDroppedFrames += newDrops;
						count += size;


						Misc::PreciseDelay(msPerRender);
					}
					drops = static_cast<std::size_t>(stream.getPerfMeasures().droppedAudioFrames);
				}
			);

			std::thread listenerAdder
			(
				[&]() 
				{
					while (!quit)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(100));

						auto entry = std::rand() % listenerTests;
						auto & listener = listeners[entry];

						if (listener.second)
						{
							if (!listener.first.detachFromSource())
								dout(warn, lvl, "Unable to remove listener %d", entry);
							else
								listener.second = false;
						}
						else
						{
							if (!listener.first.listenToSource(stream))
								dout(warn, lvl, "Unable to add listener %d in 2 seconds", entry);
							else
								listener.second = true;
						}

						stream.setAudioHistorySizeAndCapacity(std::rand() % 1000 + 100, std::rand() % 1000 + 1200);
					}

				}
			);

			std::getc(stdin);

			quit.store(true);
			audioThread.join();
			listenerAdder.join();
		}
		dout(info, lvl, "Done...\nSent %d samples, recieved %d synchronuously, and %d asynchronuously (missing %d, dropped in total: %d).\n",
			count, permListener.getRTCount(), permListener.getACount(),
			count - permListener.getACount(), drops * (cpl::CAudioStream<ftype>::AudioFrame::capacity >> 1));

		return true;
	}


};

