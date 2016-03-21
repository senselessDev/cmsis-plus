/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016 Liviu Ionescu.
 *
 * µOS++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3.
 *
 * µOS++ is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/diag/trace.h>

#include <cmsis-plus/rtos/port/os-decls.h>
#include <cmsis-plus/rtos/port/os-inlines.h>

// ----------------------------------------------------------------------------

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <new>

// ----------------------------------------------------------------------------

namespace os
{
  namespace rtos
  {
    // ------------------------------------------------------------------------

    /**
     * @details
     * The os::rtos::mqueue namespace groups message queue attributes
     * and initialisers.
     */
    namespace mqueue
    {
      /**
       * @class Attributes
       * @details
       * Allow to assign a name and custom attributes (like a static
       * address) to the message queue.
       *
       * To simplify access, the member variables are public and do not
       * require accessors or mutators.
       *
       * @par POSIX compatibility
       *  Inspired by `mq_attr`
       *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
       *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
       */

      /**
       * @var void* Attributes::mq_queue_address
       * @details
       * Set this variable to a user defined memory area large enough
       * to store the message queue. Usually this is a statically
       * allocated array of structures.
       *
       * The default value is `nullptr`, which means there is no
       * user defined message queue.
       */

      /**
       * @var std::size_t Attributes::mq_queue_size_bytes
       * @details
       * The message queue size must match exactly the allocated size. It is
       * used for validation; when the message queue is initialised,
       * this size must be large enough to accommodate the desired
       * message queue.
       *
       * If the @ref mq_queue_address is `nullptr`, this variable is not
       * checked, but it is recommended to leave it zero.
       */

      const Attributes initializer
        { nullptr };

    } /* namespace mqueue */

    // ------------------------------------------------------------------------

    /**
     * @class Message_queue
     * @details
     * POSIX message queues allow threads to exchange data in the form of
     * messages. Messages are transferred to and from a queue using
     * send() and receive(). Each message has an associated priority,
     * and messages are always delivered to the receiving process
     * highest priority first.
     *
     * @par Example
     *
     * @code{.cpp}
     * // Define the message type.
     * typedef struct {
     *   uint32_t id;
     * } msg_t;
     *
     * // Define the queue size.
     * constexpr uint32_t queue_size = 5;
     *
     * Message_queue mq { queue_size, sizeof(msg_t) };
     *
     * void
     * consumer(void)
     * {
     *   // Do something
     *   msg_t msg;
     *   for (; some_condition();)
     *     {
     *       mq.receive(&msg, sizeof(msg), nullptr);
     *       // Process message
     *       if (msg.id == 7)
     *         {
     *           // Something special
     *         }
     *     }
     *   // Do something else.
     * }
     *
     * void
     * producer(void)
     * {
     *   // Do something
     *   msg_t msg;
     *   msg.id = 7;
     *   mq.send(&msg, sizeof(msg), mqueue::default_priority);
     *   // Do something else.
     * }
     * @endcode
     *
     * @par POSIX compatibility
     *  Inspired by `mqd_t`
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     */

    /**
     * @details
     * This constructor shall initialise the message queue object
     * with the given number of messages and default settings.
     * The effect shall be equivalent to creating a message queue object
     * referring to the attributes in `mqueue::initializer`.
     * Upon successful initialisation, the state of the message queue
     * object shall become initialised, with no messages in the queue.
     *
     * Only the message queue object itself may be used for performing
     * synchronisation. It is not allowed to make copies of
     * message queue objects.
     *
     * For default message queue objects, the storage is dynamically allocated.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    Message_queue::Message_queue (mqueue::size_t msgs,
                                  mqueue::msg_size_t msg_size_bytes) :
        Message_queue (mqueue::initializer, msgs, msg_size_bytes)
    {
      ;
    }

    /**
     * @details
     * This constructor shall initialise the message queue object
     * with attributes referenced by _attr_.
     * If the attributes specified by _attr_ are modified later,
     * the memory pool attributes shall not be affected.
     * Upon successful initialisation, the state of the
     * message queue object shall become initialised.
     *
     * Only the message queue itself may be used for performing
     * synchronisation. It is not allowed to make copies of
     * message queue objects.
     *
     * In cases where default message queue attributes are
     * appropriate, the variable `mqueue::initializer` can be used to
     * initialise message queue.
     * The effect shall be equivalent to creating a message queue
     * object with the simple constructor.
     *
     * If the attributes define a storage area, it is used, otherwise
     * it is dynamically allocated.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    Message_queue::Message_queue (const mqueue::Attributes&attr,
                                  mqueue::size_t msgs,
                                  mqueue::msg_size_t msg_size_bytes) :
        Named_object
          { attr.name () }, //
        msg_size_bytes_ (msg_size_bytes), //
        msgs_ (msgs)
    {
      os_assert_throw(!scheduler::in_handler_mode (), EPERM);

      queue_addr_ = attr.mq_queue_address;
      queue_size_bytes_ = attr.mq_queue_size_bytes;
#if !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)
      std::size_t storage_size = msgs
          * (msg_size_bytes + 2 * sizeof(mqueue::index_t)
              + sizeof(mqueue::priority_t));
#endif
      if (queue_addr_ != nullptr)
        {
          os_assert_throw(queue_size_bytes_ > 0, EINVAL);
#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)
          os_assert_throw(
              queue_size_bytes_ >= (std::size_t) (msgs * msg_size_bytes),
              EINVAL);
#else
          os_assert_throw(queue_size_bytes_ >= storage_size, EINVAL);
#endif
        }

      trace::printf ("%s() @%p %s %d %d\n", __func__, this, name (), msgs_,
                     msg_size_bytes_);

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      count_ = 0;
      port::Message_queue::create (this);

#else

      head_ = mqueue::no_index;

      // The array of prev indexes follows immediately after the content array.
      prev_array_ = (mqueue::index_t*) ((char*) queue_addr_
          + msgs * msg_size_bytes);
      // The array of next indexes follows immediately the prev array.
      next_array_ = (mqueue::index_t*) ((char*) prev_array_
          + msgs * sizeof(mqueue::index_t));
      // The array of priorities follows immediately the next array.
      prio_array_ = (mqueue::priority_t*) ((char*) next_array_
          + msgs * sizeof(mqueue::index_t));

      if (queue_addr_ == nullptr)
        {
          // Dynamically allocate queue and the arrays.
          queue_addr_ = new (std::nothrow) char[storage_size];
          flags_ |= flags_allocated;
        }

      _init ();
#endif
    }

    /**
     * @details
     * This destructor shall destroy the message queue object; the object
     * becomes, in effect, uninitialised. An implementation may cause
     * the destructor to set the object to an invalid value.
     *
     * It shall be safe to destroy an initialised message queue object
     * upon which no threads are currently blocked. Attempting to
     * destroy a message queue object upon which other threads are
     * currently blocked results in undefined behaviour.
     *
     * If the storage for the message queue was dynamically allocated,
     * it is deallocated.
     *
     */
    Message_queue::~Message_queue ()
    {
      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      port::Message_queue::destroy (this);

#else

      if (flags_ | flags_allocated)
        {
          delete[] ((char*) queue_addr_);
        }

#endif
    }

    void
    Message_queue::_init (void)
    {
      count_ = 0;

#if !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      // Construct a linked list of blocks. Store the pointer at
      // the beginning of each block. Each block
      // will hold the address of the next free block,
      // or `nullptr` at the end.
      char* p = (char*) queue_addr_;
      for (std::size_t i = 1; i < msgs_; ++i)
        {
          *(void**) p = (p + msg_size_bytes_);
          p += msg_size_bytes_;
        }
      *(void**) p = nullptr;

      first_free_ = queue_addr_; // Pointer to first block.

      head_ = mqueue::no_index;

      if (!send_list_.empty ())
        {
          // Wake-up all threads, if any.
          send_list_.wakeup_all ();

          send_list_.clear ();
        }

      if (!receive_list_.empty ())
        {
          // Wake-up all threads, if any.
          receive_list_.wakeup_all ();

          receive_list_.clear ();
        }

#endif /* !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE) */

    }

#if !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

    bool
    Message_queue::_try_send (const char* msg, std::size_t nbytes,
                              mqueue::priority_t mprio)
    {
      char* dest;

        {
          interrupts::Critical_section cs; // ----- Critical section -----

          if (first_free_ == nullptr)
            {
              // No available space to send the message.
              return false;
            }

          // Get the address where the message will be copied.
          // This is the first free memory block.
          dest = (char*) first_free_;

          // Update to next free, if any (the last one has nullptr).
          first_free_ = *(void**) first_free_;

          // Using the address, compute the index in the array.
          std::size_t msg_ix = (dest - (char*) queue_addr_) / msg_size_bytes_;
          prio_array_[msg_ix] = mprio;

          if (head_ == mqueue::no_index)
            {
              // No other message in the queue, enlist this one
              // as head, with links to itself.
              head_ = (mqueue::index_t) msg_ix;
              prev_array_[msg_ix] = (mqueue::index_t) msg_ix;
              next_array_[msg_ix] = (mqueue::index_t) msg_ix;
            }
          else
            {
              std::size_t ix;
              // Arrange to insert between head and tail.
              ix = prev_array_[head_];
              // Check if the priority is higher than the head priority.
              if (mprio > prio_array_[head_])
                {
                  // Having the highest priority, the new message
                  // becomes the new head.
                  head_ = (mqueue::index_t) msg_ix;
                }
              else
                {
                  // If not higher than the head, try to insert at the tail,
                  // but advance up until the same priority is found.
                  while ((mprio > prio_array_[ix]))
                    {
                      ix = prev_array_[ix];
                    }
                }
              prev_array_[msg_ix] = (mqueue::index_t) ix;
              next_array_[msg_ix] = next_array_[ix];

              // Break the chain and insert the new index.
              std::size_t tmp_ix = next_array_[ix];
              next_array_[ix] = (mqueue::index_t) msg_ix;
              prev_array_[tmp_ix] = (mqueue::index_t) msg_ix;
            }

          // One more message added to the queue.
          ++count_;
        }

      // Copy message from user buffer to queue storage.
      std::memcpy (dest, msg, nbytes);
      if (nbytes < msg_size_bytes_)
        {
          // Fill in the remaining space with 0x00.
          std::memset (dest + nbytes, 0x00, msg_size_bytes_ - nbytes);
        }

        {
          interrupts::Critical_section cs; // ----- Critical section -----

          // Wake-up one thread, if any.
          receive_list_.wakeup_one ();
        }

      return true;
    }

#endif /* !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE) */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

    /**
     * @details
     * The `Message_queue::send()` function shall add the message
     * pointed to by the argument
     * _msg_ to the message queue. The _nbytes_ argument specifies the length
     * of the message, in bytes, pointed to by _msg_. The value of _nbytes_
     * shall be less than or equal to the _msg_size_bytes_ parameter of the
     * message queue object, or `send()` shall fail.
     *
     * If the specified message queue is not full, `send()`
     * shall behave
     * as if the message is inserted into the message queue at the
     * position indicated by the _mprio_ argument. A message with a
     * larger numeric value of _mprio_ shall be inserted before messages
     * with lower values of _mprio_. A message shall be inserted after
     * other messages in the queue, if any, with equal _mprio_. The
     * value of _mprio_ shall be less than `mqueue::max_priority`.
     *
     * If the specified message queue is full, `send()`
     * shall block
     * until space becomes available to enqueue the message, or
     * until `send()` is cancelled/interrupted.
     * If more than one
     * thread is waiting to send when space becomes available
     * in the message queue and the Priority Scheduling option is
     * supported, then the thread of the highest priority that has
     * been waiting the longest shall be unblocked to send its
     * message. Otherwise, it is unspecified which waiting thread
     * is unblocked.
     *
     * @par POSIX compatibility
     *  Inspired by [`mq_send()`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_send.html)
     *  with `O_NONBLOCK` not set,
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::send (const char* msg, std::size_t nbytes,
                         mqueue::priority_t mprio)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);
      os_assert_err(msg != nullptr, EINVAL);
      os_assert_err(nbytes >= msg_size_bytes_, EMSGSIZE);

      trace::printf ("%s(%p,%d,%d) @%p %s\n", __func__, msg, nbytes, mprio,
                     this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::send (this, msg, nbytes, mprio);

#else

      Thread& crt_thread = this_thread::thread ();
      DoubleListNodeThread node
        { crt_thread };

      for (;;)
        {

          if (_try_send (msg, nbytes, mprio))
            {
              return result::ok;
            }

            {
              // Add this thread to the message queue send waiting list.
              // It is removed when this block ends (after suspend()).
              Waiting_threads_list_guard<interrupts::Critical_section> lg
                { send_list_, node };

              this_thread::suspend ();
            }

          if (crt_thread.interrupted ())
            {
              return EINTR;
            }
        }

      /* NOTREACHED */
      return ENOTRECOVERABLE;

#endif
    }

    /**
     * @details
     * The `Message_queue::try_send()` function shall try to add the message
     * pointed to by the argument
     * _msg_ to the message queue. The _nbytes_ argument specifies the length
     * of the message, in bytes, pointed to by _msg_. The value of _nbytes_
     * shall be less than or equal to the _msg_size_bytes_ parameter of the
     * message queue object, or `try_send()` shall fail.
     *
     * If the message queue is not full, `try_send()`
     * shall behave
     * as if the message is inserted into the message queue at the
     * position indicated by the _mprio_ argument. A message with a
     * larger numeric value of _mprio_ shall be inserted before messages
     * with lower values of _mprio_. A message shall be inserted after
     * other messages in the queue, if any, with equal _mprio_. The
     * value of _mprio_ shall be less than `mqueue::max_priority`.
     *
     * If the message queue is full, the message shall
     * not be queued and `try_send()` shall return an error.
     *
     * @par POSIX compatibility
     *  Inspired by [`mq_send()`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_send.html)
     *  with `O_NONBLOCK` set,
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     *
     * @note Can be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::try_send (const char* msg, std::size_t nbytes,
                             mqueue::priority_t mprio)
    {
      os_assert_err(msg != nullptr, EINVAL);
      os_assert_err(nbytes >= msg_size_bytes_, EMSGSIZE);

      trace::printf ("%s(%p,%d,%d) @%p %s\n", __func__, msg, nbytes, mprio,
                     this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::try_send (this, msg, nbytes, mprio);

#else

      if (_try_send (msg, nbytes, mprio))
        {
          return result::ok;
        }
      else
        {
          return EAGAIN;
        }

#endif
    }

    /**
     * @details
     * The `Message_queue::timed_send()` function shall add the message
     * pointed to by the argument
     * _msg_ to the message queue. The _nbytes_ argument specifies the length
     * of the message, in bytes, pointed to by _msg_. The value of _nbytes_
     * shall be less than or equal to the _msg_size_bytes_ attribute of the
     * message queue object, or `timed_send()` shall fail.
     *
     * If the message queue is not full, `timed_send()` shall behave
     * as if the message is inserted into the message queue at the
     * position indicated by the _mprio_ argument. A message with a
     * larger numeric value of _mprio_ shall be inserted before messages
     * with lower values of _mprio_. A message shall be inserted after
     * other messages in the queue, if any, with equal _mprio_. The
     * value of _mprio_ shall be less than `mqueue::max_priority`.
     *
     * If the message queue is full, the wait for sufficient
     * room in the queue shall be terminated when the specified timeout
     * expires.
     *
     * The timeout shall expire after the number of time units (that
     * is when the value of that clock equals or exceeds (now()+timeout).
     * The resolution of the timeout shall be the resolution of the
     * clock on which it is based.
     *
     * Under no circumstance shall the operation fail with a timeout
     * if there is sufficient room in the queue to add the message
     * immediately.
     *
     * If there is not sufficient room in the queue, the
     *
     * @par POSIX compatibility
     *  Inspired by [`mq_timedsend()`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_timedsend.html)
     *  with `O_NONBLOCK` not set,
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     *  <br>Differences from the standard:
     *  - the timeout is not expressed as an absolute time point, but
     * as a relative number of timer ticks (by default, the SysTick
     * clock for CMSIS).
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::timed_send (const char* msg, std::size_t nbytes,
                               mqueue::priority_t mprio, duration_t timeout)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);
      os_assert_err(msg != nullptr, EINVAL);
      os_assert_err(nbytes >= msg_size_bytes_, EMSGSIZE);

      trace::printf ("%s(%p,%d,%d,%d_ticks) @%p %s\n", __func__, msg, nbytes,
                     mprio, timeout, this, name ());

      if (timeout == 0)
        {
          timeout = 1;
        }

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::timed_send (this, msg, nbytes, mprio, timeout);

#else

      Thread& crt_thread = this_thread::thread ();
      DoubleListNodeThread node
        { crt_thread };

      Systick_clock::rep start = Systick_clock::now ();
      for (;;)
        {
          Systick_clock::sleep_rep slept_ticks;

          if (_try_send (msg, nbytes, mprio))
            {
              return result::ok;
            }

          Systick_clock::rep now = Systick_clock::now ();
          slept_ticks = (Systick_clock::sleep_rep) (now - start);
          if (slept_ticks >= timeout)
            {
              return ETIMEDOUT;
            }

            {
              // Add this thread to the message queue send waiting list.
              // It is removed when this block ends (after wait()).
              Waiting_threads_list_guard<interrupts::Critical_section> lg
                { send_list_, node };

              Systick_clock::wait (timeout - slept_ticks);
            }

          if (crt_thread.interrupted ())
            {
              return EINTR;
            }
        }

      /* NOTREACHED */
      return ENOTRECOVERABLE;

#endif
    }

#if !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

    bool
    Message_queue::_try_receive (char* msg, std::size_t nbytes,
                                 mqueue::priority_t* mprio)
    {
      char* src;

        {
          interrupts::Critical_section cs; // ----- Critical section -----

          if (head_ == mqueue::no_index)
            {
              return false;
            }

          src = (char*) queue_addr_ + head_ * msg_size_bytes_;
        }

      // Copy message from queue to user buffer.
      memcpy (msg, src, nbytes);
      if (mprio != nullptr)
        {
          *mprio = prio_array_[head_];
        }

        {
          interrupts::Critical_section cs; // ----- Critical section -----

          if (count_ > 1)
            {
              // Remove the current element from the list.
              prev_array_[next_array_[head_]] = prev_array_[head_];
              next_array_[prev_array_[head_]] = next_array_[head_];

              // Next becomes the new head.
              head_ = next_array_[head_];
            }
          else
            {
              // If there was only one, the list is empty now.
              head_ = mqueue::no_index;
            }

          // Perform a push_front() on the single linked LIFO list,
          // i.e. add the block to the beginning of the list.

          // Link previous list to this block; may be null, but it does
          // not matter.
          *(void**) src = first_free_;

          // Now this block is the first one.
          first_free_ = src;

          --count_;
        }

        {
          interrupts::Critical_section cs; // ----- Critical section -----

          // Wake-up one thread, if any.
          send_list_.wakeup_one ();
        }

      return true;
    }

#endif /* !defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE) */

    /**
     * @details
     * The `Message_queue::receive()` function shall receive the oldest
     * of the highest
     * priority message(s) from the message queue. If the size of the
     * buffer in bytes, specified by the _nbytes_ argument, is less than
     * the _msg_size_bytes_ attribute of the message queue, the function
     * shall fail and return an error. Otherwise, the selected message
     * shall be removed from the queue and copied to the buffer pointed
     * to by the _msg_ argument.
     *
     * If the value of _nbytes_ is greater than `mqueue::max_size`, the result
     * is implementation-defined.
     *
     * If the argument _mprio_ is not nullptr, the priority of the selected
     * message shall be stored in the location referenced by _mprio_.
     *
     * If the message queue is empty, `receive()` shall block
     * until a message is enqueued on the message queue or until
     * `receive()` is cancelled/interrupted. If more than one thread
     * is waiting to receive a message when a message arrives at
     * an empty queue and the Priority Scheduling option is supported,
     * then the thread of highest priority that has been waiting the
     * longest shall be selected to receive the message. Otherwise,
     * it is unspecified which waiting thread receives the message.
     *
     * @par POSIX compatibility
     *  Inspired by [`mq_receive()`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_receive.html)
     *  with `O_NONBLOCK` not set,
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::receive (char* msg, std::size_t nbytes,
                            mqueue::priority_t* mprio)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);
      os_assert_err(msg != nullptr, EINVAL);
      os_assert_err(nbytes >= msg_size_bytes_, EMSGSIZE);
      os_assert_err(nbytes <= mqueue::max_size, EMSGSIZE);

      trace::printf ("%s(%p,%d) @%p %s\n", __func__, msg, nbytes, this,
                     name ());

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::receive (this, msg, nbytes, mprio);

#else

      Thread& crt_thread = this_thread::thread ();
      DoubleListNodeThread node
        { crt_thread };

      for (;;)
        {

          if (_try_receive (msg, nbytes, mprio))
            {
              return result::ok;
            }

            {
              // Add this thread to the message queue receive waiting list.
              // It is removed when this block ends (after suspend()).
              Waiting_threads_list_guard<interrupts::Critical_section> lg
                { receive_list_, node };

              this_thread::suspend ();
            }

          if (crt_thread.interrupted ())
            {
              return EINTR;
            }
        }

      /* NOTREACHED */
      return ENOTRECOVERABLE;

#endif
    }

    /**
     * @details
     * The `Message_queue::try_receive()` function shall try to receive the
     * oldest of the highest
     * priority message(s) from the message queue. If the size of the
     * buffer in bytes, specified by the nbytes argument, is less than
     * the _msg_size_bytes_ attribute of the message queue, the function
     * shall fail and return an error. Otherwise, the selected message
     * shall be removed from the queue and copied to the buffer pointed
     * to by the _msg_ argument.
     *
     * If the value of _nbytes_ is greater than `mqueue::max_size`, the result
     * is implementation-defined.
     *
     * If the argument _mprio_ is not nullptr, the priority of the selected
     * message shall be stored in the location referenced by _mprio_.
     *
     * If the message queue is empty, no message shall be removed
     * from the queue, and `try_receive()` shall return an error.
     *
     * @par POSIX compatibility
     *  Inspired by [`mq_receive()`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_receive.html)
     *  with `O_NONBLOCK` set,
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     *
     * @note Can be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::try_receive (char* msg, std::size_t nbytes,
                                mqueue::priority_t* mprio)
    {
      os_assert_err(msg != nullptr, EINVAL);
      os_assert_err(nbytes >= msg_size_bytes_, EMSGSIZE);
      os_assert_err(nbytes <= mqueue::max_size, EMSGSIZE);

      trace::printf ("%s(%p,%d) @%p %s\n", __func__, msg, nbytes, this,
                     name ());

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::try_receive (this, msg, nbytes, mprio);

#else

      if (_try_receive (msg, nbytes, mprio))
        {
          return result::ok;
        }
      else
        {
          return EAGAIN;
        }

#endif
    }

    /**
     * @details
     * The `Message_queue::timed_receive()` function shall receive the
     * oldest of the highest
     * priority message(s) from the message queue. If the size of the
     * buffer in bytes, specified by the _nbytes_ argument, is less than
     * the _msg_size_bytes_ attribute of the message queue, the function
     * shall fail and return an error. Otherwise, the selected message
     * shall be removed from the queue and copied to the buffer pointed
     * to by the _msg_ argument.
     *
     * If the value of _nbytes_ is greater than `mqueue::max_size`, the result
     * is implementation-defined.
     *
     * If the argument _mprio_ is not nullptr, the priority of the selected
     * message shall be stored in the location referenced by _mprio_.
     *
     * If the message queue is empty, `timed_receive()` shall block
     * until a message is enqueued on the message queue or until
     * `timed_receive()` is cancelled/interrupted. If more than one thread
     * is waiting to receive a message when a message arrives at
     * an empty queue and the Priority Scheduling option is supported,
     * then the thread of highest priority that has been waiting the
     * longest shall be selected to receive the message. Otherwise,
     * it is unspecified which waiting thread receives the message.
     *
     * The `timed_receive()` function shall receive the oldest of
     * the highest priority messages from the message queue as described
     * for the `receive()` function. However, if no message exists on the
     * queue to satisfy the receive, the wait for such a message shall
     * be terminated when the specified timeout expires.
     *
     * The timeout shall expire after the number of time units (that
     * is when the value of that clock equals or exceeds (now()+duration).
     * The resolution of the timeout shall be the resolution of the
     * clock on which it is based.
     *
     * Under no circumstance shall the operation fail with a timeout
     * if a message can be removed from the message queue immediately.
     *
     * Compatible with POSIX `mq_receive()` with O_NONBLOCK set.
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_receive.html#
     *
     * @par POSIX compatibility
     *  Inspired by [`mq_timedreceive()`](http://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_timedreceive.html)
     *  with `O_NONBLOCK` not set,
     *  from [<mqueue.h>](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/mqueue.h.html)
     *  ([IEEE Std 1003.1, 2013 Edition](http://pubs.opengroup.org/onlinepubs/9699919799/nframe.html)).
     *  <br>Differences from the standard:
     *  - the timeout is not expressed as an absolute time point, but
     * as a relative number of timer ticks (by default, the SysTick
     * clock for CMSIS).
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::timed_receive (char* msg, std::size_t nbytes,
                                  mqueue::priority_t* mprio, systicks_t timeout)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);
      os_assert_err(msg != nullptr, EINVAL);
      os_assert_err(nbytes >= msg_size_bytes_, EMSGSIZE);
      os_assert_err(nbytes <= mqueue::max_size, EMSGSIZE);

      trace::printf ("%s(%p,%d,%d_ticks) @%p %s\n", __func__, msg, nbytes,
                     timeout, this, name ());

      if (timeout == 0)
        {
          timeout = 1;
        }

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::timed_receive (this, msg, nbytes, mprio,
          timeout);

#else

      Thread& crt_thread = this_thread::thread ();
      DoubleListNodeThread node
        { crt_thread };

      Systick_clock::rep start = Systick_clock::now ();
      for (;;)
        {
          Systick_clock::sleep_rep slept_ticks;

          if (_try_receive (msg, nbytes, mprio))
            {
              return result::ok;
            }

          Systick_clock::rep now = Systick_clock::now ();
          slept_ticks = (Systick_clock::sleep_rep) (now - start);
          if (slept_ticks >= timeout)
            {
              return ETIMEDOUT;
            }

            {
              // Add this thread to the message queue receive waiting list.
              // It is removed when this block ends (after wait()).
              Waiting_threads_list_guard<interrupts::Critical_section> lg
                { receive_list_, node };

              Systick_clock::wait (timeout - slept_ticks);
            }

          if (crt_thread.interrupted ())
            {
              return EINTR;
            }
        }

      /* NOTREACHED */
      return ENOTRECOVERABLE;

#endif
    }

#pragma GCC diagnostic pop

    /**
     * @details
     * Clear both send and receive counter and return the queue to the
     * initial state.
     *
     * @par POSIX compatibility
     *  Extension to standard, no POSIX similar functionality identified.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Message_queue::reset (void)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_MESSAGE_QUEUE)

      return port::Message_queue::reset (this);

#else

      interrupts::Critical_section cs; // ----- Critical section -----

      _init ();
      return result::ok;

#endif
    }

  // --------------------------------------------------------------------------

  } /* namespace rtos */
} /* namespace os */
