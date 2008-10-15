/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ClientTableView.h"
#include "ui_ClientTableView.h"

#include <vtkAbstractArray.h>
#include <vtkConvertSelection.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataSetAttributes.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkPVDataInformation.h>
#include <vtkQtTableModelAdapter.h>
#include <vtkSelection.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>

#include <QSortFilterProxyModel>

////////////////////////////////////////////////////////////////////////////////////
// ClientTableView::implementation

class ClientTableView::implementation
{
public:
  implementation() :
    Table(vtkTable::New()),
    UpdatingSelection(false)
  {
    this->TableAdapter.setTable(this->Table);
    this->TableSort.setSourceModel(&this->TableAdapter);
  }

  ~implementation()
  {
    this->Table->Delete();
  }

  bool UpdatingSelection;
  vtkTable* const Table;
  vtkQtTableModelAdapter TableAdapter;
  QSortFilterProxyModel TableSort;

  Ui::ClientTableView Widgets;
  QWidget Widget;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientTableView

ClientTableView::ClientTableView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  pqSingleInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation())
{
  this->Implementation->Widgets.setupUi(&this->Implementation->Widget);
  this->Implementation->Widgets.tableView->setModel(&this->Implementation->TableSort);

  this->connect(this->Implementation->Widgets.tableView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
    this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

}

ClientTableView::~ClientTableView()
{
  delete this->Implementation;
}

QWidget* ClientTableView::getWidget()
{
  return &this->Implementation->Widget;
}

bool ClientTableView::canDisplay(pqOutputPort* output_port) const
{
  if(!output_port)
    return false;

  pqPipelineSource* const source = output_port->getSource();
  if(!source)
    return false;

  if(this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID())
    return false;

  vtkSMSourceProxy* source_proxy =
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!source_proxy ||
     source_proxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  const char* name = output_port->getDataClassName();
  int type = vtkDataObjectTypes::GetTypeIdFromClassName(name);
  switch(type)
    {
    case VTK_TABLE:
      return true;
    }

  return false;
}

void ClientTableView::updateRepresentation(pqRepresentation* representation)
{
  vtkSMSelectionDeliveryRepresentationProxy* const proxy = representation?
    vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;
  vtkTable *table = proxy? vtkTable::SafeDownCast(proxy->GetOutput()) : NULL;
  if (table)
    {
    this->Implementation->Table->ShallowCopy(table);
      
    vtkIntArray *selectionColumn = vtkIntArray::New();
    selectionColumn->SetName("Selected");
    selectionColumn->SetNumberOfTuples(this->Implementation->Table->GetNumberOfRows());
    this->Implementation->Table->AddColumn(selectionColumn);
    selectionColumn->Delete();

    this->Implementation->TableAdapter.reset();
    this->Implementation->Widgets.rowCount->setText(QString::number(table->GetNumberOfRows()));
    this->Implementation->Widgets.columnCount->setText(QString::number(table->GetNumberOfColumns()));

    this->Implementation->Widgets.tableView->hideColumn(this->Implementation->Table->GetNumberOfColumns()-1);
    }
}

void ClientTableView::showRepresentation(pqRepresentation* representation)
{
  this->updateRepresentation(representation);
}

void ClientTableView::hideRepresentation(pqRepresentation* representation)
{
  this->Implementation->Table->Initialize();
  this->Implementation->TableAdapter.reset();
  this->Implementation->Widgets.rowCount->setText("None");
  this->Implementation->Widgets.columnCount->setText("None");
}

void ClientTableView::renderInternal()
{
  pqRepresentation* representation = this->visibleRepresentation();
  vtkSMSelectionDeliveryRepresentationProxy* const proxy = representation?
    vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;

  if(!proxy)
    {
    return;
    }

  proxy->GetSelectionRepresentation()->Update();
  vtkSelection* sel = vtkSelection::SafeDownCast(
    proxy->GetSelectionRepresentation()->GetOutput());

  this->updateSelection(sel);
}


void ClientTableView::onSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  vtkAbstractArray *pedigreeIds = this->Implementation->Table->GetRowData()->GetPedigreeIds();
  if(!pedigreeIds)
    {
    return;
    }

  vtkSelection* selection = vtkSelection::New();
  selection->SetContentType(vtkSelection::SELECTIONS);
  selection->SetFieldType(vtkSelection::ROW);
  
  vtkAbstractArray *domainArray = this->Implementation->Table->GetColumnByName("domain");
  const QModelIndexList selectedIndices = this->Implementation->Widgets.tableView->selectionModel()->selectedRows();
  for (int i = 0; i < selectedIndices.size(); i++)
    {
    QModelIndex index = this->Implementation->TableSort.mapToSource(selectedIndices.at(i));

    QString domain;
    if(domainArray)
      {
      vtkVariant d;
      switch(domainArray->GetDataType())
        {
        vtkExtraExtendedTemplateMacro(d = *static_cast<VTK_TT*>(domainArray->GetVoidPointer(index.row())));
        }
      domain = d.ToString();
      }
    else
      {
      domain = pedigreeIds->GetName();
      }

    vtkSelection *childSelection = NULL;
    for(unsigned int j=0; j<selection->GetNumberOfChildren(); ++j)
      {
      vtkSelection *sel = selection->GetChild(j);
      if(domain == sel->GetSelectionList()->GetName())
        {
        childSelection = sel;
        break;
        }
      }

    if(!childSelection)
      {
      childSelection = vtkSelection::New();
      childSelection->SetContentType(vtkSelection::PEDIGREEIDS);
      childSelection->SetFieldType(vtkSelection::ROW);
      vtkVariantArray* childSelectionList = vtkVariantArray::New();
      childSelectionList->SetName(domain.toAscii().data());
      childSelection->SetSelectionList(childSelectionList);
      childSelectionList->Delete();
      selection->AddChild(childSelection);
      childSelection->Delete();
      }

    vtkVariant v(0);
    switch (pedigreeIds->GetDataType())
      {
      vtkExtraExtendedTemplateMacro(v = *static_cast<VTK_TT*>(pedigreeIds->GetVoidPointer(index.row())));
      }
    vtkVariantArray::SafeDownCast(childSelection->GetSelectionList())->InsertNextValue(v);
    }

  this->Implementation->UpdatingSelection = true;

  // Get the representaion's source
  pqDataRepresentation* pqRepr =
    qobject_cast<pqDataRepresentation*>(this->visibleRepresentation());
  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  // Fill the selection source with the selection from the view
  vtkSMSourceProxy* selectionSource = pqSelectionManager::createSelectionSource(
    selection, repSource->GetConnectionID());

  // Set the selection on the representation's source
  repSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);

  selectionSource->Delete();
  selection->Delete();
}

void ClientTableView::updateSelection(vtkSelection *origSelection)
{
  if (this->Implementation->UpdatingSelection)
    {
    this->Implementation->UpdatingSelection = false;
    return;
    }
  
  if(!origSelection)
    {
    return;
    }

  vtkAbstractArray *pedigreeIds = this->Implementation->Table->GetRowData()->GetPedigreeIds();
  if(!pedigreeIds)
    {
    return;
    }

  // Does the selection have a compatible field type?
  vtkSelection* selection = 0;
  if (origSelection && origSelection->GetContentType() == vtkSelection::SELECTIONS)
    {
    for (unsigned int i = 0; i < origSelection->GetNumberOfChildren(); i++)
      {
      vtkSelection* child = origSelection->GetChild(i);
      if (child && child->GetFieldType() != vtkSelection::SELECTIONS )
        {
        selection = vtkSelection::New();
        selection->ShallowCopy(child);
        break;
        }
      }
    }
  else
    {
    selection = vtkSelection::New();
    selection->ShallowCopy(origSelection);
    }
  
  if(!selection || selection->GetContentType() != vtkSelection::PEDIGREEIDS)
    {
    // Did not find a selection with the same field type
    return;
    }

  selection->SetFieldType(vtkSelection::ROW);
  vtkSelection* indexSelection = vtkConvertSelection::ToIndexSelection(selection, this->Implementation->Table);
  selection->Delete();
  vtkIdTypeArray* indexArr = vtkIdTypeArray::SafeDownCast(indexSelection->GetSelectionList());

  int rows = this->Implementation->Table->GetNumberOfRows();
  int cols = this->Implementation->Table->GetNumberOfColumns();

  // Clear the selection column
  vtkIntArray *selectionColumn = vtkIntArray::SafeDownCast(this->Implementation->Table->GetColumn(cols-1));
  for (vtkIdType r = 0; r < rows; r++)
    {
    selectionColumn->SetValue(r,0);
    }

  // modify the selection column to denote selected items
  for (vtkIdType i = 0; i < indexArr->GetNumberOfTuples(); i++)
    {
    vtkIdType selectedRow = indexArr->GetValue(i);
    selectionColumn->SetValue(indexArr->GetValue(i),1);
    }

  // HACK: This is the only way to regenerate vtkQtTableModelAdapter's hash map:
  this->Implementation->TableAdapter.setTable(0);
  this->Implementation->TableAdapter.setTable(this->Implementation->Table);

  QItemSelection list;
  for (vtkIdType i = 0; i < indexArr->GetNumberOfTuples(); i++)
    {
    vtkIdType selectedRow = indexArr->GetValue(i);
    selectionColumn->SetValue(selectedRow,1);
    QModelIndex index = 
      this->Implementation->TableAdapter.PedigreeToQModelIndex(
      this->Implementation->TableAdapter.IdToPedigree(selectedRow));
  
    QModelIndex sortIndex = this->Implementation->TableSort.mapFromSource(index);
    list.select(sortIndex, sortIndex);
    }
  
  this->disconnect(this->Implementation->Widgets.tableView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
    this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

  this->Implementation->Widgets.tableView->selectionModel()->select(list, 
    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  this->connect(this->Implementation->Widgets.tableView->selectionModel(), 
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
    this, SLOT(onSelectionChanged(const QItemSelection&, const QItemSelection&)));

/*
  int selectedColumn = 0;
  int visibleColumnCount = 0;
  for(int i = 0; i != this->Implementation->TableSort.columnCount(); ++i)
    {
    if(this->Implementation->TableSort.headerData(i,Qt::Horizontal,Qt::DisplayRole ).toString() == "Selected")
      {
      //selectedColumn = visibleColumnCount;
      selectedColumn = this->Implementation->Widgets.tableView->horizontalHeader()->visualIndex(i);
      break;
      }
    if(this->Implementation->Widgets.tableView->isColumnHidden(i) == false)
      {
      visibleColumnCount++;
      }
    }
*/
  this->Implementation->Widgets.tableView->sortByColumn(cols-1, Qt::DescendingOrder);
  this->Implementation->Widgets.tableView->scrollToTop();

  indexSelection->Delete();
}