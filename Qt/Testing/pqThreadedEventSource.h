/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqThreadedEventSource_h
#define _pqThreadedEventSource_h

#include "QtTestingExport.h"
#include "pqEventSource.h"

class QString;

/// Abstract interface for objects that can supply high-level testing events
/// on a separte thread.  This class is derived from, and run() is
/// implemented.
class QTTESTING_EXPORT pqThreadedEventSource : public pqEventSource
{
  Q_OBJECT
public:
  pqThreadedEventSource();
  ~pqThreadedEventSource();

  /** Called by the dispatcher on the GUI thread.
    Retrieves the next available event.  Returns true if an event was
    returned, false if there are no more events. 
    In the case of a threaded event source, this function is called by the GUI
    thread and waits for the other thread to pos and event. */
  virtual bool getNextEvent(
    QString& object,
    QString& command,
    QString& arguments);
  
  /** The testing thread may post an event for the GUI to process.
      This function blocks until there are no previously queued events to play.  */
  void postNextEvent(const QString& object,
                     const QString& command,
                     const QString& argument);
  
protected:

  // start the thread
  void start();

  // called by the testing thread to signify it is done
  void done();

  // run the thread, return 
  virtual void run() = 0;

protected slots:
  void unlockTestingMutex();

private:
  class pqInternal;
  friend class pqInternal;
  pqInternal* Internal;

};

#endif // !_pqThreadedEventSource_h

