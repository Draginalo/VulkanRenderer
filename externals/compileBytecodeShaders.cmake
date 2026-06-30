get_filename_component(ROOT_DIR ../ ABSOLUTE)

MESSAGE(STATUS "Root Directory For Shaders: ${ROOT_DIR}")

file(
 GLOB_RECURSE SHADERS_SRC CONFIGURE_DEPENDS
 ${ROOT_DIR}/Assets/*.vert
 ${ROOT_DIR}/Assets/*.frag
 ${ROOT_DIR}/Assets/*.comp
)

find_package(Vulkan REQUIRED COMPONENTS glslc)

set(SHADER_BINARY_DIR "${ROOT_DIR}/Assets/Shaders/ByteEncoded")
file(MAKE_DIRECTORY "${SHADER_BINARY_DIR}")

set(SPV_SHADERS "")

foreach(shader_src IN LISTS SHADERS_SRC)
	get_filename_component(FILENAME_WITH_EXT ${shader_src} NAME)
	get_filename_component(FILENAME ${shader_src} NAME_WE)

	set(SHADER_OUT ${SHADER_BINARY_DIR}/${FILENAME}.spv)
	add_custom_command(
		OUTPUT ${SHADER_OUT}
		COMMAND ${Vulkan_GLSLC_EXECUTABLE} ${shader_src} -o ${SHADER_OUT}
		DEPENDS ${shader_src} ${SHADER_BINARY_DIR}
		COMMENT "Compiling ${FILENAME_WITH_EXT}"
		VERBATIM
	)

	list(APPEND SPV_SHADERS ${SHADER_OUT})
endforeach()

add_custom_target(COMPILED_SHADERS ALL DEPENDS ${SPV_SHADERS})