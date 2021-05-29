#pragma once

class Window;

class Engine
{
public:
    static void Create();
    static void Run();
    static void Destroy();

    static std::unique_ptr<Window> window;
};