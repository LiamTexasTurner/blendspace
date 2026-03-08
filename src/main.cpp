#include "raylib.h"
#include "raymath.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <span>
#include <vector>
#include <string>
#include <iomanip>
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "common.h"
#include "mat.h"
#include "blendspace_gui.h"

int main()
{
      BlendspaceGui blend_space_gui
      {
            Vector2{10, 10},
            Vector2{300, 10},
            Vector2{10, 300},
            290,
            290
            
      };

      int screen_width = 1920;
      int screen_height = 1080;

      InitWindow(screen_width, screen_height, "reflection");

      Camera3D camera = {0};
      camera.position = Vector3{0.0f, 2.0f, 4.f};
      camera.target = Vector3{0.0f, 1.0f, 0.0f};
      camera.up = Vector3{0.0f, 1.0f, 0.0f};
      camera.fovy = 45.f;
      camera.projection = CAMERA_PERSPECTIVE;
      
      SetTargetFPS(60);

      Model model = LoadModel("chips.glb");
      int anim_count;
      ModelAnimation *anims = LoadModelAnimations("chips.glb", &anim_count);
      

      const int paramspace_width = 800;
      const int paramspace_height = 800;
      
      mat anims_uv_coords = {};
      anims_uv_coords.rows = 4;
      anims_uv_coords.cols = 2;
      anims_uv_coords.data =
      {
            0.5, 0.8,
            0.8, 0.5,
            0.5, 0.2,
            0.2, 0.5
      };
      
      mat blend_mat = init_blend_mat(anims_uv_coords);
      
      std::vector<float> distances(anims_uv_coords.rows, 0);

      while(!WindowShouldClose())
      {
            BeginDrawing();

            if(IsKeyPressed(KEY_Q))
            {
                  break;
            }

            Vector2 mouse_pos = GetMousePosition();
            UpdateCamera(&camera, CAMERA_ORBITAL);

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

            //-----DRAW 3D--------------------

            UpdateModelAnimation(model, anims[0], 0);

            DrawModel(model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            DrawGrid(10, 1.0f);

            //----STOP DRAWING 3D-----------------

            EndMode3D();

            //----DRAW GUI-----------------------
            
            draw_blendspace_gui(blend_space_gui, mouse_pos, anims_uv_coords, blend_mat, distances);

            //----STOP DRAWING GUI----------------
            EndDrawing();
      }
      
      return 0;
}
