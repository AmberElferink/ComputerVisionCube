#include <cstdlib>
#include <SDL2/SDL.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Calibration", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    if(window == nullptr)
        return EXIT_FAILURE;
    bool running = true;
    SDL_Event event;

    while(running)
    {
        //input
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT: //cross
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            running = false;
                            break;

                    }
            }
        }




    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}

