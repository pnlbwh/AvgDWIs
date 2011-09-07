project( AvgDWIs )

cmake_minimum_required(VERSION 2.6)

find_package(Slicer3 REQUIRED)
include(${Slicer3_USE_FILE})

slicer3_set_default_install_prefix_for_external_projects()

link_directories(${ModuleDescriptionParser_BINARY_DIR})
include_directories( ${SlicerBaseCLI_BINARY_DIR} ${SlicerBaseCLI_SOURCE_DIR} )

set (CLP AvgDWIs )
set (${CLP}_SOURCE ${CLP}.cxx)
generateclp(${CLP}_SOURCE ${CLP}.xml)
add_executable(${CLP} ${${CLP}_SOURCE})
target_link_libraries (${CLP}
  ITKIO
  ITKCommon
  ITKStatistics
  )
