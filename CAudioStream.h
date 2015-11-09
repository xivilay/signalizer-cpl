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

	file:CAudioStream.h
		
		A bridge system allowing effecient data processing between threads that mustn't
		suffer from priority inversion, but still handle locking.
		TODO: Implement growable listener queue
*************************************************************************************/

#ifndef _CAUDIOSTREAM_H
	#define _CAUDIOSTREAM_H
	#include <thread>
	#include "Utility.h"
	#include <vector>
	#include <atomic>
	#include "ConcurrentServices.h"

	namespace cpl
	{

		template<typename T>
			class CAudioStream;



		template<typename T>
		class CAudioStream : Utility::CNoncopyable
		{
		public:

			/// <summary>
			/// Holds info about the audio stream. Wow.
			/// </summary>
			struct AudioStreamInfo
			{
				double sampleRate;
				std::size_t anticipatedSize;
				std::size_t anticipatedChannels;
				std::size_t circularBufferSize;
			};

			template<typename T>
				class ARTAudioListener;



			typedef std::vector<ARTAudioListener<T> *> RTListenerQueue;

			static const std::size_t defaultListenerBankSize = 16;

			template<typename T>
			class ARTAudioListener : Utility::CNoncopyable
			{
			public:
				ARTAudioListener()
					: source(nullptr)
				{

				}

				/// <summary>
				/// Called from a real-time thread.
				/// </summary>
				/// <param name="source"></param>
				/// <param name="buffer"></param>
				/// <param name="numChannels"></param>
				/// <param name="numSamples"></param>
				/// <returns></returns>
				virtual bool onRTAudio(CAudioStream<T> & source, T ** buffer, std::size_t numChannels, std::size_t numSamples) = 0;

				/// <summary>
				/// May fail if the streams listener buffer is temporarily filled.
				/// Callable from any thread.
				/// </summary>
				/// <param name="audioSource"></param>
				/// <returns></returns>
				bool listenToSource(CAudioStream<T> & audioSource)
				{
					if (audioSource.addRTAudioListener(this))
					{
						internalSource = &audioSource;
						return true;
					}
					return false;
				}

				void onIncomingRTAudio(CAudioStream<T> & source, T ** buffer, std::size_t numChannels, std::size_t numSamples)
				{
					if (internalSource != &source)
						crash("Inconsistency between argument CAudioStream and internal source; corrupt listener chain.");
					else
						onRTAudio(source, buffer, numChannels, numSamples);
				}
				void sourceIsDying()
				{
					source = nullptr;
					onSourceDied();
				}

				/// <summary>
				/// Called when the current source being listened to died.
				/// May be called from any thread.
				/// </summary>
				virtual void onSourceDied() {};

				virtual ~ARTAudioListener()
				{
					if (internalSource)
						internalSource->removeRTAudioListener(this);
				}

				void sourcePropertiesChanged(const CAudioStream<T> & changedSource, const typename CAudioStream<T>::AudioStreamInfo & before)
				{
					if (&changedSource != internalSource)
						crash("Inconsistency between argument CAudioStream and internal source; corrupt listener chain.");
				}

				/// <summary>
				/// Tries to gracefully crash and disable this listener.
				/// Callable from any thread.
				/// </summary>
				/// <param name="why"></param>
				void crash(const char * why)
				{

				}

			private:
				CAudioStream<T> * internalSource;
			};


			CAudioStream()
			{
				rtListeners.tryReplace(new RTListenerQueue(defaultListenerBankSize));
				static_assert(ATOMIC_POINTER_LOCK_FREE, "CAudioStream needs atomic pointer stores");
			}

			/// <summary>
			/// Only call this from the non-RT thread
			/// </summary>
			/// <param name="info"></param>
			void initializeInfo(const AudioStreamInfo & info)
			{
				AudioStreamInfo before = internalInfo;
				internalInfo = info;
				auto & listeners = rtListeners.getObject();

				for (auto listener : listeners)
				{
					if(listener)
						listener->sourcePropertiesChanged(*this, before);
				}
			}

			bool addRTAudioListener(ARTAudioListener<T> * newListener)
			{
				auto & listeners = rtListeners.getObject();

				for (auto & listener : listeners)
				{
					if (!listener)
					{
						listener = newListener;
						return true;
					}

				}
				return false;
			}


			bool removeRTAudioListener(ARTAudioListener<T> *)
			{
				// TODO: fix following problem
				// what happens if anyone removes a audio listener
				// while a copy of the current list of listeners is being generated with a larger size?
			}

			/// <summary>
			/// 
			/// </summary>
			/// <param name="buffer"></param>
			/// <param name="numChannels"></param>
			/// <param name="numSamples"></param>
			/// <returns>True if incoming audio was changed</returns>
			bool processIncomingRTAudio(T ** buffer, std::size_t numChannels, std::size_t numSamples)
			{
				return false;
			}




		private:

			ConcurrentObjectSwapper<RTListenerQueue> rtListeners;
			AudioStreamInfo internalInfo;
		};

	};
#endif