/*

[이번 주 실습 과제: GameLoop를 이용한 움직이는 육망성 만들기]

과제 목표

*GameLoop 패턴 적용: while 루프와 PeekMessage를 이용하여 멈추지 않는 게임 엔진의 기본 구조를 이해한다.
*DirectX 실습: 삼각형 두 개를 반전시켜 겹쳐서 '육망성(별)' 모양을 화면에 출력한다.
*입력 처리: WinProc에서 상하좌우 방향키 입력에 따라 별의 위치 좌표를 실시간으로 업데이트한다.
*버전 관리: GitHub를 활용해 자신의 작업 과정을 커밋(Commit) 로그로 남기는 습관을 지닌다.

*구현 요구 사항

*윈도우 메시지 처리: GetMessage 대신 PeekMessage를 사용하여 입력이 없어도 루프가 돌게 할 것.
*별 만들기:정점(Vertex) 데이터를 구성하여 정삼각형 하나와 뒤집힌 정삼각형 하나를 그릴 것.
(TriangleList 사용) 두 삼각형이 겹쳐져 육망성 모양이 나오도록 정점 좌표를 설계할 것.
*실시간 이동:GameContext(또는 전역 구조체)에 별의 중심 위치(posX, posY) 변수를 선언할 것.
방향키(↑, ↓, ←, →)를 누르면 Update 단계에서 위치 값이 변하고, Render 단계에서 해당 위치에 별이 그려지도록 구현할 것.
(※ 아직 DeltaTime은 배우지 않았으므로 프레임당 고정 수치만큼 이동시켜도 무방함)
과제물 제출 *Github repository를 public으로 생성한 뒤 repository url과 과제물이 저장된 commit log를 첨부하세요.

*/


#include <stdio.h>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

ID3D11Device* g_pd3dDevice = nullptr;          // 리소스 생성자 (공장)
ID3D11DeviceContext* g_pImmediateContext = nullptr;   // 그리기 명령 수행 (일꾼)
IDXGISwapChain* g_pSwapChain = nullptr;          // 화면 전환 (더블 버퍼링)
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;   // 그림을 그릴 도화지(View)

float starPosX = 0.0f;
float starPosY = 0.0f;
const float moveStep = 0.005f;

struct Vertex {
	float x, y, z;
	float r, g, b, a;
};

const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f); // 3D 좌표를 4D로 확장
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col; // 정점에서 계산된 색상을 픽셀에 그대로 적용
}
)";


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_KEYDOWN:
		printf("[EVENT] Key Pressed: %c (Virtual Key: %lld)\n", (char)wParam, wParam);

		if (wParam == VK_UP || wParam == 'W')  printf("  >> 로직: 이런식으로 위쪽 이동에 대한 키 조작 가능!\n");
		if (wParam == VK_DOWN || wParam == 'S') printf("  >> 로직: 이런식으로 아래쪽 이동에 대한 키 조작 가능!\n");
		if (wParam == VK_LEFT || wParam == 'A')  printf("  >> 로직: 이런식으로 왼쪽 이동에 대한 키 조작 가능!\n");
		if (wParam == VK_RIGHT || wParam == 'D') printf("  >> 로직: 이런식으로 오른쪽 이동에 대한 키 조작 가능!\n");

		if (wParam == 'Q') {
			printf("  >> 로직: Q 입력 감지, 프로그램 종료 요청!\n");
			PostQuitMessage(0);
		}
		break;

	case WM_KEYUP:
		printf("[EVENT] Key Released: %c\n", (char)wParam);
		break;

	case WM_DESTROY:
		printf("[SYSTEM] 윈도우 파괴 메시지 수신. 루프를 탈출합니다.\n");
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"MyLectureClass";

	RegisterClassExW(&wcex);

	WNDCLASSEXW wcex2 = { sizeof(WNDCLASSEX) };
	wcex2.lpfnWndProc = WndProc;
	wcex2.hInstance = hInstance;
	wcex2.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex2.lpszClassName = L"DX11GameLoopClass";
	RegisterClassExW(&wcex2);

	HWND hWnd = CreateWindowW(L"DX11GameLoopClass", L"과제 : 움직이는 육망성 만들기",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) return -1;
	ShowWindow(hWnd, nCmdShow);

	// 2. DX11 디바이스 및 스왑 체인 초기화
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;

	// GPU와 통신할 통로(Device)와 화면(SwapChain)을 생성함.
	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

	// 렌더 타겟 설정 (도화지 준비)
	ID3D11Texture2D* pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release(); // 뷰를 생성했으므로 원본 텍스트는 바로 해제 (중요!)

	// 3. 셰이더 컴파일 및 생성
	ID3DBlob* vsBlob, * psBlob;
	D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
	D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

	ID3D11VertexShader* vShader;
	ID3D11PixelShader* pShader;
	g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
	g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);

	// 정점의 데이터 형식을 정의 (IA 단계에 알려줌)
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ID3D11InputLayout* pInputLayout;
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE; // 백페이스 컬링 끔
	rd.FrontCounterClockwise = FALSE;
	rd.DepthClipEnable = TRUE;
	ID3D11RasterizerState* pRast = nullptr;
	g_pd3dDevice->CreateRasterizerState(&rd, &pRast);
	g_pImmediateContext->RSSetState(pRast);
	g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
	vsBlob->Release(); psBlob->Release(); // 컴파일용 임시 메모리 해제

	// 4. 정점 버퍼 생성 (삼각형 데이터)
	Vertex vertices[] = {
		// 정삼각형 (위로 향한 삼각형)
		{  0.0f,  0.5f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f }, // 위 꼭짓점
		{  0.5f, -0.5f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f }, // 오른쪽 아래
		{ -0.5f, -0.5f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f }, // 왼쪽 아래

		// 뒤집힌 정삼각형 (아래로 향한 삼각형)
		{  0.0f, -0.8f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f }, // 아래 꼭짓점
		{  0.5f,  0.2f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f }, // 오른쪽 위
		{ -0.5f,  0.2f, 0.5f, 1.0f, 0.6f, 0.0f, 1.0f }  // 왼쪽 위
	};
	ID3D11Buffer* pVBuffer;
	D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
	g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);

	//메세지 루프 (peekMessage 사용)
	MSG msg = { 0 };

	while (WM_QUIT != msg.message) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		else {
			// (2) 업데이트 단계: 여기서 캐릭터의 위치나 로직을 계산함
			// (과제: GetAsyncKeyState 등을 써서 posX, posY를 변경하셈)

			if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000) {
				starPosX -= moveStep;
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
				starPosX += moveStep;
			}
			if (GetAsyncKeyState(VK_UP) & 0x8000 || GetAsyncKeyState('W') & 0x8000) {
				starPosY += moveStep;
			}
			if (GetAsyncKeyState(VK_DOWN) & 0x8000 || GetAsyncKeyState('S') & 0x8000) {
				starPosY -= moveStep;
			}

			if (starPosX < -1.0f) starPosX = -1.0f;
			if (starPosX > 1.0f) starPosX = 1.0f;
			if (starPosY < -1.0f) starPosY = -1.0f;
			if (starPosY > 1.0f) starPosY = 1.0f;

			Vertex updatedVertices[6];
			for (int i = 0; i < 6; ++i) {
				updatedVertices[i] = vertices[i];
				updatedVertices[i].x += starPosX;
				updatedVertices[i].y += starPosY;
			}

			g_pImmediateContext->UpdateSubresource(pVBuffer, 0, nullptr, updatedVertices, 0, 0);



			// (3) 출력 단계: 변한 데이터를 바탕으로 화면에 그림
			float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
			g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

			// 렌더링 파이프라인 상태 설정
			g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
			D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
			g_pImmediateContext->RSSetViewports(1, &vp);

			g_pImmediateContext->IASetInputLayout(pInputLayout);
			UINT stride = sizeof(Vertex), offset = 0;
			g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

			// Primitive Topology 설정: 삼각형 리스트로 연결하라!
			g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
			g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

			// 최종 그리기
			g_pImmediateContext->Draw(6, 0);

			// 화면 교체 (프론트 버퍼와 백 버퍼 스왑)
			g_pSwapChain->Present(0, 0);
		}
	}

	if (pVBuffer) pVBuffer->Release();
	if (pInputLayout) pInputLayout->Release();
	if (vShader) vShader->Release();
	if (pShader) pShader->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();

	return (int)msg.wParam;
}

typedef struct {
	int playerXPosition;
	int playerYPosition;
	int isRunning;
	char currentInput;
} GameContext;

//입력 단계
void ProcessInput(GameContext* context) {
	printf("[방향키(↑:위쪽, ↓:아래쪽, ←:왼쪽, :오른쪽)를 입력하세요!] : ");

	scanf_s(" %c", &(context->currentInput), (unsigned int)sizeof(char));
}

void Update(GameContext* context) {
	//종료 로직
	if (context->currentInput == 'Q' || context->currentInput == 'q') {
		context->isRunning = 0;

		return;
	}

	if (context->currentInput == VK_UP || context->currentInput == 'W') {
		context->playerXPosition++;
	}

	if (context->currentInput == VK_DOWN || context->currentInput == 'S') {
		context->playerXPosition--;
	}

	if (context->currentInput == VK_LEFT || context->currentInput == 'A') {
		context->playerYPosition++;
	}

	if (context->currentInput == VK_RIGHT || context->currentInput == 'D') {
		context->playerYPosition--;
	}

	//이 세상의 규칙
	if (context->playerXPosition < 0) context->playerXPosition = 0;
	if (context->playerXPosition > 10) context->playerXPosition = 10;

	if (context->playerYPosition < 0) context->playerXPosition = 0;
	if (context->playerYPosition > 10) context->playerXPosition = 10;
}

void Render(GameContext* context) {
	printf("\n\n\n\n\n");

	printf("========== GAME SCREEN ==========\n");
	printf(" Player X Position: %d\n", context->playerXPosition, ", Player Y Position: %d\n", context->playerYPosition);

	printf(" [");
	for (int i = 0; i <= 10; i++) {
		for (int j = 0; j <= 10; j++) {
			if (i == context->playerXPosition && j == context->playerYPosition) {
				printf("*");
			}

			else printf("_");
		}
	}
	printf("]\n");

	printf("=================================\n");
}

/*
int main() {
	GameContext game = { 5, 1, ' ' };

	printf("게임을 시작합니다. (정석 루프 패턴)\n");

	while (game.isRunning) {
		// 1. 입력: 사용자가 무엇을 했는가?
		ProcessInput(&game);

		// 2. 업데이트: 그 결과 세상은 어떻게 변했는가?
		Update(&game);

		// 3. 출력: 변한 세상을 화면에 그려라!
		Render(&game);
	}

	printf("게임이 안전하게 종료되었습니다.\n");

	return 0;
}
*/
