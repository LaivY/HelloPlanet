#pragma once
#include <algorithm>
#include <DirectXMath.h>
#include <fbxsdk.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "utilities.h"
using namespace DirectX;
using namespace std;

struct Mesh
{
	string				name;		// 이름
	bool				isLinked;	// 링크 여부
	string				parentName;	// 부모 이름
	vector<CtrlPoint>	ctrlPoints;	// 제어점
	vector<Vertex>		vertices;	// 정점
};

class FBXExporter
{
public:
	FBXExporter();
	~FBXExporter();

	void Process(const string& inputFileName, const string& outputFileName);
	void LoadMaterials();
	void LoadSkeleton(FbxNode* node, int index, int parentIndex);
	void LoadMesh(FbxNode* node);
	void LoadCtrlPoints(FbxNode* node, int meshIndex);
	void LoadAnimation(FbxNode* node, int meshIndex);
	void LoadVertices(FbxNode* node, int meshIndex);

	int GetJointIndexByName(const string& name);
	XMFLOAT3 GetNormal(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex);
	XMFLOAT2 GetUV(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex);
	XMFLOAT4 GetColor(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex);
	int GetMaterial(FbxMesh* mesh, int polygonIndex);

	void ExportMesh();
	void ExportAnimation();

private:
	FbxManager*				m_manager;			// FBX 매니저
	FbxScene*				m_scene;			// FBX 씬

	string					m_inputFileName;	// 변환할 FBX 파일 이름
	string					m_outputFileName;	// 결과 파일 이름

	vector<Material>		m_materials;		// 재질
	vector<Joint>			m_joints;			// 뼈
	vector<Mesh>			m_meshes;			// 메쉬

	//vector<CtrlPoint>		ctrlPoints;			// 제어점
	//vector<Vertex>			vertices;			// 정점

	string					m_animationName;	// 애니메이션 이름
	FbxLongLong				m_animationLength;	// 애니메이션 길이
};