#include "ImGuiController.h"

#include "../../Main.h"

#include "../../ImGui/Code/imgui.h"
#include "../../ImGui/Code/imgui_impl_win32.h"
#include "../../ImGui/Code/imgui_impl_dx12.h"

#pragma comment(lib, "x64/Debug/imgui.lib")

#include "RenderManager.h"

using namespace EngineCore;
using namespace GUIController::Gui;

void ImGuiController::Initialize() {

    auto renderManager = Render::RenderManager::GetInstance();
    assert(renderManager);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ItemSpacing.y = 8.0f;
	style.WindowPadding = ImVec2(1.0f, 1.0f);
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(GetWindow());

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = renderManager->GetDevice();
    initInfo.CommandQueue = renderManager->GetCommandQueue();
    initInfo.NumFramesInFlight = 2;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.UserData = renderManager;

    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        auto* renderManager = static_cast<Render::RenderManager*>(info->UserData);
        assert(renderManager);

        const unsigned int srvIndex = renderManager->CreateShaderResourceView(nullptr);

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = renderManager->GetSRVDescriptorCPUHandle();
        const unsigned int descriptorSize = renderManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        cpuHandle.ptr += static_cast<SIZE_T>(descriptorSize) * srvIndex;

        *out_cpu_desc_handle = cpuHandle;
        *out_gpu_desc_handle = renderManager->GetShaderResourceViewHandle(srvIndex);
    };

    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
    {
        (void)gpu_desc_handle;

        auto* renderManager = static_cast<Render::RenderManager*>(info->UserData);
        assert(renderManager);

        const unsigned int srvIndex = renderManager->GetShaderResourceViewIndex(cpu_desc_handle);
        renderManager->ReleaseShaderResourceView(srvIndex);
    };

    initInfo.SrvDescriptorHeap = renderManager->GetSRVDescriptorHeap();
    assert(ImGui_ImplDX12_Init(&initInfo));
}

void ImGuiController::BeginFrame() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawDockspace();
}

void ImGuiController::EndFrame() {

    auto renderManager = Render::RenderManager::GetInstance();

    ImGui::Render();

    ID3D12DescriptorHeap* heaps[] = { renderManager->GetSRVDescriptorHeap() };
    renderManager->GetGraphicsCommandList()->SetDescriptorHeaps(1, heaps);

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderManager->GetGraphicsCommandList());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void ImGuiController::Finalize() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiController::DrawDockspace() {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->WorkPos);
    ImGui::SetNextWindowSize(mainViewport->WorkSize);

    ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);

    m_DockspaceID = ImGui::GetID("DockSpace");
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(m_DockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);

    ImGui::End();
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ImGuiController::WindowProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
