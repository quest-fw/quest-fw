// ====================================================================== 
// \title  File.cpp
// \author bocchino
// \brief  cpp file for FileUplink::File
//
// \copyright
// Copyright 2009-2016, by the California Institute of Technology.
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

#include <Svc/FileUplink/FileUplink.hpp>
#include <Fw/Types/Assert.hpp>

namespace Svc {

  Os::File::Status FileUplink::File ::
    open(const Fw::FilePacket::StartPacket& startPacket)
  {
    const U32 length = startPacket.destinationPath.length;
    char path[length + 1];
    memcpy(path, startPacket.destinationPath.value, length);
    path[length] = 0;
    this->size = startPacket.fileSize;
    Fw::LogStringArg logStringArg(path);
    this->name = logStringArg;
    CFDP::Checksum checksum;
    this->checksum = checksum;
    return this->osFile.open(path, Os::File::OPEN_WRITE);
  }

  Os::File::Status FileUplink::File ::
    write(
        const U8 *const data,
        const U32 byteOffset,
        const U32 length
    )
  {

    Os::File::Status status;
    status = this->osFile.seek(byteOffset);
    if (status != Os::File::OP_OK)
      return status;

    NATIVE_INT_TYPE intLength = length;
    // NOTE(mereweth) - don't sync file range
    status = this->osFile.write(data, intLength, false);
    if (status != Os::File::OP_OK)
      return status;

    FW_ASSERT(static_cast<U32>(intLength) == length, intLength);
    this->checksum.update(data, byteOffset, length);
    return Os::File::OP_OK;

  }

}
