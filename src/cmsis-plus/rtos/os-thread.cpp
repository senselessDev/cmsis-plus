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

// ----------------------------------------------------------------------------

namespace os
{
  namespace rtos
  {
    // ------------------------------------------------------------------------

#pragma GCC diagnostic push
// TODO: remove it when fully implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

#if 0
    void*
    no_thread_func (void* args);

    void*
    no_thread_func (void* args)
      {
        return nullptr;
      }

    Thread no_thread
      { nullptr, (thread::func_t) no_thread_func, nullptr};
#endif

    namespace this_thread
    {
      /**
       * @details
       *
       * @warning Cannot be invoked from Interrupt Service Routines.
       */
      Thread&
      thread (void)
      {
        os_assert_throw(!scheduler::in_handler_mode (), EPERM);

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

        return port::this_thread::thread ();

#else

        // TODO

#endif
      }

      /**
       * @details
       * Pass control to next thread that is in \b READY state.
       *
       * @warning Cannot be invoked from Interrupt Service Routines.
       */
      void
      yield (void)
      {
        os_assert_throw(!scheduler::in_handler_mode (), EPERM);

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

        return port::this_thread::yield ();

#else

        // TODO

#endif
      }

    } /* namespace this_thread */

    // ======================================================================

    namespace thread
    {
      const Attributes initializer
        { nullptr };
    } /* namespace thread */

#if 0

    static unsigned portBASE_TYPE
    makeFreeRtosPriority (thread::priority_t priority)
      {
        unsigned portBASE_TYPE fr_prio = tskIDLE_PRIORITY;

        fr_prio += (priority - thread::priority::idle);

        return fr_prio;
      }

    static thread::priority_t
    makeCmsisPriority (unsigned portBASE_TYPE priority)
      {
        thread::priority_t cm_prio = thread::priority::idle;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
        cm_prio += (thread::priority_t) (priority - tskIDLE_PRIORITY);
#pragma GCC diagnostic pop

        return cm_prio;
      }
#endif

    /**
     * @details
     * Same as in POSIX, thread functions can return, and the behaviour
     * should be as the thread called the exit() function.
     *
     * This requires a proxy to run the thread function, get the result,
     * and explicitly invoke exit().
     */
    void
    Thread::_invoke_with_exit (Thread* thread)
    {
      thread->exit (thread->func_ (thread->func_args_));
    }

    /**
     * @details
     *
     * Create a new thread, with default attributes.
     *
     * The thread is created executing function with args as its
     * sole argument. If the start_routine returns, the effect
     * shall be as if there was an implicit call to exit() using
     * the return value of function as the exit status. Note that
     * the thread in which main() was originally invoked differs
     * from this. When it returns from main(), the effect shall
     * be as if there was an implicit call to exit() using the
     * return value of main() as the exit status.
     *
     * Compatible with pthread_create().
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    Thread::Thread (thread::func_t function, thread::func_args_t args) :
        Thread
          { thread::initializer, function, args }
    {
      ;
    }

    /**
     * @details
     *
     * Create a new thread, with attributes specified by attr.
     * If attr is NULL, the default attributes shall be used.
     * If the attributes specified by attr are modified later,
     * the thread's attributes shall not be affected.
     *
     * The thread is created executing function with args as its
     * sole argument. If the start_routine returns, the effect
     * shall be as if there was an implicit call to exit() using
     * the return value of function as the exit status. Note that
     * the thread in which main() was originally invoked differs
     * from this. When it returns from main(), the effect shall
     * be as if there was an implicit call to exit() using the
     * return value of main() as the exit status.
     *
     * Compatible with pthread_create().
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_create.html
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    Thread::Thread (const thread::Attributes& attr, thread::func_t function,
                    thread::func_args_t args) :
        Named_object
          { attr.name () }
    {
      os_assert_throw(!scheduler::in_handler_mode (), EPERM);

      assert(function != nullptr);
      assert(attr.th_priority != thread::priority::none);

      // Prevent the new thread to execute before all members are set.
      scheduler::Critical_section cs; // ----- Critical section -----

      // Get attributes from user structure.
      prio_ = attr.th_priority;
      stack_size_bytes_ = attr.th_stack_size_bytes;
      stack_addr_ = attr.th_stack_address;

      func_ = function;
      func_args_ = args;

      sig_mask_ = 0;

      trace::printf ("%s @%p %s %d %d\n", __func__, this, name (), prio_,
                     stack_size_bytes_);

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      port::Thread::create (this);
      sched_state_ = thread::state::ready;

#else

      // TODO: check min size
      // TODO: align stack
      if (stack_addr_ == nullptr)
        {
          // TODO: alloc default stack size
        }
      state_ = thread::state::inactive;

      scheduler::__register_thread (this);

#endif
    }

    /**
     * @details
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    Thread::~Thread ()
    {
      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      port::Thread::destroy (this);

#else

      // TODO
      scheduler::__unregister_thread (this);

#endif

      sched_state_ = thread::state::destroyed;
    }

    void
    Thread::suspend (void)
    {
      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      port::Thread::suspend (this);

#else

      // TODO

#endif
    }

    /**
     * @details
     * Internal, no POSIX equivalent.
     *
     * @note Can be invoked from Interrupt Service Routines.
     */
    void
    Thread::wakeup (void)
    {
      trace::printf ("%s() @%p %s\n", __func__, this, name ());
      wakeup_reason_ = result::ok;

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      port::Thread::wakeup (this);

#else

      // TODO

#endif
    }

#if 0
    /**
     * @details
     * Internal, no POSIX equivalent, used to notify timeouts or cancels.
     *
     * @note Can be invoked from Interrupt Service Routines.
     */
    void
    Thread::wakeup (result_t reason)
      {
        assert(reason == EINTR || reason == ETIMEDOUT);

        trace::printf ("%s(&d) @%p %s \n", __func__, reason, this, name ());
        wakeup_reason_ = reason;

        // TODO
      }
#endif

    /**
     * @details
     *
     * No POSIX equivalent.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    thread::priority_t
    Thread::sched_prio (void)
    {
      os_assert_err(!scheduler::in_handler_mode (), thread::priority::error);

      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      return port::Thread::sched_prio (this);

#else

      // TODO
      return prio_;

#endif

    }

    /**
     * @details
     * Set the scheduling priority for the thread to the value given
     * by prio.
     *
     * pthread_setschedprio()
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_setschedprio.html
     *
     * If an implementation detects use of a thread ID after the end
     * of its lifetime, it is recommended that the function should
     * fail and report an [ESRCH] error.
     *
     * The pthread_setschedprio() function shall not return an error
     * code of [EINTR].
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::sched_prio (thread::priority_t prio)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      assert(prio != thread::priority::none);

      trace::printf ("%s(%d) @%p %s\n", __func__, prio, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      return port::Thread::sched_prio (this, prio);

#else

      // TODO
      prio_ = prio;
      return result::ok;

#endif
    }

    /**
     * @details
     * Suspend execution of the calling thread until the target thread
     * terminates, unless the target thread has already terminated.
     * On return from a successful join() call with a non-NULL
     * exit_ptr argument, the value passed to exit() by the
     * terminating thread shall be made available in the location
     * referenced by exit_ptr. When a join() returns successfully,
     * the target thread has been terminated. The results of
     * multiple simultaneous calls to join() specifying the
     * same target thread are undefined. If the thread calling
     * join() is cancelled, then the target thread shall not be
     * detached.
     *
     * pthread_join()
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_join.html
     *
     * The join() function may fail if:
     * [EDEADLK] A deadlock was detected.
     *
     * The join() function shall not return an error code of [EINTR].
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::join (void** exit_ptr)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      // TODO: Must fail if current thread

      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      result_t res = port::Thread::join (this);
      if (res != result::ok)
        {
          return res;
        }

#else

      // TODO

#endif

      if (exit_ptr != nullptr)
        {
          *exit_ptr = func_result_;
        }
      trace::printf ("%s() @%p %s joined\n", __func__, this, name ());

      return result::ok;
    }

    /**
     * @details
     * Indicate to the implementation that storage for the thread
     * thread can be reclaimed when that thread terminates. If
     * thread has not terminated, detach() shall not cause it
     * to terminate. The behaviour is undefined if the value
     * specified by the thread argument to detach() does not
     * refer to a joinable thread.
     *
     * pthread_detach()
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_detach.html
     *
     * The detach() function shall not return an error code of [EINTR].
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::detach (void)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      trace::printf ("%s() @%p %s\n", __func__, this, name ());

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      result_t res = port::Thread::detach (this);
      if (res != result::ok)
        {
          return res;
        }

#else

      // TODO

#endif

      return result::ok;
    }

    /**
     * @details
     * pthread_cancel()
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_cancel.html
     *
     *
     * The cancel() function shall not return an error code of [EINTR].
     * If an implementation detects use of a thread ID after the end
     * of its lifetime, it is recommended that the function should
     * fail and report an [ESRCH] error.
     * error number is returned.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::cancel (void)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      trace::printf ("%s() @%p %s\n", __func__, this, name ());

      // TODO
      return result::ok;
    }

    bool
    Thread::interrupted (void)
    {
      return false;
    }

    /**
     * @details
     * Terminate the calling thread and make the value value_ptr
     * available to any successful join with the terminating thread.
     * Any cancellation cleanup handlers that have been pushed and
     * not yet popped shall be popped in the reverse order that
     * they were pushed and then executed. After all cancellation
     * cleanup handlers have been executed, if the thread has any
     * thread-specific data, appropriate destructor functions
     * shall be called in an unspecified order. Thread termination
     * does not release any application visible process resources,
     * including, but not limited to, mutexes and file descriptors,
     * nor does it perform any process-level cleanup actions,
     * including, but not limited to, calling any atexit() routines
     * that may exist.
     * An implicit call to exit() is made when a thread other
     * than the thread in which main() was first invoked returns
     * from the start routine that was used to create it.
     * The function's return value shall serve as the thread's
     * exit status.
     * The behaviour of exit() is undefined if called from a
     * cancellation cleanup handler or destructor function that
     * was invoked as a result of either an implicit or explicit
     * call to exit().
     * After a thread has terminated, the result of access to
     * local (auto) variables of the thread is undefined.
     * Thus, references to local variables of the exiting
     * thread should not be used for the exit() value_ptr
     * parameter value.
     *
     * pthread_exit()
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_exit.html
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    void
    Thread::exit (void* value_ptr)
    {
      os_assert_throw(!scheduler::in_handler_mode (), EPERM);

      trace::printf ("%s() @%p %s\n", __func__, this, name ());

      if (sched_state_ == thread::state::terminated)
        {
          trace::printf ("%s() @%p %s already terminated\n", __func__, this,
                         name ());
          return; // Already terminated
        }

      // If not the current thread, probably must suspend.

      func_result_ = value_ptr;
      sched_state_ = thread::state::terminated;

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      port::Thread::exit (this);

#else

      // TODO

#endif
    }

    result_t
    Thread::kill (void)
    {
      result_t res;

#if defined(OS_INCLUDE_PORT_RTOS_THREAD)

      res = port::Thread::kill (this);

#else

      // TODO
      res = result::ok;

#endif
      sched_state_ = thread::state::inactive;

      return res;
    }

    /**
     * @details
     * Set more bits in the thread current signal flags mask.
     * Use OR at bit-mask level.
     * Wake-up the thread to evaluate the signal flagss.
     *
     * @note Can be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::sig_raise (thread::sigset_t mask, thread::sigset_t* oflags)
    {
      os_assert_err(mask != 0, EINVAL);

      Critical_section_irq cs; // ----- Critical section -----

      if (oflags != nullptr)
        {
          *oflags = sig_mask_;
        }

      sig_mask_ |= mask;

      wakeup ();

      return result::ok;
    }

    /**
     * @details
     * Select the requested bits from the thread current signal mask
     * and return them. If requested, clear the selected bits in the
     * thread signal mask.
     *
     * If the mask is zero, return the full thread signal mask,
     * without any masking or subsequent clearing.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    thread::sigset_t
    Thread::sig_get (thread::sigset_t mask, flags::mode_t mode)
    {
      os_assert_err(!scheduler::in_handler_mode (), sig::error);

      Critical_section_irq cs; // ----- Critical section -----

      if (mask == 0)
        {
          // Return the entire mask.
          return sig_mask_;
        }

      thread::sigset_t ret = sig_mask_ & mask;
      if ((mode & flags::mode::clear) != 0)
        {
          // Clear the selected bits; leave the rest untouched.
          sig_mask_ &= ~mask;
        }

      // Return the selected bits.
      return ret;
    }

    /**
     * @details
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::sig_clear (thread::sigset_t mask, thread::sigset_t* oflags)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);
      os_assert_err(mask != 0, EINVAL);

      Critical_section_irq cs; // ----- Critical section -----

      if (oflags != nullptr)
        {
          *oflags = sig_mask_;
        }

      // Clear the selected bits; leave the rest untouched.
      sig_mask_ &= ~mask;

      return result::ok;
    }

    /**
     * @details
     *
     * Internal function used to test if the desired signal flags are raised.
     */
    result_t
    Thread::_try_wait (thread::sigset_t mask, thread::sigset_t* oflags,
                       flags::mode_t mode)
    {
      if ((mask != 0) && ((mode & flags::mode::all) != 0))
        {
          // Only if all desires signals are raised we're done.
          if ((sig_mask_ & mask) == mask)
            {
              if (oflags != nullptr)
                {
                  *oflags = sig_mask_;
                }
              // Clear desired signals.
              sig_mask_ &= ~mask;
              return result::ok;
            }
        }
      else if ((mask == 0) || ((mode & flags::mode::any) != 0))
        {
          // Any signal will do it.
          if (sig_mask_ != 0)
            {
              // Possibly return .
              if (oflags != nullptr)
                {
                  *oflags = sig_mask_;
                }
              // Since we returned them all, also clear them all.
              sig_mask_ = 0;
              return result::ok;
            }
        }

      return EAGAIN;
    }

    /**
     * @details
     * If the mode::all bit is set, the function expects
     * all given flags to be raised; otherwise, if the mode::any
     * bit is set, the function expects any single flag to be raised.
     *
     * If the expected signal flags are
     * raised, the function returns instantly.
     *
     * Otherwise suspend the execution of the current thread until all/any
     * specified signal flags are raised.
     *
     * When the parameter mask is 0, the thread is suspended
     * until any signal flag is raised. In this case, if any signal flags
     * are already raised, the function returns instantly.
     *
     * If the mode::clear bit is set, the signal flags that are
     * returned are automatically cleared.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::sig_wait (thread::sigset_t mask, thread::sigset_t* oflags,
                      flags::mode_t mode)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      for (;;)
        {
            {
              Critical_section_irq cs; // ----- Critical section -----

              if (_try_wait (mask, oflags, mode) == result::ok)
                {
                  return result::ok;
                }
            }

          suspend ();

          if (interrupted ())
            {
              return EINTR;
            }
        }
      return ENOTRECOVERABLE;
    }

    /**
     * @details
     * If the mode::all bit is set, the function expects
     * all given flags to be raised; otherwise, if the mode::any
     * bit is set, the function expects any single flag to be raised.
     *
     * The function does not blocks, if the expected signal flags are
     * not raised, but returns EGAIN.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::try_sig_wait (thread::sigset_t mask, thread::sigset_t* oflags,
                          flags::mode_t mode)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      Critical_section_irq cs; // ----- Critical section -----

      return _try_wait (mask, oflags, mode);
    }

    /**
     * @details
     * If the mode::all bit is set, the function expects
     * all given flags to be raised; otherwise, if the mode::any
     * bit is set, the function expects any single flag to be raised.
     *
     * If the expected signal flags are
     * raised, the function returns instantly.
     *
     * Otherwise suspend the execution of the thread until all/any
     * specified signal flags are raised.
     *
     * When the parameter mask is 0, the thread is suspended
     * until any event flag is raised. In this case, if any signal flags
     * are already raised, the function returns instantly.
     *
     * The wait shall be terminated when the specified timeout
     * expires.
     *
     * The timeout shall expire after the number of time units (that
     * is when the value of that clock equals or exceeds (now()+duration).
     * The resolution of the timeout shall be the resolution of the
     * clock on which it is based (the SysTick clock for CMSIS).
     *
     * Under no circumstance shall the operation fail with a timeout
     * if the signal flags are already raised.
     *
     * If the mode::clear bit is set, the signal flags that are
     * returned are automatically cleared.
     *
     * @warning Cannot be invoked from Interrupt Service Routines.
     */
    result_t
    Thread::timed_sig_wait (thread::sigset_t mask, thread::sigset_t* oflags,
                            flags::mode_t mode, systicks_t ticks)
    {
      os_assert_err(!scheduler::in_handler_mode (), EPERM);

      if (ticks == 0)
        {
          ticks = 1;
        }

      Systick_clock::rep start = Systick_clock::now ();
      for (;;)
        {
          Systick_clock::sleep_rep slept_ticks;
            {
              Critical_section_irq cs; // ----- Critical section -----

              if (_try_wait (mask, oflags, mode) == result::ok)
                {
                  return result::ok;
                }
            }

          Systick_clock::rep now = Systick_clock::now ();
          slept_ticks = (Systick_clock::sleep_rep) (now - start);
          if (slept_ticks >= ticks)
            {
              return ETIMEDOUT;
            }

          Systick_clock::sleep_for (ticks - slept_ticks);

          if (interrupted ())
            {
              return EINTR;
            }
        }
      return ENOTRECOVERABLE;
    }

// --------------------------------------------------------------------------

#pragma GCC diagnostic pop

  } /* namespace rtos */
} /* namespace os */