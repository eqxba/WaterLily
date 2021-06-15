#pragma once

#include "Engine/Window.hpp"

class RenderSystem;

class Engine
{
public:
    static void Create();
    static void Run();
    static void Destroy();

	// TODO: Temp
    static void OnResize(const Extent2D& newSize);
private:
    //void OnResize(const Extent2D& newSize);
	
    static std::unique_ptr<Window> window;
    static std::unique_ptr<RenderSystem> renderSystem;

    static bool renderingSuspended;
};