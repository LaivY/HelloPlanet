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

void FBXExporter::Process(const string& inputFileName)
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

	ProcessLink();

	// 출력
	ExportMesh();
	ExportAnimation();
}

void FBXExporter::LoadSkeleton(FbxNode* node, int index, int parentIndex)
{
	// Joint는 뼈를 의미한다.
	// 모든 뼈들을 순회하면서 이름과 부모-자식 관계를 불러온다.
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		Joint joint;
		joint.name = node->GetName();
		joint.parentIndex = parentIndex;
		m_joints.push_back(joint);
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
		LoadSkeleton(node->GetChild(i), m_joints.size(), index);
}

void FBXExporter::LoadMesh(FbxNode* node)
{
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		// 이름
		Mesh mesh;
		mesh.name = node->GetName();
		mesh.isLinked = node->GetParent() == m_scene->GetRootNode() ? false : true;
		if (mesh.isLinked)
		{
			mesh.parentNode = node->GetParent();

			// 크기, 회전 변환은 루트 좌표계 기준,
			// 이동 변환은 루트 좌표계에서의 (자기 좌표 - 부모 좌표) 를 저장한다.
			auto m1{ node->EvaluateGlobalTransform() };
			auto m2{ node->GetParent()->EvaluateGlobalTransform() };
			m1[3][0] -= m2[3][0];
			m1[3][1] -= m2[3][1];
			m1[3][2] -= m2[3][2];

			// 크기가 플레이어와 비교했을 때 3DMAX에서 봤을때와 달라서 임의로 맞춰줌
			// 위치
			m1[3][0] *= GUN_SCALE_FACTOR;
			m1[3][1] *= GUN_SCALE_FACTOR;
			m1[3][2] *= GUN_SCALE_FACTOR;

			// 크기
			m1[0][0] *= GUN_SCALE_FACTOR;
			m1[1][1] *= GUN_SCALE_FACTOR;
			m1[2][2] *= GUN_SCALE_FACTOR;

			mesh.transformMatrix = Utilities::Transpose(Utilities::toXMFLOAT4X4(m1));
		}
		else
		{
			// 로컬 == 글로벌
			mesh.transformMatrix = Utilities::toXMFLOAT4X4(node->EvaluateGlobalTransform());
		}
		m_meshes.push_back(move(mesh));

		// 데이터 로딩
		LoadMaterials(node, m_meshes.size() - 1);
		LoadCtrlPoints(node, m_meshes.size() - 1);
		LoadAnimation(node, m_meshes.size() - 1);
		LoadVertices(node, m_meshes.size() - 1);
	}
	for (int i = 0; i < node->GetChildCount(); ++i)
		LoadMesh(node->GetChild(i));
}

void FBXExporter::LoadMaterials(FbxNode* node, int meshIndex)
{
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
		m_meshes[meshIndex].materials.push_back(move(m));
	}
}

void FBXExporter::LoadCtrlPoints(FbxNode* node, int meshIndex)
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
		m_meshes[meshIndex].ctrlPoints.push_back(move(ctrlPoint));
	}
}

void FBXExporter::LoadAnimation(FbxNode* node, int meshIndex)
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
				m_meshes[meshIndex].ctrlPoints[index].weights.push_back(make_pair(ji, weight));
			}

			// 자기 자신의 변환 행렬
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

				// curr 시간 일 때의 T-pose -> 루트 좌표계 변환 행렬
				FbxAMatrix toRoot{ (node->EvaluateGlobalTransform(curr) * geometryTransform).Inverse() };

				Keyframe keyframe;
				keyframe.frameNum = i;
				keyframe.aniTransMatrix = toRoot * bone->EvaluateGlobalTransform(curr);
				m_joints[ji].keyframes.push_back(move(keyframe));
			}
		}
	}

	// 제어점 중 가충치를 4개 이하로 갖고있는 제어점들은 4개로 채워줌
	for (CtrlPoint& ctrlPoint : m_meshes[meshIndex].ctrlPoints)
		for (int i = ctrlPoint.weights.size(); i < 4; ++i)
			ctrlPoint.weights.push_back(make_pair(0, 0.0));

	// 제어점이 갖고있는 가중치 정보를 가중치가 높은 순서로 정렬
	for (CtrlPoint& ctrlPoint : m_meshes[meshIndex].ctrlPoints)
		partial_sort(ctrlPoint.weights.begin(), ctrlPoint.weights.begin() + 4, ctrlPoint.weights.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			});
}

void FBXExporter::LoadVertices(FbxNode* node, int meshIndex)
{
	FbxMesh* mesh{ node->GetMesh() };
	int vertexCountIndex{ 0 };
	for (int i = 0; i < mesh->GetPolygonCount(); ++i)
	{
		for (int j = 0; j < 3; ++j) // 삼각형이기 때문에 3번 반복
		{
			// i번째 삼각형의 j번째 제어점
			int ctrlPointIndex{ mesh->GetPolygonVertex(i, j) };
			const CtrlPoint& ctrlPoint{ m_meshes[meshIndex].ctrlPoints[ctrlPointIndex] };

			Vertex vertex;
			vertex.position = XMFLOAT3{ ctrlPoint.position.x, ctrlPoint.position.y, ctrlPoint.position.z };
			vertex.normal = GetNormal(mesh, ctrlPointIndex, vertexCountIndex);
			vertex.uv = GetUV(mesh, ctrlPointIndex, vertexCountIndex);
			vertex.materialIndex = GetMaterial(mesh, i);
			vertex.boneIndices = XMUINT4(ctrlPoint.weights[0].first, ctrlPoint.weights[1].first, ctrlPoint.weights[2].first, ctrlPoint.weights[3].first);
			vertex.boneWeights = XMFLOAT4(ctrlPoint.weights[0].second, ctrlPoint.weights[1].second, ctrlPoint.weights[2].second, ctrlPoint.weights[3].second);
			m_meshes[meshIndex].vertices.push_back(move(vertex));

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

XMFLOAT4 FBXExporter::GetColor(FbxMesh * mesh, int controlPointIndex, int vertexCountIndex)
{
	FbxColor result{};
	if (mesh->GetElementVertexColorCount() < 1)
		return XMFLOAT4{};

	FbxGeometryElementVertexColor* color{ mesh->GetElementVertexColor(0) };
	if (color->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		if (color->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			result = color->GetDirectArray().GetAt(controlPointIndex);
		}
		else if (color->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			int index{ color->GetIndexArray().GetAt(controlPointIndex) };
			result = color->GetDirectArray().GetAt(index);
		}
	}
	else if (color->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		if (color->GetReferenceMode() == FbxGeometryElement::eDirect)
		{
			result = color->GetDirectArray().GetAt(vertexCountIndex);
		}
		else if (color->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
		{
			int index{ color->GetIndexArray().GetAt(vertexCountIndex) };
			result = color->GetDirectArray().GetAt(index);
		}
	}
	return XMFLOAT4(result[0], result[1], result[2], result[3]);
}

int FBXExporter::GetMaterial(FbxMesh* mesh, int polygonIndex)
{
	FbxGeometryElementMaterial* material{ mesh->GetElementMaterial(0) };
	if (material->GetMappingMode() == FbxGeometryElement::eByPolygon &&
		material->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
	{
		return material->GetIndexArray().GetAt(polygonIndex);
	}
	return -1;
}

void FBXExporter::ProcessLink()
{
	FbxAnimStack* animStack{ m_scene->GetSrcObject<FbxAnimStack>(0) };
	FbxString animStackName{ animStack->GetName() };
	FbxTakeInfo* takeInfo{ m_scene->GetTakeInfo(animStackName)};
	FbxTime start{ takeInfo->mLocalTimeSpan.GetStart() };
	FbxTime end{ takeInfo->mLocalTimeSpan.GetStop() };
	for (const Mesh& m : m_meshes)
	{
		if (!m.isLinked) continue;

		ofstream file{ Utilities::GetOnlyPath(m_inputFileName) + "link.txt" };
		file << "JOINT_COUNT: " << 1 << endl;
		file << "FRAME_LENGTH: " << m_animationLength << endl << endl;
		file << "LINK" << endl;
		for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
		{
			FbxTime curr;
			curr.SetFrame(i, FbxTime::eFrames24);

			auto transform{ m.parentNode->EvaluateGlobalTransform(curr) };
			XMFLOAT4X4 temp;
			XMStoreFloat4x4(&temp, XMMatrixTranslation(transform[3][0] * GUN_SCALE_FACTOR, 
													   transform[3][1] * GUN_SCALE_FACTOR,
													   transform[3][2] * GUN_SCALE_FACTOR));
			Utilities::WriteToStream(file, temp);
			file << endl;
		}
	}
}

void FBXExporter::ExportMesh()
{
	for (const Mesh& mesh : m_meshes)
	{
		ofstream file{ Utilities::GetOnlyPath(m_inputFileName) + mesh.name + ".txt"};

		// 변환 행렬
		file << "TRANSFORM_MATRIX: ";
		Utilities::WriteToStream(file, mesh.transformMatrix);
		file << endl;

		// 정점 데이터
		file << "VERTEX_COUNT: " << mesh.vertices.size() << endl << endl;
		for (const Vertex& vertex : mesh.vertices)
		{
			// 위치, 노말, 텍스쳐 좌표, 재질 인덱스
			file << "P: " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << endl;
			file << "N: " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << endl;
			file << "T: " << vertex.uv.x << " " << vertex.uv.y << endl;
			file << "MI: " << vertex.materialIndex << endl;

			// 뼈 인덱스, 가중치
			// 링크되어 있다면 링크된 뼈의 가중치를 1로 설정
			if (mesh.isLinked)
			{
				//file << "BI: " << GetJointIndexByName(m.parentNode->GetName()) << " " << 0 << " " << 0 << " " << 0 << endl;
				file << "BI: " << 0 << " " << 0 << " " << 0 << " " << 0 << endl;
				file << "BW: " << 1.0f << " " << 0.0f << " " << 0.0f << " " << 0.0f << endl;
			}
			else
			{
				file << "BI: " << vertex.boneIndices.x << " " << vertex.boneIndices.y << " " << vertex.boneIndices.z << " " << vertex.boneIndices.w << endl;
				file << "BW: " << vertex.boneWeights.x << " " << vertex.boneWeights.y << " " << vertex.boneWeights.z << " " << vertex.boneWeights.w << endl;
			}
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

void FBXExporter::ExportAnimation()
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
			Utilities::WriteToStream(file, keyframe.aniTransMatrix * joint.globalBindposeInverseMatrix);
			file << endl;
		}
		file << endl;
	}
}