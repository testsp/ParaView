"""
vtkImageExportToArray: a NumPy front-end to vtkImageExport

This class converts a VTK image to a Numeric Python array.

To use this class, you must have the LLNL Numeric Python distribution
(http://numpy.sf.net)

Methods:

  SetInput(input) -- connect to VTK image pipeline
  GetArray()      -- execute pipeline and return a Numeric array
  
Convert VTK_UNSIGNED_SHORT to python Int:
(this might be necessary because Python doesn't support unsigned short,
the default is to cast unsigned short to signed short).

  SetConvertUnsignedShortToInt(yesno)
  ConvertUnsignedShortToIntOn()
  ConvertUnsignedShortToIntOff()

From vtkImageExport:

  GetDataExtent()
  GetDataSpacing()
  GetDataOrigin()
"""
  
import Numeric
import umath
from vtk import vtkImageExport
from vtkConstants import *

class vtkImageExportToArray:
    def __init__(self):
        self.__export = vtkImageExport()
        self.__ConvertUnsignedShortToInt = 0

    # type dictionary: note that python doesn't support
    # unsigned integers!

    __typeDict = { VTK_CHAR:Numeric.Int8,
                   VTK_UNSIGNED_CHAR:Numeric.UnsignedInt8,
                   VTK_SHORT:Numeric.Int16,
                   VTK_UNSIGNED_SHORT:Numeric.Int16,
                   VTK_INT:Numeric.Int32,
                   VTK_FLOAT:Numeric.Float32,
                   VTK_DOUBLE:Numeric.Float64 }

    __sizeDict = { VTK_CHAR:1,
                   VTK_UNSIGNED_CHAR:1,
                   VTK_SHORT:2,
                   VTK_UNSIGNED_SHORT:2,
                   VTK_INT:4,
                   VTK_FLOAT:4,
                   VTK_DOUBLE:8 }

    # convert unsigned shorts to ints, to avoid sign problems
    def SetConvertUnsignedShortToInt(self,yesno):
        self.__ConvertUnsignedShortToInt = yesno

    def GetConvertUnsignedShortToInt(self):
        return self.__ConvertUnsignedShortToInt
    
    def ConvertUnsignedShortToIntOn(self):
        self.__ConvertUnsignedShortToInt = 1

    def ConvertUnsignedShortToIntOff(self):
        self.__ConvertUnsignedShortToInt = 0

    # set the input
    def SetInput(self,input):
        return self.__export.SetInput(input)

    def GetInput(self):
        return self.__export.GetInput()

    def GetArray(self):
        input = self.__export.GetInput()
        type = input.GetScalarType()
        extent = input.GetWholeExtent()
        numComponents = input.GetNumberOfScalarComponents()
        dim = (extent[5]-extent[4]+1,
               extent[3]-extent[2]+1,
               extent[1]-extent[0]+1)
        if (numComponents > 1):
            dim = dim + (numComponents,)
        size = dim[0]*dim[1]*dim[2]*numComponents*self.__sizeDict[type]

        imString = Numeric.zeros((size,),Numeric.UnsignedInt8).tostring()
        self.__export.Export(imString)

        imArray = Numeric.fromstring(imString,self.__typeDict[type])
        imArray = Numeric.reshape(imArray,dim)

        # convert unsigned short to int to avoid sign issues
        if (type == VTK_UNSIGNED_SHORT and self.__ConvertUnsignedShortToInt):
            imArray = umath.bitwise_and(imArray.astype(Numeric.Int32),0xffff)

        # just to remind myself of the dangers of memory management
        del imString

        return imArray
        
    def GetDataExtent(self):
        return self.__export.GetInput().GetWholeExtent()
    
    def GetDataSpacing(self):
        return self.__export.GetInput().GetSpacing()
    
    def GetDataOrigin(self):
        return self.__export.GetInput().GetOrigin()
    

