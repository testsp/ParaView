/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkPointLocator - quickly locate points in 3-space
// .SECTION Description
// vtkPointLocator is a spatial search object to quickly locate points in 3D.
// vtkPointLocator works by dividing a specified region of space into a regular
// array of "rectangular" buckets, and then keeping a list of points that 
// lie in each bucket. Typical operation involves giving a position in 3D 
// and finding the closest point.
//
// vtkPointLocator has two distinct methods of interaction. In the first
// method, you supply it with a dataset, and it operates on the points in 
// the dataset. In the second method, you supply it with an array of points,
// and the object operates on the array.

// .SECTION Caveats
// Many other types of spatial locators have been developed such as 
// octrees and kd-trees. These are often more efficient for the 
// operations described here.

// .SECTION See Also
// vtkCellPicker vtkPointPicker

#ifndef __vtkPointLocator_h
#define __vtkPointLocator_h

#include "vtkLocator.h"
#include "vtkPoints.h"
#include "vtkIdList.h"

class vtkNeighborPoints;

class VTK_EXPORT vtkPointLocator : public vtkLocator
{
public:
  // Description:
  // Construct with automatic computation of divisions, averaging
  // 25 points per bucket.
  static vtkPointLocator *New();

  const char *GetClassName() {return "vtkPointLocator";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of divisions in x-y-z directions.
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);

  // Description:
  // Specify the average number of points in each bucket.
  vtkSetClampMacro(NumberOfPointsPerBucket,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfPointsPerBucket,int);

  // Description:
  // Given a position x, return the id of the point closest to it. Alternative
  // method requires separate x-y-z values.
  virtual int FindClosestPoint(float x[3]);
  int FindClosestPoint(float x, float y, float z);

  // Description:
  // Initialize the point insertion process. The newPts is an object
  // representing point coordinates into which incremental insertion methods
  // place their data. Bounds are the box that the points lie in.
  virtual int InitPointInsertion(vtkPoints *newPts, float bounds[6]);

  // Description:
  // Initialize the point insertion process. The newPts is an object
  // representing point coordinates into which incremental insertion methods
  // place their data. Bounds are the box that the points lie in.
  virtual int InitPointInsertion(vtkPoints *newPts, float bounds[6], 
				 int estSize);

  // Description:
  // Incrementally insert a point into search structure with a particular
  // index value. You should use the method IsInsertedPoint() to see whether 
  // this point has already been inserted (that is, if you desire to prevent
  // dulicate points). Before using this method you must make sure that 
  // newPts have been supplied, the bounds has been set properly, and that 
  // divs are properly set. (See InitPointInsertion().)
  virtual void InsertPoint(int ptId, float x[3]);

  // Description:
  // Incrementally insert a point into search structure. The method returns
  // the insertion location (i.e., point id). You should use the method 
  // IsInsertedPoint() to see whether this point has already been
  // inserted (that is, if you desire to prevent dulicate points).
  // Before using this method you must make sure that newPts have been
  // supplied, the bounds has been set properly, and that divs are 
  // properly set. (See InitPointInsertion().)
  virtual int InsertNextPoint(float x[3]);

  // Description:
  // Determine whether point given by x[3] has been inserted into points list.
  // Return id of previously inserted point if this is true, otherwise return
  // -1.
  int IsInsertedPoint(float x, float  y, float z)
    {
    float xyz[3];
    xyz[0] = x; xyz[1] = y; xyz[2] = z;
    return this->IsInsertedPoint (xyz);
    };
  virtual int IsInsertedPoint(float x[3]);

  // Description:
  // Determine whether point given by x[3] has been inserted into points list.
  // Return 0 if point was already in the list, otherwise return 1. If the
  // point was not in the list, it will be ADDED.  In either case, the id of
  // the point (newly inserted or not) is returned in the ptId argument.
  // Note this combines the functionality of IsInsertedPoint() followed
  // by a call to InsertNextPoint().
  virtual int InsertUniquePoint(float x[3], int &ptId);

  // Description:
  // Given a position x, return the id of the point closest to it. This method
  // is used when performing incremental point insertion.
  virtual int FindClosestInsertedPoint(float x[3]);

  // Description:
  // Find the closest N points to a position. This returns the closest
  // N points to a position. A faster method could be created that returned
  // N close points to a position, but neccesarily the exact N closest.
  // The returned points are sorted from closest to farthest.
  virtual void FindClosestNPoints(int N, float x[3], vtkIdList *result);
  virtual void FindClosestNPoints(int N, float x, float y, float z,
				  vtkIdList *result);

  // Description:
  // Find the closest points to a position such that each octent of
  // space around the position contains at least N points. Loosely 
  // limit the search to a maximum number of points evalualted, M. 
  virtual void FindDistributedPoints(int N, float x[3], 
				     vtkIdList *result, int M);
  virtual void FindDistributedPoints(int N, float x, float y, 
				     float z, vtkIdList *result, int M);

  // Description:
  // Find all points within a specified radius R of position x.
  // The result is not sorted in any specific manner.
  virtual void FindPointsWithinRadius(float R, float x[3], vtkIdList *result);
  virtual void FindPointsWithinRadius(float R, float x, float y, float z, 
				      vtkIdList *result);
  
  // Description:
  // See vtkLocator interface documentation.
  void Initialize();
  void FreeSearchStructure();
  void BuildLocator();
  void GenerateRepresentation(int level, vtkPolyData *pd);

protected:
  vtkPointLocator();
  ~vtkPointLocator();
  vtkPointLocator(const vtkPointLocator&) {};
  void operator=(const vtkPointLocator&) {};

  // place points in appropriate buckets
  void GetBucketNeighbors(int ijk[3], int ndivs[3], int level);
  void GetOverlappingBuckets(float x[3], int ijk[3], float dist, int level);
  void GenerateFace(int face, int i, int j, int k, 
                    vtkPoints *pts, vtkCellArray *polys);

  vtkPoints *Points; // Used for merging points
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  int NumberOfPointsPerBucket; //Used with previous boolean to control subdivide
  float Bounds[6]; // bounds of points
  vtkIdList **HashTable; // lists of point ids in buckets
  int NumberOfBuckets; // total size of hash table
  float H[3]; // width of each bucket in x-y-z directions
  vtkNeighborPoints *Buckets;

  float InsertionTol2;
  int InsertionPointId;
};

#endif


