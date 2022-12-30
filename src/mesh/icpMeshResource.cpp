#include "icpMeshResource.h"

INCEPTION_BEGIN_NAMESPACE

void icpMeshResource::prepareRenderResourceForMesh()
{
	m_meshData.createTextureImages();
	m_meshData.createTextureSampler();
	
	m_meshData.createVertexBuffers();
	m_meshData.createIndexBuffers();
	m_meshData.createUniformBuffers();

	m_meshData.allocateDescriptorSets();
}



INCEPTION_END_NAMESPACE