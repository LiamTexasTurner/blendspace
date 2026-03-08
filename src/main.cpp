
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
      Model model_2 = LoadModel("chips.glb");

      int AnimCount = 0;
      Animation* Anims = LoadAnimDeep("chips.glb", &AnimCount);

      Animation* Idle =(Animation*)malloc(sizeof(Animation));
      *Idle = Anims[0];

      std::vector<BoneTransform> IdlePose(model.boneCount);

      for(int i = 0; i < model.boneCount; i++)
      {
            AnimTrack translation = Idle->bones[i].translation;
            IdlePose[i].translation = GetBoneTranslationAtTime(&translation, 0.0f);

            AnimTrack rotation = Idle->bones[i].rotation;
            IdlePose[i].rotation = GetBoneRotationAtTime(&rotation, 0.0f);
      }
      FK(model, IdlePose);
      DeformMesh(model, IdlePose);
      
      
      int anim_count;
      ModelAnimation *anims = LoadModelAnimations("chips.glb", &anim_count);

      std::vector<BoneTransform> bind_pose(model.boneCount);

      std::vector<BoneTransform> out_pose(model.boneCount);

      
      for(int i = 0; i < model.boneCount; i++)
      {
            bind_pose[i].translation = RayVec3ToGLM(model.bindPose[i].translation);
            bind_pose[i].rotation    = RayQuatToGLM(model.bindPose[i].rotation);
            bind_pose[i].scale       = RayVec3ToGLM(model.bindPose[i].scale);
      }

      int frame = (frame + 1)%anims[1].frameCount;            

      std::vector<BoneTransform> bone_transforms(model.boneCount * 3);

      for(int i = 0; i < 3; i ++)
      {
            for(int j = 0; j < model.boneCount; j++)
            {

                  bone_transforms[j].translation = RayVec3ToGLM(anims[i].framePoses[frame][j].translation);
                  bone_transforms[j].rotation    = RayQuatToGLM(anims[i].framePoses[frame][j].rotation);
                  bone_transforms[j].scale       = RayVec3ToGLM(anims[i].framePoses[frame][j].scale);
            
            }
      }

            
      
      
      std::vector<BoneTransform> ls_bone_transforms(model.boneCount * 3);
      ls_bone_transforms = bone_ms_to_ls(model, bone_transforms);
      
      

      
      const int paramspace_width = 800;
      const int paramspace_height = 800;
      
      mat anims_uv_coords = {};
      anims_uv_coords.rows = 3;
      anims_uv_coords.cols = 2;
      anims_uv_coords.data =
      {
            0.5, 0.2,
            0.8, 0.8,
            0.2, 0.8,
      };
      mat blend_mat = init_blend_mat(anims_uv_coords);
      std::vector<float> distances(anims_uv_coords.rows, 0);
      unsigned int anim_current_frame = 0;

      std::vector<float> blend_weights
      {
            0.0f,
            0.0f,
            1.0f
      };
      
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

            frame = (frame + 1)%anims[1].frameCount;            

            for(int i = 0; i < 3; i ++)
            {
                  for(int j = 0; j < model.boneCount; j++)
                  {

                        bone_transforms[i * model.boneCount +j].translation = RayVec3ToGLM(anims[i].framePoses[frame][j].translation);
                        bone_transforms[i * model.boneCount +j].rotation    = RayQuatToGLM(anims[i].framePoses[frame][j].rotation);
                        bone_transforms[i * model.boneCount +j].scale       = RayVec3ToGLM(anims[i].framePoses[frame][j].scale);
            
                  }
            }
      
            ls_bone_transforms = bone_ms_to_ls(model, bone_transforms);

            std::vector<float> test
            {
                        1.0f,
                        0.0f,
                        0.0f
            };

            ThreeWayBlendPose(model, 3, blend_weights, ls_bone_transforms, out_pose, bind_pose);

            printf("%f, %f, %f\n", blend_weights[0], blend_weights[1], blend_weights[2]);
      
      

            
            FK(model_2, out_pose);
      
            DeformMesh(model_2, out_pose);

            
            
            DrawModel(model, Vector3{1.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            DrawModel(model_2, Vector3{-1.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            DrawGrid(10, 1.0f);

            //----STOP DRAWING 3D-----------------

            EndMode3D();

            //----DRAW GUI-----------------------
            
            draw_blendspace_gui(blend_space_gui, mouse_pos, anims_uv_coords, blend_mat,  distances, blend_weights);

            //----STOP DRAWING GUI----------------
            EndDrawing();
      }
      
      return 0;
}

