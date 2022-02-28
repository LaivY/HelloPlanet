#include "FBXExporter.h"

FBXExporter::FBXExporter() : m_inputFileName{}, m_manager { nullptr }, m_scene{ nullptr }, m_animationName{}, m_animationLength{}
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

void FBXExporter::Process(const string& inputFileName, bool doExportMesh, bool doExportAnimation)
{
	// 입력, 출력 파일 이름 저장
	m_inputFileName = inputFileName;

	// 임포터 생성 및 초기화
	FbxImporter* fbxImporter{ FbxImporter::Create(m_manager, "IMPORTER") };
	if (!fbxImporter->Initialize(m_inputFileName.c_str(), -1, m_manager->GetIOSettings()))
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

	// 데이터 로딩
	FbxNode* rootNode{ m_scene->GetRootNode() };
	for (int i = 0; i < rootNode->GetChildCount(); ++i)
		LoadSkeleton(rootNode->GetChild(i), 0, -1);
	for (int i = 0; i < rootNode->GetChildCount(); ++i)
		LoadMesh(rootNode->GetChild(i));

	// 링크된 메쉬 작업
	ProcessLink();

	// 출력
	if (doExportMesh)ExportMesh();
	if (doExportAnimation)ExportAnimation();
}

void FBXExporter::LoadSkeleton(FbxNode* node, int index, int parentIndex)
{
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		Joint joint;
		joint.name = node->GetName();
		joint.parentIndex = parentIndex;
		m_joints.push_back(move(joint));
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
		LoadSkeleton(node->GetChild(i), m_joints.size(), index);
}

void FBXExporter::LoadMesh(FbxNode* node)
{
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		Mesh mesh;
		LoadInfo(node, mesh);
		LoadMaterials(node, mesh);
		LoadCtrlPoints(node, mesh);
		LoadAnimation(node, mesh);
		LoadVertices(node, mesh);
		m_meshes.push_back(move(mesh));
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
		LoadMesh(node->GetChild(i));
}

void FBXExporter::LoadInfo(FbxNode* node, Mesh& mesh)
{
	/*
	* 메쉬의 이름과 링크 여부를 불러온다.
	* 링크되어 있다면 메쉬의 변환 행렬은 항등 행렬이다.
	* 링크되어 있지 않다면 전역 좌표계에서의 자신의 변환 행렬 그대로 사용한다. (이 경우 로컬 좌표계와 전역 좌표계에서의 변환 행렬은 같다.)
	*/

	mesh.name = node->GetName();
	mesh.isLinked = node->GetParent() == m_scene->GetRootNode() ? false : true;
	if (mesh.isLinked)
		mesh.parentNode = node->GetParent();
	else
		mesh.transformMatrix = node->EvaluateGlobalTransform();
	mesh.node = node;
}

void FBXExporter::LoadMaterials(FbxNode* node, Mesh& mesh)
{
	/*
	* 메쉬가 사용하는 재질들을 불러온다.
	* 색깔, 간접 조명 색깔(AmbientColor), 난반사광(DiffuseColor), 정반사광(SpecularColor), 광택도(ShininessExponent) 등
	*/

	for (int i = 0; i < node->GetMaterialCount(); ++i)
	{
		FbxSurfaceMaterial* material{ node->GetMaterial(i) };

		Material m;
		m.name = material->GetName();
		if (FbxProperty p{ material->FindPropertyHierarchical("3dsMax|Parameters|base_color") }; p.IsValid())
		{
			FbxColor baseColor{ p.Get<FbxColor>() };
			m.baseColor = XMFLOAT4{ static_cast<float>(baseColor[0]), static_cast<float>(baseColor[1]), static_cast<float>(baseColor[2]), static_cast<float>(baseColor[3]) };
		}
		//if (FbxProperty p{ material->FindProperty("AmbientColor") }; p.IsValid())
		//{
		//	FbxColor ambientColor{ p.Get<FbxColor>() };
		//	cout << "AmbientColor : " << ambientColor[0] << ", " << ambientColor[1] << ", " << ambientColor[2] << ", " << ambientColor[3] << endl;
		//}
		//if (FbxProperty p{ material->FindProperty("DiffuseColor") }; p.IsValid())
		//{
		//	FbxColor diffuseColor{ p.Get<FbxColor>() };
		//	cout << "DiffuseColor : " << diffuseColor[0] << ", " << diffuseColor[1] << ", " << diffuseColor[2] << ", " << diffuseColor[3] << endl;
		//}
		//if (FbxProperty p{ material->FindProperty("SpecularColor") }; p.IsValid())
		//{
		//	FbxColor specularColor{ p.Get<FbxColor>() };
		//	cout << "SpecularColor : " << specularColor[0] << ", " << specularColor[1] << ", " << specularColor[2] << ", " << specularColor[3] << endl;
		//}
		//if (FbxProperty p{ material->FindProperty("EmissiveColor") }; p.IsValid())
		//{
		//	FbxColor emissiveColor{ p.Get<FbxColor>() };
		//	cout << "EmissiveColor : " << emissiveColor[0] << ", " << emissiveColor[1] << ", " << emissiveColor[2] << ", " << emissiveColor[3] << endl;
		//}
		//if (FbxProperty p{ material->FindProperty("ShininessExponent") }; p.IsValid())
		//{
		//	FbxDouble shininessExponent{ p.Get<FbxDouble>() };
		//	cout << "ShininessExponent : " << shininessExponent << endl;
		//}
		//if (FbxProperty p{ material->FindProperty("TransparencyFactor") }; p.IsValid())
		//{
		//	FbxDouble transparencyFactor{ p.Get<FbxDouble>() };
		//	cout << "TransparencyFactor : " << transparencyFactor << endl;
		//}
		mesh.materials.push_back(move(m));
	}
}

void FBXExporter::LoadCtrlPoints(FbxNode* node, Mesh& mesh)
{
	/*
	* 메쉬의 제어점을 불러온다.
	* 제어점은 위치와 뼈의 가중치 정보를 갖는다.
	* 제어점의 가중치가 4개 미만이면 빈 데이터로 4개를 채워주고 가중치 순으로 정렬한다.
	* 원래 성능을 위해서는 가중치 정보를 애니메이션 데이터 처리할 때 같이 해야하지만 함수 이름에 맞는 일을 하기 위해 그러지 않았다.
	*/

	FbxMesh* m{ node->GetMesh() };

	// 제어점 위치
	for (int i = 0; i < m->GetControlPointsCount(); ++i)
	{
		CtrlPoint ctrlPoint;
		ctrlPoint.position = XMFLOAT3{ 
			static_cast<float>(m->GetControlPointAt(i)[0]),
			static_cast<float>(m->GetControlPointAt(i)[1]),
			static_cast<float>(m->GetControlPointAt(i)[2])
		};
		mesh.ctrlPoints.push_back(move(ctrlPoint));
	}

	// 제어점 가중치
	for (int i = 0; i < m->GetDeformerCount(FbxDeformer::eSkin); ++i)
	{
		FbxSkin* skin{ reinterpret_cast<FbxSkin*>(m->GetDeformer(i, FbxDeformer::eSkin)) };
		if (!skin) continue;

		for (int j = 0; j < skin->GetClusterCount(); ++j)
		{
			FbxCluster* cluster{ skin->GetCluster(j) };
			FbxNode* bone{ cluster->GetLink() };
			int jointIndex{ GetJointIndex(bone->GetName()) };

			// 제어점에 가중치 정보 추가
			for (int k = 0; k < cluster->GetControlPointIndicesCount(); ++k)
			{
				int ctrlPointIndex{ cluster->GetControlPointIndices()[k] };
				double weight{ cluster->GetControlPointWeights()[k] };
				mesh.ctrlPoints[ctrlPointIndex].weights.push_back(make_pair(jointIndex, weight));
			}
		}
	}

	// 가충치를 4개 이하로 갖고있는 제어점들은 가중치를 개수를 4개로 맞춤
	for (CtrlPoint& ctrlPoint : mesh.ctrlPoints)
		for (int i = ctrlPoint.weights.size(); i < 4; ++i)
			ctrlPoint.weights.push_back(make_pair(0, 0.0));

	// 제어점이 갖고있는 가중치 정보를 가중치가 높은 순서로 정렬
	for (CtrlPoint& ctrlPoint : mesh.ctrlPoints)
		partial_sort(ctrlPoint.weights.begin(), ctrlPoint.weights.begin() + 4, ctrlPoint.weights.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			});
}

void FBXExporter::LoadAnimation(FbxNode* node, Mesh& mesh)
{
	/*
	* 메쉬의 애니메이션을 불러온다.
	* 일정 시간 간격(24FPS)마다 모든 뼈의 애니메이션 변환 행렬을 키프레임으로 저장한다.
	*/

	FbxMesh* m{ node->GetMesh() };
	FbxAMatrix geometryTransform{ Utilities::GetGeometryTransformation(node) }; // 거의 항등행렬

	for (int i = 0; i < m->GetDeformerCount(FbxDeformer::eSkin); ++i)
	{
		FbxSkin* skin{ reinterpret_cast<FbxSkin*>(m->GetDeformer(i, FbxDeformer::eSkin)) };
		if (!skin) continue;

		for (int j = 0; j < skin->GetClusterCount(); ++j)
		{
			FbxCluster* cluster{ skin->GetCluster(j) };
			FbxNode* bone{ cluster->GetLink() };
			int jointIndex{ GetJointIndex(bone->GetName()) };

			// 부모 좌표계에서의 자신의 변환 행렬
			FbxAMatrix transformMatrix; cluster->GetTransformMatrix(transformMatrix);

			// 부모의 변환 행렬
			FbxAMatrix transformLinkMatrix; cluster->GetTransformLinkMatrix(transformLinkMatrix);

			// 루트 좌표계에서의 자신의 변환 행렬
			m_joints[jointIndex].globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

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
			for (FbxLongLong k = start.GetFrameCount(FbxTime::eFrames24); k <= end.GetFrameCount(FbxTime::eFrames24); ++k)
			{
				FbxTime curr;
				curr.SetFrame(k, FbxTime::eFrames24);

				// curr 시간 일 때의 T-pose -> 루트 좌표계 변환 행렬
				FbxAMatrix toRoot{ (node->EvaluateGlobalTransform(curr) * geometryTransform).Inverse() };

				Keyframe keyframe;
				keyframe.frameNum = k;
				keyframe.transformMatrix = toRoot * bone->EvaluateGlobalTransform(curr);
				m_joints[jointIndex].keyframes.push_back(move(keyframe));
			}
		}
	}
}

void FBXExporter::LoadVertices(FbxNode* node, Mesh& mesh)
{
	/*
	* 앞서 불러온 정보들로 메쉬의 정점을 만든다.
	* 정점은 위치, 노말, 텍스쳐 좌표, 재질 인덱스, 뼈 인덱스, 뼈 가중치를 갖는다.
	*/

	FbxMesh* m{ node->GetMesh() };
	int vertexCountIndex{ 0 };
	for (int i = 0; i < m->GetPolygonCount(); ++i)
	{
		for (int j = 0; j < 3; ++j) // 삼각형이기 때문에 3번 반복
		{
			// i번째 삼각형의 j번째 제어점
			int ctrlPointIndex{ m->GetPolygonVertex(i, j) };
			const CtrlPoint& ctrlPoint{ mesh.ctrlPoints[ctrlPointIndex] };

			Vertex vertex;
			vertex.position = XMFLOAT3{ ctrlPoint.position.x, ctrlPoint.position.y, ctrlPoint.position.z };
			vertex.normal = GetNormal(m, ctrlPointIndex, vertexCountIndex);
			vertex.uv = GetUV(m, ctrlPointIndex, vertexCountIndex);
			vertex.materialIndex = GetMaterial(m, i);
			vertex.boneIndices = XMUINT4(ctrlPoint.weights[0].first, ctrlPoint.weights[1].first, ctrlPoint.weights[2].first, ctrlPoint.weights[3].first);
			vertex.boneWeights = XMFLOAT4(ctrlPoint.weights[0].second, ctrlPoint.weights[1].second, ctrlPoint.weights[2].second, ctrlPoint.weights[3].second);
			mesh.vertices.push_back(move(vertex));

			// 정점 개수 증가
			++vertexCountIndex;
		}
	}
}

void FBXExporter::ProcessLink()
{
	/*
	* 링크되어 있는 메쉬를 처리하는 함수이다.
	* 전역 좌표계에서의 부모의 애니메이션 변환 행렬을 저장한 뼈를 추가한다.
	* 자식 메쉬는 방금 추가한 뼈의 가중치를 1.0으로 설정해준다.
	*/

	FbxAnimStack* animStack{ m_scene->GetSrcObject<FbxAnimStack>(0) };
	FbxString animStackName{ animStack->GetName() };
	FbxTakeInfo* takeInfo{ m_scene->GetTakeInfo(animStackName) };
	FbxTime start{ takeInfo->mLocalTimeSpan.GetStart() };
	FbxTime end{ takeInfo->mLocalTimeSpan.GetStop() };
	FbxAMatrix identity; identity.SetIdentity();

	for (Mesh& mesh : m_meshes)
	{
		if (!mesh.isLinked) continue;

		Joint joint;
		joint.name = "LINK";
		joint.parentIndex = -1;
		joint.globalBindposeInverseMatrix = identity;
		for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
		{
			FbxTime curr; curr.SetFrame(i, FbxTime::eFrames24);

			FbxAMatrix transformMatrix{ mesh.node->EvaluateGlobalTransform(curr) };
			FbxAMatrix halfScaling{ Utilities::toFbxAMatrix(XMMatrixScaling(0.5f, 0.5f, 0.5f)) };
			transformMatrix = halfScaling * transformMatrix;

			Keyframe keyframe;
			keyframe.frameNum = i;
			keyframe.transformMatrix = transformMatrix;
			joint.keyframes.push_back(move(keyframe));
		}
		for (Vertex& vertex : mesh.vertices)
		{
			vertex.boneIndices = XMUINT4(m_joints.size(), 0, 0, 0);
			vertex.boneWeights = XMFLOAT4{ 1.0f, 0.0f, 0.0f, 0.0f };
		}
		m_joints.push_back(move(joint));
	}
}

int FBXExporter::GetJointIndex(const string & name) const
{
	auto i = find_if(m_joints.begin(), m_joints.end(), [&name](const Joint& joint) {
		return joint.name == name;
		});
	if (i == m_joints.end())
		return -1;
	return distance(m_joints.begin(), i);
}

XMFLOAT3 FBXExporter::GetNormal(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex) const
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

XMFLOAT2 FBXExporter::GetUV(FbxMesh* mesh, int controlPointIndex, int vertexCountIndex) const
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

int FBXExporter::GetMaterial(FbxMesh* mesh, int polygonIndex) const
{
	FbxGeometryElementMaterial* material{ mesh->GetElementMaterial(0) };
	if (material->GetMappingMode() == FbxGeometryElement::eByPolygon &&
		material->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
	{
		return material->GetIndexArray().GetAt(polygonIndex);
	}
	return -1;
}

void FBXExporter::ExportMesh() const
{
	for (const Mesh& mesh : m_meshes)
	{
		ofstream file{ Utilities::GetOnlyPath(m_inputFileName) + mesh.name + ".txt"};

		// 변환 행렬
		file << "TRANSFORM_MATRIX: ";
		Utilities::WriteToStream(file, mesh.transformMatrix.Transpose());
		file << endl;

		// 정점 데이터
		file << "VERTEX_COUNT: " << mesh.vertices.size() << endl << endl;
		for (const Vertex& vertex : mesh.vertices)
		{
			file << "P: " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << endl;
			file << "N: " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << endl;
			file << "T: " << vertex.uv.x << " " << vertex.uv.y << endl;
			file << "MI: " << vertex.materialIndex << endl;
			file << "BI: " << vertex.boneIndices.x << " " << vertex.boneIndices.y << " " << vertex.boneIndices.z << " " << vertex.boneIndices.w << endl;
			file << "BW: " << vertex.boneWeights.x << " " << vertex.boneWeights.y << " " << vertex.boneWeights.z << " " << vertex.boneWeights.w << endl;
			file << endl;
		}

		// 재질 데이터
		file << "MATERIAL_COUNT: " << mesh.materials.size() << endl << endl;
		for (const Material& material : mesh.materials)
		{
			file << "NAME: " << material.name << endl;
			file << "COLOR: " << material.baseColor.x << " " << material.baseColor.y << " " << material.baseColor.z << " " << material.baseColor.w << endl;
			file << endl;
		}
	}
}

void FBXExporter::ExportAnimation() const
{
	ofstream file{ Utilities::GetOnlyPath(m_inputFileName) + "animation.txt" };

	// 조인트 개수, 애니메이션 길이
	file << "JOINT_COUNT: " << m_joints.size() << endl;
	file << "FRAME_LENGTH: " << m_animationLength << endl << endl;

	// 애니메이션 변환 행렬
	for (const Joint& joint : m_joints)
	{
		file << joint.name << endl;

		// 애니메이션 데이터가 없으면 프레임 길이만큼 항등행렬을 저장
		if (joint.keyframes.empty())
		{
			FbxAMatrix identity;
			identity.SetIdentity();
			for (int i = 0; i < m_animationLength; ++i)
			{
				Utilities::WriteToStream(file, identity);
				file << endl;
			}
			file << endl;
			continue;
		}

		// 애니메이션 데이터가 있으면 해당 데이터를 저장
		for (const Keyframe& keyframe : joint.keyframes)
		{
			Utilities::WriteToStream(file, (keyframe.transformMatrix * joint.globalBindposeInverseMatrix).Transpose());
			file << endl;
		}
		file << endl;
	}
}