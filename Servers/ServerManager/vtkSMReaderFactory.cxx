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
#include "vtkSMReaderFactory.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkStringList.h"

#include <vtkstd/list>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

class vtkSMReaderFactory::vtkInternals
{
public:
  struct vtkValue
    {
    vtkstd::string Group;
    vtkstd::string Name;
    vtkstd::vector<vtkstd::string> Extensions;
    vtkstd::string Description;

    void FillInformation()
      {
      vtkSMProxy* prototype =
        vtkSMProxyManager::GetProxyManager()->GetPrototypeProxy(
          this->Group.c_str(), this->Name.c_str());
      if (!prototype || !prototype->GetHints())
        {
        return;
        }
      vtkPVXMLElement* rfHint =
        prototype->GetHints()->FindNestedElementByName("ReaderFactory");
      if (!rfHint)
        {
        return;
        }

      this->Extensions.clear();
      const char* exts = rfHint->GetAttribute("extensions");
      if (exts)
        {
        vtksys::SystemTools::Split(exts, this->Extensions,' ');
        }
      this->Description = rfHint->GetAttribute("file_description");
      }

    // Returns true is a prototype proxy can be created on the given connection.
    // For now, the connection is totally ignored since ServerManager doesn't
    // support that.
    bool CanCreatePrototype(vtkIdType vtkNotUsed(cid))
      {
      return (vtkSMProxyManager::GetProxyManager()->GetPrototypeProxy(
        this->Group.c_str(), this->Name.c_str()) != NULL);
      }

    // Returns true if the reader can read the file. More correctly, it returns
    // false is the reader reports that it cannot read the file.
    bool CanReadFile(const char* filename,
      const vtkstd::vector<vtkstd::string>& extensions, vtkIdType cid);

    // Tests if 'any' of the strings in extensions is contained in
    // this->Extensions.
    bool ExtensionTest(const vtkstd::vector<vtkstd::string>& extensions);
    };

  void BuildExtensions(
    const char* filename, vtkstd::vector<vtkstd::string>& extensions)
    {
    // basically we are filling up extensions with all possible extension
    // combintations eg. myfilename.tar.gz.vtk.000 results in
    // 000, vtk.000, gz.vtk.000, tar.gz.vtk.000,
    // vtk, gz.vtk, tar.gz.vtk
    // gz, tar.gz
    // tar, tar.gz
    // gz
    // in that order.
    vtkstd::string extension =
      vtksys::SystemTools::GetFilenameExtension(filename);
    if (extension.size() > 0)
      {
      extension.erase(extension.begin()); // remove the first "."
      }
    vtkstd::vector<vtkstd::string> parts;
    vtksys::SystemTools::Split(extension.c_str(), parts, '.');
    int num_parts = static_cast<int>(parts.size());
    for (int cc=num_parts-1; cc >= 0; cc--)
      {
      for (int kk=cc; kk >=0; kk--)
        {
        vtkstd::string cur_string;
        for (int ii=kk; ii <=cc; ii++)
          {
          if (parts[ii].size() == 0)
            {
            continue;//skip empty parts.
            }
          if (ii != kk)
            {
            cur_string += ".";
            }
          cur_string += parts[ii];
          }
        extensions.push_back(cur_string);
        }
      }
    }

  typedef vtkstd::list<vtkValue> PrototypesType;
  PrototypesType Prototypes;
  vtkstd::string SupportedFileTypes;
};

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::ExtensionTest(
  const vtkstd::vector<vtkstd::string>& extensions)
{
  if (this->Extensions.size() == 0 || extensions.size() == 0)
    {
    return true;
    }

  vtkstd::vector<vtkstd::string>::const_iterator iter1;
  for (iter1 = extensions.begin(); iter1 != extensions.end(); ++iter1)
    {
    vtkstd::vector<vtkstd::string>::const_iterator iter2;
    for (iter2 = this->Extensions.begin(); iter2 != this->Extensions.end();
      ++iter2)
      {
      if (*iter1 == *iter2)
        {
        return true;
        }
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::CanReadFile(
  const char* filename, 
  const vtkstd::vector<vtkstd::string>& extensions, vtkIdType cid)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy(
    this->Group.c_str(), this->Name.c_str());
  if (!prototype)
    {
    return false;
    }

  if (!this->ExtensionTest(extensions))
    {
    return false;
    }

  if (strcmp(prototype->GetXMLName(), "ImageReader") == 0)
    {
    // ImageReader always returns 0 so don't test it
    return true;
    }

  vtkSMProxy* proxy = pxm->NewProxy(this->Group.c_str(), this->Name.c_str());
  proxy->SetConnectionID(cid);
  proxy->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  proxy->UpdateVTKObjects();
  bool canRead = vtkSMReaderFactory::CanReadFile(filename, proxy);
  proxy->Delete();
  return canRead;
}

vtkStandardNewMacro(vtkSMReaderFactory);
vtkCxxRevisionMacro(vtkSMReaderFactory, "$Revision$");
//----------------------------------------------------------------------------
vtkSMReaderFactory::vtkSMReaderFactory()
{
  this->Internals = new vtkInternals();
  this->Readers = vtkStringList::New();
  this->ReaderName = 0;
  this->ReaderGroup = 0;
}

//----------------------------------------------------------------------------
vtkSMReaderFactory::~vtkSMReaderFactory()
{
  delete this->Internals;
  this->SetReaderName(0);
  this->SetReaderGroup(0);
  this->Readers->Delete();
  this->Readers = 0;
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::Initialize()
{
  this->Internals->Prototypes.clear();
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototype(const char* xmlgroup, const char* xmlname)
{
  // If already present, we remove old one and append again so that the priority
  // rule still works.
  this->UnRegisterPrototype(xmlgroup, xmlname);
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;
  
  // fills extension information etc. from the prototype.
  value.FillInformation(); 

  this->Internals->Prototypes.push_front(value);
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototype(
  const char* xmlgroup, const char* xmlname,
  const char* extensions, const char* description)

{
  // If already present, we remove old one and append again so that the priority
  // rule still works.
  this->UnRegisterPrototype(xmlgroup, xmlname);
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;
  
  // fills extension information etc. from the prototype.
  value.FillInformation(); 
  if (description)
    {
    value.Description = description;
    }
  if (extensions)
    {
    vtksys::SystemTools::Split(extensions, value.Extensions, ' ');
    }
  this->Internals->Prototypes.push_front(value);
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::UnRegisterPrototype(
  const char* xmlgroup, const char* xmlname)
{
  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->Group == xmlgroup  && iter->Name == xmlname)
      {
      this->Internals->Prototypes.erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::LoadConfigurationFile(const char* filename)
{
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(filename);
  if (!parser->Parse())
    {
    vtkErrorMacro("Failed to parse file: " << filename);
    return false;
    }

  return this->LoadConfiguration(parser->GetRootElement());
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::LoadConfiguration(const char* xmlcontents)
{
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();

  if (!parser->Parse(xmlcontents))
    {
    vtkErrorMacro("Failed to parse xml. Not a valid XML.");
    return false;
    }

  vtkPVXMLElement* rootElement = parser->GetRootElement();
  return this->LoadConfiguration(rootElement);
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::LoadConfiguration(vtkPVXMLElement* elem)
{
  if (!elem)
    {
    return false;
    }

  if (elem->GetName() && 
    strcmp(elem->GetName(), "ParaViewReaders") != 0)
    {
    return this->LoadConfiguration(
      elem->FindNestedElementByName("ParaViewReaders"));
    }

  unsigned int num = elem->GetNumberOfNestedElements();
  for(unsigned int i=0; i<num; i++)
    {
    vtkPVXMLElement* reader = elem->GetNestedElement(i);
    if (reader->GetName() && 
      (strcmp(reader->GetName(),"Reader") == 0 || 
       strcmp(reader->GetName(), "Proxy") == 0))
      {
      const char* name = reader->GetAttribute("name");
      const char* group = reader->GetAttribute("group");
      group = group ? group : "sources";
      if (name && group)
        {
        // NOTE this is N^2. We may want to use a separate set or something to
        // test of existence if this becomes an issue.
        this->RegisterPrototype(group, name,
          reader->GetAttribute("extensions"),
          reader->GetAttribute("file_description"));
        }
      }
    }
  return true;
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetReaders(vtkIdType cid)
{
  return this->GetPossibleReaders(NULL, cid);
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetPossibleReaders(const char* filename,
  vtkIdType cid)
{
  this->Readers->RemoveAllItems();

  if (!filename || filename[0] == 0)
    {
    return this->Readers;
    }

  vtkstd::vector<vtkstd::string> extensions;
  // purposefully set the extensions to empty, since we don't want the extension
  // test to be used for this case.
  // this->Internals->BuildExtensions(filename, extensions);

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(cid) &&
      (!filename || iter->CanReadFile(filename, extensions, cid)))
      {
      this->Readers->AddString(iter->Group.c_str());
      this->Readers->AddString(iter->Name.c_str());
      this->Readers->AddString(iter->Description.c_str());
      }
    }

  return this->Readers;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, vtkIdType cid)
{
  this->SetReaderGroup(0);
  this->SetReaderName(0);

  if (!filename || filename[0] == 0)
    {
    return false;
    }

  vtkstd::vector<vtkstd::string> extensions;
  this->Internals->BuildExtensions(filename, extensions);

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(cid) && iter->CanReadFile(filename, extensions, cid))
      {
      this->SetReaderGroup(iter->Group.c_str());
      this->SetReaderName(iter->Name.c_str());
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
static vtkstd::string vtkJoin(
  const vtkstd::vector<vtkstd::string> exts, const char* prefix,
  const char* suffix)
{
  vtksys_ios::ostringstream stream;
  vtkstd::vector<vtkstd::string>::const_iterator iter;
  for (iter = exts.begin(); iter != exts.end(); ++iter)
    {
    stream << prefix << *iter << suffix;
    }
  return stream.str();
}

//----------------------------------------------------------------------------
const char* vtkSMReaderFactory::GetSupportedFileTypes(vtkIdType cid)
{
  vtksys_ios::ostringstream all_types;
  all_types << "Supported Files (";

  vtkstd::set<vtkstd::string> sorted_types;

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(cid))
      {
      if (iter->Extensions.size() > 0)
        {
        vtkstd::string ext_join = ::vtkJoin(iter->Extensions, "*.", " ");
        vtksys_ios::ostringstream stream;
        stream << iter->Description << "(" << ext_join << ")";
        sorted_types.insert(stream.str());
        all_types << ext_join << " ";
        }
      }
    }
  all_types << ")";
  
  vtkstd::set<vtkstd::string>::iterator iter2;
  for (iter2 = sorted_types.begin(); iter2 != sorted_types.end(); ++iter2)
    {
    all_types << ";;" << (*iter2);
    }
  this->Internals->SupportedFileTypes = all_types.str();
  return this->Internals->SupportedFileTypes.c_str();
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::TestFileReadability(const char* filename, vtkIdType cid)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("file_listing", "ServerFileListing"));
  if (!proxy)
    {
    vtkGenericWarningMacro("Failed to create ServerFileListing proxy.");
    return false;
    }

  proxy->SetConnectionID(cid);
  proxy->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  vtkSMPropertyHelper(proxy, "ActiveFileName").Set(filename);
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();

  if (vtkSMPropertyHelper(proxy, "ActiveFileIsReadable").GetAsInt() != 0)
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, vtkSMProxy* proxy)
{
  // Assume that it can read the file if CanReadFile does not exist.
  int canRead = 1;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() 
         << "SetReportInterpreterErrors" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << proxy->GetID() << "CanReadFile" << filename
         << vtkClientServerStream::End;
  pm->SendStream(proxy->GetConnectionID(), 
    vtkProcessModule::GetRootId(proxy->GetServers()), stream);
  pm->GetLastResult(proxy->GetConnectionID(),
    vtkProcessModule::GetRootId(proxy->GetServers())).GetArgument(0, 0, &canRead);
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() 
         << "SetReportInterpreterErrors" << 1
         << vtkClientServerStream::End;
  pm->SendStream(proxy->GetConnectionID(), 
    vtkProcessModule::GetRootId(proxy->GetServers()), stream);
  return (canRead != 0);
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, 
  const char* readerxmlgroup, const char* readerxmlname, vtkIdType cid)
{
  vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->NewProxy(
    readerxmlgroup, readerxmlname);
  if (!proxy)
    {
    return false;
    }
  proxy->SetConnectionID(cid);
  proxy->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  proxy->UpdateVTKObjects();
  bool canRead = vtkSMReaderFactory::CanReadFile(filename, proxy);
  proxy->Delete();
  return canRead;
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

