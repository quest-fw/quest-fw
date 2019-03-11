// ====================================================================== 
// \title  BlspSpiDriver.hpp
// \author tcanham
// \brief  cpp file for BlspSpiDriver test harness implementation class
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

#include "Tester.hpp"
#include <SnapdragonFlight/DspRpcAllocator/DspRpcAllocator.hpp>

#define INSTANCE 0
#define MAX_HISTORY_SIZE 10

namespace SnapdragonFlight {

  // ----------------------------------------------------------------------
  // Construction and destruction 
  // ----------------------------------------------------------------------

  Tester ::
    Tester(void) : 
#if FW_OBJECT_NAMES == 1
    BlspSpiDriverTesterBase("Tester", MAX_HISTORY_SIZE),
      component("BlspSpiDriver")
#else
    BlspSpiDriverTesterBase(MAX_HISTORY_SIZE),
      component()
#endif
  {
    this->initComponents();
    this->connectPorts();
  }

  Tester ::
    ~Tester(void) 
  {
    
  }

  // ----------------------------------------------------------------------
  // Helper methods 
  // ----------------------------------------------------------------------

  void Tester ::
    connectPorts(void) 
  {

    // SpiReadWrite
    this->connect_to_SpiReadWrite(
        0,
        this->component.get_SpiReadWrite_InputPort(0)
    );

    // Tlm
    this->component.set_Tlm_OutputPort(
        0, 
        this->get_from_Tlm(0)
    );

    // Time
    this->component.set_Time_OutputPort(
        0, 
        this->get_from_Time(0)
    );

    // Log
    this->component.set_Log_OutputPort(
        0, 
        this->get_from_Log(0)
    );

    // LogText
    this->component.set_LogText_OutputPort(
        0, 
        this->get_from_LogText(0)
    );

  }

  void Tester ::
    initComponents(void) 
  {
    this->init();
    this->component.init(
        INSTANCE
    );

    this->component.open(8,SPI_FREQUENCY_1MHZ);
  }

  void Tester::textLogIn(const FwEventIdType id, //!< The event ID
          Fw::Time& timeTag, //!< The time
          const Fw::TextLogSeverity severity, //!< The severity
          const Fw::TextLogString& text //!< The event string
          ) {
      TextLogEntry e = { id, timeTag, severity, text };

      printTextLogHistoryEntry(e, stdout);
  }

  void Tester::sendBuffer(BYTE* buffer, NATIVE_INT_TYPE size) {
      Fw::Buffer w;

      DspRpcAllocator allocator;


      w.setdata((U64)buffer);
      w.setsize(size);

      printf("WRITE: ");
      for (NATIVE_INT_TYPE byte = 0; byte < size; byte++) {
          printf("0x%02X ",buffer[byte]);
      }
      printf("\n");

      BYTE* rb = 0;
      rb = (BYTE*) allocator.allocate(0,size);


      Fw::Buffer r(0,0,(U64)rb,size);

      this->invoke_to_SpiReadWrite(0,w,r);

      BYTE* d = (BYTE*)r.getdata();
      printf("READ: ");
      for (NATIVE_INT_TYPE byte = 0; byte < size; byte++) {
          printf("0x%02X ",d[byte]);
      }
      printf("\n");
  }

} // end namespace SnapdragonFlight
