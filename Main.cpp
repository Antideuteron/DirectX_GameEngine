
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hprevinst, LPSTR cmdline, int ncmdshow)
{
  if (Application::Initialize(hInstance))
  {
    Application::Start();
  }

  Application::Finish();

	return 0;
}
