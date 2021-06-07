#pragma once

class Window;
class RenderSystem;

class Engine
{
public:
    static void Create();
    static void Run();
    static void Destroy();

private:
    static std::unique_ptr<Window> window;
    static std::unique_ptr<RenderSystem> renderSystem;
};