#include<Windows.h>
#include<cstdint>
#include<d3d12.h>
#include<dxgi1_6.h>
#include <string>
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

//�E�B���h�E�v���V�[�W��
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//���b�Z�[�W�ɉ����ăQ�[���ŗL�̏������s��
	switch (msg)
	{
		//�E�B���h�E���j�����ꂽ
	case WM_DESTROY:
		//OS�ɑ΂��āA�A�v���̏I����`����
		PostQuitMessage(0);
		return 0;
	}

	//�W���̃��b�Z�[�W�������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);

	/*---�E�B���h�E�N���X�̓o�^---*/
	WNDCLASS wc{};
	//�E�B���h�E�v���V�[�W��
	wc.lpfnWndProc = WindowProc;
	//�E�B���h�E�N���X��
	wc.lpszClassName = L"CG2MyWindowClass";
	//�C���X�^���X�n���h��
	wc.hInstance = GetModuleHandle(nullptr);
	//�J�[�\��
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//�E�B���h�E�N���X�̓o�^
	RegisterClass(&wc);

	//�N���C�A���g�̈�̃T�C�Y
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//�E�B���h�T�C�Y��\���\���̂ɃN���C�A���g�̈�̃T�C�Y������
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	//�N���C�A���g�̈�̃T�C�Y���E�B���h�E�T�C�Y�ɕϊ�����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//�E�B���h�E�̐���
	hwnd = CreateWindow(
		wc.lpszClassName,     //�N���X��
		L"CG2MyWindow",       //�^�C�g���o�[��
		WS_OVERLAPPEDWINDOW,  //�E�B���h�E�X�^�C��
		CW_USEDEFAULT,        //�\����X�W(Windows�ɔC����)
		CW_USEDEFAULT,        //�\��Y���W(WindowsOS�ɔC����)
		wrc.right - wrc.left, //�E�B���h�E����
		wrc.bottom - wrc.top, //�E�B���h�E�c��
		nullptr,              //�e�E�B���h�E�n���h��
		nullptr,              //���j���[�n���h��
		wc.hInstance,         //�C���X�^���X�n���h��
		nullptr               //�I�v�V����
	);

	//�E�B���h�E�̕\��
	ShowWindow(hwnd, SW_SHOW);
}



	//DXGI�t�@�N�g���[�̐���
	IDXGIFactory6* dxgiFactory = nullptr;

	/*HRESULT��Windows�n�̃G���[�R�[�h�ł���
	 �֐�������������SUCCEEDED�}�N���Ŕ���ł���*/
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	/*�������̍��{�I�ȕ����ŃG���[���o���ꍇ���v���O�������Ԉ���Ă��邩�A
	�ǂ��ɂ��o���Ȃ��ꍇ�������̂�assert�ɂ��Ă���*/
	assert(SUCCEEDED(hr));


	MSG msg{};

	//�E�B���h�E��x�{�^�����������܂Ń��[�v
	while (msg.message != WM_QUIT)
	{
		//windows�Ƀ��b�Z�[�W��������ŗD��ŏ�������
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//�Q�[���̍X�V����
			//�Q�[���̕`�揈��
		}
	}

	return 0;
}


void Log(const std::string& message)
{
	//�W���o�͂Ƀ��b�Z�[�W���o��
	OutputDebugStringA(message.c_str());
}
