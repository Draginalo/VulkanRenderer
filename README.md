# VulkanRenderer

Building a custom Vulkan renderer!

<img width="195" height="125" alt="vulkan" src="https://github.com/user-attachments/assets/cfc144cc-8d7c-410d-babb-48bbd387990e" />

Supports dynamic rendering, MSAA, graphics + compute pipelines, GPU CPU synchronization, pipeline changes at runtime, CMake build systems, and more.

### Archatecture

The general idea is to make a streamlined way to create and maintain graphics/compute pipelines used for rendering models, or other types of data, in a very fast and optomized manner.

A lot of the archatecture revolves around descriptor sets, where the order of rendering is based on the the frequency of descriptor sets needing to be updated. A "Render Graph" structure is used to order the rendering of game objects (or execution of compute shaders), where all the active pipelines that will be used to render game objects that are active in the current scene will be looped through with their pipeline specific descriptor sets being bound. Following this, for each pipeline, all the active materials that are derived from that pipeline's descriptor layout which are being used by active game objects in the scene will be looped through and their material specific descriptor sets will be bound. Finally, all the game objects that use that pipeline-material configuration are rendered, minimizing the amount of descriptor set re-bindings. Additionally, the descriptor sets are all allocated at once and pooled together when constructing the pipelines to be used.
