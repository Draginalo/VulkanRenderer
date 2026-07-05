#include "GameObject.h"

GameObject::GameObject() {}

GameObject::GameObject(const Material* pMaterial, const Mesh3D* pMeshToRender) : mpMaterial(pMaterial), mpMeshToRender(pMeshToRender) {}

const Material* GameObject::getMaterial() const { return mpMaterial; }
const Mesh3D* GameObject::getMesh() const { return mpMeshToRender; }