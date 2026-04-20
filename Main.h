#pragma once

#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <memory>

#include <array>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <algorithm>

#include <string>
#include <cassert>

#include <io.h>
#include <typeinfo>

#include <Windows.h>
#include <mmsystem.h>

#include <d3d12.h>
#include <d3d12shader.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using DirectX::XMFLOAT2;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;


#include <Xinput.h>





#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment (lib, "xinput.lib")


namespace EngineCore {
	HWND GetWindow();
}

