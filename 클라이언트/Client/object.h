#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

class Camera;

enum class GameObjectType {
	DEFAULT, PLAYER, BULLET
};

class GameObject
{
public:
	GameObject();
	~GameObject() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader=nullptr) const;
	virtual void Update(FLOAT deltaTime);
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	void SetWorldMatrix(const XMFLOAT4X4& worldMatrix) { m_worldMatrix = worldMatrix; }
	void SetPosition(const XMFLOAT3& position);
	void SetMesh(const shared_ptr<Mesh>& Mesh);
	void SetShader(const shared_ptr<Shader>& shader);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetTextureInfo(unique_ptr<TextureInfo>& textureInfo);

	GameObjectType GetType() const { return m_type; }
	bool isDeleted() const { return m_isDeleted; }
	XMFLOAT4X4 GetWorldMatrix() const { return m_worldMatrix; }
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetLocalXAxis() const { return m_localXAxis; }
	XMFLOAT3 GetUp() const { return m_localYAxis; }
	XMFLOAT3 GetFront() const { return m_localZAxis; }
	XMFLOAT3 GetRollPitchYaw() const { return XMFLOAT3{ m_roll, m_pitch, m_yaw }; }

	XMFLOAT3 GetNormal() const;
	XMFLOAT3 GetLook() const;

protected:
	XMFLOAT4X4				m_worldMatrix;		// 월드 변환 행렬
	XMFLOAT3				m_localXAxis;		// 게임오브젝트에 y축 회전만 적용됬을 때의 로컬 x축
	XMFLOAT3				m_localYAxis;		// 로컬 y축
	XMFLOAT3				m_localZAxis;		// 로컬 z축
	FLOAT					m_roll;				// z축 회전각
	FLOAT					m_pitch;			// x축 회전각
	FLOAT					m_yaw;				// y축 회전각

	shared_ptr<Mesh>		m_mesh;				// 메쉬
	shared_ptr<Shader>		m_shader;			// 셰이더
	shared_ptr<Texture>		m_texture;			// 텍스쳐
	unique_ptr<TextureInfo>	m_textureInfo;		// 텍스쳐 애니메이션 정보 구조체

	GameObjectType			m_type;				// 게임오브젝트 타입
	bool					m_isDeleted;		// 삭제 여부
};

class Particle : public GameObject
{
public:
	Particle() = default;
	~Particle() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr) const;
};