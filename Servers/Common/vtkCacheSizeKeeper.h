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
// .NAME vtkCacheSizeKeeper - keeps track of amount of memory consumed
// by caches in vtkPVUpateSupressor objects. 
// .SECTION Description:
// vtkCacheSizeKeeper keeps track of the amount of memory cached
// by several vtkPVUpdateSuppressor objects.

#ifndef __vtkCacheSizeKeeper_h
#define __vtkCacheSizeKeeper_h

#include "vtkObject.h"

class VTK_EXPORT vtkCacheSizeKeeper : public vtkObject
{
public:
  static vtkCacheSizeKeeper* New();
  vtkTypeRevisionMacro(vtkCacheSizeKeeper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Report increase in cache size (in kbytes).
  void AddCacheSize(unsigned long kbytes)
    {
    if (this->CacheFull)
      {
      vtkErrorMacro("Cache is full. Cannot add more cached data.");
      }
    else
      {
      this->CacheSize += kbytes;
      }
    }

  // Description:
  // Report decrease in cache size (in bytes).
  void FreeCacheSize(unsigned long kbytes)
    {
    this->CacheSize = (this->CacheSize > kbytes)?
      (this->CacheSize-kbytes) : 0;
    }

  // Description:
  // Get the size of cache reported to this keeper.
  vtkGetMacro(CacheSize, unsigned long);


  // Description:
  // Get/Set if the cache is full. 
  vtkGetMacro(CacheFull, int);
  vtkSetMacro(CacheFull, int);

protected:
  vtkCacheSizeKeeper();
  ~vtkCacheSizeKeeper();

  unsigned long CacheSize;
  int CacheFull;
private:
  vtkCacheSizeKeeper(const vtkCacheSizeKeeper&); // Not implemented.
  void operator=(const vtkCacheSizeKeeper&); // Not implemented.
};

#endif

