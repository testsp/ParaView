// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqParts.h"
#include "pqServer.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

vtkSMDisplayProxy* pqAddPart(pqServer* Server, vtkSMSourceProxy* Part)
{
  // without this, you will get runtime errors from the part display
  // (connected below). this should be fixed
  Part->CreateParts();

  // Create part display.
  vtkSMRenderModuleProxy *rm = Server->GetRenderModule();
  vtkSMDisplayProxy *partdisplay = rm->CreateDisplayProxy();

  // Set the part as input to the part display.
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(partdisplay->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(Part);

  vtkSMDataObjectDisplayProxy *dod
    = vtkSMDataObjectDisplayProxy::SafeDownCast(partdisplay);
  if (dod)
    {
    dod->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
    }

  partdisplay->UpdateVTKObjects();

  // Add the part display to the render module.
  pp = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->AddProxy(partdisplay);
  rm->UpdateVTKObjects();

  // Allow the render module proxy to maintain the part display.
//  partdisplay->Delete();

  return partdisplay;
}

void pqRemovePart(pqServer* Server, vtkSMDisplayProxy* Part)
{
  vtkSMRenderModuleProxy *rm = Server->GetRenderModule();
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->RemoveProxy(Part);
  rm->UpdateVTKObjects();
}
