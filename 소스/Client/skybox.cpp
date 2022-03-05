#include "skybox.h"

Skybox::Skybox(const array<shared_ptr<Mesh>, 6>& meshes, const shared_ptr<Shader>& shader, const array<shared_ptr<Texture>, 6>& textures) : m_faces{ new GameObject[6] }
{
	// 앞, 좌, 우, 뒤, 상, 하
	for (int i = 0; i < 6; ++i)
	{
		m_faces[i].SetMesh(meshes[i]);
		m_faces[i].SetShader(shader);
		m_faces[i].SetTexture(textures[i]);
	}
}

void Skybox::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	for (int i = 0; i < 6; ++i)
		m_faces[i].Render(commandList);
}

void Skybox::Update()
{
	if (m_camera) SetPosition(m_camera->GetEye());
}

void Skybox::SetCamera(const shared_ptr<Camera>& camera)
{
	if (m_camera) m_camera.reset();
	m_camera = camera;
}

void Skybox::SetPosition(XMFLOAT3 position)
{
	for (int i = 0; i < 6; ++i)
		m_faces[i].SetPosition(position);
}