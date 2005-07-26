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
#include "vtkSMCaveRenderModuleProxy.h"

#include <sys/stat.h>
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkSMProxyProperty.h"
#include "vtkCaveRenderManager.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkRenderWindow.h"
#include "vtkSMProxyManager.h"
#include "vtkPVServerInformation.h"

vtkStandardNewMacro(vtkSMCaveRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMCaveRenderModuleProxy, "$Revision$");
//-----------------------------------------------------------------------------
vtkSMCaveRenderModuleProxy::vtkSMCaveRenderModuleProxy()
{
  this->SetDisplayXMLName("MultiDisplay");
}

//-----------------------------------------------------------------------------
vtkSMCaveRenderModuleProxy::~vtkSMCaveRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::CreateCompositeManager()
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

  vtkSMProxy* cm = pxm->NewProxy("composite_managers", "CaveRenderManager" );
  if (!cm)
    {
    vtkErrorMacro("Failed to create CompositeManagerProxy.");
    return;
    }
  cm->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->AddSubProxy("CompositeManager", cm);
  cm->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::InitializeCompositingPipeline()
{
  if (!this->CompositeManagerProxy)
    {
    vtkErrorMacro("CompositeManagerProxy not set.");
    return;
    }
  
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  unsigned int i;
  vtkClientServerStream stream;

  // We had trouble with SGI/aliasing with compositing.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions() > 1))
    {
    for (i=0; i < this->RenderWindowProxy->GetNumberOfIDs(); i++)
      {
      stream << vtkClientServerStream::Invoke
        << this->RenderWindowProxy->GetID(i) 
        << "SetMultiSamples" << 0
        << vtkClientServerStream::End;
      }
    pm->SendStream(this->RenderWindowProxy->GetServers(), stream);
    }

  if (pm->GetOptions()->GetClientMode())
    {
    // using vtkClientCompositeManager. 
    for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
      {
      // Clean up this mess !!!!!!!!!!!!!
      // Even a cast to vtkPVClientServerModule would be better than this.
      // How can we syncronize the process modules and render modules?
      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetClientMode" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i) 
        << "SetClientFlag"
        << vtkClientServerStream::LastResult << vtkClientServerStream::End;
      
      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetRenderServerSocketController" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i)
        << "SetSocketController" << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;
      }
    pm->SendStream(this->CompositeManagerProxy->GetServers(), stream);
    }

  this->Superclass::InitializeCompositingPipeline();
  //TODO: I am almost certain that I have messed up the "Timing is 
  //critical" stuff in vtkPVCaveRenderModule. Must verify that.

  if (pm->GetOptions()->GetClientMode())
    {
    int numDisplays;
    numDisplays = pm->GetNumberOfPartitions();
    // Setup the tiles.
    // We need a better way to retreive the number of processes
    vtkMPIMToNSocketConnection* m2n = NULL;
    if (pm->GetMPIMToNSocketConnectionID().ID)
      {
      m2n = vtkMPIMToNSocketConnection::SafeDownCast(
        pm->GetObjectFromID(pm->GetMPIMToNSocketConnectionID()));
      }   
    if (m2n)
      {
      numDisplays = m2n->GetNumberOfConnections();
      }    
    this->ConfigureFromServerInformation();
    }
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::ConfigureFromServerInformation()
{
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());
  vtkPVServerInformation* serverInfo = pm->GetServerInformation();
  vtkCaveRenderManager* crm = 
    vtkCaveRenderManager::SafeDownCast(pm->GetObjectFromID(
        this->CompositeManagerProxy->GetID(0)));

  unsigned int idx;
  unsigned int numMachines = serverInfo->GetNumberOfMachines();
  for (idx = 0; idx < numMachines; idx++)
    {
    pm->SetProcessEnvironmentVariable(idx, serverInfo->GetEnvironment(idx));
    crm->DefineDisplay(idx, serverInfo->GetLowerLeft(idx),
                       serverInfo->GetLowerRight(idx),
                       serverInfo->GetUpperLeft(idx));
    }
}

//-----------------------------------------------------------------------------
int vtkSMCaveRenderModuleProxy::GetLocalRenderDecision(unsigned long,
   int vtkNotUsed(stillRender))
{
  return 1; // for Cave, always 1.
}

//-----------------------------------------------------------------------------
void vtkSMCaveRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
