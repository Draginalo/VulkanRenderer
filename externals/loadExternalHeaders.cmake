#STB_Image
set(EXTERNAL_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/externals/headers")

add_library(external_headers INTERFACE)
target_include_directories(external_headers SYSTEM INTERFACE ${EXTERNAL_HEADERS_DIR})