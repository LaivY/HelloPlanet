#include "FBXExporter.h"

FBXExporter::FBXExporter() : m_scene{ nullptr }, m_animationLength{}
{
	// Fbx 매니저 생성
	m_manager = FbxManager::Create();

	// 씬 생성
	m_scene = FbxScene::Create(m_manager, "SCENE");

	// IOSettings 생성 및 설정
	FbxIOSettings* ioSettings{ FbxIOSettings::Create(m_manager, IOSROOT) };
	m_manager->SetIOSettings(ioSettings);
}

FBXExporter::~FBXExporter()
{
	if (m_manager) m_manager->Destroy();
}

void FBXExporter::Process(const string& inputFileName, const string& outputFileName)
{
	// 입력, 출력 파일 이름 저장
	m_inputFileName = inputFileName;
	m_outputFileName = outputFileName;

	// 임포터 생성 및 초기화
	FbxImporter* fbxImporter{ FbxImporter::Create(m_manager, "IMPORTER") };
	if (!fbxImporter->Initialize(inputFileName.c_str(), -1, m_manager->GetIOSettings()))
	{
		cout << "FbxImporter 초기화 실패" << endl;
		cout << fbxImporter->GetStatus().GetErrorString() << endl;
		exit(-1);
	}

	// 씬 로드 후 임포터 삭제
	fbxImporter->Import(m_scene);
	fbxImporter->Destroy();

	// 가능한 모든 노드 삼각형화
	FbxGeometryConverter geometryConverter{ m_manager };
	geometryConverter.Triangulate(m_scene, true);

	// 루트 노드 선언
	FbxNode* rootNode{ m_scene->GetRootNode() };

	// 데이터 로딩
	for (int i = 0; i < rootNode->GetChildCount(); ++i)
		LoadSkeleton(rootNode->GetChild(i), 0, -1);
	for (int i = 0; i < rootNode->GetChildCount(); ++i)
		LoadMesh(rootNode->GetChild(i));

	// 출력
	ExportMesh();
	ExportAnimation();
}

void FBXExporter::LoadSkeleton(FbxNode* node, int index, int parentIndex)
{
	/*
	Joint는 뼈를 의미한다.
	모든 뼈들을 순회하면서 이름과 부모-자식 관계를 불러온다.
	*/

	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		Joint joint;
		joint.name = node->GetName();
		joint.parentIndex = parentIndex;
		m_joints.push_back(joint);
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
	{
		LoadSkeleton(node->GetChild(i), m_joints.size(), index);
	}
}

void FBXExporter::LoadMesh(FbxNode* node)
{
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		LoadCtrlPoints(node);
		LoadAnimation(node);
		LoadVertices(node);
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
		LoadMesh(node->GetChild(i));
}

void FBXExporter::LoadCtrlPoints(FbxNode* node)
{
	FbxMesh* mesh{ node->GetMesh() };
	for (int i = 0; i < mesh->GetControlPointsCount(); ++i)
	{
		CtrlPoint ctrlPoint;
		ctrlPoint.position = XMFLOAT3{ 
			static_cast<float>(mesh->GetControlPointAt(i)[0]), 
			static_cast<float>(mesh->GetControlPointAt(i)[1]),
			static_cast<float>(mesh->GetControlPointAt(i)[2])
		};
		m_ctrlPoints.push_back(ctrlPoint);
	}
}

void FBXExporter::LoadAnimation(FbxNode* node)
{
	FbxMesh* mesh{ node->GetMesh() };
	FbxAMatrix geometryTransform{ Utilities::GetGeometryTransformation(node) }; // 거의 항등행렬

	for (int di = 0; di < mesh->GetDeformerCount(FbxDeformer::eSkin); ++di)
	{
		FbxSkin* skin{ reinterpret_cast<FbxSkin*>(mesh->GetDeformer(di, FbxDeformer::eSkin)) };
		if (!skin) continue;

		for (int ci = 0; ci < skin->GetClusterCount(); ++ci)
		{
			FbxCluster* cluster{ skin->GetCluster(ci) };
			FbxNode* bone{ cluster->GetLink() };
			int ji{ GetJointIndexByName(bone->GetName()) };

			// 제어점에 가중치 정보 추가
			for (int cpi = 0; cpi < cluster->GetControlPointIndicesCount(); ++cpi)
			{
				int index{ cluster->GetControlPointIndices()[cpi] };
				double weight{ cluster->GetControlPointWeights()[cpi] };
				m_ctrlPoints[index].weights.push_back(make_pair(ji, weight));
			}

			// 메쉬 변환 행렬
			FbxAMatrix transformMatrix; cluster->GetTransformMatrix(transformMatrix);

			// 뼈 좌표계 -> 루트 좌표계 변환 행렬
			FbxAMatrix transformLinkMatrix; cluster->GetTransformLinkMatrix(transformLinkMatrix);

			// 루트 좌표계 -> 뼈 좌표계 변환 행렬
			m_joints[ji].globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

			// 애니메이션 정보
			FbxAnimStack* animStack{ m_scene->GetSrcObject<FbxAnimStack>(0) };
			FbxString animStackName{ animStack->GetName() };
			m_animationName = animStackName.Buffer();

			// 애니메이션 프레임 정보
			FbxTakeInfo* takeInfo{ m_scene->GetTakeInfo(animStackName) };
			FbxTime start{ takeInfo->mLocalTimeSpan.GetStart() };
			FbxTime end{ takeInfo->mLocalTimeSpan.GetStop() };
			m_animationLength = end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1;

			// 애니메이션은 T-pose일 때를 기준으로 설정되어있다.
			// 따라서 애니메이션 변환 행렬을 계산할 때 루트 좌표계 기준으로 계산해야한다.
			for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
			{
				FbxTime curr;
				curr.SetFrame(i, FbxTime::eFrames24);

				// curr일 때의 T-pose -> 루트 좌표계 변환 행렬
				FbxAMatrix toRoot{ (node->EvaluateGlobalTransform(curr) * geometryTransform).Inverse() };

				Keyframe keyframe;
				keyframe.frameNum = i;
				keyframe.aniTransMatrix = toRoot * bone->EvaluateGlobalTransform(curr);
				m_joints[ji].keyframes.push_back(keyframe);
			}
		}
	}

	// 제어점 중 가충치를 4개 이하로 갖고있는 제어점들은 4개로 채워줌
	for (CtrlPoint& ctrlPoint : m_ctrlPoints)
		for (int i = ctrlPoint.weights.size(); i < 4; ++i)
			ctrlPoint.weights.push_back(make_pair(0, 0.0));

	// 제어점이 갖고있는 가중치 정보를 가중치가 높은 순서로 정렬
	for (CtrlPoint& ctrlPoint : m_ctrlPoints)
	{
		partial_sort(ctrlPoint.weights.begin(), ctrlPoint.weights.begin() + 4, ctrlPoint.weights.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			});
	}
}

void FBXExporter::LoadVertices(FbxNode* node)
{
	FbxMesh* mesh{ node->GetMesh() };

	int vertexCountIndex{ 0 };
	for (int i = 0; i < mesh->GetPolygonCount(); ++i)
	{
		for (int j = 0; j < 3; ++j) // 삼각형이기 때문에 3번 반복
		{
			// i번째 삼각형의 j번째 제어점
			int ctrlPointIndex{ mesh->GetPolygonVertex(i, j) };
			const CtrlPoint& ctrlPoint{ m_ctrlPoints[ctrlPointIndex] };

			Vertex vertex;
			vertex.position = ctrlPoint.position;
			vertex.normal = GetNormal(mesh, ctrlPointIndex, vertexCountIndex);
			vertex.uv = GetUV(mesh, ctrlPointIndex, vertexCountIndex);
			vertex.boneIndices = XMUINT4( ctrlPoint.weights[0].first, ctrlPoint.weights[1].first, ctrlPoint.weights[2].first, ctrlPoint.weights[3].first );
			vertex.boneWeights = XMFLOAT4(
				ctrlPoint.weights[0].second,
				ctrlPoint.weights[1].second,
				ctrlPoint.weights[2].second,
				ctrlPoint.weights[3].second
			);
			m_vertices.push_back(vertex);

			// 정점 개수 증가
			++vertexCountIndex;
		}
	}
}

int FBXExporter::GetJointIndexByName(const string & name)
{
	auto i = find_if(m_joints.begin(), m_joints.end(), [&name](const Joint& joint) {
		return joint.name == name;
		});
	if (i == m_joints.end())
		return -1;
	return distance(m_joints.begin(), i);
}

XMFLOAT3 FBXExporter::GetNormal(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex)
{
	FbxVector4 result{};
	if (mesh->GetElementNormalCount() < 1)
		return XMFLOAT3{};

	FbxGeometryElementNormal* normal{ mesh->GetElementNormal(0) };
	if (normal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			result = normal->GetDirectArray().GetAt(controlPointIndex);
		}
		else if (normal->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			int index{ normal->GetIndexArray().GetAt(controlPointIndex) };
			result = normal->GetDirectArray().GetAt(index);
		}
	}
	else if (normal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (normal->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			result = normal->GetDirectArray().GetAt(vertexCountIndex);
		}
		else if (normal->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			int index{ normal->GetIndexArray().GetAt(vertexCountIndex) };
			result = normal->GetDirectArray().GetAt(index);
		}
	}
	return XMFLOAT3(result[0], result[1], result[2]);
}

XMFLOAT2 FBXExporter::GetUV(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex)
{
	FbxVector2 result{};
	if (mesh->GetElementUVCount() < 1)
		return XMFLOAT2{};

	FbxGeometryElementUV* uv{ mesh->GetElementUV(0) };
	if (uv->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (uv->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			result = uv->GetDirectArray().GetAt(controlPointIndex);
		}
		else if (uv->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			int index{ uv->GetIndexArray().GetAt(controlPointIndex) };
			result = uv->GetDirectArray().GetAt(index);
		}
	}
	else if (uv->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (uv->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			result = uv->GetDirectArray().GetAt(vertexCountIndex);
		}
		else if (uv->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			int index{ uv->GetIndexArray().GetAt(vertexCountIndex) };
			result = uv->GetDirectArray().GetAt(index);
		}
	}
	return XMFLOAT2(result[0], result[1]);
}

void FBXExporter::ExportMesh()
{
	// 아직 개발 단계이기 때문에 메모장으로 열어 확인할 수 있도록 저장한다.
	ofstream file{ m_outputFileName };

	// 정점 개수
	file << "VC: " << m_vertices.size() << endl << endl;
	for (const auto& v : m_vertices)
	{
		// 위치, 노말, 텍스쳐
		file << "P: " << v.position.x << " " << v.position.y << " " << v.position.z << endl;
		file << "N: " << v.normal.x << " " << v.normal.y << " " << v.normal.z << endl;
		file << "T: " << v.uv.x << " " << v.uv.y << endl;

		// 뼈 인덱스
		file << "BI: " << v.boneIndices.x << " " << v.boneIndices.y << " " << v.boneIndices.z << " " << v.boneIndices.w << endl;

		// 뼈 가중치
		file << "BW: " << v.boneWeights.x << " " << v.boneWeights.y << " " << v.boneWeights.z << " " << v.boneWeights.w << endl << endl;
	}
}

void FBXExporter::ExportAnimation()
{
	ofstream file{ m_animationName + "_" + m_outputFileName };

	// 조인트 개수, 애니메이션 길이
	file << "JOINT_COUNT: " << m_joints.size() << endl;
	file << "FRAME_LENGTH: " << m_animationLength << endl << endl;

	// 애니메이션 변환 행렬
	for (const Joint& j : m_joints)
	{
		file << j.name << endl;
		for (const Keyframe& k : j.keyframes)
		{
			Utilities::WriteFbxAMatrixToStream(file, k.aniTransMatrix * j.globalBindposeInverseMatrix);
			file << endl;
		}
		file << endl;
	}
}