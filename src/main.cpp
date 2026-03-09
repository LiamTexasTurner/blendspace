
#include "glm/common.hpp"
#include "glm/geometric.hpp"
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
#include "input.h"

int main()
{
      BlendspaceGui blend_space_gui
      {
            Vector2{10, 10},
            Vector2{500, 10},
            Vector2{10, 500},
            490,
            490
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
      GuiSetIconScale(1);   

      Model model = LoadModel("chips.glb");
      
      int anim_count;
      ModelAnimation *anims = LoadModelAnimations("chips.glb", &anim_count);
      int frame = 0;            

      std::vector<BoneTransform> bind_pose(model.boneCount);
      
      for(int i = 0; i < model.boneCount; i++)
      {
            bind_pose[i].translation = RayVec3ToGLM(model.bindPose[i].translation);
            bind_pose[i].rotation    = RayQuatToGLM(model.bindPose[i].rotation);
            bind_pose[i].scale       = RayVec3ToGLM(model.bindPose[i].scale);
      }
      
      std::vector<BoneTransform> bone_transforms(model.boneCount * anim_count);

      for(int i = 0; i < anim_count; i ++)
      {
            for(int j = 0; j < model.boneCount; j++)
            {

                  bone_transforms[j].translation = RayVec3ToGLM(anims[i].framePoses[frame][j].translation);
                  bone_transforms[j].rotation    = RayQuatToGLM(anims[i].framePoses[frame][j].rotation);
                  bone_transforms[j].scale       = RayVec3ToGLM(anims[i].framePoses[frame][j].scale);
            }
      }
            
      std::vector<BoneTransform> ls_bone_transforms(model.boneCount * anim_count);
      ls_bone_transforms = bone_ms_to_ls(model, bone_transforms, anim_count);
      
      std::vector<BoneTransform> out_pose(model.boneCount);
      
      const int paramspace_width = 800;
      const int paramspace_height = 800;
      
      mat anims_uv_coords = {};
      anims_uv_coords.rows = anim_count;
      anims_uv_coords.cols = 2;
      anims_uv_coords.data =
      {
            1.0, 0.5,
            0.5, 0.0,
            0.0, 0.5,
            0.5, 1.0
      };
      
      mat blend_mat = init_blend_mat(anims_uv_coords);
      std::vector<float> distances(anim_count, 0);
      unsigned int anim_current_frame = 0;

      std::vector<float> blend_weights(anim_count);

      glm::vec2 input;
      glm::vec2 uv{0.5f, 0.5f};
      
      while(!WindowShouldClose())
      {
            BeginDrawing();

            tick_input(GetFrameTime(), input);
            
            UpdateCamera(&camera, CAMERA_ORBITAL);

            ClearBackground(RAYWHITE);
            
            //-----DRAW 3D--------------------

            BeginMode3D(camera);

            for(int i = 0; i < anim_count; i ++)
            {
                  for(int j = 0; j < model.boneCount; j++)
                  {

                        bone_transforms[i * model.boneCount +j].translation = RayVec3ToGLM(anims[i].framePoses[frame][j].translation);
                        bone_transforms[i * model.boneCount +j].rotation    = RayQuatToGLM(anims[i].framePoses[frame][j].rotation);
                        bone_transforms[i * model.boneCount +j].scale       = RayVec3ToGLM(anims[i].framePoses[frame][j].scale);
            
                  }
            }
      
            ls_bone_transforms = bone_ms_to_ls(model, bone_transforms, anim_count);

            ThreeWayBlendPose(model, anim_count, blend_weights, ls_bone_transforms, out_pose, bind_pose);
            
            FK(model, out_pose);
            
            DeformMesh(model, out_pose);

            
            DrawModel(model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            frame = (frame + 1)%(40);

            DrawGrid(10, 1.0f);

            EndMode3D();

            //----STOP DRAWING 3D-----------------

            

            

            //----DRAW GUI-----------------------
            
            
            

            compute_blendspace_pos(GetFrameTime(), input, uv);
            
            tick_blendspace_gui(blend_space_gui, anim_count, uv, anims_uv_coords, blend_mat,  distances, blend_weights);

            EndDrawing();

            //----STOP DRAWING GUI----------------
            
      }
      
      return 0;
}

