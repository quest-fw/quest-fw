// ======================================================================
// \title  BasicMixerImpl.hpp
// \author mereweth
// \brief  hpp file for BasicMixer component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the Office
// of Technology Transfer at the California Institute of Technology.
//
// This software may be subject to U.S. export control laws and
// regulations.  By accepting this document, the user agrees to comply
// with all U.S. export laws and regulations.  User has the
// responsibility to obtain export licenses, or other export authority
// as may be required before exporting such information to foreign
// countries or providing access to foreign persons.
// ======================================================================

#ifndef BasicMixer_HPP
#define BasicMixer_HPP

#include "Gnc/Ctrl/BasicMixer/BasicMixerComponentAc.hpp"
#include "quest_gnc/mixer/basic_mixer.h"

namespace Gnc {

  class BasicMixerComponentImpl :
    public BasicMixerComponentBase
  {

    public:

      // ----------------------------------------------------------------------
      // Construction, initialization, and destruction
      // ----------------------------------------------------------------------

      //! Construct object BasicMixer
      //!
      BasicMixerComponentImpl(
#if FW_OBJECT_NAMES == 1
          const char *const compName /*!< The component name*/
#else
          void
#endif
      );

      //! Initialize object BasicMixer
      //!
      void init(
          const NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
      );

      //! Destroy object BasicMixer
      //!
      ~BasicMixerComponentImpl(void);

    PRIVATE:

      // ----------------------------------------------------------------------
      // Handler implementations for user-defined typed input ports
      // ----------------------------------------------------------------------

      //! Handler implementation for controls
      //!
      void controls_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          ROS::mav_msgs::TorqueThrust &TorqueThrust
      );

      //! Handler implementation for sched
      //!
      void sched_handler(
          const NATIVE_INT_TYPE portNum, /*!< The port number*/
          NATIVE_UINT_TYPE context /*!< The call order*/
      );

      quest_gnc::multirotor::BasicMixer basicMixer;

    };

} // end namespace Gnc

#endif
