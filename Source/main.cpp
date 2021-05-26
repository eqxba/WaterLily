#include "Engine/Engine.hpp"

int main()
{
    Engine::Create();
    Engine::Run();
    Engine::Destroy();

    return 0;
}