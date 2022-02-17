﻿#pragma once
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
	XMUINT4		boneIndices;
	XMFLOAT4	boneWeights;
	int			materialIndex;
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
		aniTransMatrix.SetIdentity();
	}

	FbxLongLong frameNum;
	FbxAMatrix	aniTransMatrix;
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
	Mesh() : transformMatrix{ Utilities::Identity() }, isLinked{ false }, parentNode{ nullptr }
	{

	}

	string				name;				// 이름
	vector<Material>	materials;			// 재질
	vector<CtrlPoint>	ctrlPoints;			// 제어점
	vector<Vertex>		vertices;			// 정점
	XMFLOAT4X4			transformMatrix;	// 메쉬 변환 행렬

	bool				isLinked;			// 링크 여부
	FbxNode*			parentNode;			// 부모 노드
};

class FBXExporter
{
public:
	FBXExporter();
	~FBXExporter();

	void Process(const string& inputFileName);
	void LoadSkeleton(FbxNode* node, int index, int parentIndex);
	void LoadMesh(FbxNode* node);
	void LoadMaterials(FbxNode* node, int meshIndex);
	void LoadCtrlPoints(FbxNode* node, int meshIndex);
	void LoadAnimation(FbxNode* node, int meshIndex);
	void LoadVertices(FbxNode* node, int meshIndex);

	int GetJointIndexByName(const string& name);
	XMFLOAT3 GetNormal(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex);
	XMFLOAT2 GetUV(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex);
	XMFLOAT4 GetColor(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex);
	int GetMaterial(FbxMesh* mesh, int polygonIndex);

	void ProcessLink();
	void ExportMesh();
	void ExportAnimation();

private:
	FbxManager*				m_manager;			// FBX 매니저
	FbxScene*				m_scene;			// FBX 씬

	string					m_inputFileName;	// 변환할 FBX 파일 이름

	vector<Mesh>			m_meshes;			// 메쉬
	vector<Joint>			m_joints;			// 뼈
	string					m_animationName;	// 애니메이션 이름
	FbxLongLong				m_animationLength;	// 애니메이션 길이
};