<div align="center">
  <h1>⭐Hello, Planet!⭐</h1>
  <h3>한국공학대학교 Hello, Planet!팀 졸업작품</h3>
  <h3>TEAM MEMBER</h3>
  
  <table>
    <th>이름</th> <th>역할</th>
    <tr><!-- 첫번째 줄 시작 -->
      <td>원윤식</td>
      <td>클라이언트</td>
    </tr><!-- 첫번째 줄 끝 -->
    <tr><!-- 두번째 줄 시작 -->
      <td>김재원</td>
      <td>서버</td>
    </tr><!-- 두번째 줄 끝 -->
    <tr>
      <td>김연중</td>
      <td>기획, 그래픽, 서브 클라이언트</td>
    </tr>
  </table>

  <h3>DEVELOPMENT ENVIRONMENT</h3>
  <img src="https://img.shields.io/badge/C++-00599C?style={flat}&logo=C%2B%2B&logoColor=white"/>
  <img src="https://img.shields.io/badge/DirectX12-5E5E5E?style={flat}&logo=microsoft&logoColor=white"/>
  <img src="https://img.shields.io/badge/Blender-F5792A?style={flat}&logo=blender&logoColor=white"/>
  <img src="https://img.shields.io/badge/Github-181717?style={flat}&logo=github&logoColor=white"/>
  <img src="https://img.shields.io/badge/SourceTree-0052CC?style={flat}&logo=sourcetree&logoColor=white"/>
  <img src="https://img.shields.io/badge/Notion-000?style={flat}&logo=notion&logoColor=white"/>
</div>

# ✏️ 기획
## 1. 연구 목적
- DirectX 12를 이용한 3D게임 제작
  - DirectX 파이프라인 이해
  - 조명, 그림자, 3D 애니메이션 구현
  - 멀티쓰레딩을 이용한 렌더링 시간 단축

- BOOST/ASIO 비동기 서버 구축으로 멀티플레이 구현
  - 비동기 I/O 프로그래밍 이해
  - 시야 처리와 멀티쓰레드 프로그래밍으로 높은 동시접속자수를 처리할 수 있는 서버 구현

## 2. 게임 특징
- 외계 행성에서 몰려오는 몬스터를 처치하는 멀티플레이 슈팅 게임
- 플레이어는 3가지 무기 중 하나를 선택해서 플레이
- 게임은 라운드 형식으로 진행되며 각 라운드 종료 후 보상 획득
- 마지막 라운드의 보스를 처치하는 것이 최종 목표

## 3. 기술적 요소 및 중점 연구 분야
- 클라이언트
  - 3D 애니메이션<br>FBX SDK를 이용한 애니메이션 구현
  - 조명 및 그림자<br>방향성 조명과 그림자 구현
  - 멀티쓰레딩<br>멀티쓰레딩을 이용한 렌더링 속도 향상
- 서버
  - 시야 처리<br>필요한 계산만 처리하여 서버 부하 감소
  - NPC AI<br>A* 알고리즘과 Path mesh로 몬스터 AI 구현
  - BOOST/ASIO 라이브러리<br>C++ 표준 라이브러리로 제작한 서버로 리눅스 등에서도 작동하도록 구축

# 📂 소스
## 1. 클라이언트
### 1-1. 클라이언트 구조
```cpp
· class GameFramework
├── · class Timer
└── · class Scene
    ├── · class Camera
    │   └─ · class Player
    ├── · class Mesh
    │	└─ · struct Vertex
    │	└─ · struct Material
    │	└─ · struct Joint
    │	└─ · struct Animation
    ├── · class Shader
    ├── · class Texture
    └─┬─ · class GameObject
      │  └─ · class Mesh
      │  └─ · class Shader
      │  └─ · class Texture
      │  └─ · struct TextureInfo
      │  └─ · struct AnimationInfo
      ├─ · class UIObject : public GameObject
      ├─ · class Skybox : public GameObject
      ├─ · class Bullet : public GameObject
      └─ · class Player : public GameObject
         └─ · class Camera
```
## 2. 서버
