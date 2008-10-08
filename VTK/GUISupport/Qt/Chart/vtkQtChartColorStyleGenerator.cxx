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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartColorStyleGenerator.cxx
/// \date September 22, 2008

#include "vtkQtChartColorStyleGenerator.h"

#include <QVector>


class vtkQtChartColorStyleGeneratorInternal
{
public:
  vtkQtChartColorStyleGeneratorInternal();
  ~vtkQtChartColorStyleGeneratorInternal() {}

  QVector<Qt::PenStyle> Styles;
};


//-----------------------------------------------------------------------------
vtkQtChartColorStyleGeneratorInternal::vtkQtChartColorStyleGeneratorInternal()
  : Styles()
{
}


//-----------------------------------------------------------------------------
vtkQtChartColorStyleGenerator::vtkQtChartColorStyleGenerator(
    QObject *parentObject, vtkQtChartColors::ColorScheme scheme)
  : vtkQtChartStyleGenerator(parentObject)
{
  this->Internal = new vtkQtChartColorStyleGeneratorInternal();
  this->Colors = new vtkQtChartColors(scheme);

  // Add the default list of pen styles.
  this->Internal->Styles.append(Qt::SolidLine);
  this->Internal->Styles.append(Qt::DashLine);
  this->Internal->Styles.append(Qt::DotLine);
  this->Internal->Styles.append(Qt::DashDotLine);
  this->Internal->Styles.append(Qt::DashDotDotLine);
}

vtkQtChartColorStyleGenerator::~vtkQtChartColorStyleGenerator()
{
  delete this->Internal;
  delete this->Colors;
}

QBrush vtkQtChartColorStyleGenerator::getSeriesBrush(int index) const
{
  if(index >= 0 && this->Colors->getNumberOfColors()>0)
    {
    index = index % this->Colors->getNumberOfColors();
    return QBrush(this->Colors->getColor(index));
    }

  return QBrush();
}

QPen vtkQtChartColorStyleGenerator::getSeriesPen(int index) const
{
  QPen pen;
  if(index >= 0)
    {
    int numColors = this->Colors->getNumberOfColors();
    if(numColors > 0)
      {
      pen.setColor(this->Colors->getColor(index % numColors));
      index /= numColors;
      }

    int numStyles = this->Internal->Styles.size();
    if(numStyles > 0)
      {
      pen.setStyle(this->Internal->Styles[index % numStyles]);
      }
    }

  return pen;
}

int vtkQtChartColorStyleGenerator::getNumberOfStyles() const
{
  return this->Internal->Styles.size();
}

Qt::PenStyle vtkQtChartColorStyleGenerator::getPenStyle(int index) const
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    return this->Internal->Styles[index];
    }

  return Qt::SolidLine;
}

void vtkQtChartColorStyleGenerator::setPenStyle(int index, Qt::PenStyle style)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles[index] = style;
    }
}

void vtkQtChartColorStyleGenerator::clearPenStyles()
{
  this->Internal->Styles.clear();
}

void vtkQtChartColorStyleGenerator::addPenStyle(Qt::PenStyle style)
{
  this->Internal->Styles.append(style);
}

void vtkQtChartColorStyleGenerator::insertPenStyle(int index,
    Qt::PenStyle style)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles.insert(index, style);
    }
}

void vtkQtChartColorStyleGenerator::removePenStyle(int index)
{
  if(index >= 0 && index < this->Internal->Styles.size())
    {
    this->Internal->Styles.remove(index);
    }
}

