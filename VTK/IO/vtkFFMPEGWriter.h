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
// .NAME vtkFFMPEGWriter - Uses the FFMPEG library to write video files.
// .SECTION Description
// vtkFFMPEGWriter is an adapter that allows VTK to use the LGPL'd FFMPEG 
// library to write movie files. FFMPEG can create a variety of multimedia
// file formats and can use a variety of encoding algorithms (codecs). 
// This class creates .avi files containing MP43 encoded video without audio.
//
// .SECTION See Also vtkGenericMovieWriter vtkAVIWriter vtkMPEG2Writer

#ifndef __vtkFFMPEGWriter_h
#define __vtkFFMPEGWriter_h

#include "vtkGenericMovieWriter.h"

class vtkFFMPEGWriterInternal;

class VTK_IO_EXPORT vtkFFMPEGWriter : public vtkGenericMovieWriter
{
public:
  static vtkFFMPEGWriter *New();
  vtkTypeRevisionMacro(vtkFFMPEGWriter,vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // These methods start writing an Movie file, write a frame to the file
  // and then end the writing process.
  void Start();
  void Write();
  void End();

  // Description:
  // Set/Get the compression quality.
  // 0 means worst quality and smallest file size
  // 2 means best quality and largest file size
  vtkSetClampMacro(Quality, int, 0, 2);
  vtkGetMacro(Quality, int);
  
protected:
  vtkFFMPEGWriter();
  ~vtkFFMPEGWriter();

  vtkFFMPEGWriterInternal *Internals;

  int Initialized;
  int Quality;

private:
  vtkFFMPEGWriter(const vtkFFMPEGWriter&); // Not implemented
  void operator=(const vtkFFMPEGWriter&); // Not implemented
};

#endif



