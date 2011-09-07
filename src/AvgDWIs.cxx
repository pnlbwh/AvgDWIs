#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif

#include <sstream>
#include <iostream>
#include <algorithm>
#include <string>
#include <itkMetaDataObject.h>
#include <itkImage.h>
#include <itkVector.h>
#include <itkVectorImage.h>

#include "itkPluginUtilities.h"
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkCastImageFilter.h>
#include "itkAddImageFilter.h"

#include "itkMultiplyByConstantImageFilter.h"
#include "itkDivideImageFilter.h"

#include "AvgDWIsCLP.h"

namespace 
{

template<typename DoubleDiffusionImageType, typename DiffusionImageType>
void MakeZeroVolume(typename DoubleDiffusionImageType::Pointer &zero_image, typename DiffusionImageType::Pointer image)
{
  typename DoubleDiffusionImageType::RegionType region;
  typename DoubleDiffusionImageType::IndexType start;
  region.SetSize( image->GetLargestPossibleRegion().GetSize() );
  start.Fill(0);
  region.SetIndex( start );
  zero_image->SetDirection( image->GetDirection() );
  zero_image->SetOrigin( image->GetOrigin() );
  zero_image->SetSpacing( image->GetSpacing());
  zero_image->SetVectorLength( image->GetVectorLength());
  zero_image->SetRegions( region );
  zero_image->Allocate();
  zero_image->FillBuffer( 0.0 );

  //SetSpacing(zero_image);
}

template<typename DiffusionImageType>
void GetImage(std::string filename, typename DiffusionImageType::Pointer& image)
{
  typedef itk::ImageFileReader< DiffusionImageType >  DiffusionImageReaderType;
  typename DiffusionImageReaderType::Pointer imageReader = DiffusionImageReaderType::New();
  imageReader->SetFileName(filename);
  try
  {
    imageReader->Update();
  }
  catch( itk::ExceptionObject& err )
  {
    std::cout << "Could not load volume from disk" << std::endl;
    std::cout << err << std::endl;
    exit( EXIT_FAILURE );
  }
  image = imageReader->GetOutput();
  image->DisconnectPipeline();
}

template<typename WriterType>
void UpdateWriter(typename WriterType::Pointer &writer, std::string error_msg)
{
  writer->SetUseCompression( true );
  try
  {
    writer->Update();
  }
  catch( itk::ExceptionObject& err )
  {
    std::cout << error_msg << std::endl;
    std::cout << err << std::endl;
    exit( EXIT_FAILURE );
  }
}

template<class PixelType>
int ComputeMean( std::vector<std::string> filenames, std::string filename, PixelType)
{
  typedef itk::VectorImage<PixelType, 3>  DiffusionImageType;
  typedef itk::VectorImage<double, 3>  DoubleDiffusionImageType;
  typedef itk::ImageFileWriter< DoubleDiffusionImageType >  DoubleDiffusionWriterType;
  typedef itk::ImageFileWriter< DiffusionImageType >  DiffusionWriterType;
  typedef itk::AddImageFilter<DoubleDiffusionImageType, DoubleDiffusionImageType, DoubleDiffusionImageType>  AdderType;
  typedef itk::MultiplyByConstantImageFilter< DoubleDiffusionImageType, double, DoubleDiffusionImageType >   MultiplyByConstantType;
  typedef itk::CastImageFilter<DiffusionImageType, DoubleDiffusionImageType> CastImageFilterType;
  typedef itk::CastImageFilter<DoubleDiffusionImageType, DiffusionImageType> PixelTypeCastImageFilterType;

  typename CastImageFilterType::Pointer castImageFilter = CastImageFilterType::New();

  typename AdderType::Pointer adder = AdderType::New();
  typename DoubleDiffusionImageType::Pointer avgImg =  DoubleDiffusionImageType::New();
  typename DiffusionImageType::Pointer          image;

  itk::MetaDataDictionary input_dico; 

  std::cout << "Computing mean:" << std::endl;
  for (unsigned int i=0; i < filenames.size(); i++)
  {
    std::cout << filenames[i] << std::endl;

    GetImage<DiffusionImageType>(filenames[i], image);
    //std::cout << image->GetOrigin() << std::endl;

    if (i==0)
    {
      MakeZeroVolume<DoubleDiffusionImageType, DiffusionImageType>(avgImg, image);
      input_dico = image->GetMetaDataDictionary();
    }
    adder->SetInput1( avgImg );
    castImageFilter->SetInput( image );
    adder->SetInput2( castImageFilter->GetOutput() );
    //adder->SetInput2( image );
    adder->SetInput2( castImageFilter->GetOutput() );
    adder->Update();
    avgImg = adder->GetOutput();
  }
  //avgImg->SetMetaDataDictionary(input_dico);

  typename PixelTypeCastImageFilterType::Pointer pixelTypeCastImageFilter = PixelTypeCastImageFilterType::New();

  typename MultiplyByConstantType::Pointer  multiplier = MultiplyByConstantType::New();
  multiplier->SetConstant( 1.0/filenames.size() );
  multiplier->SetInput( avgImg );
  typename DiffusionWriterType::Pointer writer =  DiffusionWriterType::New();
  writer->SetFileName( filename );
  writer->SetUseCompression( true );
  pixelTypeCastImageFilter->SetInput(multiplier->GetOutput());
  pixelTypeCastImageFilter->GetOutput()->SetMetaDataDictionary(input_dico);
  writer->SetInput( pixelTypeCastImageFilter->GetOutput() );
  //writer->SetInput( multiplier->GetOutput() );
  //UpdateWriter<DoubleDiffusionWriterType>(writer, "Failed to compute average");
  UpdateWriter<DiffusionWriterType>(writer, "Failed to compute average");

  return EXIT_SUCCESS;
}

} //end namespace

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  for (unsigned int i = 0; i < volumeFileNames.size(); ++i)
  {
  if (!itksys::SystemTools::FileExists(volumeFileNames[i].c_str()))
    {
    std::cerr << "Error: volume file " << i << " does not exist." << std::endl;
    std::cerr << volumeFileNames[i] << std::endl;
    return EXIT_FAILURE;
    }
  }

  itk::ImageIOBase::IOPixelType pixelType;                                                                                                                                                                                               
  itk::ImageIOBase::IOComponentType componentType;
  
  //try
  // {
  itk::GetImageType (volumeFileNames[0], pixelType, componentType);

 switch (componentType)
 {
   #ifndef WIN32
   case itk::ImageIOBase::UCHAR:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<unsigned char>(0));
     break;
   case itk::ImageIOBase::CHAR:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<char>(0));
     break;
   #endif
   case itk::ImageIOBase::USHORT:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<unsigned short>(0));
     break;
   case itk::ImageIOBase::SHORT:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<short>(0));
     break;
   case itk::ImageIOBase::UINT:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<unsigned int>(0));
     break;
   case itk::ImageIOBase::INT:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<int>(0));
     break;
   #ifndef WIN32
   case itk::ImageIOBase::ULONG:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<unsigned long>(0));
     break;
   case itk::ImageIOBase::LONG:
     return ComputeMean( volumeFileNames, outputVolume.c_str(), static_cast<long>(0));
     break;
   #endif
   case itk::ImageIOBase::FLOAT:
     std::cout << "FLOAT type not currently supported." << std::endl;
     break;
   case itk::ImageIOBase::DOUBLE:
     std::cout << "DOUBLE type not currently supported." << std::endl;
     break;
   case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
   default:
     std::cout << "unknown component type" << std::endl;
     break;
 }

  return EXIT_SUCCESS;
}
