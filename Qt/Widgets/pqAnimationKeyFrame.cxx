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

=========================================================================*/

#include "pqAnimationKeyFrame.h"

#include <QPainter>
#include <QFontMetrics>
#include <QWidget>
#include <QGraphicsView>
#include "pqAnimationTrack.h"
#include "pqAnimationModel.h"

pqAnimationKeyFrame::pqAnimationKeyFrame(pqAnimationTrack* p, QGraphicsScene* s)
  : QObject(p), QGraphicsItem(p,s),
  StartTime(0), EndTime(1),
  Interpolation(Linear), Rect(0,0,1,1)
{
}

pqAnimationKeyFrame::~pqAnimationKeyFrame()
{
}

QVariant pqAnimationKeyFrame::startValue() const
{
  return this->StartValue;
}
QVariant pqAnimationKeyFrame::endValue() const
{
  return this->EndValue;
}

pqAnimationKeyFrame::InterpolationType 
pqAnimationKeyFrame::interpolation() const
{
  return this->Interpolation;
}

double pqAnimationKeyFrame::startTime() const
{
  return this->StartTime;
}
double pqAnimationKeyFrame::endTime() const
{
  return this->EndTime;
}
void pqAnimationKeyFrame::setStartTime(double t)
{
  this->StartTime = t;
  this->adjustRect();
}
void pqAnimationKeyFrame::setEndTime(double t)
{
  this->EndTime = t;
  this->adjustRect();
}

void pqAnimationKeyFrame::setStartValue(const QVariant& v)
{
  this->StartValue = v;
  this->update();
}

void pqAnimationKeyFrame::setEndValue(const QVariant& v)
{
  this->EndValue = v;
  this->update();
}

void pqAnimationKeyFrame::setInterpolation(
  pqAnimationKeyFrame::InterpolationType i)
{
  this->Interpolation = i;
  this->update();
}
  
QRectF pqAnimationKeyFrame::boundingRect() const
{ 
  return this->Rect;
}
  
void pqAnimationKeyFrame::setBoundingRect(const QRectF& r)
{
  this->removeFromIndex();
  this->Rect = r;
  this->addToIndex();
  this->update();
}

void pqAnimationKeyFrame::adjustRect()
{
  pqAnimationTrack* track = qobject_cast<pqAnimationTrack*>(this->parent());
  pqAnimationModel* model = qobject_cast<pqAnimationModel*>(track->parent());
  QRectF trackRect = track->boundingRect();

  double w = trackRect.width();
  double totalTime = model->endTime() - model->startTime();

  double left = trackRect.left() + w * (this->startTime() - model->startTime()) / totalTime;
  double width = trackRect.width() * (this->endTime() - this->startTime()) / totalTime;

  this->setBoundingRect(QRectF(left, trackRect.top(), width, trackRect.height()));
}


void pqAnimationKeyFrame::paint(QPainter* p,
                   const QStyleOptionGraphicsItem *,
                   QWidget * widget)
{
  p->save();
  p->setBrush(QBrush(QColor(255,255,255)));
  QPen pen(QColor(0,0,0));
  pen.setWidth(0);
  p->setPen(pen);
  QRectF keyFrameRect(this->boundingRect());
  p->drawRect(keyFrameRect);

  QFontMetrics metrics(widget->font());
  double halfWidth = keyFrameRect.width()/2.0 - 5;
  
  QString label = metrics.elidedText(startValue().toString(), Qt::ElideRight,
    qRound(halfWidth));
  QPointF pt(keyFrameRect.left()+3.0, 
            keyFrameRect.top() + 0.5*keyFrameRect.height() + metrics.height() / 2.0 - 1.0);
  p->drawText(pt, label);
  
  label = metrics.elidedText(endValue().toString(), Qt::ElideRight,
    qRound(halfWidth));
  pt = QPointF(keyFrameRect.right() - metrics.width(label) - 3.0, 
            keyFrameRect.top() + 0.5*keyFrameRect.height() + metrics.height() / 2.0 - 1.0);
  
  p->drawText(pt, label);

  p->restore();
}

