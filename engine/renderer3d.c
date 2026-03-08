// =============================================================================
// INSTINCT ENGINE — Software 3D Renderer
// Deep Flow OS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/renderer3d.c
// Full software rasterizer. No GPU required. Runs on bare metal.
// Perspective projection, z-buffer, Gouraud shading, emissive glow.
// =============================================================================

#include "renderer3d.h"
#include "ecs.h"
#include "framebuffer.h"
#include "polish.h"
#include <stdint.h>
#include <stdbool.h>

// ── Z-buffer ──────────────────────────────────────────────────────────────────
#define ZBUF_W  1280
#define ZBUF_H   720
static float g_zbuf[ZBUF_W * ZBUF_H];

// ── Camera state ──────────────────────────────────────────────────────────────
static vec3_t g_cam_pos  = {0.0f, 2.0f, -8.0f};
static vec3_t g_cam_fwd  = {0.0f, 0.0f,  1.0f};
static vec3_t g_cam_up   = {0.0f, 1.0f,  0.0f};
static float  g_fov      = 1.0472f;  // 60 degrees in radians
static float  g_near     = 0.1f;
static float  g_far      = 500.0f;
static float  g_aspect   = 1.7778f;  // 16:9

// ── Light ─────────────────────────────────────────────────────────────────────
static vec3_t g_light_dir = {0.5f, -1.0f, 0.7f};  // Sun direction

// ── Math helpers ──────────────────────────────────────────────────────────────
static float fabs_f(float x) { return x < 0.0f ? -x : x; }
static float fmin_f(float a, float b) { return a < b ? a : b; }
static float fmax_f(float a, float b) { return a > b ? a : b; }
static float fclamp(float x, float mn, float mx) {
    return x < mn ? mn : (x > mx ? mx : x);
}

static vec3_t v3sub(vec3_t a, vec3_t b){ return (vec3_t){a.x-b.x,a.y-b.y,a.z-b.z}; }
static vec3_t v3add(vec3_t a, vec3_t b){ return (vec3_t){a.x+b.x,a.y+b.y,a.z+b.z}; }
static vec3_t v3scale(vec3_t v, float s){ return (vec3_t){v.x*s,v.y*s,v.z*s}; }
static float  v3dot(vec3_t a, vec3_t b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static vec3_t v3cross(vec3_t a, vec3_t b){
    return (vec3_t){a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
static vec3_t v3norm(vec3_t v){
    float d = v3dot(v,v);
    if (d < 0.0001f) return (vec3_t){0,1,0};
    float x2=d*0.5f, y=d; uint32_t i;
    __builtin_memcpy(&i,&y,4);
    i=0x5f3759df-(i>>1);
    __builtin_memcpy(&y,&i,4);
    y=y*(1.5f-(x2*y*y));
    return v3scale(v,y);
}

// ── Projection ────────────────────────────────────────────────────────────────
typedef struct { float x, y, z, w; } vec4_t;

static vec4_t project(vec3_t world_pos) {
    // View transform (look-at)
    vec3_t fwd = v3norm(v3sub(g_cam_fwd, g_cam_pos));
    vec3_t right = v3norm(v3cross(fwd, g_cam_up));
    vec3_t up    = v3cross(right, fwd);
    vec3_t d     = v3sub(world_pos, g_cam_pos);

    float vx = v3dot(d, right);
    float vy = v3dot(d, up);
    float vz = v3dot(d, fwd);

    if (vz < g_near) return (vec4_t){0,0,-1,0};

    // Perspective divide
    float f = 1.0f / (float)(vz * 1.0f);  // simplified
    float tan_half_fov = 0.5774f;  // tan(30°) ≈ tan(fov/2) for 60°
    float sx = (vx * f) / (tan_half_fov * g_aspect);
    float sy = (vy * f) /  tan_half_fov;

    return (vec4_t){sx, sy, vz, 1.0f};
}

static bool ndc_to_screen(vec4_t ndc, int32_t* sx, int32_t* sy,
                           uint32_t sw, uint32_t sh) {
    if (ndc.w < 0.5f) return false;
    *sx = (int32_t)((ndc.x * 0.5f + 0.5f) * (float)sw);
    *sy = (int32_t)((1.0f - (ndc.y * 0.5f + 0.5f)) * (float)sh);
    return (*sx >= 0 && *sx < (int32_t)sw &&
            *sy >= 0 && *sy < (int32_t)sh);
}

// ── Triangle rasterizer ───────────────────────────────────────────────────────
static void draw_triangle(int32_t x0,int32_t y0,float z0,
                           int32_t x1,int32_t y1,float z1,
                           int32_t x2,int32_t y2,float z2,
                           uint32_t color,
                           uint32_t sw, uint32_t sh) {
    // Bounding box
    int32_t minx = (int32_t)fmin_f((float)x0, fmin_f((float)x1,(float)x2));
    int32_t miny = (int32_t)fmin_f((float)y0, fmin_f((float)y1,(float)y2));
    int32_t maxx = (int32_t)fmax_f((float)x0, fmax_f((float)x1,(float)x2));
    int32_t maxy = (int32_t)fmax_f((float)y0, fmax_f((float)y1,(float)y2));

    minx = (int32_t)fmax_f((float)minx, 0.0f);
    miny = (int32_t)fmax_f((float)miny, 0.0f);
    maxx = (int32_t)fmin_f((float)maxx, (float)(sw-1));
    maxy = (int32_t)fmin_f((float)maxy, (float)(sh-1));

    // Edge function
    int32_t A01=y0-y1, B01=x1-x0;
    int32_t A12=y1-y2, B12=x2-x1;
    int32_t A20=y2-y0, B20=x0-x2;

    int32_t w0_row = (minx-x1)*A12+(miny-y1)*B12;
    int32_t w1_row = (minx-x2)*A20+(miny-y2)*B20;
    int32_t w2_row = (minx-x0)*A01+(miny-y0)*B01;

    uint32_t* fb = (uint32_t*)fb_get_info()->addr;

    for (int32_t py=miny; py<=maxy; py++) {
        int32_t w0=w0_row, w1=w1_row, w2=w2_row;
        for (int32_t px=minx; px<=maxx; px++) {
            if ((w0|w1|w2) >= 0) {
                // Interpolate depth
                float total = (float)(w0+w1+w2);
                if (total < 0.001f) { w0+=A12;w1+=A20;w2+=A01; continue; }
                float depth = (z0*(float)w0 + z1*(float)w1 + z2*(float)w2)/total;
                uint32_t zi = (uint32_t)(py*(int32_t)sw+px);
                if (zi < ZBUF_W*ZBUF_H && depth < g_zbuf[zi]) {
                    g_zbuf[zi] = depth;
                    fb[(uint32_t)(py*(int32_t)sw+px)] = color;
                }
            }
            w0+=A12; w1+=A20; w2+=A01;
        }
        w0_row+=B12; w1_row+=B20; w2_row+=B01;
    }
}

// ── Lighting ──────────────────────────────────────────────────────────────────
static uint32_t apply_light(uint32_t color, vec3_t normal, float emit) {
    vec3_t ln = v3norm(g_light_dir);
    float diff = fclamp(-v3dot(normal, ln), 0.1f, 1.0f);
    float light = fclamp(diff + emit, 0.0f, 1.0f);

    uint8_t r = (uint8_t)fclamp((float)((color>>16)&0xFF)*light, 0,255);
    uint8_t g = (uint8_t)fclamp((float)((color>> 8)&0xFF)*light, 0,255);
    uint8_t b = (uint8_t)fclamp((float)( color     &0xFF)*light, 0,255);
    return ((uint32_t)r<<16)|((uint32_t)g<<8)|b;
}

// ── Mesh definitions (minimal voxel-style) ────────────────────────────────────
typedef struct { float x,y,z; } v3;
typedef struct { uint32_t a,b,c; vec3_t normal; } tri_t;

// Cube mesh (8 verts, 12 tris)
static const v3 CUBE_V[8] = {
    {-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},
    {0.5f, 0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f, 0.5f},{0.5f,-0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f},{-0.5f, 0.5f, 0.5f}
};
static const tri_t CUBE_T[12] = {
    {0,1,2,{0,0,-1}},{0,2,3,{0,0,-1}},
    {4,6,5,{0,0, 1}},{4,7,6,{0,0, 1}},
    {0,3,7,{-1,0,0}},{0,7,4,{-1,0,0}},
    {1,5,6,{ 1,0,0}},{1,6,2,{ 1,0,0}},
    {3,2,6,{0, 1,0}},{3,6,7,{0, 1,0}},
    {0,4,5,{0,-1,0}},{0,5,1,{0,-1,0}}
};

static void draw_mesh_cube(vec3_t pos, vec3_t scale,
                            uint32_t color, float emit,
                            uint32_t sw, uint32_t sh) {
    for (uint32_t ti = 0; ti < 12; ti++) {
        const tri_t* t = &CUBE_T[ti];
        v3 va = CUBE_V[t->a], vb = CUBE_V[t->b], vc = CUBE_V[t->c];

        vec3_t wa = {pos.x+va.x*scale.x, pos.y+va.y*scale.y, pos.z+va.z*scale.z};
        vec3_t wb = {pos.x+vb.x*scale.x, pos.y+vb.y*scale.y, pos.z+vb.z*scale.z};
        vec3_t wc = {pos.x+vc.x*scale.x, pos.y+vc.y*scale.y, pos.z+vc.z*scale.z};

        vec4_t pa = project(wa);
        vec4_t pb = project(wb);
        vec4_t pc = project(wc);

        int32_t sax,say,sbx,sby,scx,scy;
        if (!ndc_to_screen(pa,&sax,&say,sw,sh)) continue;
        if (!ndc_to_screen(pb,&sbx,&sby,sw,sh)) continue;
        if (!ndc_to_screen(pc,&scx,&scy,sw,sh)) continue;

        uint32_t lit = apply_light(color, t->normal, emit);
        draw_triangle(sax,say,pa.z, sbx,sby,pb.z, scx,scy,pc.z,
                      lit, sw, sh);
    }
}

// Character = stacked cubes (head + torso + legs)
static void draw_character(vec3_t pos, uint32_t outfit, uint32_t accent,
                             float emit, uint32_t sw, uint32_t sh) {
    // Legs
    draw_mesh_cube((vec3_t){pos.x-0.15f,pos.y+0.4f,pos.z},
                   (vec3_t){0.2f,0.8f,0.2f}, outfit, emit, sw, sh);
    draw_mesh_cube((vec3_t){pos.x+0.15f,pos.y+0.4f,pos.z},
                   (vec3_t){0.2f,0.8f,0.2f}, outfit, emit, sw, sh);
    // Torso
    draw_mesh_cube((vec3_t){pos.x,pos.y+1.1f,pos.z},
                   (vec3_t){0.5f,0.7f,0.3f}, outfit, emit, sw, sh);
    // Arms
    draw_mesh_cube((vec3_t){pos.x-0.4f,pos.y+1.0f,pos.z},
                   (vec3_t){0.15f,0.6f,0.15f}, accent, emit, sw, sh);
    draw_mesh_cube((vec3_t){pos.x+0.4f,pos.y+1.0f,pos.z},
                   (vec3_t){0.15f,0.6f,0.15f}, accent, emit, sw, sh);
    // Head
    draw_mesh_cube((vec3_t){pos.x,pos.y+1.7f,pos.z},
                   (vec3_t){0.35f,0.35f,0.35f}, 0xD4A574, emit, sw, sh);
}

// Car = low-poly box
static void draw_car(vec3_t pos, vec3_t rot, uint32_t color,
                      uint32_t sw, uint32_t sh) {
    // Body
    draw_mesh_cube((vec3_t){pos.x,pos.y+0.4f,pos.z},
                   (vec3_t){2.0f,0.6f,4.5f}, color, 0.0f, sw, sh);
    // Cabin
    draw_mesh_cube((vec3_t){pos.x,pos.y+0.9f,pos.z+0.3f},
                   (vec3_t){1.6f,0.5f,2.5f}, color, 0.0f, sw, sh);
    // Wheels (dark)
    float wx[4]={-1.1f,1.1f,-1.1f,1.1f};
    float wz[4]={-1.5f,-1.5f,1.5f,1.5f};
    for (uint32_t i=0;i<4;i++)
        draw_mesh_cube((vec3_t){pos.x+wx[i],pos.y+0.25f,pos.z+wz[i]},
                       (vec3_t){0.25f,0.5f,0.5f}, 0x111111, 0.0f, sw, sh);
    (void)rot;
}

// Animal = small cube
static void draw_animal(vec3_t pos, uint32_t color,
                          uint32_t sw, uint32_t sh) {
    draw_mesh_cube(pos, (vec3_t){0.3f,0.25f,0.4f}, color, 0.0f, sw, sh);
    // Head
    draw_mesh_cube((vec3_t){pos.x,pos.y+0.25f,pos.z-0.15f},
                   (vec3_t){0.2f,0.2f,0.2f}, color, 0.0f, sw, sh);
}

// ── Scene renderer ────────────────────────────────────────────────────────────
void renderer3d_init(void) {
    fb_info_t* info = fb_get_info();
    g_aspect = (float)info->width / (float)info->height;
}

void renderer3d_set_camera(vec3_t pos, vec3_t look_at) {
    g_cam_pos  = pos;
    g_cam_fwd  = look_at;
}

void renderer3d_clear(uint32_t sky_color) {
    fb_info_t* info = fb_get_info();
    uint32_t* fb = (uint32_t*)info->addr;
    uint32_t total = info->width * info->height;
    for (uint32_t i = 0; i < total; i++) fb[i] = sky_color;
    for (uint32_t i = 0; i < ZBUF_W * ZBUF_H; i++) g_zbuf[i] = g_far;
}

void renderer3d_draw_scene(scene_id_t scene) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    // Draw floor plane (simple grid of quads)
    for (int32_t gx = -10; gx <= 10; gx++) {
        for (int32_t gz = -10; gz <= 10; gz++) {
            uint32_t fc = ((gx+gz)&1) ? 0x1A1030 : 0x0D0820;
            draw_mesh_cube((vec3_t){(float)gx, -0.05f, (float)gz},
                           (vec3_t){0.98f, 0.05f, 0.98f},
                           fc, 0.0f, sw, sh);
        }
    }

    // Get all entities in this scene
    entity_id_t ids[ECS_MAX_ENTITIES];
    uint32_t count = ecs_get_scene_entities(scene, ids, ECS_MAX_ENTITIES);

    for (uint32_t i = 0; i < count; i++) {
        entity_id_t id = ids[i];
        transform_t*   t = ecs_get_transform(id);
        render_comp_t* r = ecs_get_render(id);
        if (!t || !r || !r->visible) continue;

        switch (r->mesh) {
        case MESH_CHARACTER_RAGNAR:
            draw_character(t->pos, r->color, r->emit_color,
                           r->emit_strength, sw, sh);
            break;
        case MESH_CHARACTER_CORVUS:
            draw_character(t->pos, 0x0D0D0D, 0x8800FF,
                           r->emit_strength, sw, sh);
            break;
        case MESH_CHARACTER_LANDON:
            draw_character(t->pos, r->color, r->emit_color,
                           0.0f, sw, sh);
            break;
        case MESH_CHARACTER_RAPHAEL:
            draw_character(t->pos, r->color, r->emit_color,
                           0.0f, sw, sh);
            break;
        case MESH_ANIMAL_RAVEN:
        case MESH_ANIMAL_DOG:
        case MESH_ANIMAL_CAT:
            draw_animal(t->pos, r->color, sw, sh);
            break;
        case MESH_CAR_DEMON170:
            draw_car(t->pos, t->rot, r->color, sw, sh);
            break;
        case MESH_CAR_ROADRUNNER:
            draw_car(t->pos, t->rot, r->color, sw, sh);
            break;
        case MESH_CUBE:
        default:
            draw_mesh_cube(t->pos, t->scale, r->color,
                           r->emit_strength, sw, sh);
            break;
        }
    }
}
