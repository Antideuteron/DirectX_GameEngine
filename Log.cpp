#include <iostream>

void Log::Info(std::string message)
{
	Log::Info(std::wstring(message.begin(), message.end()));
}

void Log::Info(std::wstring message)
{
	OutputDebugString((std::wstringstream() << L"INFO: " << message << '\n').str().c_str());
}

void Log::Error(std::string message)
{
	Log::Error(std::wstring(message.begin(), message.end()));
}

void Log::Error(std::wstring message)
{
	OutputDebugString((std::wstringstream() << L"ERROR: " << message << '\n').str().c_str());
	
	MessageBox(nullptr, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
}