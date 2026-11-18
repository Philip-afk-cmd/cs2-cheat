#include "pch.h"
#include <d3d11.h>
#include <dxgi.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "MinHook.h"

// ==========================================
// SWIFT.FLY CS2 MENU STYLE CONFIGURATION
// ==========================================
namespace MenuConfig {
    constexpr UINT TOGGLE_KEY = VK_INSERT;
    constexpr float ROUNDING_MAIN = 0.0f;
    constexpr float ROUNDING_FRAME = 0.0f;

    // Swift.fly colors
    const ImVec4 AccentPurple = ImVec4(0.64f, 0.35f, 1.00f, 1.00f);
    const ImVec4 AccentPurpleHover = ImVec4(0.74f, 0.45f, 1.00f, 1.00f);
    const ImVec4 TextPrimary = ImVec4(0.92f, 0.92f, 0.95f, 1.00f);
    const ImVec4 TextSecondary = ImVec4(0.55f, 0.55f, 0.58f, 1.00f);
    const ImVec4 WindowBg = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
    const ImVec4 ChildBg = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    const ImVec4 FrameBg = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    const ImVec4 FrameBgHover = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    const ImVec4 FrameBgActive = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    const ImVec4 BorderColor = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    const ImVec4 HeaderColor = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    const ImVec4 TabActive = ImVec4(0.64f, 0.35f, 1.00f, 1.00f);
    const ImVec4 TabHover = ImVec4(0.64f, 0.35f, 1.00f, 0.60f);
    const ImVec4 TabInactive = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    const ImVec4 SliderGrab = ImVec4(0.64f, 0.35f, 1.00f, 1.00f);
    const ImVec4 CheckMark = ImVec4(0.64f, 0.35f, 1.00f, 1.00f);
    const ImVec4 ButtonColor = ImVec4(0.64f, 0.35f, 1.00f, 0.40f);
    const ImVec4 ButtonHover = ImVec4(0.64f, 0.35f, 1.00f, 0.70f);
    const ImVec4 ButtonActive = ImVec4(0.64f, 0.35f, 1.00f, 1.00f);
}

// Function Signatures & Globals
typedef HRESULT(__stdcall* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present oPresent = nullptr;

ID3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;
ID3D11RenderTargetView* g_pMainRenderTargetView = nullptr;
WNDPROC                 g_oWndProc = nullptr;
HWND                    g_Window = nullptr;
HMODULE                 g_hModule = nullptr;

bool g_Init = false;
bool g_ShowMenu = true;
bool g_UnloadRequested = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==========================================
// CLEANUP & UNHOOK ROUTINE
// ==========================================
DWORD WINAPI ExitThreadRoutine(LPVOID lpParam) {
    Sleep(100);

    if (g_Window && g_oWndProc) {
        SetWindowLongPtr(g_Window, GWLP_WNDPROC, (LONG_PTR)g_oWndProc);
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (g_Init) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    if (g_pMainRenderTargetView) { g_pMainRenderTargetView->Release(); g_pMainRenderTargetView = nullptr; }
    if (g_pContext) { g_pContext->Release(); g_pContext = nullptr; }
    if (g_pDevice) { g_pDevice->Release(); g_pDevice = nullptr; }

    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == MenuConfig::TOGGLE_KEY) {
        g_ShowMenu = !g_ShowMenu;
        return 0;
    }

    if (g_ShowMenu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        switch (uMsg) {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL:
        case WM_MOUSEMOVE:
        case WM_KEYDOWN:     case WM_KEYUP:
        case WM_CHAR:
            return TRUE;
        }
    }

    return CallWindowProc(g_oWndProc, hWnd, uMsg, wParam, lParam);
}

void ApplySwiftStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Sharp corners
    style.WindowRounding = MenuConfig::ROUNDING_MAIN;
    style.ChildRounding = MenuConfig::ROUNDING_FRAME;
    style.FrameRounding = MenuConfig::ROUNDING_FRAME;
    style.PopupRounding = MenuConfig::ROUNDING_FRAME;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = MenuConfig::ROUNDING_FRAME;

    // Swift.fly spacing
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 12.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    // Colors - Swift.fly style
    colors[ImGuiCol_Text] = MenuConfig::TextPrimary;
    colors[ImGuiCol_TextDisabled] = MenuConfig::TextSecondary;
    colors[ImGuiCol_WindowBg] = MenuConfig::WindowBg;
    colors[ImGuiCol_ChildBg] = MenuConfig::ChildBg;
    colors[ImGuiCol_PopupBg] = MenuConfig::WindowBg;
    colors[ImGuiCol_Border] = MenuConfig::BorderColor;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = MenuConfig::FrameBg;
    colors[ImGuiCol_FrameBgHovered] = MenuConfig::FrameBgHover;
    colors[ImGuiCol_FrameBgActive] = MenuConfig::FrameBgActive;
    colors[ImGuiCol_TitleBg] = MenuConfig::HeaderColor;
    colors[ImGuiCol_TitleBgActive] = MenuConfig::HeaderColor;
    colors[ImGuiCol_TitleBgCollapsed] = MenuConfig::HeaderColor;
    colors[ImGuiCol_MenuBarBg] = MenuConfig::HeaderColor;
    colors[ImGuiCol_ScrollbarBg] = MenuConfig::FrameBg;
    colors[ImGuiCol_ScrollbarGrab] = MenuConfig::FrameBgHover;
    colors[ImGuiCol_ScrollbarGrabHovered] = MenuConfig::FrameBgActive;
    colors[ImGuiCol_ScrollbarGrabActive] = MenuConfig::AccentPurple;
    colors[ImGuiCol_CheckMark] = MenuConfig::CheckMark;
    colors[ImGuiCol_SliderGrab] = MenuConfig::SliderGrab;
    colors[ImGuiCol_SliderGrabActive] = MenuConfig::AccentPurpleHover;
    colors[ImGuiCol_Button] = MenuConfig::ButtonColor;
    colors[ImGuiCol_ButtonHovered] = MenuConfig::ButtonHover;
    colors[ImGuiCol_ButtonActive] = MenuConfig::ButtonActive;
    colors[ImGuiCol_Header] = MenuConfig::AccentPurple;
    colors[ImGuiCol_HeaderHovered] = MenuConfig::ButtonHover;
    colors[ImGuiCol_HeaderActive] = MenuConfig::ButtonActive;
    colors[ImGuiCol_Separator] = MenuConfig::BorderColor;
    colors[ImGuiCol_SeparatorHovered] = MenuConfig::AccentPurple;
    colors[ImGuiCol_SeparatorActive] = MenuConfig::AccentPurpleHover;
    colors[ImGuiCol_ResizeGrip] = MenuConfig::FrameBg;
    colors[ImGuiCol_ResizeGripHovered] = MenuConfig::FrameBgHover;
    colors[ImGuiCol_ResizeGripActive] = MenuConfig::FrameBgActive;
    colors[ImGuiCol_Tab] = MenuConfig::TabInactive;
    colors[ImGuiCol_TabHovered] = MenuConfig::TabHover;
    colors[ImGuiCol_TabActive] = MenuConfig::TabActive;
    colors[ImGuiCol_TabUnfocused] = MenuConfig::TabInactive;
    colors[ImGuiCol_TabUnfocusedActive] = MenuConfig::TabInactive;
    colors[ImGuiCol_PlotLines] = MenuConfig::AccentPurple;
    colors[ImGuiCol_PlotLinesHovered] = MenuConfig::AccentPurpleHover;
    colors[ImGuiCol_PlotHistogram] = MenuConfig::AccentPurple;
    colors[ImGuiCol_PlotHistogramHovered] = MenuConfig::AccentPurpleHover;
    colors[ImGuiCol_TableHeaderBg] = MenuConfig::HeaderColor;
    colors[ImGuiCol_TableBorderStrong] = MenuConfig::BorderColor;
    colors[ImGuiCol_TableBorderLight] = MenuConfig::BorderColor;
    colors[ImGuiCol_TableRowBg] = MenuConfig::ChildBg;
    colors[ImGuiCol_TableRowBgAlt] = MenuConfig::FrameBg;
    colors[ImGuiCol_TextSelectedBg] = MenuConfig::AccentPurple;
    colors[ImGuiCol_DragDropTarget] = MenuConfig::AccentPurple;
    colors[ImGuiCol_NavHighlight] = MenuConfig::AccentPurple;
    colors[ImGuiCol_NavWindowingHighlight] = MenuConfig::AccentPurple;
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
}

// Custom square toggle button using ImGui::Checkbox with custom styling
bool SquareToggle(const char* label, bool* v) {
    // Use ImGui::Checkbox but override the drawing with our custom style
    bool pressed = ImGui::Checkbox(label, v);
    return pressed;
}

void RenderBackdrop(float displayWidth, float displayHeight) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(displayWidth, displayHeight));
    constexpr ImGuiWindowFlags backdropFlags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.20f));
    ImGui::Begin("##ModalBackdrop", nullptr, backdropFlags);
    ImGui::End();
    ImGui::PopStyleColor();
}

void RenderSwiftTabs() {
    ImGui::PushStyleColor(ImGuiCol_Text, MenuConfig::TextPrimary);

    if (!ImGui::BeginTabBar("SwiftTabBar", ImGuiTabBarFlags_NoTooltip)) {
        ImGui::PopStyleColor();
        return;
    }

    // Aim Tab
    if (ImGui::BeginTabItem("AIM")) {
        ImGui::Spacing();
        static bool aimbot = true;
        static bool silent = false;
        static bool smooth = true;
        static float smoothness = 50.0f;
        static float fov = 30.0f;

        // Custom styled checkbox - square with purple accent when enabled
        ImGui::PushStyleColor(ImGuiCol_CheckMark, MenuConfig::AccentPurple);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, MenuConfig::FrameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, MenuConfig::FrameBgHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, MenuConfig::FrameBgActive);

        // Make checkboxes square
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

        SquareToggle("Aimbot", &aimbot);
        SquareToggle("Silent Aim", &silent);
        SquareToggle("Smooth", &smooth);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        if (smooth) {
            ImGui::Indent();
            ImGui::SliderFloat("Smoothness", &smoothness, 0.0f, 100.0f, "%.0f%%");
            ImGui::Unindent();
        }
        ImGui::SliderFloat("FOV", &fov, 0.0f, 180.0f, "%.1f");
        ImGui::EndTabItem();
    }

    // Visuals Tab
    if (ImGui::BeginTabItem("VISUALS")) {
        ImGui::Spacing();
        static bool esp = true;
        static bool box = true;
        static bool health = true;
        static bool name = true;
        static bool skeleton = false;
        static bool glow = false;

        ImGui::PushStyleColor(ImGuiCol_CheckMark, MenuConfig::AccentPurple);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, MenuConfig::FrameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, MenuConfig::FrameBgHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, MenuConfig::FrameBgActive);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

        SquareToggle("ESP", &esp);
        if (esp) {
            ImGui::Indent();
            SquareToggle("Box", &box);
            SquareToggle("Health Bar", &health);
            SquareToggle("Name", &name);
            SquareToggle("Skeleton", &skeleton);
            ImGui::Unindent();
        }
        SquareToggle("Glow", &glow);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::EndTabItem();
    }

    // Misc Tab
    if (ImGui::BeginTabItem("MISC")) {
        ImGui::Spacing();
        static bool bhop = true;
        static bool triggerbot = false;
        static bool autoStrafe = false;
        static bool radar = true;

        ImGui::PushStyleColor(ImGuiCol_CheckMark, MenuConfig::AccentPurple);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, MenuConfig::FrameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, MenuConfig::FrameBgHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, MenuConfig::FrameBgActive);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

        SquareToggle("Bunny Hop", &bhop);
        SquareToggle("Triggerbot", &triggerbot);
        SquareToggle("Auto Strafe", &autoStrafe);
        SquareToggle("Radar", &radar);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::EndTabItem();
    }

    // Config Tab
    if (ImGui::BeginTabItem("CONFIG")) {
        ImGui::Spacing();
        static char configName[64] = "default";
        ImGui::InputText("Config Name", configName, sizeof(configName));
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, MenuConfig::ButtonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, MenuConfig::ButtonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, MenuConfig::ButtonActive);

        if (ImGui::Button("Load", ImVec2(80, 25))) { /* Load logic */ }
        ImGui::SameLine();
        if (ImGui::Button("Save", ImVec2(80, 25))) { /* Save logic */ }
        ImGui::SameLine();
        if (ImGui::Button("Reset", ImVec2(80, 25))) { /* Reset logic */ }

        ImGui::PopStyleColor(3);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(MenuConfig::TextSecondary, "Unload Menu");
        if (ImGui::Button("Unload", ImVec2(120, 28))) {
            g_UnloadRequested = true;
        }
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::PopStyleColor();
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_Init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice))) {
            g_pDevice->GetImmediateContext(&g_pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_Window = sd.OutputWindow;

            ID3D11Texture2D* pBackBuffer = nullptr;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            if (pBackBuffer) {
                g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pMainRenderTargetView);
                pBackBuffer->Release();
            }

            g_oWndProc = (WNDPROC)SetWindowLongPtr(g_Window, GWLP_WNDPROC, (LONG_PTR)WndProc);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ApplySwiftStyle();

            ImGui_ImplWin32_Init(g_Window);
            ImGui_ImplDX11_Init(g_pDevice, g_pContext);

            g_Init = true;
        }
        else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    if (g_UnloadRequested) {
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (!g_pMainRenderTargetView) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        if (pBackBuffer) {
            g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pMainRenderTargetView);
            pBackBuffer->Release();
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::GetIO().MouseDrawCursor = true;
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

        RECT rect;
        GetClientRect(g_Window, &rect);
        const float displayWidth = static_cast<float>(rect.right - rect.left);
        const float displayHeight = static_cast<float>(rect.bottom - rect.top);

        RenderBackdrop(displayWidth, displayHeight);

        // Swift.fly style window - fixed rectangular size
        const float menuWidth = 520.0f;
        const float menuHeight = 480.0f;
        ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2((displayWidth - menuWidth) * 0.5f, (displayHeight - menuHeight) * 0.35f), ImGuiCond_FirstUseEver);

        ImGui::Begin("swift.fly", &g_ShowMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        // Custom title bar
        ImGui::PushStyleColor(ImGuiCol_Text, MenuConfig::TextPrimary);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        // Title with purple accent
        ImGui::TextColored(MenuConfig::AccentPurple, "swift.fly");
        ImGui::SameLine();
        ImGui::TextColored(MenuConfig::TextSecondary, "|  CS2");

        // Close button
        ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, MenuConfig::TextSecondary);
        if (ImGui::Button("x", ImVec2(20, 20))) {
            g_ShowMenu = false;
        }
        ImGui::PopStyleColor();

        ImGui::PopStyleColor(4);
        ImGui::Separator();
        ImGui::Spacing();

        RenderSwiftTabs();

        ImGui::End();
    }
    else {
        ImGui::GetIO().MouseDrawCursor = false;
    }

    ImGui::Render();
    g_pContext->OMSetRenderTargets(1, &g_pMainRenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (g_UnloadRequested) {
        g_ShowMenu = false;
        ImGui::GetIO().MouseDrawCursor = false;
        CreateThread(nullptr, 0, ExitThreadRoutine, nullptr, 0, nullptr);
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    g_hModule = (HMODULE)lpParam;

    while (true) {
        WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L, GetModuleHandleA(nullptr), nullptr, nullptr, nullptr, nullptr, "DummyDX11Window", nullptr };
        if (!RegisterClassExA(&wc)) { Sleep(100); continue; }

        HWND dummyWindow = CreateWindowA("DummyDX11Window", "DX11 Dummy", WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, nullptr, nullptr, wc.hInstance, nullptr);
        if (!dummyWindow) { UnregisterClassA("DummyDX11Window", wc.hInstance); Sleep(100); continue; }

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.Width = 300;
        sd.BufferDesc.Height = 300;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = dummyWindow;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;

        D3D_FEATURE_LEVEL fl;
        IDXGISwapChain* pSwapChain = nullptr;
        ID3D11Device* pDummyDevice = nullptr;
        ID3D11DeviceContext* pDummyContext = nullptr;

        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd, &pSwapChain, &pDummyDevice, &fl, &pDummyContext) == S_OK) {
            void** pVTable = *reinterpret_cast<void***>(pSwapChain);

            if (MH_Initialize() == MH_OK) {
                if (MH_CreateHook(pVTable[8], &hkPresent, reinterpret_cast<void**>(&oPresent)) == MH_OK) {
                    MH_EnableHook(pVTable[8]);
                }
            }

            pSwapChain->Release();
            pDummyDevice->Release();
            pDummyContext->Release();
            DestroyWindow(dummyWindow);
            UnregisterClassA("DummyDX11Window", wc.hInstance);
            break;
        }

        DestroyWindow(dummyWindow);
        UnregisterClassA("DummyDX11Window", wc.hInstance);
        Sleep(200);
    }

    return 0;
}