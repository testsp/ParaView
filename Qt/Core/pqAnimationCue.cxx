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

========================================================================*/
#include "pqAnimationCue.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

#include <QList>
#include <QtDebug>

#include "pqSMAdaptor.h"
#include "pqServer.h"

class pqAnimationCue::pqInternals
{
public:
  vtkSmartPointer<vtkSMProxy> Manipulator;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqInternals()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqAnimationCue::pqAnimationCue(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->ManipulatorType = "KeyFrameAnimationCueManipulator";
  this->KeyFrameType = "CompositeKeyFrame";

  this->Internal = new pqAnimationCue::pqInternals();

  this->Internal->VTKConnect->Connect(
    proxy->GetProperty("Manipulator"), vtkCommand::ModifiedEvent,
    this, SLOT(onManipulatorModified()));

  this->Internal->VTKConnect->Connect(
    proxy->GetProperty("AnimatedProxy"), vtkCommand::ModifiedEvent,
    this, SIGNAL(modified()));
  this->Internal->VTKConnect->Connect(
    proxy->GetProperty("AnimatedPropertyName"), vtkCommand::ModifiedEvent,
    this, SIGNAL(modified()));
  this->Internal->VTKConnect->Connect(
    proxy->GetProperty("AnimatedElement"), vtkCommand::ModifiedEvent,
    this, SIGNAL(modified()));

  this->onManipulatorModified();
}

//-----------------------------------------------------------------------------
pqAnimationCue::~pqAnimationCue()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getManipulatorProxy() const
{
  return this->Internal->Manipulator;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getAnimatedProxy() const
{
  vtkSMProxy* proxy = pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("AnimatedProxy"));
  return proxy;
}

//-----------------------------------------------------------------------------
vtkSMProperty* pqAnimationCue::getAnimatedProperty() const
{
  vtkSMProxy* proxy = pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("AnimatedProxy"));
  if (proxy)
    {
    QString pname = pqSMAdaptor::getElementProperty(
      this->getProxy()->GetProperty("AnimatedPropertyName")).toString();
    if (pname != "")
      {
      return proxy->GetProperty(pname.toAscii().data());
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqAnimationCue::getAnimatedPropertyIndex() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("AnimatedElement")).toInt();
}

//-----------------------------------------------------------------------------
void pqAnimationCue::setDefaults()
{
  vtkSMProxy* proxy = this->getProxy();
  if (!this->Internal->Manipulator)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* manip = 
      pxm->NewProxy("animation_manipulators", 
        this->ManipulatorType.toAscii().data());
    manip->SetConnectionID(this->getServer()->GetConnectionID());
    manip->SetServers(vtkProcessModule::CLIENT);
    this->addInternalProxy("Manipulator", manip);
    manip->Delete();
    pqSMAdaptor::setProxyProperty(proxy->GetProperty("Manipulator"),
      manip);
    }

  // All cues are always normalized, this ensures that the
  // Cue times are valid even when the scene times are changed.
  pqSMAdaptor::setEnumerationProperty(proxy->GetProperty("TimeMode"),
    "Normalized");
  proxy->UpdateVTKObjects();

}

//-----------------------------------------------------------------------------
void pqAnimationCue::onManipulatorModified()
{
  vtkSMProxy *manip = pqSMAdaptor::getProxyProperty(
    this->getProxy()->GetProperty("Manipulator"));

  if (manip != this->Internal->Manipulator)
    {
    if (this->Internal->Manipulator)
      {
      this->Internal->VTKConnect->Disconnect(
        this->Internal->Manipulator, 0, this, 0);
      }

    this->Internal->Manipulator = manip;

    if (this->Internal->Manipulator)
      {
      this->Internal->VTKConnect->Connect(
        this->Internal->Manipulator, vtkCommand::ModifiedEvent,
        this, SIGNAL(keyframesModified()));
      }
    emit this->keyframesModified();
    }
}

//-----------------------------------------------------------------------------
int pqAnimationCue::getNumberOfKeyFrames() const
{
  if (this->Internal->Manipulator)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Internal->Manipulator->GetProperty("KeyFrames"));
    return (pp? pp->GetNumberOfProxies(): 0);
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::getKeyFrame(int index) const
{
  if (this->Internal->Manipulator)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Internal->Manipulator->GetProperty("KeyFrames"));
    if (pp && index >=0 && (int)(pp->GetNumberOfProxies()) > index )
      {
      return pp->GetProxy(index);
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqAnimationCue::getKeyFrames() const
{
  QList<vtkSMProxy*> list;
  if (this->Internal->Manipulator)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      this->Internal->Manipulator->GetProperty("KeyFrames"));
    for (unsigned int cc=0; pp && cc < pp->GetNumberOfProxies(); cc++)
      {
      list.push_back(pp->GetProxy(cc));
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void pqAnimationCue::deleteKeyFrame(int index)
{
  if (!this->Internal->Manipulator)
    {
    qDebug() << "Cue does not have a KeyFrame manipulator. "
      << "One cannot delete keyframes to this Cue.";
    return;
    }

  QList<vtkSMProxy*> keyframes = this->getKeyFrames();
  if (index <0 || index >= keyframes.size())
    {
    qDebug() << "Invalid index " << index;
    return;
    }

  vtkSMProxy* keyframe = keyframes[index];
  keyframes.removeAt(index);

  vtkSMProxyProperty* pp =vtkSMProxyProperty::SafeDownCast(
    this->Internal->Manipulator->GetProperty("KeyFrames"));
  pp->RemoveAllProxies();

  foreach(vtkSMProxy* curKf, keyframes)
    {
    pp->AddProxy(curKf);
    }
  this->Internal->Manipulator->UpdateVTKObjects();

  this->removeInternalProxy("KeyFrame", keyframe);
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimationCue::insertKeyFrame(int index)
{
  if (!this->Internal->Manipulator)
    {
    qDebug() << "Cue does not have a KeyFrame manipulator. "
      << "One cannot add keyframes to this Cue.";
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Get the current keyframes.
  QList<vtkSMProxy*> keyframes = this->getKeyFrames();
  
  vtkSMProxy* kf = pxm->NewProxy("animation_keyframes", 
    this->KeyFrameType.toAscii().data());
  if (!kf)
    {
    qDebug() << "Could not create new proxy " << this->KeyFrameType;
    return 0;
    }
  kf->SetConnectionID(this->getServer()->GetConnectionID());
  kf->SetServers(vtkProcessModule::CLIENT);

  keyframes.insert(index, kf);
  double keyTime;
  if (index == 0)
    {
    keyTime = 0.0;
    }
  else if (index == keyframes.size()-1)
    {
    keyTime = 1.0;
    }
  else 
    {
    double prev_time = pqSMAdaptor::getElementProperty(
      keyframes[index-1]->GetProperty("KeyTime")).toDouble();
    double next_time = pqSMAdaptor::getElementProperty(
      keyframes[index+1]->GetProperty("KeyTime")).toDouble();
    keyTime = (prev_time + next_time)/2.0;
    }

  pqSMAdaptor::setElementProperty(kf->GetProperty("KeyTime"), keyTime);
  kf->UpdateVTKObjects();
  this->addInternalProxy("KeyFrame", kf);

  vtkSMProxyProperty* pp =vtkSMProxyProperty::SafeDownCast(
    this->Internal->Manipulator->GetProperty("KeyFrames"));
  pp->RemoveAllProxies();

  foreach(vtkSMProxy* curKf, keyframes)
    {
    pp->AddProxy(curKf);
    }
  this->Internal->Manipulator->UpdateVTKObjects();

  kf->Delete();
  return kf;
}
