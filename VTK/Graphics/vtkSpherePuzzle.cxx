/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkSpherePuzzle.h"
#include "vtkSphereSource.h"
#include "vtkTransformFilter.h"
#include "vtkLinearExtrusionFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkSpherePuzzle* vtkSpherePuzzle::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSpherePuzzle");
  if(ret)
    {
    return (vtkSpherePuzzle*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSpherePuzzle;
}

//----------------------------------------------------------------------------
// Construct a new puzzle.
vtkSpherePuzzle::vtkSpherePuzzle()
{
  this->Transform = vtkTransform::New();
  this->Reset();
}

//----------------------------------------------------------------------------
// Construct a new puzzle.
vtkSpherePuzzle::~vtkSpherePuzzle()
{
  this->Transform->Delete();
  this->Transform = NULL;
}


//----------------------------------------------------------------------------
void vtkSpherePuzzle::Reset()
{
  int idx;

  this->Modified();
  for (idx = 0; idx < 32; ++idx)
    {
    this->State[idx] = idx;
    this->PieceMask[idx] = 0;
    }
  this->Transform->Identity();
  for (idx = 0; idx < 4; ++idx)
    {
    this->Colors[0 + idx*8*3] = 255;
    this->Colors[1 + idx*8*3] = 0;
    this->Colors[2 + idx*8*3] = 0;

    this->Colors[3 + idx*8*3] = 255;
    this->Colors[4 + idx*8*3] = 175;
    this->Colors[5 + idx*8*3] = 0;

    this->Colors[6 + idx*8*3] = 255;
    this->Colors[7 + idx*8*3] = 255;
    this->Colors[8 + idx*8*3] = 0;

    this->Colors[9 + idx*8*3] = 0;
    this->Colors[10 + idx*8*3] = 255;
    this->Colors[11 + idx*8*3] = 0;

    this->Colors[12 + idx*8*3] = 0;
    this->Colors[13 + idx*8*3] = 255;
    this->Colors[14 + idx*8*3] = 255;

    this->Colors[15 + idx*8*3] = 0;
    this->Colors[16 + idx*8*3] = 0;
    this->Colors[17 + idx*8*3] = 255;

    this->Colors[18 + idx*8*3] = 175;
    this->Colors[19 + idx*8*3] = 0;
    this->Colors[20 + idx*8*3] = 255;

    this->Colors[21 + idx*8*3] = 255;
    this->Colors[22 + idx*8*3] = 50;
    this->Colors[23 + idx*8*3] = 150;
    }
}


//----------------------------------------------------------------------------
void vtkSpherePuzzle::Execute()
{
  int i, j, k, num;
  int color;
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  vtkSphereSource *sphere = vtkSphereSource::New();
  vtkTransformFilter *tf = vtkTransformFilter::New();
  vtkUnsignedCharArray *scalars = vtkUnsignedCharArray::New();
  vtkPolyData *tmp;
  vtkPolyData *originalSphereOutput;
  int count = 0;
  unsigned char r, g, b;

  scalars->SetNumberOfComponents(3);

  sphere->SetPhiResolution(4);
  sphere->SetThetaResolution(4);

  tf->SetTransform(this->Transform);
  tf->SetInput(sphere->GetOutput());
  originalSphereOutput = sphere->GetOutput();

  for (j = 0; j < 4; ++j)
    {
    for (i = 0; i < 8; ++i)
      {
      color = this->State[count] * 3;
      sphere->SetStartTheta((360.0 * (float)(i) / 8.0));
      sphere->SetEndTheta((360.0 * (float)(i+1) / 8.0));
      sphere->SetStartPhi((180.0 * (float)(j) / 4.0));
      sphere->SetEndPhi((180.0 * (float)(j+1) / 4.0));
      tmp = vtkPolyData::New();
      if (this->PieceMask[count])
        { // Spheres original output is transforms input. Put it back.
        sphere->SetOutput(originalSphereOutput);
        tf->SetOutput(tmp);
        tmp->Update();
        tf->SetOutput(NULL);
        }
      else
        { // Piece not involved in partial move. Just use the sphere.
        sphere->SetOutput(tmp);
        tmp->Update();
        sphere->SetOutput(NULL);
        }
      // Now create the colors for the faces.
      num = tmp->GetNumberOfPoints();
      for (k = 0; k < num; ++k)
        {
        r = this->Colors[color];
        g = this->Colors[color+1];
        b = this->Colors[color+2];
        // Lighten the active pieces
        if (this->Active && this->PieceMask[count])
          {
          r = r + (unsigned char)((255 - r) * 0.4);
          g = g + (unsigned char)((255 - g) * 0.4);
          b = b + (unsigned char)((255 - b) * 0.4);
          }
        scalars->InsertNextValue(r);
        scalars->InsertNextValue(g);
        scalars->InsertNextValue(b);
        }

      // append all the pieces.
      append->AddInput(tmp);
      tmp->Delete();
      tmp = NULL;
      ++count;
      }
    }

  append->Update();

  // Move the data to the output.
  tmp = this->GetOutput();
  tmp->CopyStructure(append->GetOutput());
  tmp->GetPointData()->PassData(append->GetOutput()->GetPointData());
  tmp->GetPointData()->SetScalars(scalars);

  append->Delete();
  tf->Delete();
}



//----------------------------------------------------------------------------
void vtkSpherePuzzle::MarkHorizontal(int section)
{
  int i;

  for (i = 0; i < 32; ++i)
    {
    this->PieceMask[i] = 0;
    }
  // Find the start of the section.
  section = section * 8;
  for (i = 0; i < 8; ++i)
    {
    this->PieceMask[i+section] = 1; 
    }
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MarkVertical(int section)
{
  int i, j, offset;

  for (i = 0; i < 32; ++i)
    {
    this->PieceMask[i] = 1;
    }
  for (i = 0; i < 4; ++i)
    {
    offset = (section + i) % 8;
    for (j = 0; j < 4; ++j)
      {
      this->PieceMask[offset+(j*8)] = 0; 
      }
    }
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MoveHorizontal(int slab, int percentage, int rightFlag)
{
  int offset;
  int tmp;
  int i;

  this->Modified();

  // Clear out previous partial moves.
  this->Transform->Identity();
  this->MarkHorizontal(slab);

  // Move zero does nothing.
  if (percentage <= 0)
    {
    return;
    }
  
  // Offset is used to determine which pieces are involved.
  offset = slab * 8;

  // Move 100 percent changes state.
  if (percentage >= 100)
    { // Just do the state change.
    if (rightFlag)
      {
      tmp = this->State[offset+7];
      for (i = 7; i > 0; --i)
        {
        this->State[i+offset] = this->State[i-1+offset]; 
        }
      this->State[offset] = tmp;
      }
    else
      {
      tmp = this->State[offset];
      for (i = 0; i < 7; ++i)
        {
        this->State[i+offset] = this->State[i+1+offset]; 
        }
      this->State[offset+7] = tmp;
      }
    return;
    }

  // Partial move.
  // This does not change the state.  It is ust for animating
  // the move.
  // Setup the pieces that are involved in the move.
  if ( ! rightFlag)
    {
    percentage = -percentage;
    }
  this->Transform->RotateZ(((float)(percentage) / 100.0)
                           * (360.0 / 8.0) );
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::MoveVertical(int half, int percentage, int rightFlag) 
{
  int tmp;
  int off0, off1, off2, off3;
  double theta;

  this->Modified();

  // Clear out previous partial moves.
  this->Transform->Identity();
  this->MarkVertical(half);

  // Move zero does nothing.
  if (percentage <= 0)
    {
    return;
    }

  off0 = (4+half) % 8;
  off1 = (5+half) % 8;
  off2 = (6+half) % 8;
  off3 = (7+half) % 8;

  // Move 100 percent changes state.
  if (percentage >= 100)
    { // Just do the state change.
    tmp = this->State[off0];
    this->State[off0] = this->State[24+off3];
    this->State[24+off3] = tmp;
  
    tmp = this->State[off1];
    this->State[off1] = this->State[24+off2];
    this->State[24+off2] = tmp;

    tmp = this->State[off2];
    this->State[off2] = this->State[24+off1];
    this->State[24+off1] = tmp;

    tmp = this->State[off3];
    this->State[off3] = this->State[24+off0];
    this->State[24+off0] = tmp;


    tmp = this->State[8+off0];
    this->State[8+off0] = this->State[16+off3];
    this->State[16+off3] = tmp;
  
    tmp = this->State[8+off1];
    this->State[8+off1] = this->State[16+off2];
    this->State[16+off2] = tmp;

    tmp = this->State[8+off2];
    this->State[8+off2] = this->State[16+off1];
    this->State[16+off1] = tmp;

    tmp = this->State[8+off3];
    this->State[8+off3] = this->State[16+off0];
    this->State[16+off0] = tmp;
    return;
    }

  // Partial move.
  // This does not change the state.  It is use for animating the move.
  if (rightFlag)
    {
    percentage = -percentage;
    }
  theta = (double)(half) * vtkMath::Pi() / 4.0;
  this->Transform->RotateWXYZ(((float)(percentage)/100.0)*(360.0/2.0),
                              sin(theta), -cos(theta), 0.0);

}


//----------------------------------------------------------------------------
int vtkSpherePuzzle::SetPoint(float x, float y, float z) 
{
  double pt[3];
  double theta, phi;
  int xi, yi;
  double xp, yp;
  double xn, yn;
  
  this->Modified();

  if (x < 0.2 && x > -0.2 && y < 0.2 && y > -0.2 && z < 0.2 && z > -0.2)
    {
    this->Active = 0;
    return 0;
    }

  // normalize
  pt[0] = x;
  pt[1] = y;
  pt[2] = z;
  vtkMath::Normalize(pt);

  // Convert this into phi and theta.
  theta = 180.0 - atan2(pt[0], pt[1]) * 180 / vtkMath::Pi();
  phi = 90.0 - asin(pt[2]) * 180 / vtkMath::Pi();

  // Compute the piece the point is in.
  xi = theta * 8.0 / 360.0;
  yi = phi * 8 / 360.0;
  xn = (theta/(360.0/8.0)) - (double)(xi);
  yn = (phi/(360.0/8.0)) - (double)(yi);

  //vtkErrorMacro("point: " << x << ", " << y << ", " << z);
  //vtkErrorMacro("theta: " << theta << ",  phi: " << phi);
  //vtkErrorMacro("theta: " << xi << ", " << xn << ",  phi: " << yi << ", " << y);

  xp = 1.0 - xn;
  yp = 1.0 - yn;
  if (xn > 0.2 && xp > 0.2 && yn > 0.2 && yp > 0.2)
    { // Do nothing in the center of the face.
    this->Active = 0;
    return 0;
    }
  this->Active = 1;
  if (xn < xp && xn < yp && xn < yn)
    {
    this->VerticalFlag = 1;
    this->RightFlag = (yn < yp);
    this->Section = xi+2;
    this->MarkVertical(this->Section);
    return this->Section + this->VerticalFlag * 10 + this->RightFlag * 100;
    }
  if (xp < xn && xp < yp && xp < yn)
    {
    this->VerticalFlag = 1;
    this->RightFlag = (yp < yn);
    this->Section = xi+7;
    this->MarkVertical(this->Section);
    return this->Section + this->VerticalFlag * 10 + this->RightFlag * 100;
    } 
  // The remaining options move a horizontal slab.
  this->VerticalFlag = 0;
  this->RightFlag = (xn > xp);
  this->Section = yi;
  this->MarkHorizontal(this->Section);
  return this->Section + this->VerticalFlag * 10 + this->RightFlag * 100;
}


//----------------------------------------------------------------------------
void vtkSpherePuzzle::MovePoint(int percentage) 
{
  if ( ! this->Active)
    {
    return;
    }
  this->Modified();

  if (this->VerticalFlag)
    {
    this->MoveVertical(this->Section, percentage, this->RightFlag);
    }
  else
    {
    this->MoveHorizontal(this->Section, percentage, this->RightFlag);
    }
}

//----------------------------------------------------------------------------
void vtkSpherePuzzle::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  vtkPolyDataSource::PrintSelf(os,indent);

  os << indent << "State: " << this->State[0];
  for (idx = 1; idx < 16; ++idx)
    {
    os << ", " << this->State[idx];
    }
  os << endl;
}

