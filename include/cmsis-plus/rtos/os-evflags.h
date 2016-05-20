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

#ifndef CMSIS_PLUS_RTOS_OS_EVFLAGS_H_
#define CMSIS_PLUS_RTOS_OS_EVFLAGS_H_

// ----------------------------------------------------------------------------

#if defined(__cplusplus)

#include <cmsis-plus/rtos/os-decls.h>

// ----------------------------------------------------------------------------

namespace os
{
  namespace rtos
  {
    namespace evflags
    {

      // ======================================================================

      /**
       * @brief Event flags attributes.
       * @headerfile os.h <cmsis-plus/rtos/os.h>
       */
      class Attributes : public Clocked_attributes
      {
      public:

        /**
         * @name Constructors & Destructor
         * @{
         */

        /**
         * @brief Create event flags attributes.
         * @param [in] name Null terminated name. If `nullptr`, "-" is assigned.
         */
        constexpr
        Attributes (const char* name);

        /**
         * @cond ignore
         */
        Attributes (const Attributes&) = default;
        Attributes (Attributes&&) = default;
        Attributes&
        operator= (const Attributes&) = default;
        Attributes&
        operator= (Attributes&&) = default;
        /**
         * @endcond
         */

        /**
         * @brief Destroy the event flags attributes.
         */
        ~Attributes () = default;

        /**
         * @}
         */

      public:

        /**
         * @name Public Member Variables
         * @{
         */

        // Public members, no accessors and mutators required.
        // Warning: must match the type & order of the C file header.
        //
        // TODO: add clock ID.
        //
        // Add more attributes.
        /**
         * @}
         */
      };

      /**
       * @brief Default event flags initialiser.
       */
      extern const Attributes initializer;

    } /* namespace mqueue */

    // ========================================================================

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"

    /**
     * @brief Synchronised **event flags**.
     * @headerfile os.h <cmsis-plus/rtos/os.h>
     * @ingroup cmsis-plus-rtos
     */
    class Event_flags : public Named_object
    {
    public:

      /**
       * @name Constructors & Destructor
       * @{
       */

      /**
       * @brief Create event flags with default attributes.
       * @par Parameters
       *  None
       */
      Event_flags ();

      /**
       * @brief Create named event flags with default attributes.
       * @param [in] name Pointer to name.
       */
      Event_flags (const char* name);

      /**
       * @brief Create event flags with custom settings.
       * @param [in] attr Reference to attributes.
       */
      Event_flags (const evflags::Attributes& attr);

      /**
       * @brief Create named event flags with custom settings.
       * @param [in] name Pointer to name.
       * @param [in] attr Reference to attributes.
       */
      Event_flags (const char* name, const evflags::Attributes& attr);

      /**
       * @cond ignore
       */
      Event_flags (const Event_flags&) = delete;
      Event_flags (Event_flags&&) = delete;
      Event_flags&
      operator= (const Event_flags&) = delete;
      Event_flags&
      operator= (Event_flags&&) = delete;
      /**
       * @endcond
       */

      /**
       * @brief Destroy the event flags.
       */
      ~Event_flags ();

      /**
       * @}
       * @name Operators
       * @{
       */

      /**
       * @brief Compare event flags.
       * @retval true The given event flags object is the same as this
       *  event flags.
       * @retval false The event flags are different.
       */
      bool
      operator== (const Event_flags& rhs) const;

      /**
       * @}
       */

    public:

      /**
       * @name Public Member Functions
       * @{
       */

      /**
       * @brief Wait for event flags.
       * @param [in] mask The expected flags (OR-ed bit-mask);
       *  may be zero.
       * @param [out] oflags Pointer where to store the current flags;
       *  may be `nullptr`.
       * @param [in] mode Mode bits to select if either all or any flags
       *  are expected, and if the flags should be cleared.
       * @retval result::ok All expected flags were raised.
       * @retval EPERM Cannot be invoked from an Interrupt Service Routines.
       * @retval EINVAL The mask is outside of the permitted range.
       * @retval EINTR The operation was interrupted.
       * @retval ENOTRECOVERABLE Wait failed.
       */
      result_t
      wait (flags::mask_t mask, flags::mask_t* oflags,
            flags::mode_t mode = flags::mode::all | flags::mode::clear);

      /**
       * @brief Try to wait for event flags.
       * @param [in] mask The expected flags (OR-ed bit-mask);
       *  may be zero.
       * @param [out] oflags Pointer where to store the current flags;
       *  may be `nullptr`.
       * @param [in] mode Mode bits to select if either all or any flags
       *  are expected, and if the flags should be cleared.
       * @retval result::ok All expected flags were raised.
       * @retval EINVAL The mask is outside of the permitted range.
       * @retval EWOULDBLOCK The expected condition did not occur.
       * @retval ENOTRECOVERABLE Wait failed.
       */
      result_t
      try_wait (flags::mask_t mask, flags::mask_t* oflags = nullptr,
                flags::mode_t mode = flags::mode::all | flags::mode::clear);

      /**
       * @brief Timed wait for event flags.
       * @param [in] mask The expected flags (OR-ed bit-mask);
       *  may be zero.
       * @param [out] oflags Pointer where to store the current flags;
       *  may be `nullptr`.
       * @param [in] mode Mode bits to select if either all or any flags
       *  are expected, and if the flags should be cleared.
       * @param [in] timeout Timeout to wait.
       * @retval result::ok All expected flags are raised.
       * @retval EPERM Cannot be invoked from an Interrupt Service Routines.
       * @retval ETIMEDOUT The expected condition did not occur during the
       *  entire timeout duration.
       * @retval EINVAL The mask is outside of the permitted range.
       * @retval EINTR The operation was interrupted.
       * @retval ENOTRECOVERABLE Wait failed.
       */
      result_t
      timed_wait (flags::mask_t mask, clock::duration_t timeout,
                  flags::mask_t* oflags = nullptr,
                  flags::mode_t mode = flags::mode::all | flags::mode::clear);

      /**
       * @brief Raise event flags.
       * @param [in] mask The OR-ed flags to raise.
       * @param [out] oflags Optional pointer where to store the
       *  new value of the flags; may be `nullptr`.
       * @retval result::ok The flags were raised.
       * @retval EINVAL The mask is zero.
       * @retval ENOTRECOVERABLE Raise failed.
       */
      result_t
      raise (flags::mask_t mask, flags::mask_t* oflags = nullptr);

      /**
       * @brief Clear event flags.
       * @param [in] mask The OR-ed flags to clear.
       * @param [out] oflags Optional pointer where to store the
       *  previous value of the flags; may be `nullptr`.
       * @retval result::ok The flags were cleared.
       * @retval EINVAL The mask is zero.
       */
      result_t
      clear (flags::mask_t mask, flags::mask_t* oflags = nullptr);

      /**
       * @brief Get (and possibly clear) event flags.
       * @param [in] mask The OR-ed flags to get/clear; may be zero.
       * @param [in] mode Mode bits to select if the flags should be
       *  cleared (the other bits are ignored).
       * @return The selected bits from the flags mask.
       */
      flags::mask_t
      get (flags::mask_t mask, flags::mode_t mode = flags::mode::clear);

      /**
       * @brief Check if there are threads waiting.
       * @par Parameters
       *  None
       * @retval true There are threads waiting.
       * @retval false There are no threads waiting.
       */
      bool
      waiting (void);

      /**
       * @}
       */

    protected:

      /**
       * @name Private Member Functions
       * @{
       */

      /**
       * @brief Internal function to check the flags condition.
       * @param [in] mask The expected flags (OR-ed bit-mask);
       *  may be zero.
       * @param [out] oflags Pointer where to store the current flags;
       *  may be `nullptr`.
       * @param [in] mode Mode bits to select if either all or any flags
       *  are expected, and if the flags should be cleared.
       * @retval result::ok All expected flags are raised.
       * @retval EINVAL The mask is outside of the permitted range.
       * @retval EWOULDBLOCK The expected condition did not occur.
       * @retval ENOTRECOVERABLE Wait failed.
       */
      bool
      _try_wait (flags::mask_t mask, flags::mask_t* oflags, flags::mode_t mode);

      /**
       * @}
       */

    protected:

      /**
       * @name Private Member Variables
       * @{
       */

#if !defined(OS_INCLUDE_RTOS_PORT_EVENT_FLAGS)
      Waiting_threads_list list_;
      Clock& clock_;
#endif

#if defined(OS_INCLUDE_RTOS_PORT_EVENT_FLAGS)
      friend class port::Event_flags;
      os_evflags_port_data_t port_;
#endif

      /**
       * @brief The event flags.
       */
      flags::mask_t flags_;

      /**
       * @}
       */
    };

#pragma GCC diagnostic pop

  } /* namespace rtos */
} /* namespace os */

// ===== Inline & template implementations ====================================

namespace os
{
  namespace rtos
  {
    namespace evflags
    {
      constexpr
      Attributes::Attributes (const char* name) :
          Clocked_attributes
            { name }
      {
        ;
      }
    } /* namespace evflags */

  } /* namespace rtos */
} /* namespace os */

// ----------------------------------------------------------------------------

#endif /* __cplusplus */

#endif /* CMSIS_PLUS_RTOS_OS_EVFLAGS_H_ */
