#STB_Image
set(STB_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/externals/headers")

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${STB_IMAGE_DIR})