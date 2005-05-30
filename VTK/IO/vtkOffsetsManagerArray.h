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
// .NAME OffsetsManager - Helper class due to PIMPL excess
// .SECTION Description
// This class is deisgned to work within vtkXMLWriter. It store a position
// in a file (unsigned long) and associate a offset in the file. This is
// usefull when writting TimeStep XML file when you want to forward the 
// same offset from the AppendData section on every entry in let say
// <Points> definition
// Example:
// <Points>
//   <DataArray type="Float32" TimeStep="0" format="appended" offset="268" />
//   <DataArray type="Float32" TimeStep="1" format="appended" offset="268" />
//   ...
// </Points>
// Therefore data is only stored once in the XML file. At read time the 
// offset value is stored to determine whenever we need to read data 
// (ie when offset different from previously stored offset)

// .SECTION See Also
// vtkXMLWriter
// .SECTION Warning
// Do not include this file in a header file, it will break PIMPL convention

#ifndef __vtkOffsetsManagerArray_h
#define __vtkOffsetsManagerArray_h

#include "vtkSystemIncludes.h"
#include <vtkstd/vector>
#include <assert.h>

//----------------------------------------------------------------------------
class OffsetsManager
{
public:
  // Construct with default (unsigned long)-1  MTime
  OffsetsManager()
    {
    this->LastMTime = (unsigned long)-1; //almost invalid state
    }
  ~OffsetsManager()
    {
    }
  void Allocate(int numTimeStep)
    {
    assert( numTimeStep > 0);
    this->Positions.resize(numTimeStep);
    this->OffsetValues.resize(numTimeStep);
    }
  unsigned long &GetPosition(unsigned int t)
    {
    assert( t < this->Positions.size());
    return this->Positions[t];
    }
  unsigned long &GetOffsetValue(unsigned int t)
    {
    assert( t < this->OffsetValues.size());
    return this->OffsetValues[t];
    }
  unsigned long &GetLastMTime()
    {
    return this->LastMTime;
    }
private:
  unsigned long LastMTime; // Previously written dataarray mtime 
  vtkstd::vector<unsigned long> Positions; // Position in the stream to write the offset
  vtkstd::vector<unsigned long> OffsetValues;    // Value of offset
};

//----------------------------------------------------------------------------
class OffsetsManagerGroup
{
public:
  // This is kind of a hack since we need to consider both the case of Points with
  // only one array over time and PointData with possibly multiple array over time
  // therefore we need to use a OffsetsManagerGroup for representing offset from
  // Points but OffsetsManagerArray for PointData. In both case the toplevel structure
  // is a container of Pieces...
  OffsetsManager &GetPiece(unsigned int index)
    {
    assert( index < this->Internals.size());
    OffsetsManager &e = this->Internals[index];
    return e;
    }
  // GetElement should be used when manipulating a OffsetsManagerArray
  OffsetsManager &GetElement(unsigned int index)
    {
    // commenting the following out, this is an heisenbug which only appears on gcc
    // when exporting GLIBCPP_NEW=1. If you try to print the value or run through gdb
    // it desepears
    //assert( index < this->Internals.size());
    OffsetsManager &e = this->Internals[index];
    return e;
    }
  unsigned int GetNumberOfElements()
    {
    return this->Internals.size();
    }
  void Allocate(int numElements)
    {
    assert(numElements >= 0); //allow 0 for empty FieldData
    this->Internals.resize(numElements);
    }
  void Allocate(int numElements, int numTimeSteps)
    {
    assert(numElements > 0);
    assert(numTimeSteps > 0);
    this->Internals.resize(numElements);
    for(int i=0; i<numElements; i++)
      {
      this->Internals[i].Allocate(numTimeSteps);
      }
    }
private:
  vtkstd::vector<OffsetsManager> Internals;
};

//----------------------------------------------------------------------------
class OffsetsManagerArray
{
public:
  OffsetsManagerGroup &GetPiece(unsigned int index)
    {
    assert( index < this->Internals.size());
    return this->Internals[index];
    }
  void Allocate(int numPieces)
    {
    assert(numPieces > 0);
    this->Internals.resize(numPieces);
    }
  void Allocate(int numPieces, int numElements, int numTimeSteps)
    {
    assert(numPieces > 0);
    assert(numElements > 0);
    assert(numTimeSteps > 0);

    this->Internals.resize(numPieces);
    for(int i=0; i<numPieces; i++)
      {
      this->Internals[i].Allocate(numElements, numTimeSteps);
      }
    }
private:
  vtkstd::vector<OffsetsManagerGroup> Internals;
};

#endif
