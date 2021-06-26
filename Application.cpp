#include "Application.hpp"

#include "Mouse.hpp"
#include "Keyboard.hpp"
#include "Display.hpp"

static bool s_Initialized = false;
static HINSTANCE s_Instance = nullptr;
static bool isRunning = false;

static Graphics* s_Graphics = nullptr;

static Display* display = nullptr;

static void keyH(void) { Log::Info("Hallo Welt"); }
static void keyX(void) { Log::Error("Eine MessageBox"); }
static void keyEsc(void) { if (display) display->RequestStop(); }
static void keyF11(void) { if (display) display->SwitchDisplayMode(); }

bool Application::Initialize(HINSTANCE instance) noexcept
{
	if (s_Initialized) return true;

	s_Instance = instance;

  DisplayCreateInfo dci;

  dci.Width = 1600;
  dci.Height = 900;
  dci.Fullscreen = false;
  dci.Title = L"Window Title";

  try
  {
    display = new Display(&dci);
  }
  catch (const std::runtime_error re) { Log::Error(re.what()); return false; }

  Mouse::Init(display->Handle());
  Keyboard::Init();

  s_Graphics = new Graphics();

  if (!s_Graphics->Init(display->Handle(), dci.Width, dci.Height)) return false;

	s_Initialized = true;

  return true;
}

void Application::Finish(void) noexcept
{
  s_Graphics->Release();

  delete display;
}

void Application::Start(void) noexcept
{
	isRunning = true;

  while (isRunning)
  {
    Mouse::Reset();
    Keyboard::Update();

    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    /*
    #########################
    TEMPORÄR, DIE KEY ABFRAGEN HIER
    #########################
    */
    if (Keyboard::IsPressed(sc_h)) keyH();
    if (Keyboard::IsReleased(sc_x)) keyX();
    if (Keyboard::IsReleased(sc_f11)) keyF11();
    if (Keyboard::IsReleased(sc_escape)) keyEsc();

    if (!s_Graphics->Render()) return;
  }
}

void Application::Stop(void) noexcept
{
	isRunning = false;
}

HINSTANCE Application::WinAppInstance(void) noexcept
{
	return s_Instance;
}

HWND Application::WindowHandle(void) noexcept
{
  return display->Handle();
}

Graphics* Application::GetGraphics(void) noexcept { return s_Graphics; }
