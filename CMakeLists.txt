cmake_minimum_required (VERSION 2.6)
cmake_policy(SET CMP0015 NEW)

project (dwrap)

set(SOURCE_DIR "./Source")
set(EXTERNAL_DIR "./External")

#add_library(fltk STATIC IMPORTED)
#set_property(TARGET fltk PROPERTY IMPORTED_LOCATION ${EXTERNAL_DIR}/fltk-1.3.4/lib/fltk.lib)

file(GLOB SOURCE_FILES
    "${SOURCE_DIR}/*.h"
    "${SOURCE_DIR}/*.cpp"
)

include_directories(${EXTERNAL_DIR}/fltk-1.3.4)
link_directories(${EXTERNAL_DIR}/fltk-1.3.4/lib)

# dirent.h does not exist on win32
if (${WIN32})
include_directories(${EXTERNAL_DIR}/dirent-1.21/include)
endif()

# add the executable
add_executable(dwrap ${SOURCE_FILES})
#add_dependencies(dwrap fltk)

target_link_libraries(dwrap fltk)
