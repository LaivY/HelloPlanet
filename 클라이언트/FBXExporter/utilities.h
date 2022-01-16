#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <fbxsdk.h>
using namespace DirectX;
using namespace std;

// 참고한 깃허브 주소
// https://github.com/lang1991/FBXExporter
// https://github.com/YujinJung/FBX-Loader

// 애니메이션 강의 유튜브
// https://www.youtube.com/watch?v=RYW-ShkhY6Q

struct Vertex
{
	Vertex() : position{}, normal{}, uv{}, boneIndices{}, boneWeights{}, materialIndex{ -1 } { }

	XMFLOAT3	position;
	XMFLOAT3	normal;
	XMFLOAT2	uv;
	XMUINT4		boneIndices;
	XMFLOAT4	boneWeights;
	int			materialIndex;
};

struct Material
{
	string		name;
	XMFLOAT4	baseColor;
};

struct CtrlPoint
{
	CtrlPoint() : position{} { }

	XMFLOAT3					position;
	vector<pair<int, double>>	weights;
};

struct Keyframe
{
	Keyframe() : frameNum{}
	{
		aniTransMatrix.SetIdentity(); 
	}

	FbxLongLong frameNum;
	FbxAMatrix	aniTransMatrix;
};

struct Joint
{
	Joint() : name{}, parentIndex{ -1 }
	{
		globalBindposeInverseMatrix.SetIdentity();
	}

	string				name;
	int					parentIndex;
	FbxAMatrix			globalBindposeInverseMatrix;
	vector<Keyframe>	keyframes;
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
	}

	inline void PrintFbxAMatrix(const FbxAMatrix& m)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
				cout << m.Get(i, j) << " ";
			cout << endl;
		}
	}
}