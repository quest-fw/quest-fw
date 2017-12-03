// ======================================================================
// \title  Queue.cpp
// \author dinkel
// \brief  Queue implementation using the pthread library. This is NOT
//         an IPC queue. It is meant to be used between threads within 
//         the same address space.
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

#include <Os/Pthreads/BufferQueue.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Queue.hpp>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>

namespace Os {
  
  // A helper class which stores variables for the queue handle.
  // The queue itself, a pthread condition variable, and pthread
  // mutex are contained within this container class.
  class QueueHandle {
    public:
    QueueHandle() {
      int ret;
      ret = pthread_cond_init(&this->queueNotEmpty, NULL);
      FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.
      ret = pthread_cond_init(&this->queueNotFull, NULL);
      FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.

#ifdef BUILD_DSPAL
      pthread_mutexattr_t attr;
      ret = pthread_mutexattr_init(&attr);
      FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.
      ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
      FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.
      ret = pthread_mutex_init(&this->queueLock, &attr);
#else
      ret = pthread_mutex_init(&this->queueLock, NULL);
#endif // BUILD_DSPAL
      FW_ASSERT(ret == 0, ret); // If this fails, something horrible happened.
    }
    ~QueueHandle() { 
      (void) pthread_cond_destroy(&this->queueNotEmpty);
      (void) pthread_cond_destroy(&this->queueNotFull);
      (void) pthread_mutex_destroy(&this->queueLock);
    }
    bool create(NATIVE_INT_TYPE depth, NATIVE_INT_TYPE msgSize) {
      return queue.create(depth, msgSize);
    }
    BufferQueue queue;
    pthread_cond_t queueNotEmpty;
    pthread_cond_t queueNotFull;
    pthread_mutex_t queueLock;
  };

  Queue::Queue() :
    m_handle((POINTER_CAST) NULL) {
  }

  Queue::QueueStatus Queue::create(const Fw::StringBase &name, NATIVE_INT_TYPE depth, NATIVE_INT_TYPE msgSize) {
    QueueHandle* queueHandle = (QueueHandle*) this->m_handle;

    // Queue has already been created... remove it and try again:
    if (NULL != queueHandle) {
        delete queueHandle;
        queueHandle = NULL;
    }

    // Create queue handle:
    queueHandle = new QueueHandle;
    if (NULL == queueHandle) {
      return QUEUE_UNINITIALIZED;
    }
    if( !queueHandle->create(depth, msgSize) ) {
      return QUEUE_UNINITIALIZED;
    }
    this->m_handle = (POINTER_CAST) queueHandle;

#if FW_QUEUE_REGISTRATION
    if (this->s_queueRegistry) {
        this->s_queueRegistry->regQueue(this);
    }
#endif

    return QUEUE_OK;
  }

  Queue::~Queue() {
    // Clean up the queue handle:
    QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
    if (NULL != queueHandle) {
      delete queueHandle;
    }
    this->m_handle = (POINTER_CAST) NULL;
  }

  Queue::QueueStatus sendNonBlock(QueueHandle* queueHandle, const U8* buffer, NATIVE_INT_TYPE size, NATIVE_INT_TYPE priority) {

    BufferQueue* queue = &queueHandle->queue;
    pthread_cond_t* queueNotEmpty = &queueHandle->queueNotEmpty;
    pthread_mutex_t* queueLock = &queueHandle->queueLock;
    NATIVE_INT_TYPE ret;
    Queue::QueueStatus status = Queue::QUEUE_OK;

    ////////////////////////////////
    // Locked Section
    ///////////////////////////////
    ret = pthread_mutex_lock(queueLock);
    FW_ASSERT(ret == 0, errno);
    ///////////////////////////////

    // Push item onto queue:
    bool pushSucceeded = queue->push(buffer, size, priority);

    if(pushSucceeded) {
      // Push worked - wake up a thread that might be waiting on 
      // the other end of the queue:
      NATIVE_INT_TYPE ret = pthread_cond_signal(queueNotEmpty);
      FW_ASSERT(ret == 0, errno); // If this fails, something horrible happened.
    }
    else {
      // Push failed - the queue is full:
      status = Queue::QUEUE_FULL;
    }

    ///////////////////////////////
    ret = pthread_mutex_unlock(queueLock);
    FW_ASSERT(ret == 0, errno);
    ////////////////////////////////
    ///////////////////////////////
   
    return status;
  }

  Queue::QueueStatus sendBlock(QueueHandle* queueHandle, const U8* buffer, NATIVE_INT_TYPE size, NATIVE_INT_TYPE priority) {

    BufferQueue* queue = &queueHandle->queue;
    pthread_cond_t* queueNotEmpty = &queueHandle->queueNotEmpty;
    pthread_cond_t* queueNotFull = &queueHandle->queueNotFull;
    pthread_mutex_t* queueLock = &queueHandle->queueLock;
    NATIVE_INT_TYPE ret;

    ////////////////////////////////
    // Locked Section
    ///////////////////////////////
    ret = pthread_mutex_lock(queueLock);
    FW_ASSERT(ret == 0, errno);
    ///////////////////////////////

    // If the queue is full, wait until a message is taken off the queue:
    while( queue->isFull() ) {
      NATIVE_INT_TYPE ret = pthread_cond_wait(queueNotFull, queueLock);
      FW_ASSERT(ret == 0, errno);
    }
    
    // Push item onto queue:
    bool pushSucceeded = queue->push(buffer, size, priority);

    // The only reason push would not succeed is if the queue
    // was full. Since we waited for the queue to NOT be full
    // before sending on the queue, the push must have succeeded
    // unless there was a programming error or a bit flip.
    FW_ASSERT(pushSucceeded, pushSucceeded);

    // Push worked - wake up a thread that might be waiting on 
    // the other end of the queue:
    ret = pthread_cond_signal(queueNotEmpty);
    FW_ASSERT(ret == 0, errno); // If this fails, something horrible happened.

    ///////////////////////////////
    ret = pthread_mutex_unlock(queueLock);
    FW_ASSERT(ret == 0, errno);
    ////////////////////////////////
    ///////////////////////////////

    return Queue::QUEUE_OK;
  }


  Queue::QueueStatus Queue::send(const U8* buffer, NATIVE_INT_TYPE size, NATIVE_INT_TYPE priority, QueueBlocking block) {
    (void) block; // Always non-blocking for now
    QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
    BufferQueue* queue = &queueHandle->queue;

    if (NULL == queueHandle) {
        return QUEUE_UNINITIALIZED;
    }
    
    if (NULL == buffer) {
        return QUEUE_EMPTY_BUFFER;
    }

    if (size < 0 || (NATIVE_UINT_TYPE) size > queue->getMsgSize()) {
        return QUEUE_SIZE_MISMATCH;
    }

    if( QUEUE_NONBLOCKING == block ) {
      return sendNonBlock(queueHandle, buffer, size, priority);
    }

    return sendBlock(queueHandle, buffer, size, priority);
  }

  Queue::QueueStatus receiveNonBlock(QueueHandle* queueHandle, U8* buffer, NATIVE_INT_TYPE capacity, NATIVE_INT_TYPE &actualSize, NATIVE_INT_TYPE &priority) {

      BufferQueue* queue = &queueHandle->queue;
      pthread_mutex_t* queueLock = &queueHandle->queueLock;
      pthread_cond_t* queueNotFull = &queueHandle->queueNotFull;
      NATIVE_INT_TYPE ret;

      NATIVE_UINT_TYPE size = capacity;
      NATIVE_INT_TYPE pri = 0;
      Queue::QueueStatus status = Queue::QUEUE_OK;

      ////////////////////////////////
      // Locked Section
      ///////////////////////////////
      ret = pthread_mutex_lock(queueLock);
      FW_ASSERT(ret == 0, errno);
      ///////////////////////////////

      // Get an item off of the queue:
      bool popSucceeded = queue->pop(buffer, size, pri);

      if(popSucceeded) {
        // Pop worked - set the return size and priority:
        actualSize = (NATIVE_INT_TYPE) size;
        priority = pri;

        // Pop worked - wake up a thread that might be waiting on 
        // the send end of the queue:
        NATIVE_INT_TYPE ret = pthread_cond_signal(queueNotFull);
        FW_ASSERT(ret == 0, errno); // If this fails, something horrible happened.
      } 
      else {
        actualSize = 0;
        if( size > (NATIVE_UINT_TYPE) capacity ) {
          // The buffer capacity was too small!
          status = Queue::QUEUE_SIZE_MISMATCH;
        }
        else if( size == 0 ) {
          // The queue is empty:
          status = Queue::QUEUE_NO_MORE_MSGS;
        }
        else {
          // If this happens, a programming error or bit flip occured:
          FW_ASSERT(0);
        }
      }

      ///////////////////////////////
      ret = pthread_mutex_unlock(queueLock);
      FW_ASSERT(ret == 0, errno);
      ////////////////////////////////
      ///////////////////////////////

      return status;
  }

  Queue::QueueStatus receiveBlock(QueueHandle* queueHandle, U8* buffer, NATIVE_INT_TYPE capacity, NATIVE_INT_TYPE &actualSize, NATIVE_INT_TYPE &priority) {

      BufferQueue* queue = &queueHandle->queue;
      pthread_cond_t* queueNotEmpty = &queueHandle->queueNotEmpty;
      pthread_cond_t* queueNotFull = &queueHandle->queueNotFull;
      pthread_mutex_t* queueLock = &queueHandle->queueLock;
      NATIVE_INT_TYPE ret;

      NATIVE_UINT_TYPE size = capacity;
      NATIVE_INT_TYPE pri = 0;
      Queue::QueueStatus status = Queue::QUEUE_OK;

      ////////////////////////////////
      // Locked Section
      ///////////////////////////////
      ret = pthread_mutex_lock(queueLock);
      FW_ASSERT(ret == 0, errno);
      ///////////////////////////////

      // If the queue is empty, wait until a message is put on the queue:
      while( queue->isEmpty() ) {
        NATIVE_INT_TYPE ret = pthread_cond_wait(queueNotEmpty, queueLock);
        FW_ASSERT(ret == 0, errno);
      }
      
      // Get an item off of the queue:
      bool popSucceeded = queue->pop(buffer, size, pri);

      if(popSucceeded) {
        // Pop worked - set the return size and priority:
        actualSize = (NATIVE_INT_TYPE) size;
        priority = pri;

        // Pop worked - wake up a thread that might be waiting on 
        // the send end of the queue:
        NATIVE_INT_TYPE ret = pthread_cond_signal(queueNotFull);
        FW_ASSERT(ret == 0, errno); // If this fails, something horrible happened.
      } 
      else {
        actualSize = 0;
        if( size > (NATIVE_UINT_TYPE) capacity ) {
          // The buffer capacity was too small!
          status = Queue::QUEUE_SIZE_MISMATCH;
        }
        else {
          // If this happens, a programming error or bit flip occured:
          // The only reason a pop should fail is if the user's buffer
          // was too small.
          FW_ASSERT(0);
        }
      }

      ///////////////////////////////
      ret = pthread_mutex_unlock(queueLock);
      FW_ASSERT(ret == 0, errno);
      ////////////////////////////////
      ///////////////////////////////

      return status;
  }

  Queue::QueueStatus Queue::receive(U8* buffer, NATIVE_INT_TYPE capacity, NATIVE_INT_TYPE &actualSize, NATIVE_INT_TYPE &priority, QueueBlocking block) {

      if( (POINTER_CAST) NULL == this->m_handle ) {
        return QUEUE_UNINITIALIZED;
      }

      QueueHandle* queueHandle = (QueueHandle*) this->m_handle;

      if (NULL == queueHandle) {
        return QUEUE_UNINITIALIZED;
      }

      // Do not need to check the upper bound of capacity, We don't care
      // how big the user's buffer is.. as long as it's big enough.
      if (capacity < 0) {
          return QUEUE_SIZE_MISMATCH;
      }

      if( QUEUE_NONBLOCKING == block ) {
        return receiveNonBlock(queueHandle, buffer, capacity, actualSize, priority);
      }

      return receiveBlock(queueHandle, buffer, capacity, actualSize, priority);
  }

  NATIVE_INT_TYPE Queue::getNumMsgs(void) const {
      QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
      if (NULL == queueHandle) {
          return 0;
      }
      BufferQueue* queue = &queueHandle->queue;
      return queue->getCount();
  }

  NATIVE_INT_TYPE Queue::getMaxMsgs(void) const {
      QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
      if (NULL == queueHandle) {
          return 0;
      }
      BufferQueue* queue = &queueHandle->queue;
      return queue->getMaxCount();
  }

  NATIVE_INT_TYPE Queue::getQueueSize(void) const {
      QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
      if (NULL == queueHandle) {
          return 0;
      }
      BufferQueue* queue = &queueHandle->queue;
      return queue->getDepth();
  }

  NATIVE_INT_TYPE Queue::getMsgSize(void) const {
      QueueHandle* queueHandle = (QueueHandle*) this->m_handle;
      if (NULL == queueHandle) {
          return 0;
      }
      BufferQueue* queue = &queueHandle->queue;
      return queue->getMsgSize();
  }

}

