
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
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
#include "helpers.h"
#include "raygui.h"
#include "common.h"
#include "mat.h"
#include "animation.h"
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

      std::vector<BoneTransform> bind_pose(model.boneCount);

      for(int i = 0; i < model.boneCount; i++)
      {
            bind_pose[i].translation = RayVec3ToGLM(model.bindPose[i].translation);
            bind_pose[i].rotation    = RayQuatToGLM(model.bindPose[i].rotation);
            bind_pose[i].scale       = RayVec3ToGLM(model.bindPose[i].scale);
      }
      
      std::vector<BoneTransform> bone_transforms(model.boneCount * anim_count);
      int frame = 15;
      //mesh space
      for(int i = 0; i < anim_count; i++)
      {
            for(int j = 0; j < model.boneCount; j++)
            {
                  bone_transforms[j].translation = RayVec3ToGLM(anims[i].framePoses[frame][j].translation);
                  bone_transforms[j].rotation    = RayQuatToGLM(anims[i].framePoses[frame][j].rotation);
                  bone_transforms[j].scale       = RayVec3ToGLM(anims[i].framePoses[frame][j].scale);
            }      
      }

      std::vector<BoneTransform> ls_bone_transforms(model.boneCount);
      ls_bone_transforms = bone_ms_to_ls(model, bone_transforms);

      

      std::vector<BoneTransform> out_pose(model.boneCount);

      ThreeWayBlendPose(model, 1, ls_bone_transforms, out_pose, bind_pose);

      // FK(model, out_pose);

      std::vector<BoneTransform> ms_bone_transforms(model.boneCount);
      ms_bone_transforms = bone_ls_to_ms(model, out_pose);

      
      
      DeformMesh(model, ms_bone_transforms);
      
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
      unsigned int anim_current_frame = 0;
      
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
