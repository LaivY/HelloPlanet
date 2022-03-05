#pragma once
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <fbxsdk.h>
#include "utilities.h"
using namespace std;
using namespace DirectX;

#define GUN_SCALE_FACTOR 0.5

struct Vertex
{
	Vertex() : position{}, normal{}, uv{}, boneIndices{}, boneWeights{}, materialIndex{ -1 } { }

	XMFLOAT3	position;
	XMFLOAT3	normal;
	XMFLOAT2	uv;
	int			materialIndex;
	XMUINT4		boneIndices;
	XMFLOAT4	boneWeights;
};

struct Material
{
	Material() : name{}, baseColor{} { }

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
		transformMatrix.SetIdentity();
	}

	FbxLongLong frameNum;
	FbxAMatrix	transformMatrix;
};

struct Joint
{
	Joint() : parentIndex{ -1 }
	{
		globalBindposeInverseMatrix.SetIdentity();
	}

	string				name;
	int					parentIndex;
	FbxAMatrix			globalBindposeInverseMatrix;
	vector<Keyframe>	keyframes;
};

struct Mesh
{
	Mesh() : isLinked{ false }, parentNode{ nullptr }, node{ nullptr }
	{
		transformMatrix.SetIdentity();
	}

	string				name;				// 이름
	vector<Material>	materials;			// 재질
	vector<CtrlPoint>	ctrlPoints;			// 제어점
	vector<Vertex>		vertices;			// 정점
	bool				isLinked;			// 링크 여부
	FbxNode*			node;				// 자신 노드
	FbxNode*			parentNode;			// 부모 노드
	FbxAMatrix			transformMatrix;	// 메쉬 변환 행렬
};

class FBXExporter
{
public:
	FBXExporter();
	~FBXExporter();

	void Process(const string& inputFileName, bool doExportMesh = true, bool doExportAnimation = true);
	void LoadSkeleton(FbxNode* node, int index, int parentIndex);
	void LoadMesh(FbxNode* node);
	void LoadInfo(FbxNode* node, Mesh& mesh);
	void LoadMaterials(FbxNode* node, Mesh& mesh);
	void LoadCtrlPoints(FbxNode* node, Mesh& mesh);
	void LoadAnimation(FbxNode* node, Mesh& mesh);
	void LoadVertices(FbxNode* node, Mesh& mesh);
	void ProcessLink();

	int GetJointIndex(const string& name) const;
	XMFLOAT3 GetNormal(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex) const;
	XMFLOAT2 GetUV(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex) const;
	int GetMaterial(FbxMesh* mesh, int polygonIndex) const;

	void ExportMesh() const;
	void ExportAnimation() const;

private:
	string					m_inputFileName;	// 변환할 FBX 파일 이름
	FbxManager*				m_manager;			// FBX 매니저
	FbxScene*				m_scene;			// FBX 씬

	vector<Mesh>			m_meshes;			// 메쉬
	vector<Joint>			m_joints;			// 뼈
	string					m_animationName;	// 애니메이션 이름
	FbxLongLong				m_animationLength;	// 애니메이션 길이
};