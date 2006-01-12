
#include "pqSMAdaptor.h"

#include <assert.h>
#include <map>

#include <QString>
#include <QVariant>
#include <QByteArray>
#include <QSignalMapper>

#include "vtkEventQtSlotConnect.h"

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDomain.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMStringVectorProperty.h"


namespace {
  // store SM side of connection information
  class SMGroup
    {
  public:
    SMGroup(vtkSMProxy* a, vtkSMProperty* b, int c)
      : Proxy(a), Property(b), Index(c) {}
    SMGroup& operator=(const SMGroup& copy)
      {
      this->Proxy = copy.Proxy;
      this->Property = copy.Property;
      this->Index = copy.Index;
      return *this;
      }
    bool operator<(SMGroup const& other) const
      {
      if(this->Proxy < other.Proxy)
        return true;
      else if(this->Proxy > other.Proxy)
        return false;

      if(this->Property < other.Property)
        return true;
      else if(this->Property > other.Property)
        return false;

      return this->Index < other.Index;
      }
    vtkSMProxy* Proxy;
    vtkSMProperty* Property;
    int Index;
    };
  // store Qt side of connection information
  typedef vtkstd::pair<QObject*, QByteArray> QtGroup;
}


class pqSMAdaptorInternal
{
public:
  pqSMAdaptorInternal()
    {
    this->VTKConnections = vtkEventQtSlotConnect::New();
    this->QtConnections = new QSignalMapper;
    this->SettingProperty = false;
    }
  ~pqSMAdaptorInternal()
    {
    this->VTKConnections->Delete();
    delete this->QtConnections;
    }

  // managed links
  typedef vtkstd::multimap<SMGroup, QtGroup> LinkMap;
  LinkMap SMLinks;
  LinkMap QtLinks;

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  
  // handle changes from the Qt side
  QSignalMapper* QtConnections;

  // handle domain connections
  typedef vtkstd::multimap<vtkSMProperty*, vtkstd::pair<QObject*, QByteArray> > DomainConnectionMap;
  DomainConnectionMap DomainConnections;

  bool SettingProperty;
};


pqSMAdaptor *pqSMAdaptor::Instance = 0;

pqSMAdaptor::pqSMAdaptor()
{
  this->Internal = new pqSMAdaptorInternal;
  QObject::connect(this->Internal->QtConnections, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(qtLinkedPropertyChanged(QWidget*)));

  if(!pqSMAdaptor::Instance)
    pqSMAdaptor::Instance = this;
}

pqSMAdaptor::~pqSMAdaptor()
{
  if(pqSMAdaptor::Instance == this)
    pqSMAdaptor::Instance = 0;

  delete this->Internal;
}

pqSMAdaptor* pqSMAdaptor::instance()
{
  return pqSMAdaptor::Instance;
}

void pqSMAdaptor::setProperty(vtkSMProperty* Property, QVariant QtProperty)
{
  QList<QVariant> props;
  if(QtProperty.type() == QVariant::List)
    props = QtProperty.toList();
  else
    props.push_back(QtProperty);
  
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  assert(VectorProperty != NULL);
  
  vtkSMPropertyAdaptor* adapter = vtkSMPropertyAdaptor::New();
  adapter->SetProperty(Property);

  if(adapter->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    assert(props.size() <= (QList<QVariant>::size_type)adapter->GetNumberOfSelectionElements());
    }
  else
    {
    assert(props.size() <= (QList<QVariant>::size_type)VectorProperty->GetNumberOfElements());
    }

  for(int i=0; i<props.size(); i++)
    {
    this->setProperty(Property, i, props[i]);
    }
}

QVariant pqSMAdaptor::getProperty(vtkSMProperty* Property)
{
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(VectorProperty == NULL)
    {
    return QVariant();
    }

  int numElems = 0;
  vtkSMPropertyAdaptor* adapter = vtkSMPropertyAdaptor::New();
  adapter->SetProperty(Property);

  if(adapter->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    numElems = adapter->GetNumberOfSelectionElements();
    }
  else
    {
    numElems = VectorProperty->GetNumberOfElements();
    if(numElems == 1)
      {
      return this->getProperty(Property, 0);
      }
    }

  QList<QVariant> props;
  for(int i=0; i<numElems; i++)
    {
    props.push_back(this->getProperty(Property, i));
    }
  
  adapter->Delete();
  return props;
}

void pqSMAdaptor::setProperty(vtkSMProperty* Property, int Index, QVariant QtProperty)
{
  this->Internal->SettingProperty = true;
  vtkSMPropertyAdaptor* adapter = vtkSMPropertyAdaptor::New();
  adapter->SetProperty(Property);
  if(adapter->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION &&
      adapter->GetElementType() == vtkSMPropertyAdaptor::INT)
    {
    adapter->SetEnumerationValue(QtProperty.toString().toAscii().data());
    }
  else if(adapter->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION && Property->GetInformationProperty())
    {
    const char* name = adapter->GetSelectionName(Index);
    if(QtProperty.type() == QVariant::Bool)
      QtProperty = QtProperty.toInt();
    adapter->SetRangeValue(0, name);
    adapter->SetRangeValue(1, QtProperty.toString().toAscii().data());
    Property->Modified();  // push to server  // TODO: this doesn't work
    }
  else
    {
    // bools expand to "true" or "false" instead of "1" or "0"
    if(QtProperty.type() == QVariant::Bool)
      QtProperty = QtProperty.toInt();
    adapter->SetRangeValue(Index, QtProperty.toString().toAscii().data());
    }

  adapter->Delete();
  this->Internal->SettingProperty = false;
}

QVariant pqSMAdaptor::getProperty(vtkSMProperty* Property, int Index)
{

  vtkSMPropertyAdaptor* adapter = vtkSMPropertyAdaptor::New();
  adapter->SetProperty(Property);

  int propertyType = adapter->GetPropertyType();
  QString name;
  QVariant var;

  if(vtkSMPropertyAdaptor::SELECTION == propertyType)
    {
    name = adapter->GetSelectionName(Index);
    vtkSMStringVectorProperty* infoProp = vtkSMStringVectorProperty::SafeDownCast(Property->GetInformationProperty());
    if(infoProp)
      {
      int exists;
      int idx = infoProp->GetElementIndex(name.toAscii().data(), exists);
      var = infoProp->GetElement(idx+1);
      }
    else
      {
      var = adapter->GetSelectionValue(Index);
      }
    }
  else
    {
    var = adapter->GetRangeValue(Index);
    }

  // Convert the variant to the appropriate type.
  switch(adapter->GetElementType())
    {
    case vtkSMPropertyAdaptor::INT:
      {
      if(adapter->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION)
        {
        var = adapter->GetEnumerationValue();
        }
      if(var.canConvert(QVariant::Int))
        {
        var.convert(QVariant::Int);
        }
      }
      break;
    case vtkSMPropertyAdaptor::DOUBLE:
      {
      if(var.canConvert(QVariant::Double))
        {
        var.convert(QVariant::Double);
        }
      }
      break;
    case vtkSMPropertyAdaptor::BOOLEAN:
      {
      if(var.canConvert(QVariant::Bool))
        {
        var.convert(QVariant::Bool);
        }
      }
      break;
    }

  adapter->Delete();

  if(!name.isNull())
    {
    QList<QVariant> newvar;
    newvar.push_back(name);
    newvar.push_back(var);
    var = newvar;
    }
  return var;
}


void pqSMAdaptor::linkPropertyTo(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                                        QObject* qObject, const char* qProperty)
{
  if(!Property || !qObject)
    return;

  // set the property on the QObject, so they start in-sync
  QVariant val = this->getProperty(Property, Index);
  if(val.type() == QVariant::List)
    {
    val = val.toList()[1];
    }
  qObject->setProperty(qProperty, val);

  pqSMAdaptorInternal::LinkMap::iterator iter = this->Internal->SMLinks.find(SMGroup(Proxy, Property, Index));
  bool found = iter != this->Internal->SMLinks.end();
    
  iter = this->Internal->SMLinks.insert(iter, 
                               pqSMAdaptorInternal::LinkMap::value_type(
                                 SMGroup(Proxy, Property, Index), QtGroup(qObject, qProperty)));


  if(!found)
    {
    // connect SM property changed to QObject set property
    this->Internal->VTKConnections->Connect(Property, vtkCommand::ModifiedEvent,
                                            this, SLOT(smLinkedPropertyChanged(vtkObject*, unsigned long, void*)),
                                            (void*)&(iter->first));
    }
}

void pqSMAdaptor::unlinkPropertyFrom(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                                            QObject* qObject, const char* qProperty)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  
  Iters iters = this->Internal->SMLinks.equal_range(SMGroup(Proxy, Property, Index));

  bool all = true;

  pqSMAdaptorInternal::LinkMap::iterator qiter;
  for(qiter = iters.first; qiter != iters.second; )
    {
    if((qObject == NULL || qiter->second.first == qObject) &&
       (qProperty == NULL || qiter->second.second == qProperty))
      {
      this->Internal->SMLinks.erase(qiter++);
      }
    else
      {
      ++qiter;
      all = false;
      }
    }
  
  if(all)
    {
    this->Internal->VTKConnections->Disconnect(Property, vtkCommand::ModifiedEvent, this);
    }
}

void pqSMAdaptor::linkPropertyTo(QObject* qObject, const char* qProperty, const char* signal,
                                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject)
    return;

  QVariant val = this->getProperty(Property, Index);
  if(val.type() == QVariant::List)
    {
    val = val.toList()[1];
    }
  qObject->setProperty(qProperty, val);

  pqSMAdaptorInternal::LinkMap::iterator iter = this->Internal->QtLinks.insert(this->Internal->QtLinks.end(), 
                               pqSMAdaptorInternal::LinkMap::value_type(
                                 SMGroup(Proxy, Property, Index), QtGroup(qObject, qProperty)));

  QObject::connect(qObject, signal, this->Internal->QtConnections, SLOT(map()));
  this->Internal->QtConnections->setMapping(qObject, reinterpret_cast<QWidget*>(&*iter));

}

void pqSMAdaptor::unlinkPropertyFrom(QObject* qObject, const char* qProperty, const char* signal,
                                            vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  
  Iters iters = this->Internal->QtLinks.equal_range(SMGroup(Proxy, Property, Index));

  bool all = true;

  pqSMAdaptorInternal::LinkMap::iterator qiter;
  for(qiter = iters.first; qiter != iters.second; )
    {
    if((qObject == NULL || qiter->second.first == qObject) &&
       (qProperty == NULL || qiter->second.second == qProperty))
      {
      QObject::disconnect(qiter->second.first, signal, this->Internal->QtConnections, SLOT(map()));
      this->Internal->QtConnections->removeMappings(qiter->second.first);
      this->Internal->QtLinks.erase(qiter++);
      }
    else
      {
      ++qiter;
      all = false;
      }
    }
}

void pqSMAdaptor::smLinkedPropertyChanged(vtkObject*, unsigned long, void* data)
{
  if(this->Internal->SettingProperty == true)
    {
    // when setting properties on selection properties, this slot gets called in the middle of
    // setting the value, so it messes up the state of things
    return;
    }

  SMGroup* d = static_cast<SMGroup*>(data);
  
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  pqSMAdaptorInternal::LinkMap::iterator iter;
  
  // is there a way to not do a lookup?
  Iters iters = this->Internal->SMLinks.equal_range(*d);
  
  QVariant var = this->getProperty(d->Property, d->Index);
  if(var.type() == QVariant::List)
    {
    var = var.toList()[1];
    }
  
  for(iter = iters.first; iter != iters.second; ++iter)
    {
    QVariant old = iter->second.first->property(iter->second.second.data());
    if(old != var)
      iter->second.first->setProperty(iter->second.second.data(), var);
    }
}

void pqSMAdaptor::qtLinkedPropertyChanged(QWidget* data)
{
  // map::value_type is masked as a QWidget
  pqSMAdaptorInternal::LinkMap::value_type* iter = 
    reinterpret_cast<pqSMAdaptorInternal::LinkMap::value_type*>(data);
  
  QVariant prop = iter->second.first->property(iter->second.second.data());
  QVariant old = this->getProperty(iter->first.Property, iter->first.Index);
  if(prop.type() == QVariant::List)
    {
    prop = prop.toList()[1];
    }
  if(prop != old)
    {
    this->setProperty(iter->first.Property, iter->first.Index, prop);
    iter->first.Proxy->UpdateVTKObjects();
    }
}

void pqSMAdaptor::connectDomain(vtkSMProperty* prop, QObject* qObject, const char* slot)
{
  if(!QMetaObject::checkConnectArgs(SIGNAL(foo(vtkSMProperty*)), slot))
    {
    qWarning("Incorrect slot %s::%s for pqSMAdaptor::ConnectDomain\n", 
              qObject->metaObject()->className(),
              slot+1);
    return;
    }

  vtkSMDomainIterator* domainIter = prop->NewDomainIterator();
  for(; !domainIter->IsAtEnd(); domainIter->Next())
    {
    pqSMAdaptorInternal::DomainConnectionMap::iterator iter =
      this->Internal->DomainConnections.insert(this->Internal->DomainConnections.end(),
                                               pqSMAdaptorInternal::DomainConnectionMap::value_type(
                                                 prop, vtkstd::pair<QObject*, QByteArray>(
                                                   qObject, slot)));
    this->Internal->VTKConnections->Connect(domainIter->GetDomain(), vtkCommand::DomainModifiedEvent,
                                            this, SLOT(smDomainChanged(vtkObject*, unsigned long, void*)),
                                            &*iter);
    }
  domainIter->Delete();
}

void pqSMAdaptor::disconnectDomain(vtkSMProperty* prop, QObject* qObject, const char* slot)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::DomainConnectionMap::iterator, pqSMAdaptorInternal::DomainConnectionMap::iterator> PairIter;

  PairIter iters = this->Internal->DomainConnections.equal_range(prop);

  for(; iters.first != iters.second; ++iters.first)
    {
    if(iters.first->second.first == qObject && iters.first->second.second == slot)
      {
      vtkSMDomainIterator* domainIter = prop->NewDomainIterator();
      for(; !domainIter->IsAtEnd(); domainIter->Next())
        {
        this->Internal->VTKConnections->Disconnect(domainIter->GetDomain(), vtkCommand::DomainModifiedEvent,
                                                   this, SLOT(smDomainChanged(vtkObject*, unsigned long, void*)),
                                                   &*iters.first);
        this->Internal->DomainConnections.erase(iters.first);
        }
      domainIter->Delete();
      return;
      }
    }
}

void pqSMAdaptor::smDomainChanged(vtkObject*, unsigned long /*event*/, void* data)
{
  pqSMAdaptorInternal::DomainConnectionMap::value_type* call = 
    reinterpret_cast<pqSMAdaptorInternal::DomainConnectionMap::value_type*>(data);
  QMetaObject::invokeMethod(call->second.first, call->second.second.data(), Q_ARG(vtkSMProperty*, call->first));
}

