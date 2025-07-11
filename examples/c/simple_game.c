#include "NexusC.h"
#include <stdio.h>
#include <math.h>

// Game state
typedef struct {
    NexusVector3 playerPosition;
    float rotation;
    int score;
} GameState;

static GameState gameState = {{0, 0, 0}, 0, 0};

void update_game(float deltaTime, void* userData) {
    NexusEngine* engine = (NexusEngine*)userData;
    NexusInput* input = nexus_engine_get_input(engine);
    
    // Move player based on input
    if (nexus_input_is_key_down(input, NEXUS_KEY_W)) {
        gameState.playerPosition.z += 5.0f * deltaTime;
    }
    if (nexus_input_is_key_down(input, NEXUS_KEY_S)) {
        gameState.playerPosition.z -= 5.0f * deltaTime;
    }
    if (nexus_input_is_key_down(input, NEXUS_KEY_A)) {
        gameState.playerPosition.x -= 5.0f * deltaTime;
    }
    if (nexus_input_is_key_down(input, NEXUS_KEY_D)) {
        gameState.playerPosition.x += 5.0f * deltaTime;
    }
    
    // Rotate player
    gameState.rotation += deltaTime;
    
    // Update score
    gameState.score++;
}

void render_game(NexusGraphics* graphics, void* userData) {
    // Clear screen
    NexusColor clearColor = {0.2f, 0.3f, 0.8f, 1.0f};
    nexus_graphics_clear(graphics, clearColor);
    
    // Draw player as a cube
    NexusVector3 cubeSize = {1.0f, 1.0f, 1.0f};
    NexusColor cubeColor = {1.0f, 0.5f, 0.0f, 1.0f};
    nexus_graphics_draw_cube(graphics, gameState.playerPosition, cubeSize, cubeColor);
    
    // Draw some environment
    for (int i = -10; i <= 10; i += 2) {
        for (int j = -10; j <= 10; j += 2) {
            if (i != 0 || j != 0) {
                NexusVector3 pos = {(float)i, -1.0f, (float)j};
                NexusVector3 size = {0.5f, 0.1f, 0.5f};
                NexusColor color = {0.3f, 0.7f, 0.3f, 1.0f};
                nexus_graphics_draw_cube(graphics, pos, size, color);
            }
        }
    }
    
    // Draw UI text
    char scoreText[32];
    sprintf(scoreText, "Score: %d", gameState.score);
    NexusColor textColor = {1.0f, 1.0f, 1.0f, 1.0f};
    nexus_graphics_draw_text(graphics, scoreText, 10, 10, textColor);
    
    nexus_graphics_draw_text(graphics, "WASD to move, ESC to quit", 10, 30, textColor);
}

int main() {
    printf("Starting Nexus Engine C Game...\n");
    
    // Create and initialize engine
    NexusEngine* engine = nexus_engine_create();
    if (!engine) {
        printf("Failed to create engine!\n");
        return -1;
    }
    
    if (!nexus_engine_initialize(engine)) {
        printf("Failed to initialize engine!\n");
        nexus_engine_destroy(engine);
        return -1;
    }
    
    // Set up callbacks
    nexus_engine_set_update_callback(engine, update_game, engine);
    nexus_engine_set_render_callback(engine, render_game, NULL);
    
    printf("Engine initialized! Starting game loop...\n");
    
    // Run the game
    nexus_engine_run(engine);
    
    // Cleanup
    nexus_engine_shutdown(engine);
    nexus_engine_destroy(engine);
    
    printf("Game ended. Goodbye!\n");
    return 0;
}