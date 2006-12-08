/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id$  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "XdmfDOM.h"

#include <libxml/parser.h>
#include <libxml/tree.h>


XdmfDOM *HandleToXdmfDOM( XdmfConstString Source ){
  XdmfObject  *TempObj;
  XdmfDOM   *DOM;

  TempObj = HandleToXdmfObject( Source );
  DOM = (XdmfDOM *)TempObj;
  return( DOM );
  }


XdmfDOM::XdmfDOM(){
  this->WorkingDirectory = 0;
  this->NdgmHost = 0;
  this->LastDOMGet = new char[256];
  this->tree = NULL;
  this->xml = NULL;
  this->Output = &cout;
  this->Input = &cin;
  this->Doc = NULL;

  this->OutputFileName = 0;
  this->InputFileName = 0;
  XDMF_STRING_DUPLICATE(this->OutputFileName, "stdout");
  XDMF_STRING_DUPLICATE(this->InputFileName, "stdin");
  this->SetNdgmHost( "" );
  this->SetWorkingDirectory( "" );
}

XdmfDOM::~XdmfDOM(){
  if (this->xml != NULL) free(this->xml);
  if( ( this->Output != &cout ) && ( this->Output != &cerr ) ) {
    ofstream *OldOutput = ( ofstream *)this->Output;
    OldOutput->close();
  }
  if( this->Input != &cin ) {
    ifstream *OldInput = ( ifstream *)this->Input;
    OldInput->close();
    delete this->Input;
    this->Input = &cin;
  }
  if ( this->LastDOMGet )
    {
    delete [] this->LastDOMGet;
    }
  this->SetWorkingDirectory(0);
  this->SetNdgmHost(0);
  if ( this->InputFileName )
    {
    delete [] this->InputFileName;
    }
  if ( this->OutputFileName )
    {
    delete [] this->OutputFileName;
    }
  XdmfInt32 idx;
  for ( idx = 0; idx < this->NumberOfParameters; ++ idx )
    {
    delete this->Parameters[idx];
    }
  delete [] this->Parameters;
}

XdmfInt32
XdmfDOM::GetNumberOfAttributes( XdmfXmlNode *Node ){
XdmfInt32  NumberOfAttributes;

NumberOfAttributes = Node->GetSize();
// Don't include NodeType, NodeDepth, CData
if( this->Get(Node, "NodeType") != NULL ){
  NumberOfAttributes--;
  }
if( this->Get(Node, "NodeDepth") != NULL ){
  NumberOfAttributes--;
  }
if( this->Get(Node, "CData") != NULL ){
  NumberOfAttributes--;
  }
return( NumberOfAttributes );
}

XdmfConstString
XdmfDOM::GetAttribute( XdmfXmlNode *Node, XdmfInt32 Index ){
XdmfConstString  Attribute;
XdmfInt32  EIndex = 0;

do {
  Attribute = Node->GetNameByIndex( EIndex );
  if( XDMF_WORD_CMP( Attribute, "NodeType") ||
    XDMF_WORD_CMP( Attribute, "NodeDepth") ||
    XDMF_WORD_CMP( Attribute, "CData" ) ) {
    Index++;
  } else {
    if( EIndex >= Index ) return( Attribute );
    }
  EIndex++;
  } while( Attribute != NULL );
return( NULL );
}

XdmfInt32
XdmfDOM::IsChild( XdmfXmlNode *ChildToCheck, XdmfXmlNode *Start ) {

XDMF_TREE_NODE *TreeStart;
void    **ChildPtr;
XdmfTreeSearch  Search;

if( !Start ) {
  TreeStart = this->tree;
} else {    
  TreeStart = ( XDMF_TREE_NODE *)Start->GetClientData();
  }

ChildPtr = ( void **)&ChildToCheck;
Search.Occurance = 0;
Search.Status = 0;
Search.Found = NULL;
Search.LookFor = ChildPtr;
XdmfTree_walk( TreeStart, C_CheckIsChild, &Search);
if( Search.Status ) {
  // Found
  XdmfDebug("Node is a Child");
  return( XDMF_SUCCESS );
} else {
  // Tree Walk Complete without being Found
  XdmfDebug("Node is not a Child");
  }
return( XDMF_FAIL );
}

XdmfInt32
XdmfDOM::SetOutputFileName( XdmfConstString Filename ){

  if( ( this->Output != &cout ) && ( this->Output != &cerr ) ) {
          ofstream *OldOutput = ( ofstream *)this->Output;
          OldOutput->close();
        }
  if( XDMF_WORD_CMP( Filename, "stdin" ) ) {
          this->Output = &cout;
  } else if( XDMF_WORD_CMP( Filename, "stderr" ) ) {
          this->Output = &cerr;
  } else {
          ofstream        *NewOutput = new ofstream( Filename );
          if( !NewOutput ) {
                  XdmfErrorMessage("Can't Open Output File " << Filename );
                  return( XDMF_FAIL );
                }
          this->Output = NewOutput;
        }
  if ( this->OutputFileName )
    {
    delete [] this->OutputFileName;
    }
  XDMF_STRING_DUPLICATE(this->InputFileName, Filename);
  return( XDMF_SUCCESS );

}

XdmfInt32
XdmfDOM::SetInputFileName( XdmfConstString Filename ){

  if( this->Input != &cin ) {
    ifstream *OldInput = ( ifstream *)this->Input;
    OldInput->close();
    delete this->Input;
    this->Input = &cin;
  }
  if( XDMF_WORD_CMP( Filename, "stdin" ) ) {
    this->Input = &cin;
  } else {
    ifstream        *NewInput = new ifstream( Filename );
    if( !NewInput ) {
      XdmfErrorMessage("Can't Open Input File " << Filename );
      return( XDMF_FAIL );
    }
    this->Input = NewInput;
  }
  if ( this->InputFileName )
    {
    delete [] this->InputFileName;
    }
  XDMF_STRING_DUPLICATE(this->InputFileName, Filename);
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDOM::GenerateHead() {
  *this->Output << "<?xml version=\"1.0\" ?><!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\"><Xdmf>\n";
  this->Output->flush();
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfDOM::Puts( XdmfConstString String ){
  *this->Output << String;
  this->Output->flush();
  return( XDMF_SUCCESS );
}

XdmfConstString
XdmfDOM::Gets( void ) {
  if ( this->Input == &cin ) {
    char ch, *cptr;
    istream *from = this->Input;

    if( this->xml ) free( this->xml );
    // Bad Hack
    cptr = this->xml = ( XdmfString )malloc( 100000 );
    while( !from->get( ch ) ) *cptr++ = ch;
    *cptr = '\0';
  } else  {
    XdmfCharArray  c;
    c.SetFromFile( this->GetInputFileName() );
    if( this->xml ) free( this->xml );
    this->xml = ( XdmfString )malloc( c.GetNumberOfElements() + 1 );
    strcpy( this->xml, c.GetString() );
  }
return( this->xml );
}

XdmfInt32
XdmfDOM::GenerateTail() {
  *this->Output << "</Xdmf>";
  this->Output->flush();
  return( XDMF_SUCCESS );
}

XDMF_TREE_NODE *
XdmfDOM::GetTree(){
  return(this->tree);
}

XdmfConstString
XdmfDOM::Serialize(XdmfXmlNode *node) {
  XDMF_TREE_NODE *Start;
  int BLOCKSIZE = 1024;
  XdmfInt32 size;

  if( node == NULL ) {
    Start = this->tree;
  } else {
    Start = ( XDMF_TREE_NODE *)node->GetClientData();
  }
  if (this->xml != NULL) {
    free(this->xml);
    this->xml = NULL;    
  }
  this->xml = (XdmfString)malloc(BLOCKSIZE);
  this->xml[0] = '\0';
  this->xml[BLOCKSIZE - 1] = '\0';
  size = BLOCKSIZE;
  this->xml = strcat(this->xml, "\n");
  size = size - 2;
  this->ExpandNode(Start, &size);  
  this->xml = strcat(this->xml,"\0");
  return( this->xml );
}

void
XdmfDOM::ReNew(XdmfInt32 *size) {

int BLOCKSIZE = 1024;
int blocks = 0;
int len = 0;
int tmp = 0;

// XdmfDebug("*size = " << *size);
while(*size <= 0) {
  *size = *size + BLOCKSIZE;
  blocks++;
}
// Add a gratuitous block to cover for extra chars
blocks++;
len = strlen(this->xml);
// Round up to nearest block
len = (( len / BLOCKSIZE ) + 1 ) * BLOCKSIZE;
tmp = 10+len+1+(BLOCKSIZE*blocks);
// XdmfDebug("len = " << len << " tmp = " << tmp << " blocks = " << blocks);
this->xml = (XdmfString)realloc(this->xml,tmp);
this->xml[tmp-1] = '\0';

}

void 
XdmfDOM::ExpandNode(XDMF_TREE_NODE *node, XdmfInt32 *size) {
        XdmfString type;
        XdmfString ndata=NULL;
        XdmfString resname;
        XdmfString resdata;
        int  nchild;
        int  i, j;
  int NumEl;
        int len1;
  int len2;
  int pi;

  XdmfXmlNode *data;
        XDMF_TREE_NODE **children;

        children = XDMF_TREE_CHILDREN(node);
        nchild = XDMF_TREE_NCHILD(node);
        data = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(node);
        type = data->Get("NodeType");
  pi = 0;
  if (type != NULL) {
  if (*type != 0 ) {
          ndata = data->Get("CData");
    len1 = strlen(type);  
    *size = *size - (len1+2);
    if (*size <= 0) this->ReNew(size);
    this->xml = strcat(this->xml, "<");
    if( strcmp( type, "ProcessingInstruction" ) == 0 ){
      pi = 1;
      this->xml = strcat(this->xml, "?");
      this->xml = strcat(this->xml, data->Get("Target") );
    } else {
      this->xml = strcat(this->xml, type);
    }
          NumEl = data->GetSize();
          for(j = 0; j < NumEl; j++){
            resname = data->GetNameByIndex(j);
            resdata = data->GetDataByIndex(j);
            if ((strcmp(resname, "NodeType") != 0)&&
          (STRNCASECMP(resname, "_Xdmf", 5 ) != 0) &&
                      (strcmp(resname, "CData") != 0)&&
                      (strcmp(resname, "NodeDepth") != 0)) {
          if( pi && ( strcmp(resname, "Target" ) == 0 ) ) {
          } else {
          len1 = strlen(resname);
          len2 = strlen(resdata);
          *size = *size - (len1+len2 + 6);
          if (*size <= 0) this->ReNew(size);
          this->xml = strcat(this->xml, " ");
          this->xml = strcat(this->xml, resname);
          this->xml = strcat(this->xml, "=\"");
          this->xml = strcat(this->xml, resdata);
          this->xml = strcat(this->xml, "\"\n");
      }
            }
          }
          *size = *size - 3;
          if (*size <= 0) this->ReNew(size);
    if( strcmp( type, "ProcessingInstruction" ) == 0 ){
      this->xml = strcat(this->xml, " ?>\n");
    } else {
      if( ndata ) {
        this->xml = strcat(this->xml, ">\n");
        len1 = strlen(ndata);
        *size = *size - (len1+2);
        if (*size <= 0) this->ReNew(size);
        this->xml = strcat(this->xml, ndata);
        this->xml = strcat(this->xml, "\n");
      } else {
        if( nchild > 0 ) {
          this->xml = strcat(this->xml, ">\n");
          len1 = 0;
          if (*size <= 0) this->ReNew(size);
          this->xml = strcat(this->xml, "\n");
        } else {
          len1 = 0;
          if (*size <= 0) this->ReNew(size);
          this->xml = strcat(this->xml, "/>\n");
          this->xml = strcat(this->xml, "\n");
        }
      }
    }
  }
  }

        for(i=0;i<nchild;i++){
          this->ExpandNode(children[i], size);
        }

  len2 = strlen(type);
  *size = *size - (len2+5);
  if (*size <= 0) this->ReNew(size);
  if ( !pi && (type != NULL) )  {
  if (*type != 0 ) {
    if(ndata || ( nchild > 0 ) ){
    this->xml = strcat(this->xml, "</");
    this->xml = strcat(this->xml, type);
    this->xml = strcat(this->xml, ">\n");
    } else {
    // It's already ended with <Tag ... />
    }
  }
  }

}

XDMF_TREE_NODE *
XdmfDOM::__Parse( XdmfConstString inxml) {

XMLUserData data;
XdmfXmlNode *node = new XdmfXmlNode;

node->Set("NodeType", "\0");
node->Set("CData", "\0");
data.Root = XdmfTree_add(NULL, node);
data.Node = data.Root;
data.Parent = data.Root;
data.Depth = 0;
data.NumElements = 0;

XML_Parser parser = XML_ParserCreate(NULL);
XML_SetUserData(parser, &data);
if( XDMF_WORD_CMP( this->InputFileName, "stdin" ) ) {
  XML_SetBase( parser, "");
} else {
  XML_SetBase( parser, this->InputFileName );
}
XML_SetElementHandler(parser, StartElement, EndElement);
XML_SetDefaultHandlerExpand(parser, GetDefaultData );
XML_SetCharacterDataHandler(parser, GetData);
XML_SetProcessingInstructionHandler( parser, ProcessingElement );
XML_SetParamEntityParsing( parser, XML_PARAM_ENTITY_PARSING_ALWAYS );
XML_SetExternalEntityRefHandler( parser, ExternalEntity );
if (!XML_Parse(parser, inxml, strlen(inxml), 1)) {
  XdmfErrorMessage("Parse Error at XML line " <<
    XML_GetCurrentLineNumber(parser) <<
    " : " << XML_ErrorString(XML_GetErrorCode(parser)));
    XML_ParserFree(parser);
    delete node;
    return (NULL);
}
XML_ParserFree(parser);
return( data.Root );
}

XdmfInt32
XdmfDOM::Parse(XdmfConstString inxml) {

XDMF_TREE_NODE *Root;
XdmfXmlNode    *Node;
XdmfConstString  Attribute;

// Remove Previous Data
if (this->tree != NULL) XdmfTree_remove(this->tree,C__XdmfXmlNodeDelete);
this->tree = NULL;
// if (this->xml != NULL) free(this->xml);
// this->xml = NULL;
this->SetDocType(NULL);
this->SetSystem(NULL);

if( inxml == NULL ) inxml = this->Gets();
if( strlen(inxml) < 7 ){
  // At least <?xml>
  return( XDMF_FAIL );
  }

if ( inxml && ( Root = this->__Parse( inxml ) ) ) {
  this->tree = Root;
} else {
  return(XDMF_FAIL);
  }

Node = this->FindElement( "Xdmf", 0, NULL );
if( Node != NULL ){
  Attribute = this->Get( Node, "NdgmHost" );
  if( Attribute != NULL ){
    XdmfDebug("NdgmHost = " << Attribute );
    this->SetNdgmHost( Attribute );
    }
  Attribute = this->Get( Node, "WorkingDirectory" );
  if( Attribute != NULL ){
    XdmfDebug("WorkingDirectory = " << Attribute );
    this->SetWorkingDirectory( Attribute );
    }
  // See if there are any Parameters and set this->NumberOfParameters > 0 
  // for faster GET()
  if( this->GetParameter(0, Node) != NULL ){
    XdmfDebug("DOM Contains " << this->FindNumberOfParameters() << " Parameters");
  }
  }
return( XDMF_SUCCESS );
}


XdmfInt32
XdmfDOM::DeleteNode( XdmfXmlNode *Node ) {

XDMF_TREE_NODE *node;

if( Node == NULL ) return(XDMF_SUCCESS);
/* printf("Deleting TREENODE \n"); */
node = (XDMF_TREE_NODE *)Node->GetClientData();
XdmfTree_remove(node, C__XdmfXmlNodeDelete);
return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDOM::InsertFromString(XdmfXmlNode *Parent, XdmfConstString inxml) {

XDMF_TREE_NODE *NewNode;

if( ( NewNode = this->__Parse( inxml ) ) ) {
  XdmfTree_add_branch( (XDMF_TREE_NODE *)Parent->GetClientData(), NewNode );
  return( XDMF_SUCCESS );
  }
return( XDMF_FAIL );
}

XdmfInt32
XdmfDOM::Insert(XdmfXmlNode *Parent, XdmfXmlNode *Child, XdmfInt32 Level ) {

int i;
int nchildren;
XDMF_TREE_NODE *parent;
XDMF_TREE_NODE *newnode;
XDMF_TREE_NODE **children;
XdmfString Attribute;
XdmfXmlNode *ndata, *pdata;
// XdmfXmlNode *NewNodeData = new XdmfXmlNode;
XdmfXmlNode *NewNodeData;
static XDMF_TREE_NODE *original = NULL;


parent = (XDMF_TREE_NODE *)Parent->GetClientData();

if( Child->GetClientData() == NULL ) {
  NewNodeData = Child;
} else {
  NewNodeData = new XdmfXmlNode;
  memcpy(NewNodeData,Child,sizeof(XdmfXmlNode) );
}
newnode = XdmfTree_add(parent, NewNodeData);
NewNodeData->SetClientData(newnode);
XdmfTree_walk(newnode, C_SetDepth, NULL);

Attribute = Child->Get("NodeType");
if( Attribute == NULL ){
  Child->Set( "NodeType", "User" );
  }
if (Level == 0) {
  original = (XDMF_TREE_NODE *)Child->GetClientData();
  if( original == NULL ){
     original = newnode;
    }
}
nchildren = XDMF_TREE_NCHILD(original);
children = XDMF_TREE_CHILDREN(original);
for(i=0;i<nchildren;i++) {
  original = children[i];
  ndata = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(original);
  pdata = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(newnode);
  Level++;
  Insert(pdata, ndata, Level);
  Level--;
}
return(XDMF_SUCCESS);
}

XdmfXmlNode
*XdmfDOM::GetChild( XdmfInt64 Index, XdmfXmlNode *Node ){
XDMF_TREE_NODE *Start;
XDMF_TREE_NODE **children;
XdmfXmlNode  *node;
XdmfInt64  nchildren;
if( !Node ) {
  Start = this->tree;
} else {
  Start = ( XDMF_TREE_NODE *)Node->GetClientData();
}
nchildren = XDMF_TREE_NCHILD(Start);
if( Index >= nchildren){
  // XdmfErrorMessage("Index > Number of Children");
  return( NULL );
  }
children = XDMF_TREE_CHILDREN(Start);
node = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(children[ Index ]);
node->SetClientData( children[ Index ] );
return(node);
}


XdmfInt64
XdmfDOM::GetNumberOfChildren( XdmfXmlNode *Node ){
XDMF_TREE_NODE *Start;
if( !Node ) {
  Start = this->tree;
} else {
  Start = ( XDMF_TREE_NODE *)Node->GetClientData();
}
return( XDMF_TREE_NCHILD(Start) );
}

XdmfXmlNode * 
XdmfDOM::GetRoot( void ) {
if( this->tree ) {
  return( (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(this->tree));
}
return( NULL );
}

XdmfXmlNode * 
XdmfDOM::FindElement(XdmfConstString TagName, XdmfInt32 Index, XdmfXmlNode *Node ) {

XdmfString type = (XdmfString )TagName;
int  occurance = Index;
XDMF_TREE_NODE *Start;
XDMF_TREE_NODE **children;
XdmfXmlNode *node;
int nchildren;
FindNodeData fndata;

// XdmfDebug( " IN FindElement , type = " << type << " Node = " << Node << " #  " << occurance);
if( !Node ) {
  Start = this->tree;
} else {
  Start = ( XDMF_TREE_NODE *)Node->GetClientData();
}
if( !Start ) return( NULL );
if ( type ) {
  if( STRNCASECMP( type, "NULL", 4 ) == 0 ) type = NULL;
}
if ( !type ) {
  nchildren = XDMF_TREE_NCHILD(Start);
  // XdmfDebug("Type == NULL occurance = " << occurance << " nchildren = " << nchildren );
  if (occurance >= nchildren) {
    return(NULL);
  } else {
    children = XDMF_TREE_CHILDREN(Start);
    node = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(children[ occurance ]);
    node->SetClientData( children[ occurance ] );
    return(node);
  }
} else {
  // XdmfDebug("Type = " << type << " occurance = " << occurance);
  fndata.tag = type;
  fndata.occurance = occurance + 1;
  XdmfTree_walk(Start, C_FindXMLNode, &fndata);
  if( fndata.node ) {
    node = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(fndata.node);
  } else {
    return( NULL );
  }
  if( node != NULL ){
    // XdmfDebug("Found Element");
    node->SetClientData( fndata.node );
  } else {
    // XdmfDebug("Element Does Not Exist");
  }
  return(node);
}

}

XdmfXmlNode * 
XdmfDOM::FindElementByAttribute(XdmfConstString Attribute,
    XdmfConstString Value, XdmfInt32 Index, XdmfXmlNode *Node ) {

XdmfString attribute = (XdmfString )Attribute;
int  occurance = Index;
XDMF_TREE_NODE * Start;
XdmfXmlNode *node;
FindNodeData fndata;

// XdmfDebug( " IN FindElement , type = " << type << " Node = " << Node << " #  " << occurance);
if( !Node ) {
  Start = this->tree;
} else {
  Start = ( XDMF_TREE_NODE *)Node->GetClientData();
}
if( !Start ) return( NULL );
if ( !attribute ) {
  return( NULL );
} else {
  // XdmfDebug("Type = " << type << " occurance = " << occurance);
  fndata.attribute = attribute;
  fndata.value = ( XdmfString )Value;
  fndata.occurance = occurance + 1;
  XdmfTree_walk(Start, C_FindXMLNodeByAttribute, &fndata);
  if( fndata.node ) {
    node = (XdmfXmlNode *)XDMF_TREE_CLIENT_DATA(fndata.node);
  } else {
    return( NULL );
  }
  if( node != NULL ){
    // XdmfDebug("Found Element");
    node->SetClientData( fndata.node );
  } else {
    // XdmfDebug("Element Does Not Exist");
  }
  return(node);
}

}

XdmfInt32
XdmfDOM::FindNumberOfElements(XdmfConstString TagName, XdmfXmlNode *Node ) {

XDMF_TREE_NODE *Start;
FindNodeData fndata;

fndata.tag = TagName;
fndata.occurance = 0;
if( Node == NULL ) {
  Start = this->tree;
} else {
  Start = ( XDMF_TREE_NODE *)Node->GetClientData();
}
if( TagName != NULL ) {
  if( STRNCASECMP( TagName, "NULL", 4 ) == 0 ) TagName = NULL;
  }
if (TagName == NULL) {
  // XdmfDebug("Type == NULL XDMF_TREE_CLIENT_DATA( Start ) = " << XDMF_TREE_CLIENT_DATA( Start ));
  return(XDMF_TREE_NCHILD(Start));
} else {
  // XdmfDebug("Type = " << TagName );
  XdmfTree_walk(Start, C_FindXMLNumberOfNodes, &fndata);
  return(fndata.occurance);
}

}

XdmfInt32
XdmfDOM::FindNumberOfElementsByAttribute(XdmfConstString Attribute, 
    XdmfConstString Value, XdmfXmlNode *Node ) {

XDMF_TREE_NODE *Start;
FindNodeData fndata;

fndata.attribute = Attribute;
fndata.value = Value;
fndata.occurance = 0;
if( Node == NULL ) {
  Start = this->tree;
} else {
  Start = ( XDMF_TREE_NODE *)Node->GetClientData();
}
XdmfTree_walk(Start, C_FindXMLNumberOfNodesByAttribute, &fndata);
return(fndata.occurance);
}

XdmfInt32
XdmfDOM::FindNumberOfProcessingInstructions( XdmfConstString Target, XdmfXmlNode *Node )
{
XdmfInt32  Count = 0, i = 0;
XdmfXmlNode *Next = NULL;

if ( Target ) {
  if( STRNCASECMP( Target, "NULL", 4 ) == 0 ) Target = NULL;
}
Next = this->FindElement("ProcessingInstruction",
    i++,
    Node );
while( Next ){
  if( Target ) {
    if( XDMF_WORD_CMP( Target, Next->Get( "Target" ) ) ) {
      Count++;
      }
  } else {
    Count++;
    }  
  Next = this->FindElement("ProcessingInstruction",
      i++,
      Node );
  }
return( Count );
}

XdmfXmlNode *
XdmfDOM::FindProcessingInstruction( XdmfConstString Target,
      XdmfInt32 occurance, XdmfXmlNode *Node ){

XdmfInt32  Count = 0, i=0;
XdmfXmlNode   *PI;


if ( Target ) {
  if( STRNCASECMP( Target, "NULL", 4 ) == 0 ) Target = NULL;
}
PI = this->FindElement("ProcessingInstruction",
    i++,
    Node );
while( PI ){
  if( Target ) {
    if( XDMF_WORD_CMP( Target, PI->Get( "Target" ) )) {
      if( Count == occurance ) {
        return( PI );
        }
      Count++;
      }  
  } else {
    if( Count == occurance ) {
      return( PI );
    }
    Count++;
  }
  PI = this->FindElement("ProcessingInstruction",
      i++,
      Node );
  }
return( NULL );
}

int GetXNodeSize( XdmfXmlNode *Node ) { return( Node->GetSize() ); };
XdmfConstString GetXNodeName(XdmfXmlNode *Node, int index) { return( Node->GetNameByIndex( index ) ); };
XdmfConstString GetXNodeData(XdmfXmlNode *Node, int index) { return( Node->GetDataByIndex( index ) ); };

XdmfConstString
XdmfDOM::Get( XdmfXmlNode *Node, XdmfConstString Attribute ) {

xmlNode *node;

if( !Node ) {
  Node = this->FindElement( NULL, 0, NULL );
  if( !Node ) {
    return( NULL );
  }
}
node = (xmlNode *)Node;
if( STRNCASECMP( Attribute, "CDATA", 5 ) == 0 ){
    char    *txt;
    delete [] this->LastDOMGet;
    txt = XdmfConstString(xmlNodeListGetString(this->Doc, node->xmlChildrenNode, 1));
    this->LastDOMGet = new char[ strlen(txt) + 2];
    strcpy(this->LastDOMGet, txt);
    xmlFree(txt);
    return((XdmfConstString)this->LastDOMGet());
}
return((XdmfConstString)xmlGetProp(node, Attribute));
}

