#pragma once

class Monster abstract
{
public:
	Monster();
	virtual ~Monster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);
	virtual void OnAttack(int clientId);

	void SetId(CHAR id);
	void SetType(eMobType type);
	void SetAnimationType(eMobAnimationType type);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetYaw(FLOAT yaw);
	void SetHp(INT hp);
	void SetTargetId(UCHAR id);
	void SetRandomPosition();

	MonsterData GetData() const;
	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::BoundingOrientedBox GetBoundingBox() const;
	INT GetHp() const;
	CHAR GetId() const;
	UCHAR GetTargetId() const;
	eMobAnimationType GetAnimationType() const;
	FLOAT GetAtkTimer() const;
	
protected:
	// struct MonsterData 멤버 변수들
	CHAR							m_id;
	eMobType						m_mobType;
	eMobAnimationType				m_aniType;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT							m_yaw;

	// 몬스터 스탯 변수들
	INT								m_hp;			// 체력
	INT								m_damage;		// 데미지
	FLOAT							m_speed;		// 이동속도

	// 서버 계산에 필요한 변수들
	DirectX::XMFLOAT4X4				m_worldMatrix;	// 월드변환행렬
	DirectX::BoundingOrientedBox	m_boundingBox;	// 바운딩박스
	FLOAT							m_hitTimer;		// 피격당한 시점부터 시작되는 타이머
	FLOAT							m_atkTimer;		// 공격한 시점부터 시작되는 타이머
	UCHAR							m_target;		// 공격 대상 플레이어 id
	bool							m_wasAttack;	// 공격 애니메이션 때 공격했는지
};

class GarooMonster : public Monster
{
public:
	GarooMonster();
	~GarooMonster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);
	virtual void OnAttack(int clientId);
};