/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPProcessor - Base for co-proccessing implementations.
// .SECTION Description
//
// There are 3 distinct phases for the operation of a co-processor.
//
// 1) Initialization -- set up for the run.
// 2) Processing -- the run.
// 3) Finalization -- clean up before exit.
//
// The processing phase occurs repeatedly and is composed of two distinct steps,
// namely 1) Configuration (see vtkCPProcessor::ProcessDataDescription) and
// 2) Processing (see vtkCPProcessor::ProcessData).
//
// Configuration step:
// In the first step the Co-Processor implemntation is called with a
// vtkDataDescription describing the data that the simulation can provide
// This gives the Co-Processor implemntation a chance to identify what
// (if any) of the available data it will process during this pass. By
// default all of the avaible data is selected, so that if the Co-Processor
// implementation does nothing it will receive all data during the Processing 
// step. The Co-Processor implementation should extract what ever meta-data
// it will need (or alternatively can save a reference to the DataDescription),
// during the Processing step.
//
// Processing step:
// In the second step the Co-Processor implementation is called with the
// actual data that it has been asked to provide, if any. If no data was
// selected during the Configuration Step than the priovided vtkDataObject
// may be NULL.
//

#ifndef vtkCPProcessor_h
#define vtkCPProcessor_h

#include "vtkObject.h"

struct vtkCPProcessorInternals;
class vtkCPDataDescription;
class vtkDataObject;
class vtkFieldData;
class vtkCPPipeline;

class VTK_EXPORT vtkCPProcessor : public vtkObject
{

public:
  static vtkCPProcessor* New();
  vtkTypeRevisionMacro(vtkCPProcessor,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add in a pipeline that is externally configured. Returns 1 if 
  // successful and 0 otherwise.
  virtual int AddPipeline(vtkCPPipeline* Pipeline);

  // Description:
  // Initialize the co-proccesor. Returns 1 if successful and 0
  // otherwise.
  virtual int Initialize();

  // Description:
  // Configuration Step:
  // The coprocessor first determines if any coprocessing needs to be done
  // at this TimeStep/Time combination returning 1 if it does and 0
  // otherwise.  If coprocessing does need to be performed this time step
  // it fills in the FieldNames array that the coprocessor requires
  // in order to fulfill all the coprocessing requests for this
  // TimeStep/Time combination.
  virtual int RequestDataDescription(
    vtkCPDataDescription* DataDescription);

  // Description:
  // Processing Step:
  // Provides the grid and the field data for the co-procesor to process.
  virtual int CoProcess(vtkCPDataDescription* DataDescription);

  // Description:
  // Called after all co-processing is complete giving the Co-Processor 
  // implementation an opportunity to clean up, before it is destroyed.
  virtual int Finalize();

protected:
  vtkCPProcessor();
  virtual ~vtkCPProcessor();

private:
  vtkCPProcessor(const vtkCPProcessor&); // Not implemented
  void operator=(const vtkCPProcessor&); // Not implemented

  vtkCPProcessorInternals* Internal;
};

#endif
