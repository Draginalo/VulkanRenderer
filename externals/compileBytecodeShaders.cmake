MESSAGE(STATUS "Root Directory For Shaders: ${PROJECT_SOURCE_DIR}")

file(
 GLOB_RECURSE SHADERS_SRC CONFIGURE_DEPENDS
 ${PROJECT_SOURCE_DIR}/Assets/*.vert
 ${PROJECT_SOURCE_DIR}/Assets/*.frag
 ${PROJECT_SOURCE_DIR}/Assets/*.comp
)

find_package(Vulkan REQUIRED COMPONENTS glslc)

set(SHADER_BINARY_DIR "${PROJECT_SOURCE_DIR}/Assets/Shaders/ByteEncoded")
file(MAKE_DIRECTORY "${SHADER_BINARY_DIR}")

set(SPV_SHADERS "")

foreach(shader_src IN LISTS SHADERS_SRC)
	get_filename_component(FILENAME_WITH_EXT ${shader_src} NAME)
	get_filename_component(FILENAME ${shader_src} NAME_WE)

	set(SHADER_OUT ${SHADER_BINARY_DIR}/${FILENAME}.spv)
	add_custom_command(
		OUTPUT ${SHADER_OUT}
		COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${shader_src} -o ${SHADER_OUT}
		COMMENT "Compiling ${FILENAME_WITH_EXT}"
		VERBATIM
	)

	list(APPEND SPV_SHADERS ${SHADER_OUT})
endforeach()

add_custom_target(COMPILED_SHADERS ALL DEPENDS ${SPV_SHADERS})