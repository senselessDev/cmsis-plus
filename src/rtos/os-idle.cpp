/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2016 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cassert>

#include <cmsis-plus/rtos/os.h>
#include <cmsis-plus/rtos/port/os-inlines.h>

#include <cmsis-plus/diag/trace.h>

// ----------------------------------------------------------------------------

using namespace os;
using namespace os::rtos;

/**
 * @cond ignore
 */

#pragma GCC diagnostic push
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif
static thread_static<OS_INTEGER_RTOS_IDLE_STACK_SIZE_BYTES> os_idle_thread
  { "idle", os_idle, nullptr };
#pragma GCC diagnostic pop

void*
os_idle (thread::func_args_t args __attribute__((unused)))
{

  // The thread was created with the default priority, and the
  // idle thread must run with th lowest possible priority.

#if defined(OS_BOOL_RTOS_THREAD_IDLE_PRIORITY_BELOW_IDLE)
  // The CMSIS RTOS validator creates threads with `priority::idle`,
  // so, to be sure that the system idle thread has the lowest priority,
  // go one step below the idle priority.
  this_thread::thread ().sched_prio (thread::priority::idle-1);
#else
  this_thread::thread ().sched_prio (thread::priority::idle);
#endif

  while (true)
    {
      while (!scheduler::terminated_threads_list_.empty ())
        {
          waiting_thread_node* node;
            {
              interrupts::critical_section ics; // ----- Critical section -----
              node =
                  const_cast<waiting_thread_node*> (scheduler::terminated_threads_list_.head ());
              node->unlink ();
            }
          node->thread_._destroy ();

          this_thread::yield ();
        }

#if !defined(OS_USE_RTOS_PORT_SCHEDULER)
      port::scheduler::_wait_for_interrupt ();
#endif /* !defined(OS_USE_RTOS_PORT_SCHEDULER) */
      this_thread::yield ();
    }
}

/**
 * @endcond
 */

// ------------------------------------------------------------------------
