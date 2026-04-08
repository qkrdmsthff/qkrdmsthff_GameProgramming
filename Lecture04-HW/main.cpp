/*
DirectX 11: 컴포넌트 기반 게임 오브젝트 시스템 구현

* 과제 개요
본 과제는 Lecture04-GameWorld에서 학습한 게임 루프(Game Loop) 및 컴포넌트 패턴(Component Pattern)의 핵심 개념을 실제 코드로 구현하는 능력을 검증해보는게 목표.
학생들은 Win32 API와 C++ 기반의 DirectX 11 환경에서 프레임 독립적인 객체 이동과 구조적인 엔진 설계 방식을 본인이 익혔는지 확인해본다.

* 개발 환경 및 기술 제약
프로그래밍 언어: C++ (Modern C++ 권장)
프레임워크/API: Win32 API 및 DirectX 11 (D3D11)
화면 설정: 해상도 800 * 600 (Fixed Size)
핵심 설계: GameLoop, GameObject와 Component 클래스 구조를 반드시 Lecture04-GameWorld와 같이 적용할 것

* 세부 구현 요구사항
A. 게임 엔진 구조 (Game Engine Architecture)
	- 프레임 독립적 이동 (DeltaTime):
		- PeekMessage 기반의 Non-blocking 게임 루프를 구축하십시오.
		- 고해상도 타이머를 사용하여 프레임 간 시간 간격인 DeltaTime(dt)을 초 단위로 계산하십시오.
	- 모든 객체의 이동은 반드시 다음의 공식을 따라야 합니다:
		- Position = Position + (Velocity * DeltaTime)
	- 객체 및 컴포넌트 설계 (GameObject/Component):
		- GameObject 클래스는 위치(Position) 정보를 가지며, 부착된 Component들의 Update와 Render를 일괄 호출해야 합니다.
		- 추상 클래스 Component를 설계하고, 이를 상속받아 삼각형을 그리는 Renderer 컴포넌트를 구현하십시오.

B. 과제물 기능 구현 (Functional Requirements)
	- 삼각형 GameObject 생성:
		- 서로 다른 색상을 가진 두 개의 삼각형을 생성하고, 각각 독립적인 GameObject 인스턴스로 관리하십시오.
	- 개별 조작 시스템:
		- 삼각형 1 (Player 1): 방향키(상, 하, 좌, 우)를 이용하여 이동합니다.
		- 삼각형 2 (Player 2): W, A, S, D 키를 이용하여 상하좌우로 이동합니다.
	- 시스템 제어 기능:
		- ESC 키: 프로그램 즉시 종료 및 관련 메모리 해제.
		- F 키: 창 모드(Windowed)와 전체 화면(Full Screen) 모드를 전환(Toggle)하는 기능을 구현하십시오.
*/

#include <iostream>
#include <chrono>
#include <thread>
#include <windows.h>
#include <vector>
#include <string>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct VertexPosition {
	float x;
	float y;
	float z;
};

struct VertexColor {
	float r;
	float g;
	float b;
	float a;
};

struct Vertex {
	VertexPosition position;
	VertexColor color;
};

struct VertexConstantBuffer {
	float offsetX;
	float offsetY;
	float offsetZ;
	float padding;
};

// 전역 변수
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11InputLayout* g_pInputLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;
ID3D11Buffer* g_pConstantBuffer = nullptr;

void MovieCursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

class Component {
public:
	class GameObject* pOwner = nullptr;

	bool isStarted = 0;

	virtual void Start() = 0;
	virtual void Input() {}
	virtual void Update(float deltaTime) {}
	virtual void Render() {}

	virtual ~Component() {}
};

class GameObject {
public:
	VertexPosition position = { 0.0f, 0.0f, 0.0f };

	std::string name;
	std::vector<Component*> components;

	GameObject(std::string name);
	~GameObject();

	void AddComponent(Component* pComp);
};

GameObject::GameObject(std::string name) {
	this->name = name;
}

GameObject::~GameObject() {
	for (int i = 0; i < (int)components.size(); i++) {
		delete components[i];
	}
}

void GameObject::AddComponent(Component* pComp) {
	pComp->pOwner = this;
	pComp->isStarted = false;

	components.push_back(pComp);
}

class PlayerControl : public Component {
public:
	float speed = 1.0f;
	bool usePlayerArrowKeys = true;

	void Start() override;
	void Input() override {}
	void Update(float deltaTime) override;
	void Render() override {}

	PlayerControl(bool usePlayerArrowKeys);
};

PlayerControl::PlayerControl(bool usePlayerArrowKeys) {
	speed = 1.0f;
	this->usePlayerArrowKeys = usePlayerArrowKeys;
}

void PlayerControl::Start() {
	printf("[%s] PlayerControl 기능 시작!\n", pOwner->name.c_str());
}

/*
void PlayerControl::Input() {
	if (usePlayerArrowKeys) {
		moveUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
		moveDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
		moveLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
		moveRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
	}

	else {
		moveUp = (GetAsyncKeyState('W') & 0x8000) != 0;
		moveDown = (GetAsyncKeyState('S') & 0x8000) != 0;
		moveLeft = (GetAsyncKeyState('A') & 0x8000) != 0;
		moveRight = (GetAsyncKeyState('D') & 0x8000) != 0;
	}
}
*/

void PlayerControl::Update(float deltaTime) {
	if (usePlayerArrowKeys) {
		if (GetAsyncKeyState(VK_UP)) { pOwner->position.y += speed * deltaTime; }
		if (GetAsyncKeyState(VK_DOWN)) { pOwner->position.y -= speed * deltaTime; }
		if (GetAsyncKeyState(VK_LEFT)) { pOwner->position.x -= speed * deltaTime; }
		if (GetAsyncKeyState(VK_RIGHT)) { pOwner->position.x += speed * deltaTime; }
	}
	else {
		if (GetAsyncKeyState('W')) { pOwner->position.y += speed * deltaTime; }
		if (GetAsyncKeyState('S')) { pOwner->position.y -= speed * deltaTime; }
		if (GetAsyncKeyState('A')) { pOwner->position.x -= speed * deltaTime; }
		if (GetAsyncKeyState('D')) { pOwner->position.x += speed * deltaTime; }
	}
}

/*
void PlayerControl::Render() {
	if (x < 10.0f) {
		x = 10.0f;
	}

	if (y < 45.0f) {
		y = 45.0f;
	}

	int positionX = (int)(y / 15.0f);
	int positionY = (int)(x / 10.0f);

	MovieCursor(positionX, positionY);

	printf("★");
}
*/

class TriangleRendering : public Component {
private:
	ID3D11Buffer* pVB = nullptr;
	VertexColor color;

public:
	TriangleRendering(VertexColor col) : color(col) {}
	~TriangleRendering() { if (pVB) pVB->Release(); }

	void Start() override;
	void Render() override;
};

void TriangleRendering::Start() {
	Vertex vertices[] = {
			{ { 0.0f, 0.2f, 0.0f }, color },
			{ { 0.2f, -0.2f, 0.0f }, color },
			{ { -0.2f, -0.2f, 0.0f }, color }
	};

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd = { vertices, 0, 0 };
	g_pd3dDevice->CreateBuffer(&bd, &sd, &pVB);
}

void TriangleRendering::Render() {
	VertexConstantBuffer cb;
	cb.offsetX = pOwner->position.x;
	cb.offsetY = pOwner->position.y;
	cb.offsetZ = pOwner->position.z;
	cb.padding = 0.0f;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pVB, &stride, &offset);
	g_pImmediateContext->Draw(3, 0);
}

class InformationDisplay : public Component {
public:
	float totalTime = 0.0f;

	void Start() override;
	void Update(float deltaTime) override;
	void Render() override;
};

void InformationDisplay::Start() {
	totalTime = 0.0f;

	printf("[%s] InformationDisplay 기능 시작!\n", pOwner->name.c_str());
}

void InformationDisplay::Update(float deltaTime) {
	totalTime += deltaTime;
}

void InformationDisplay::Render() {
	MovieCursor(0, 0);

	printf("Total Time: %.2f seconds ", totalTime, "\n");
	printf("첫 번째 삼각형(Player1) 움직이기 : 방향키(상, 하, 좌, 우) \n");
	printf("두 번째 삼각형(Player2) 움직이기 : W, A, S, D \n");
	printf("Exit: ESC | 화면 전환 : F\n");
}


/*
class GameLoop {
public:
	bool isRunning;

	HWND hWnd = nullptr;

	std::vector<GameObject*> gameWorld;
	std::chrono::high_resolution_clock::time_point prevTime;

	float deltaTime;

	void Initialize() {}
	void Input() {}
	void Update() {}
	void Render() {}
	void Run() {}

	GameLoop() {}
	~GameLoop() {}
};

GameLoop::GameLoop() {
	Initialize();
}

GameLoop::~GameLoop() {
	for (int i = 0; i < (int)gameWorld.size(); i++) {
		delete gameWorld[i];
	}
}

void GameLoop::Initialize() {
	isRunning = true;

	gameWorld.clear();

	GameObject* player1 = new GameObject("Player1");
	player1->AddComponent(new PlayerControl());
	player1->AddComponent(new InformationDisplay());
	gameWorld.push_back(player1);

	GameObject* player2 = new GameObject("Player2");
	player2->AddComponent(new PlayerControl());
	player2->AddComponent(new InformationDisplay());
	gameWorld.push_back(player2);

	prevTime = std::chrono::high_resolution_clock::now();

	deltaTime = 0.0f;
}

void GameLoop::Input() {
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

	for (int i = 0; i < (int)gameWorld.size(); i++) {
		for (int j = 0; j < (int)gameWorld[i]->components.size(); j++) {
			gameWorld[i]->components[j]->Input();
		}
	}
}

void GameLoop::Update() {
	for (int i = 0; i < (int)gameWorld.size(); i++)
	{
		for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
		{
			// Start()가 호출된 적 없다면 여기서 호출 (유니티 방식)
			if (gameWorld[i]->components[j]->isStarted == false)
			{
				gameWorld[i]->components[j]->Start();
				gameWorld[i]->components[j]->isStarted = true;
			}
		}
	}

	// D. 업데이트 단계 (Update Phase)
	for (int i = 0; i < (int)gameWorld.size(); i++)
	{
		for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
		{
			gameWorld[i]->components[j]->Update(deltaTime);
		}
	}
}

void GameLoop::Render() {
	system("cls");
	for (int i = 0; i < (int)gameWorld.size(); i++)
	{
		for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
		{
			gameWorld[i]->components[j]->Render();
		}
	}
}

void GameLoop::Run() {
	while (isRunning) {

		// A. 시간 관리 (DeltaTime 계산)
		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed = currentTime - prevTime;
		deltaTime = elapsed.count();
		prevTime = currentTime;

		Input();
		Update();
		Render();

		// CPU 과부하 방지 (약 60~100 FPS 유지 시도)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
*/

class GameLoop {
	HWND hWnd;
	bool isRunning = true;
	std::vector<GameObject*> world;

public:
	bool Init(HINSTANCE hInst);
	void Run();
	void CleanUp();
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};

bool GameLoop::Init(HINSTANCE hInst) {
	// 1. 윈도우 생성
	WNDCLASSEXW wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0, 0, hInst, 0, 0, 0, 0, L"DX11", 0 };
	RegisterClassExW(&wc);
	hWnd = CreateWindowW(L"DX11", L"DX11 Component System", WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, 0, 0, hInst, 0);
	ShowWindow(hWnd, SW_SHOW);

	// 2. DX11 디바이스 & 스왑체인
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 800;
	sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;

	D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, 0, &g_pImmediateContext);

	// 3. 렌더타겟 뷰
	ID3D11Texture2D* pBB = nullptr;
	g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBB);
	g_pd3dDevice->CreateRenderTargetView(pBB, 0, &g_pRenderTargetView);
	pBB->Release();

	// 4. 셰이더 컴파일
	const char* src = "cbuffer CB : register(b0) { float4 off; };"
		"struct VI { float3 p : POSITION; float4 c : COLOR; };"
		"struct PI { float4 p : SV_POSITION; float4 c : COLOR; };"
		"PI VS(VI i) { PI o; o.p = float4(i.p + off.xyz, 1); o.c = i.c; return o; }"
		"float4 PS(PI i) : SV_Target { return i.c; }";

	ID3DBlob* vsB, * psB;
	D3DCompile(src, strlen(src), 0, 0, 0, "VS", "vs_4_0", 0, 0, &vsB, 0);
	D3DCompile(src, strlen(src), 0, 0, 0, "PS", "ps_4_0", 0, 0, &psB, 0);
	g_pd3dDevice->CreateVertexShader(vsB->GetBufferPointer(), vsB->GetBufferSize(), 0, &g_pVertexShader);
	g_pd3dDevice->CreatePixelShader(psB->GetBufferPointer(), psB->GetBufferSize(), 0, &g_pPixelShader);

	D3D11_INPUT_ELEMENT_DESC ied[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	g_pd3dDevice->CreateInputLayout(ied, 2, vsB->GetBufferPointer(), vsB->GetBufferSize(), &g_pInputLayout);
	vsB->Release(); psB->Release();

	// 5. 상수 버퍼
	D3D11_BUFFER_DESC cbd = { sizeof(VertexConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
	g_pd3dDevice->CreateBuffer(&cbd, 0, &g_pConstantBuffer);

	return true;
}

void GameLoop::Run() {
	GameObject* info = new GameObject("Info");
	info->AddComponent(new InformationDisplay());
	world.push_back(info);

	GameObject* p1 = new GameObject("Player1");
	p1->position = { -0.4f, 0.0f, 0.0f };
	p1->AddComponent(new TriangleRendering({ 1, 0, 0, 1 })); // 빨강
	p1->AddComponent(new PlayerControl(true));
	world.push_back(p1);

	GameObject* p2 = new GameObject("Player2");
	p2->position = { 0.4f, 0.0f, 0.0f };
	p2->AddComponent(new TriangleRendering({ 0, 1, 1, 1 })); // 시안
	p2->AddComponent(new PlayerControl(false));
	world.push_back(p2);

	auto prev = std::chrono::high_resolution_clock::now();
	MSG msg = {};
	while (msg.message != WM_QUIT && isRunning) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg); DispatchMessage(&msg);
		}
		else {
			auto curr = std::chrono::high_resolution_clock::now();
			float dt = std::chrono::duration<float>(curr - prev).count();
			prev = curr;

			if (GetAsyncKeyState(VK_ESCAPE)) isRunning = false;
			if (GetAsyncKeyState('F') & 0x0001) {
				static bool fs = false; fs = !fs;
				g_pSwapChain->SetFullscreenState(fs, 0);
			}

			// Update & Render
			float bg[] = { 0.1f, 0.1f, 0.1f, 1 };
			g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, bg);
			D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0, 1 };
			g_pImmediateContext->RSSetViewports(1, &vp);
			g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, 0);
			g_pImmediateContext->IASetInputLayout(g_pInputLayout);
			g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g_pImmediateContext->VSSetShader(g_pVertexShader, 0, 0);
			g_pImmediateContext->PSSetShader(g_pPixelShader, 0, 0);

			for (auto obj : world) {
				for (auto comp : obj->components) {
					if (!comp->isStarted) { comp->Start(); comp->isStarted = true; }
					comp->Update(dt);
					comp->Render();
				}
			}
			g_pSwapChain->Present(1, 0);
		}
	}
}

void GameLoop::CleanUp() {
	for (auto obj : world) delete obj;
	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if (g_pInputLayout) g_pInputLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

LRESULT CALLBACK GameLoop::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
	return DefWindowProc(hWnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int) {
	GameLoop game;
	if (game.Init(hI)) {
		game.Run();
		game.CleanUp();
	}
	return 0;
}

/*
int main() {
	GameLoop gameLoop;

	GameObject* sysInfo = new GameObject("SystemManager");
	InformationDisplay* pInfo = new InformationDisplay();
	sysInfo->AddComponent(pInfo);
	gameLoop.gameWorld.push_back(sysInfo);

	GameObject* player1 = new GameObject("Player1");
	PlayerControl* pControl1 = new PlayerControl();
	pControl1->usePlayerArrowKeys = true;
	player1->AddComponent(pControl1);
	gameLoop.gameWorld.push_back(player1);

	GameObject* player2 = new GameObject("Player2");
	PlayerControl* pControl2 = new PlayerControl();
	pControl2->usePlayerArrowKeys = false;
	player2->AddComponent(pControl2);
	gameLoop.gameWorld.push_back(player2);

	gameLoop.Initialize(hInstance, 800, 600);
	gameLoop.Run();

	return 0;
}
*/