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
// .NAME vtkGPUInfoListArray - Internal class vtkGPUInfoList.
// .SECTION Description
// vtkGPUInfoListArray is just a PIMPL mechanism for vtkGPUInfoList.

#ifndef __vtkGPUInfoListArray_h
#define __vtkGPUInfoListArray_h

#include "vtkGPUInfo.h"
#include <vtkstd/vector> // STL Header

class vtkGPUInfoListArray
{
public:
  vtkstd::vector<vtkGPUInfo *> v;
};

#endif
