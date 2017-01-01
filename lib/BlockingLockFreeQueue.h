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

	file:BlockLockFreeQueue.h
	
		See class summary

*************************************************************************************/

#ifndef SBLOCKFREE_DATAQUEUE_H
	#define SBLOCKFREE_DATAQUEUE_H
	
	#include "readerwriterqueue/readerwriterqueue.h"
	#include <vector>
	#include "../CMutex.h"
	#include "../Utility.h"
	#include <atomic>

	#if defined(__C11__) && defined(CPL_CLANG)
		#include <stdatomic.h>
	#endif

	#if ATOMIC_LLONG_LOCK_FREE != 2
		#pragma message cwarn("warning: Atomic integer operations are not lock-free for this platform!") 
	#endif

	#if ATOMIC_POINTER_LOCK_FREE != 2
		#pragma message cwarn("warning: Atomic pointer operations are not lock-free for this platform!") 
	#endif

	namespace cpl
	{

		/// <summary>
		/// See CLockFreeDataQueue.
		/// 
		/// This queue relies on objects being easy to copy & move construct, thus objects are not allocated on the heap.
		/// Use CLockFreeDataQueue if you only want your objects to be constructed and destructed once.
		/// 
		/// This queue is further specialized in that pop's block the consumer thread until an entry is produced, if it
		/// is empty. 
		/// </summary>
		template<typename T>
		class CBlockingLockFreeQueue
		{
		public:
			struct ElementAccess;
			friend struct ElementAccess;

			CBlockingLockFreeQueue(std::size_t initialSize, std::size_t maxSize)
			: 
				queue(new moodycamel::ReaderWriterQueue<T>(initialSize)), 
				oldQueue(nullptr), 
				currentNumElements(initialSize), 
				enqueuedDataAllocations(false),
				enqueuedQueueAllocations(false),
				maxElements(maxSize)
			{

			}

			/// <summary>
			/// PRODUCER ONLY.
			/// Tries to enqueue the input data.
			/// if allocOnFail is false, it will never allocate memory and the complexity is deterministic (wait-free).
			/// If enqueueNewAllocations is set, the consumer thread might increase the size at another time, if this call fails.
			/// </summary>
			template<bool allocOnFail = false,  bool enqueueNewAllocations = true>
			bool pushElement(const T & data)
			{
				if (allocOnFail)
				{
					if (queue.load()->enqueue(data))
					{
						semaphore.signal();
						return true;
					}
				}
				else
				{
					if (queue.load()->try_enqueue(data))
					{
						semaphore.signal();
						return true;
					}
				}
				if (enqueueNewAllocations)
				{
					enqueuedDataAllocations = true;
				}
				return false;
			}

			/// <summary>
			/// CONSUMER ONLY.
			/// If true is returned, input data is filled with the firstly enqueued data.
			/// If it returns false, someone else signaled the semaphore, probably indicating 
			/// the queue wont be filled again.
			/// </summary>
			/// <param name="d"></param>
			/// <returns></returns>
			bool popElementBlocking(T & data)
			{
				semaphore.wait();
				if (oldQueue)
				{
					if (oldQueue->try_dequeue(data))
					{
						return true;
					}

				}
				// note that if the old queue is empty, and the new has an enqueued element, we can conclusively
				// prove that it is safe to delete the old queue since it is (a) empty and (b) the thread state 
				// for the producer is updated such that it uses all the new entities and will never use the old again.
				// if we successfully dequeue an element, we can delete the old (if it exists).
				if (queue.load()->try_dequeue(data))
				{
					if (oldQueue)
					{
						delete oldQueue;
						oldQueue = nullptr;
					}
					return true;
				}
				return false;
			}

			/// <summary>
			/// ANY THREAD.
			/// Releases the producer thread if it is currently waiting to pop an element, causing the popElementBlocking() to return false.
			/// </summary>
			void releaseConsumer()
			{
				semaphore.signal();
			}

			/// <summary>
			/// CONSUMER ONLY.
			/// If any operations has failed and signaled the need for more allocations, this might be done now.
			/// May allocate memory.
			/// 
			/// Will grow the queue to minimumSize at least. If growth is set, then:
			///		if space_used > total_space * growthRequirement then grow(max(minimumSize, size() * growthFactor)
			/// 
			/// Thus you can use this function to automatically grow the queue if it begins to fill up, thereby
			/// avoiding a full queue. This will NOT delete any enqueued elements, issue locks or in any way mess up 
			/// ordering of concurrently enqueued elements.
			/// 
			/// returns true if the queue was grown, otherwise false.
			/// 
			/// Note: You should always grow the queue preemptively, that is, before you dequeue it fully.
			/// </summary>
			/// <returns></returns>
			bool grow(std::size_t minimumSize = 0, bool growth = false, float growthRequirement = 1.0f, int growthFactor = 2)
			{
				// old resize not done yet!
				if (oldQueue)
					return false;

				auto currentSpaceFilled = queue.load()->size_approx();

				std::size_t newSize(currentNumElements);

				if ((growth && currentSpaceFilled > currentNumElements * growthRequirement) || enqueuedDataAllocations || enqueuedQueueAllocations)
				{
					newSize *= growthFactor;
				}

				newSize = std::min(maxElements, std::max(newSize, minimumSize));

				if(newSize > currentNumElements)
				{ 
					oldQueue = queue.exchange(new moodycamel::ReaderWriterQueue<T>(newSize));
					enqueuedDataAllocations = enqueuedQueueAllocations = false;
					currentNumElements = newSize;
					return true;
				}
				return false;
			}

			/// <summary>
			/// ANY THREAD.
			/// Returns the current capacity
			/// </summary>
			std::size_t size() const noexcept
			{
				return currentNumElements;
			}
			/// <summary>
			/// ANY THREAD.
			/// Returns the current amount of enqueued elements (estimated)
			/// </summary>
			std::size_t enqueuededElements() const noexcept
			{
				return queue.load()->size_approx();
			}

			~CBlockingLockFreeQueue()
			{
				auto * q = queue.load();
				if (oldQueue)
				{
					delete oldQueue;
				}
				delete q;
				
				
			}

		private:
			moodycamel::spsc_sema::Semaphore semaphore;
			std::atomic<moodycamel::ReaderWriterQueue<T> *> queue;
			moodycamel::ReaderWriterQueue<T> * oldQueue;

			std::size_t currentNumElements, maxElements;

			/// <summary>
			/// If set, try to grow the freeElements queue
			/// </summary>
			volatile bool enqueuedDataAllocations;
			/// <summary>
			/// If set, try to grow the queue.
			/// </summary>
			volatile bool enqueuedQueueAllocations;
		};
	};
#endif
