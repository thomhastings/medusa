cmake_minimum_required(VERSION 2.8)

find_package(Boost 1.46 COMPONENTS Python REQUIRED)
find_package(PythonLibs REQUIRED)

set(SRC
  pydusa.cpp

  py_address.cpp
  py_address.hpp

  py_binary_stream.cpp
  py_binary_stream.hpp

  py_cell.cpp
  py_cell.hpp

  py_configuration.cpp
  py_configuration.hpp

  py_memory_area.cpp
  py_memory_area.hpp

  py_database.cpp
  py_database.hpp

  py_exception.cpp
  py_exception.hpp

  py_architecture.cpp
  py_architecture.hpp

  py_loader.cpp
  py_loader.hpp

  py_logger.cpp
  py_logger.hpp

  py_operand.cpp
  py_operand.hpp

  py_instruction.cpp
  py_instruction.hpp

  py_document.cpp
  py_document.hpp

  py_medusa.cpp
  py_medusa.hpp

  py_xrefs.cpp
  py_xrefs.hpp
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
if(WIN32)
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".pyd")
endif()

include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
add_library(pydusa SHARED ${SRC})
target_link_libraries(pydusa ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} Medusa)
