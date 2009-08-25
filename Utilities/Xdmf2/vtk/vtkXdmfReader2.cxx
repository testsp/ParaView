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
#include "vtkXdmfReader2.h"
#include "vtkXdmfReader2Internal.h"
#include "vtkXdmfHeavyData.h"

#include "vtkCharArray.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataObjectTypes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkXdmfReader2);
vtkCxxRevisionMacro(vtkXdmfReader2, "$Revision$");
//----------------------------------------------------------------------------
vtkXdmfReader2::vtkXdmfReader2()
{
  this->DomainName = 0;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->XdmfDocument = new vtkXdmfDocument();
  this->LastTimeIndex = 0;
  this->SILUpdateStamp = 0;
}

//----------------------------------------------------------------------------
vtkXdmfReader2::~vtkXdmfReader2()
{
  this->SetDomainName(0);
  delete this->XdmfDocument;
  this->XdmfDocument = 0;
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::CanReadFile(const char* filename)
{
  vtkXdmfDocument doc;
  return doc.Parse(filename)? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::ProcessRequest(vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkXdmfReader2::PrepareDocument()
{
  // Calling this method repeatedly is okay. It does work only when something
  // has changed.
  if (this->GetReadFromInputString())
    {
    const char* data=0;
    unsigned int data_length=0;
    if (this->InputArray)
      {
      data = this->InputArray->GetPointer(0);
      data_length = static_cast<unsigned int>(
        this->InputArray->GetNumberOfTuples()*
        this->InputArray->GetNumberOfComponents());
      }
    else if (this->InputString)
      {
      data = this->InputString;
      data_length = this->InputStringLength;
      }
    else
      {
      vtkErrorMacro("No input string specified");
      return false;
      }
    if (!this->XdmfDocument->ParseString(data, data_length))
      {
      vtkErrorMacro("Failed to parse xmf.");
      return false;
      }
    }
  else
    {
    // Parse the file...
    if (!this->FileName )
      {
      vtkErrorMacro("File name not set");
      return false;
      }

    // First make sure the file exists.  This prevents an empty file
    // from being created on older compilers.
    if (!vtksys::SystemTools::FileExists(this->FileName))
      {
      vtkErrorMacro("Error opening file " << this->FileName);
      return false;
      }

    if (!this->XdmfDocument->Parse(this->FileName))
      {
      vtkErrorMacro("Failed to parse xmf file: " << this->FileName);
      return false;
      }
    }

  if (this->DomainName)
    {
    if (!this->XdmfDocument->SetActiveDomain(this->DomainName))
      {
      vtkErrorMacro("Invalid domain: " << this->DomainName);
      return false;
      }
    }
  else
    {
    this->XdmfDocument->SetActiveDomain(static_cast<int>(0));
    }
  this->SILUpdateStamp++;

  this->LastTimeIndex = 0; // reset time index when the file changes.
  return (this->XdmfDocument->GetActiveDomain() != 0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::RequestDataObject(vtkInformationVector *outputVector)
{
  if (!this->PrepareDocument())
    {
    return 0;
    }

  int vtk_type = this->XdmfDocument->GetActiveDomain()->GetVTKDataType();

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  if (!output || output->GetDataObjectType() != vtk_type)
    {
    output = vtkDataObjectTypes::NewDataObject(vtk_type);
    output->SetPipelineInformation(outputVector->GetInformationObject(0));
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::RequestInformation(vtkInformation *, vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if (!this->PrepareDocument())
    {
    return 0;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkXdmfDomain* domain = this->XdmfDocument->GetActiveDomain();

  // * Publish the fact that this reader can satisfy any piece request.
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  this->LastTimeIndex = this->ChooseTimeStep(outInfo);

  // Set the requested time-step on the domain. Thus, now when we go to get
  // information, we can (ideally) get information about that time-step.
  // this->XdmfDocument->GetActiveDomain()->SetTimeStep(this->LastTimeIndex);

  // * If producing structured dataset put information about whole extents etc.
  if (domain->GetNumberOfGrids() == 1 &&
    domain->IsStructured(domain->GetGrid(0)))
    {
    XdmfGrid* xmfGrid = domain->GetGrid(0);
    // just in the case the top-level grid is a temporal collection, then pick
    // the sub-grid to fetch the extents etc.
    xmfGrid = domain->GetGrid(xmfGrid,
      domain->GetTimeForIndex(this->LastTimeIndex));
    int whole_extent[6];
    if (domain->GetWholeExtent(xmfGrid, whole_extent))
      {
      // re-scale the whole_extent using the stride.
      whole_extent[1] /= this->Stride[0];
      whole_extent[3] /= this->Stride[1];
      whole_extent[5] /= this->Stride[2];

      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        whole_extent, 6);
      }
    double origin[3];
    double spacing[3];
    if (domain->GetOriginAndSpacing(xmfGrid, origin, spacing))
      {
      spacing[0] *= this->Stride[0];
      spacing[1] *= this->Stride[1];
      spacing[2] *= this->Stride[2];
      outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);
      outInfo->Set(vtkDataObject::SPACING(), origin, 3);
      }
    }

  // * Publish the SIL which provides information about the grid hierarchy.
  outInfo->Set(vtkDataObject::SIL(), domain->GetSIL());

  // * Publish time information.
  vtkstd::vector<double> time_steps(domain->GetTimeSteps().begin(),
    domain->GetTimeSteps().end());

  if (time_steps.size() > 0)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
      &time_steps[0], time_steps.size());
    double timeRange[2];
    timeRange[0] = time_steps.front();
    timeRange[1] = time_steps.back();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::RequestData(vtkInformation *, vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if (!this->PrepareDocument())
    {
    return 0;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // * Collect information about what part of the data is requested.
  unsigned int updatePiece = 0;
  unsigned int updateNumPieces = 1;
  int ghost_levels = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    updatePiece = static_cast<unsigned int>(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    updateNumPieces =  static_cast<unsigned int>(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    }
  if (outInfo->Has(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    ghost_levels = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    }

  // will be set for structured datasets only.
  int update_extent[6] = {0, -1, 0, -1, 0, -1};
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      update_extent);
    }

  this->LastTimeIndex = this->ChooseTimeStep(outInfo);

  vtkXdmfHeavyData dataReader(this->XdmfDocument->GetActiveDomain());
  dataReader.Piece = updatePiece;
  dataReader.NumberOfPieces = updateNumPieces;
  dataReader.GhostLevels = ghost_levels;
  dataReader.Extents[0] = update_extent[0]*this->Stride[0];
  dataReader.Extents[1] = update_extent[1]*this->Stride[0];
  dataReader.Extents[2] = update_extent[2]*this->Stride[1];
  dataReader.Extents[3] = update_extent[3]*this->Stride[1];
  dataReader.Extents[4] = update_extent[4]*this->Stride[2];
  dataReader.Extents[5] = update_extent[5]*this->Stride[2];
  dataReader.Stride[0] = this->Stride[0];
  dataReader.Stride[1] = this->Stride[1];
  dataReader.Stride[2] = this->Stride[2];
  dataReader.Time = this->XdmfDocument->GetActiveDomain()->GetTimeForIndex(
    this->LastTimeIndex);

  vtkDataObject* data = dataReader.ReadData();
  if (!data)
    {
    vtkErrorMacro("Failed to read data.");
    return 0;
    }

  vtkDataObject* output = vtkDataObject::GetData(outInfo);
  output->ShallowCopy(data);
  data->Delete();

  if (this->LastTimeIndex <
    this->XdmfDocument->GetActiveDomain()->GetTimeSteps().size())
    {
    double time = 
      this->XdmfDocument->GetActiveDomain()->GetTimeForIndex(this->LastTimeIndex);
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), &time, 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader2::ChooseTimeStep(vtkInformation* outInfo)
{
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    // we do not support multiple timestep requests.
    double time =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())[0];

    return this->XdmfDocument->GetActiveDomain()->GetIndexForTime(time);
    }

  // if no timestep was requested, just return what we read last.
  return this->LastTimeIndex;
}

//----------------------------------------------------------------------------
void vtkXdmfReader2::SetGridStatus(const char* gridname, int status)
{
  if (status)
    {
    this->XdmfDocument->GetActiveDomain()->GetGridSelection()->EnableArray(gridname);
    }
  else
    {
    this->XdmfDocument->GetActiveDomain()->GetGridSelection()->DisableArray(gridname);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkXdmfReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
