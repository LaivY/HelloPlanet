#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <fbxsdk.h>
using namespace DirectX;
using namespace std;

// 참고한 깃허브 주소
// https://github.com/lang1991/FBXExporter

// 블렌딩 정보는 조인트 인덱스와 가중치를 갖는다.
struct BlendingDatum
{
	BlendingDatum() : blendingIndex{}, blendingWeight{}
	{

	}

	int		blendingIndex;
	double	blendingWeight;
};

// 제어점은 좌표와 4개의 가중치 정보를 갖는다.
struct CtrlPoint
{
	CtrlPoint() : position{}
	{
		blendingData.reserve(4);
	}

	XMFLOAT3				position;
	vector<BlendingDatum>	blendingData;
};

// 키프레임은 몇 번째 프레임인지와 해당 프레임일 때 제어점의 모델 좌표계 변환 행렬을 갖는다.
struct Keyframe
{
	Keyframe() : frameNum{}
	{
		globalTransformMatrix.SetIdentity();
	}

	FbxLongLong frameNum;
	FbxAMatrix	globalTransformMatrix;
};

// 조인트는 이름, 부모 조인트의 인덱스, 로컬 -> 모델 좌표계 변환 행렬, 노드, 키프레임을 갖는다.
struct Joint
{
	Joint() : name{}, parentIndex{ -1 }, node{ nullptr }
	{
		globalBindposeInverseMatrix.SetIdentity();
	}

	string				name;
	int					parentIndex;
	FbxAMatrix			globalBindposeInverseMatrix;
	FbxNode*			node;
	vector<Keyframe>	keyframes;
};

// 정점은 좌표, 노말, 텍스쳐 좌표, 블렌딩 정보를 갖는다.
struct Vertex
{
	Vertex() : position{}, normal{}, uv{}
	{

	}

	XMFLOAT3				position;
	XMFLOAT3				normal;
	XMFLOAT2				uv;
	vector<BlendingDatum>	blendingData;
};

namespace Utilities
{
	inline FbxAMatrix GetGeometryTransformation(FbxNode* node)
	{
		return FbxAMatrix(
			node->GetGeometricTranslation(FbxNode::eSourcePivot),
			node->GetGeometricRotation(FbxNode::eSourcePivot),
			node->GetGeometricScaling(FbxNode::eSourcePivot)
		);
	}

	inline XMFLOAT4 FbxVector4ToXMFLOAT4(const FbxVector4& v)
	{
		return XMFLOAT4{ 
			static_cast<float>(v.mData[0]),
			static_cast<float>(v.mData[1]),
			static_cast<float>(v.mData[2]),
			static_cast<float>(v.mData[3])
		};
	}

	inline void WriteFbxAMatrixToStream(ostream& s, const FbxAMatrix& m)
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				s << static_cast<float>(m.Get(i, j)) << " ";
		s << endl;
	}
}