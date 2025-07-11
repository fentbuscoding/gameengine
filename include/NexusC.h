#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Core types
typedef struct NexusEngine NexusEngine;
typedef struct NexusGraphics NexusGraphics;
typedef struct NexusAudio NexusAudio;
typedef struct NexusInput NexusInput;
typedef struct NexusPhysics NexusPhysics;

// Math types for C
typedef struct {
    float x, y, z;
} NexusVector3;

typedef struct {
    float x, y, z, w;
} NexusVector4;

typedef struct {
    float m[16]; // 4x4 matrix in row-major order
} NexusMatrix4;

typedef struct {
    float r, g, b, a;
} NexusColor;

// Engine management
NexusEngine* nexus_engine_create(void);
bool nexus_engine_initialize(NexusEngine* engine);
void nexus_engine_run(NexusEngine* engine);
void nexus_engine_shutdown(NexusEngine* engine);
void nexus_engine_destroy(NexusEngine* engine);

bool nexus_engine_is_running(NexusEngine* engine);
void nexus_engine_request_exit(NexusEngine* engine);
float nexus_engine_get_delta_time(NexusEngine* engine);
int nexus_engine_get_fps(NexusEngine* engine);
void nexus_engine_set_target_fps(NexusEngine* engine, float fps);

// Graphics API
NexusGraphics* nexus_engine_get_graphics(NexusEngine* engine);
void nexus_graphics_begin_frame(NexusGraphics* graphics);
void nexus_graphics_end_frame(NexusGraphics* graphics);
void nexus_graphics_present(NexusGraphics* graphics);
void nexus_graphics_clear(NexusGraphics* graphics, NexusColor color);
void nexus_graphics_set_viewport(NexusGraphics* graphics, int x, int y, int width, int height);

// Basic rendering
void nexus_graphics_draw_line(NexusGraphics* graphics, NexusVector3 start, NexusVector3 end, NexusColor color);
void nexus_graphics_draw_cube(NexusGraphics* graphics, NexusVector3 position, NexusVector3 size, NexusColor color);
void nexus_graphics_draw_sphere(NexusGraphics* graphics, NexusVector3 position, float radius, NexusColor color);
void nexus_graphics_draw_text(NexusGraphics* graphics, const char* text, int x, int y, NexusColor color);

// Input API
NexusInput* nexus_engine_get_input(NexusEngine* engine);
bool nexus_input_is_key_pressed(NexusInput* input, int keyCode);
bool nexus_input_is_key_down(NexusInput* input, int keyCode);
bool nexus_input_is_key_released(NexusInput* input, int keyCode);
bool nexus_input_is_mouse_button_pressed(NexusInput* input, int button);
void nexus_input_get_mouse_position(NexusInput* input, int* x, int* y);

// Audio API
NexusAudio* nexus_engine_get_audio(NexusEngine* engine);
void nexus_audio_play_sound(NexusAudio* audio, const char* filename);
void nexus_audio_play_music(NexusAudio* audio, const char* filename);
void nexus_audio_stop_music(NexusAudio* audio);
void nexus_audio_set_master_volume(NexusAudio* audio, float volume);

// Physics API
NexusPhysics* nexus_engine_get_physics(NexusEngine* engine);
int nexus_physics_create_box(NexusPhysics* physics, NexusVector3 position, NexusVector3 size, float mass);
int nexus_physics_create_sphere(NexusPhysics* physics, NexusVector3 position, float radius, float mass);
void nexus_physics_set_position(NexusPhysics* physics, int objectId, NexusVector3 position);
NexusVector3 nexus_physics_get_position(NexusPhysics* physics, int objectId);
void nexus_physics_apply_force(NexusPhysics* physics, int objectId, NexusVector3 force);

// Math utilities
NexusVector3 nexus_vector3_add(NexusVector3 a, NexusVector3 b);
NexusVector3 nexus_vector3_subtract(NexusVector3 a, NexusVector3 b);
NexusVector3 nexus_vector3_multiply(NexusVector3 v, float scalar);
float nexus_vector3_dot(NexusVector3 a, NexusVector3 b);
NexusVector3 nexus_vector3_cross(NexusVector3 a, NexusVector3 b);
float nexus_vector3_length(NexusVector3 v);
NexusVector3 nexus_vector3_normalize(NexusVector3 v);

NexusMatrix4 nexus_matrix4_identity(void);
NexusMatrix4 nexus_matrix4_multiply(NexusMatrix4 a, NexusMatrix4 b);
NexusMatrix4 nexus_matrix4_translate(NexusVector3 translation);
NexusMatrix4 nexus_matrix4_rotate(NexusVector3 axis, float angle);
NexusMatrix4 nexus_matrix4_scale(NexusVector3 scale);

// Callback types for C
typedef void (*NexusUpdateCallback)(float deltaTime, void* userData);
typedef void (*NexusRenderCallback)(NexusGraphics* graphics, void* userData);
typedef void (*NexusInputCallback)(NexusInput* input, void* userData);

// Event system
void nexus_engine_set_update_callback(NexusEngine* engine, NexusUpdateCallback callback, void* userData);
void nexus_engine_set_render_callback(NexusEngine* engine, NexusRenderCallback callback, void* userData);
void nexus_engine_set_input_callback(NexusEngine* engine, NexusInputCallback callback, void* userData);

// Resource loading
int nexus_graphics_load_texture(NexusGraphics* graphics, const char* filename);
int nexus_graphics_load_model(NexusGraphics* graphics, const char* filename);
void nexus_graphics_draw_model(NexusGraphics* graphics, int modelId, NexusMatrix4 transform);

// Key codes (subset of common keys)
#define NEXUS_KEY_SPACE         32
#define NEXUS_KEY_A             65
#define NEXUS_KEY_B             66
#define NEXUS_KEY_C             67
#define NEXUS_KEY_D             68
#define NEXUS_KEY_E             69
#define NEXUS_KEY_F             70
#define NEXUS_KEY_G             71
#define NEXUS_KEY_H             72
#define NEXUS_KEY_I             73
#define NEXUS_KEY_J             74
#define NEXUS_KEY_K             75
#define NEXUS_KEY_L             76
#define NEXUS_KEY_M             77
#define NEXUS_KEY_N             78
#define NEXUS_KEY_O             79
#define NEXUS_KEY_P             80
#define NEXUS_KEY_Q             81
#define NEXUS_KEY_R             82
#define NEXUS_KEY_S             83
#define NEXUS_KEY_T             84
#define NEXUS_KEY_U             85
#define NEXUS_KEY_V             86
#define NEXUS_KEY_W             87
#define NEXUS_KEY_X             88
#define NEXUS_KEY_Y             89
#define NEXUS_KEY_Z             90
#define NEXUS_KEY_ESCAPE        27
#define NEXUS_KEY_ENTER         13
#define NEXUS_KEY_TAB           9
#define NEXUS_KEY_SHIFT         16
#define NEXUS_KEY_CTRL          17
#define NEXUS_KEY_ALT           18
#define NEXUS_KEY_LEFT          37
#define NEXUS_KEY_UP            38
#define NEXUS_KEY_RIGHT         39
#define NEXUS_KEY_DOWN          40

// Mouse buttons
#define NEXUS_MOUSE_LEFT        0
#define NEXUS_MOUSE_RIGHT       1
#define NEXUS_MOUSE_MIDDLE      2

#ifdef __cplusplus
}
#endif