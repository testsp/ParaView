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
// .NAME vtkSMUniformGridVolumeRepresentationProxy - representation that can be used to
// show a uniform grid volume in a render view.
// .SECTION Description
// vtkSMUniformGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the uniform grid volume in a vtkSMRenderViewProxy. 
// It supports rendering the uniform grid volume data.

#ifndef __vtkSMUniformGridVolumeRepresentationProxy_h
#define __vtkSMUniformGridVolumeRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

class VTK_EXPORT vtkSMUniformGridVolumeRepresentationProxy : 
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMUniformGridVolumeRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMUniformGridVolumeRepresentationProxy, 
    vtkSMPropRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMUniformGridVolumeRepresentationProxy();
  ~vtkSMUniformGridVolumeRepresentationProxy();

  // Description:
  // This representation needs a uniform grid volume compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

    // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Structured grid volume rendering classes
  vtkSMProxy* VolumeFixedPointRayCastMapper;

  // Common volume rendering classes
  vtkSMProxy* VolumeActor;
  vtkSMProxy* VolumeProperty;

private:
  vtkSMUniformGridVolumeRepresentationProxy(const vtkSMUniformGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSMUniformGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif

