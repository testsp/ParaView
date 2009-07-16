/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSMPointSpriteRepresentationProxy
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkSMPointSpriteRepresentationProxy.h"

#include "vtkAbstractMapper.h"
#include "vtkObjectFactory.h"
#include "vtkGlyphSource2D.h"
#include "vtkProperty.h"

#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMViewProxy.h"
#include "vtkType.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProxyIterator.h"

#include "pqSMAdaptor.h"
#include <QList>

vtkStandardNewMacro(vtkSMPointSpriteRepresentationProxy)
vtkCxxRevisionMacro(vtkSMPointSpriteRepresentationProxy, "$Revision$")
//----------------------------------------------------------------------------
vtkSMPointSpriteRepresentationProxy::vtkSMPointSpriteRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMPointSpriteRepresentationProxy::~vtkSMPointSpriteRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMPointSpriteRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->ArrayToRadiusFilter = vtkSMSourceProxy::SafeDownCast(this->GetSubProxy(
      "ArrayToRadiusFilter"));
  this->ArrayToOpacityFilter = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy("ArrayToOpacityFilter"));

  this->LODArrayToRadiusFilter = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy("LODArrayToRadiusFilter"));
  this->LODArrayToOpacityFilter = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy("LODArrayToOpacityFilter"));

  this->OpacityTransferFunctionChooser = this->GetSubProxy(
      "OpacityTransferFunctionChooser");
  this->RadiusTransferFunctionChooser = this->GetSubProxy(
      "RadiusTransferFunctionChooser");

  this->OpacityTableTransferFunction = this->GetSubProxy(
      "OpacityTableTransferFunction");
  this->RadiusTableTransferFunction = this->GetSubProxy(
      "RadiusTableTransferFunction");

  this->OpacityGaussianTransferFunction = this->GetSubProxy(
      "OpacityGaussianTransferFunction");
  this->RadiusGaussianTransferFunction = this->GetSubProxy(
      "RadiusGaussianTransferFunction");

  this->DepthSortPainter = this->GetSubProxy("DepthSortPainter");
  this->LODDepthSortPainter = this->GetSubProxy("LODDepthSortPainter");

  this->ScalarsToColorsPainter = this->GetSubProxy("ScalarsToColorsPainter");
  this->LODScalarsToColorsPainter = this->GetSubProxy(
      "LODScalarsToColorsPainter");

  this->PointSpriteDefaultPainter = this->GetSubProxy(
      "PointSpriteDefaultPainter");
  this->LODPointSpriteDefaultPainter = this->GetSubProxy(
      "LODPointSpriteDefaultPainter");

  // set the serveur flags
  this->ArrayToRadiusFilter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->ArrayToOpacityFilter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->LODArrayToRadiusFilter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->LODArrayToOpacityFilter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->OpacityTransferFunctionChooser->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->RadiusTransferFunctionChooser->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->OpacityTableTransferFunction->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->RadiusTableTransferFunction->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->OpacityGaussianTransferFunction->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->RadiusGaussianTransferFunction->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);

  this->PointSpriteDefaultPainter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->LODPointSpriteDefaultPainter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->ScalarsToColorsPainter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->LODScalarsToColorsPainter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->DepthSortPainter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);
  this->LODDepthSortPainter->SetServers(vtkProcessModule::CLIENT
      | vtkProcessModule::RENDER_SERVER);

  return true;
}

bool vtkSMPointSpriteRepresentationProxy::EndCreateVTKObjects()
{
  // InterpolateScalarsBeforeMapping creates colors sending them to the GPU.
  // if set, a 1D texture is created, which conflicts with the points sprites
  vtkSMPropertyHelper(this->Mapper, "InterpolateScalarsBeforeMapping").Set(0);
  this->Mapper->UpdateProperty("InterpolateScalarsBeforeMapping");

  // disable the ArrayToRadius and ArrayToOpacity filters by default
  vtkSMPropertyHelper(this->ArrayToRadiusFilter, "Enabled").Set(0);
  vtkSMPropertyHelper(this->ArrayToOpacityFilter, "Enabled").Set(0);

  //disable the opacity mapping by default
  vtkSMPropertyHelper(this->ScalarsToColorsPainter, "EnableOpacity").Set(0);

  // set the name of the mapped arrays
  vtkSMPropertyHelper(this->ArrayToRadiusFilter,
      "ConcatenateOutputNameWithInput").Set(0);
  vtkSMPropertyHelper(this->ArrayToOpacityFilter,
      "ConcatenateOutputNameWithInput").Set(0);
  vtkSMPropertyHelper(this->ArrayToRadiusFilter, "OutputArrayName").Set(
      "ArrayMappedToRadius");
  vtkSMPropertyHelper(this->ArrayToOpacityFilter, "OutputArrayName").Set(
      "ArrayMappedToOpacity");

  // set the name of the radius array to the property so that it can map it to the vertex attribute
  vtkSMPropertyHelper(this->Property, "RadiusArrayName").Set(
      "ArrayMappedToRadius");

  // set the name of the opacity array to the ScalarsToColorsPainter so that it can blend it with the color
  vtkSMPropertyHelper(this->ScalarsToColorsPainter, "OpacityArrayName").Set(
      "ArrayMappedToOpacity");

  // set the type of the mapped arrays
  vtkSMPropertyHelper(this->ArrayToRadiusFilter, "ForceSameTypeAsInputArray").Set(
      0);
  vtkSMPropertyHelper(this->ArrayToOpacityFilter, "ForceSameTypeAsInputArray").Set(
      0);
  vtkSMPropertyHelper(this->ArrayToRadiusFilter, "OutputArrayType").Set(
      VTK_FLOAT);
  vtkSMPropertyHelper(this->ArrayToOpacityFilter, "OutputArrayType").Set(
      VTK_FLOAT);

  // set the opacity transfer functions
  vtkSMPropertyHelper(this->ArrayToOpacityFilter, "TransferFunction").Set(
      this->OpacityTransferFunctionChooser);
  vtkSMPropertyHelper(this->OpacityTransferFunctionChooser,
      "GaussianTransferFunction").Set(this->OpacityGaussianTransferFunction);
  vtkSMPropertyHelper(this->OpacityTransferFunctionChooser,
      "LookupTableTransferFunction").Set(this->OpacityTableTransferFunction);
  // idem for the radius
  vtkSMPropertyHelper(this->ArrayToRadiusFilter, "TransferFunction").Set(
      this->RadiusTransferFunctionChooser);
  vtkSMPropertyHelper(this->RadiusTransferFunctionChooser,
      "GaussianTransferFunction").Set(this->RadiusGaussianTransferFunction);
  vtkSMPropertyHelper(this->RadiusTransferFunctionChooser,
      "LookupTableTransferFunction").Set(this->RadiusTableTransferFunction);

  // set the painter chain
  vtkSMPropertyHelper(this->PointSpriteDefaultPainter, "ScalarsToColorsPainter").Set(
      this->ScalarsToColorsPainter);
  vtkSMPropertyHelper(this->PointSpriteDefaultPainter, "DepthSortPainter").Set(
      this->DepthSortPainter);
  // LOD painter chain
  vtkSMPropertyHelper(this->LODPointSpriteDefaultPainter,
      "ScalarsToColorsPainter").Set(this->LODScalarsToColorsPainter);
  vtkSMPropertyHelper(this->LODPointSpriteDefaultPainter, "DepthSortPainter").Set(
      this->LODDepthSortPainter);

  // insert the point sprite default painter instead of the default painter
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << this->Mapper->GetID()
      << "GetPainter" << vtkClientServerStream::End
      << vtkClientServerStream::Invoke << vtkClientServerStream::LastResult
      << "GetDelegatePainter" << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->PointSpriteDefaultPainter->GetID() << "SetDelegatePainter"
      << vtkClientServerStream::LastResult << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke << this->Mapper->GetID()
      << "SetPainter" << this->PointSpriteDefaultPainter->GetID()
      << vtkClientServerStream::End;

  // idem for the LOD painter chain
  stream << vtkClientServerStream::Invoke << this->LODMapper->GetID()
      << "GetPainter" << vtkClientServerStream::End
      << vtkClientServerStream::Invoke << vtkClientServerStream::LastResult
      << "GetDelegatePainter" << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODPointSpriteDefaultPainter->GetID() << "SetDelegatePainter"
      << vtkClientServerStream::LastResult << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke << this->LODMapper->GetID()
      << "SetPainter" << this->LODPointSpriteDefaultPainter->GetID()
      << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->GetConnectionID(), vtkProcessModule::RENDER_SERVER
      | vtkProcessModule::CLIENT, stream);

  // the surface representation proxy will connect the input to the geometry filter.
  this->Superclass::EndCreateVTKObjects();

  this->UpdateVTKObjects();

  return 1;
}

//----------------------------------------------------------------------------
bool vtkSMPointSpriteRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  // Since we use a geometry filter, the data type fed into the strategy is
  // always polydata.
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(view->NewStrategy(VTK_POLY_DATA));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
        << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }

  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  strategy->SetEnableLOD(true);

  // we add the two ArrayToRadiusFilter and ArrayToOpacityFilter after the strategy
  this->Connect(this->GeometryFilter, strategy);
  this->Connect(strategy->GetOutput(), this->ArrayToRadiusFilter);
  this->Connect(this->ArrayToRadiusFilter, this->ArrayToOpacityFilter);
  this->Connect(this->ArrayToOpacityFilter, this->Mapper);
  // and the same for the LOD
  this->Connect(strategy->GetLODOutput(), this->LODArrayToRadiusFilter);
  this->Connect(this->LODArrayToRadiusFilter, this->LODArrayToOpacityFilter);
  this->Connect(this->LODArrayToOpacityFilter, this->LODMapper);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();

  this->AddStrategy(strategy);

  // we bypass the vtkSMSurfaceRepresentation::InitializeStrategy method because we want to add
  // the ArrayToRadius and ArrayToOpacity filters before the strategy
  bool res = this->Superclass::Superclass::InitializeStrategy(view);

  if (res)
    this->InitializeDefaultValues();

  return res;
}

void vtkSMPointSpriteRepresentationProxy::InitializeDefaultValues()
{
  // Initialize the default radius if needed.
  bool radiusInitialized = pqSMAdaptor::getElementProperty(this->GetProperty(
      "RadiusInitialized")).toBool();

  if (!radiusInitialized)
    {
    double radius = this->ComputeInitialRadius(
        this->GetRepresentedDataInformation());
    pqSMAdaptor::setElementProperty(this->GetProperty("ConstantRadius"), radius);
    QList<QVariant> radiusRange;
    radiusRange.append(QVariant(0.0));
    radiusRange.append(QVariant(radius));
    pqSMAdaptor::setMultipleElementProperty(this->GetProperty("RadiusRange"),
        radiusRange);
    pqSMAdaptor::setElementProperty(this->GetProperty("RadiusInitialized"), 1);

    }

  // Initialize the Transfer functions if needed
  QList<QVariant> opacityTableValues = pqSMAdaptor::getMultipleElementProperty(
      this->GetProperty("OpacityTableValues"));
  if (opacityTableValues.size() == 0)
    {
    InitializeTableValues(this->GetProperty("OpacityTableValues"));
    }

  QList<QVariant> radiusTableValues = pqSMAdaptor::getMultipleElementProperty(
      this->GetProperty("RadiusTableValues"));
  if (opacityTableValues.size() == 0)
    {
    InitializeTableValues(this->GetProperty("RadiusTableValues"));
    }

  InitializeSpriteTextures();
}

double vtkSMPointSpriteRepresentationProxy::ComputeInitialRadius(vtkPVDataInformation* info)
{
  vtkIdType npts = info->GetNumberOfPoints();
  if (npts == 0)
    npts = 1;
  double bounds[6];
  info->GetBounds(bounds);

  double diag = sqrt(((bounds[1] - bounds[0]) * (bounds[1] - bounds[0])
      + (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) + (bounds[5]
      - bounds[4]) * (bounds[5] - bounds[4])) / 3.0);

  double nn = pow(static_cast<double>(npts), 1.0 / 3.0) - 1.0;
  if (nn < 1.0)
    nn = 1.0;

  return diag / nn / 2.0;
}

void vtkSMPointSpriteRepresentationProxy::InitializeTableValues(vtkSMProperty* prop)
{
  QList<QVariant> values;
  for (int i = 0; i < 256; i++)
    {
    values.append(QVariant(((double) i) / 256.0));
    }
  pqSMAdaptor::setMultipleElementProperty(prop, values);
}

void vtkSMPointSpriteRepresentationProxy::InitializeSpriteTextures()
{
  vtkSMProxyIterator* proxyIter;
  QString texName;
  bool created;
  QMap<QString, int> countMap;
  QList<QVariant> extent;
  vtkSMProxy* texture;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  texName = "Sphere";
  created = false;
  proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();
  for (proxyIter->Begin("textures"); !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    QString name = proxyIter->GetKey();
    if (name == texName)
      {
      created = true;
      break;
      }
    }
  proxyIter->Delete();

  if (!created)
    {
    // create the texture proxy
    texture = pxm->NewProxy("textures", "SpriteTexture");
    texture->SetConnectionID(this->GetConnectionID());
    texture->SetServers(vtkProcessModule::CLIENT
        | vtkProcessModule::RENDER_SERVER);
    pxm->RegisterProxy("textures", texName.toAscii().data(), texture);
    texture->Delete();

    // set the texture parameters
    extent.clear();
    extent.append(0.0);
    extent.append(65.0);
    extent.append(0.0);
    extent.append(65.0);
    extent.append(0.0);
    extent.append(0.0);
    pqSMAdaptor::setMultipleElementProperty(texture->GetProperty("WholeExtent"), extent);
    pqSMAdaptor::setElementProperty(texture->GetProperty("Maximum"), 255);
    pqSMAdaptor::setElementProperty(texture->GetProperty("StandardDeviation"), 0.3);
    pqSMAdaptor::setElementProperty(texture->GetProperty("AlphaMethod"), 2);
    pqSMAdaptor::setElementProperty(texture->GetProperty("AlphaThreshold"), 63);
    texture->UpdateVTKObjects();

    vtkSMProperty* textureProperty = this->GetProperty("Texture");

    if(pqSMAdaptor::getProxyProperty(textureProperty) == NULL)
      {
      // set this texture as default texture
      pqSMAdaptor::setProxyProperty(textureProperty, texture);
      this->UpdateVTKObjects();
      }
    }

  texName = "Blur";
  created = false;
  proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetModeToOneGroup();
  for (proxyIter->Begin("textures"); !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    QString name = proxyIter->GetKey();
    if (name == texName)
      {
      created = true;
      break;
      }
    }

  if (!created)
    {
    // create the texture proxy
    // create the texture proxy
    texture = pxm->NewProxy("textures", "SpriteTexture");
    texture->SetConnectionID(this->GetConnectionID());
    texture->SetServers(vtkProcessModule::CLIENT
        | vtkProcessModule::RENDER_SERVER);
    pxm->RegisterProxy("textures", texName.toAscii().data(), texture);

    extent.clear();
    extent.append(0.0);
    extent.append(65.0);
    extent.append(0.0);
    extent.append(65.0);
    extent.append(0.0);
    extent.append(0.0);
    pqSMAdaptor::setMultipleElementProperty(texture->GetProperty("WholeExtent"), extent);
    pqSMAdaptor::setElementProperty(texture->GetProperty("Maximum"), 255);
    pqSMAdaptor::setElementProperty(texture->GetProperty("StandardDeviation"), 0.2);
    pqSMAdaptor::setElementProperty(texture->GetProperty("AlphaMethod"), 1);

    texture->UpdateVTKObjects();

    texture->Delete();
    }
  proxyIter->Delete();

}

