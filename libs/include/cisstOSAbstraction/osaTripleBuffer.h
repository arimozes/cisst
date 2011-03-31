/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id$

  Author(s): Martin Kelly, Anton Deguet
  Created on: 2011-03-15

  (C) Copyright 2011 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

#include <cisstConfig.h> // to define CISST_OS and CISST_COMPILER
#include <cisstOSAbstraction/osaConfig.h> // to know which atomic operations are available

#include <cisstCommon/cmnAssert.h>

#if CISST_OSA_HAS_sync_bool_compare_and_swap
#define OSA_TRIPLE_BUFFER_HAS_ATOMIC_CAS 1
#define OSA_ATOMIC_POINTER_CAS(destination, compareTo, newValue) \
    __sync_bool_compare_and_swap(&(destination), (compareTo), (newValue))

#elif defined (CISST_COMPILER_IS_MSVC)
#include <windows.h>
#define OSA_TRIPLE_BUFFER_HAS_ATOMIC_CAS 1
#define OSA_ATOMIC_POINTER_CAS(destination, compareTo, newValue) \
    InterlockedCompareExchangePointer((PVOID *)(&(destination)), (PVOID)(newValue), ((PVOID)(compareTo)))

#else
    #define OSA_TRIPLE_BUFFER_HAS_ATOMIC_CAS 0 
#endif

#include <cisstOSAbstraction/osaMutex.h>

/*!  Triple buffer to implement a thread safe, lock free, single
  reader single writer container.  This relies on a LIFO circular
  buffer with only three slots, one to read the latest value and two
  to write (assuming the reading thread can be slow to read).

  This class assumes read and write operations are performed in two
  different threads.  The reader must trigger the following calls to
  be safe: BeginRead, GetReadPointer and finally EndRead.  Similarly,
  the writer must call BeginWrite, GetWritePointer and EndWrite.  It
  is important to not cache the results of both GetReadPointer and
  GetWritePointer.

  The triple buffer can be constructed using three existing pointers
  on valid memory slots or allocate the memory itself (see
  constructors).

  When compiled with a compiler/OS that supports atomic compare and
  swap operations (CAS) such as gcc (__sync_bool_compare_and_swap) or
  Visual Studio (InterlockedCompareAndExchange), this container is lock
  free.  Otherwise, the implementation relies on a mutex (using
  osaMutex) to make sure the BeginRead, EndRead, BeginWrite and
  EndWrite methods are thread safe.  Even in the later case, the mutex
  is used for very brief operations so there shouldn't be any long
  wait times.
 */
template <class _elementType>
class osaTripleBuffer
{
    friend class osaTripleBufferTest;

    typedef _elementType value_type;

    typedef osaTripleBuffer<value_type> ThisType;

    typedef value_type * pointer;
    typedef const value_type * const_pointer;
    typedef value_type & reference;
    typedef const value_type & const_reference;

    // did the buffer allocate memory or used existing pointers
    bool OwnMemory;
    pointer Memory;

    // circular buffer node
    struct Node {
        pointer Pointer;
        Node * Next;
    };

    // last write node (behaves like a head in our lifo circular buffer
    Node * LastWriteNode;
    Node * WriteNode;
    Node * ReadNode;

    osaMutex Mutex;

public:
    /*! Constructor that allocates memory for the triple buffer using
      the default constructor for each element. */
    inline osaTripleBuffer(void):
        OwnMemory(true)
    {
        this->Memory = new value_type[3];
        SetupNodes(this->Memory,
                   this->Memory + 1,
                   this->Memory + 2);
    }

    /*! Constructor that allocates memory for the triple buffer using
      the copy constructor for each element.  User has to provide an
      value which will be used to initialize the buffer elements. */
    inline osaTripleBuffer(const_reference initialValue):
        OwnMemory(true),
        Memory(0)
    {
        SetupNodes(new value_type(initialValue),
                   new value_type(initialValue),
                   new value_type(initialValue));
    }

    /*! Constructor that doesn't allocate any memory, user has to
      provide 3 valid pointers on 3 different pre-allocated objects.
      This can be used in combination with libraries such as VTK which
      have private constructors. */
    inline osaTripleBuffer(pointer pointer1, pointer pointer2, pointer pointer3):
        OwnMemory(false),
        Memory(0)
    {
        SetupNodes(pointer1, pointer2, pointer3);
    }

    /*! Internal method to setup all nodes of the circular buffer.  It
      requires 3 valid pointers. */
    inline void SetupNodes(pointer pointer1, pointer pointer2, pointer pointer3) {
        CMN_ASSERT(pointer1);
        CMN_ASSERT(pointer2);
        CMN_ASSERT(pointer3);
        this->LastWriteNode = new Node();
        this->LastWriteNode->Pointer = pointer1;
        this->LastWriteNode->Next = new Node;
        this->LastWriteNode->Next->Pointer = pointer2;
        this->LastWriteNode->Next->Next = new Node;
        this->LastWriteNode->Next->Next->Pointer = pointer3;
        this->LastWriteNode->Next->Next->Next = this->LastWriteNode;
    }

    /*! Destructor.  If the memory is owned, it will delete the 3
      objects allocated.  All nodes get deleted. */
    inline ~osaTripleBuffer() {
        if (this->OwnMemory) {
            if (this->Memory) {
                delete[] this->Memory;
            } else {
                delete this->LastWriteNode->Next->Next->Pointer;
                delete this->LastWriteNode->Next->Pointer;
                delete this->LastWriteNode->Pointer;
            }
        }
        delete this->LastWriteNode->Next->Next;
        delete this->LastWriteNode->Next;
        delete this->LastWriteNode;
    }

    /*! Calls BeginRead, assign the last written value using the
      operator = and then calls EndRead. */
    inline void Read(reference placeHolder) {
        this->BeginRead();
        placeHolder = *(ReadNode->Pointer);
        this->EndRead();
    }

    /*! Calls BeginWrite, assign the new value to the current write
      location using the operator = and then calls EndWrite. */
    inline void Write(const_reference newValue) {
        this->BeginWrite();
        *(WriteNode->Pointer) = newValue;
        this->EndWrite();
    }

    /*! Function to access the memory to read safely.  This method
      call must be preceeded by a call to BeginRead and followed by
      a call to EndRead.  All three calls must be performed in the
      same thread space. */ 
    inline const_pointer GetReadPointer(void) const {
        return this->ReadNode->Pointer;
    }

    /*! Function to access the memory to write safely.  This method
      call must be preceeded by a call to BeginWrite and followed by
      a call to EndWrite.  All three calls must be performed in the
      same thread space. */ 
    inline pointer GetWritePointer(void) const {
        return this->WriteNode->Pointer;
    }

    /*! Method used to find and lock the read node in the triple
      buffer.  To access the actual memory, use GetReadPointer. */
    inline void BeginRead(void) {
        Mutex.Lock(); {
            this->ReadNode = this->LastWriteNode;
        } Mutex.Unlock();
    }

    /*! Method to unlock the read node. */
    inline void EndRead(void) {
        Mutex.Lock(); {
            this->ReadNode = 0;
        } Mutex.Unlock();
    }


    /*! Method used to find and lock the write node in the triple
      buffer.  To access the actual memory, use GetReadPointer. */
    inline void BeginWrite(void) {
        this->WriteNode = this->LastWriteNode->Next;
#if !OSA_TRIPLE_BUFFER_HAS_ATOMIC_CAS
        Mutex.Lock(); {
            if (this->WriteNode == this->ReadNode) {
                this->WriteNode = this->WriteNode->Next;
            }
        } Mutex.Unlock();
#else
        OSA_ATOMIC_POINTER_CAS(this->WriteNode, this->ReadNode, this->WriteNode->Next);
#endif // OSA_TRIPLE_BUFFER_HAS_ATOMIC_CAS
    }


    /*! Method to unlock the write node. */
    inline void EndWrite(void) {
        this->LastWriteNode = this->WriteNode;
    }
};
