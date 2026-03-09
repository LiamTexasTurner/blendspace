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

struct BlendspaceGui
{
      Vector2 p00;
      Vector2 p10;
      Vector2 p01;
      float width;
      float height;
};
struct BlendspaceNode
{
      float x;
      float y;
      int id;
};
struct Blendspace
{
      mat uv;
      std::vector<BlendspaceNode> nodes;
};

#include "floating_window.h"
#include "blendspace_gui.h"
#include "animation.h"
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

      Blendspace blendspace {};

      blendspace.nodes.emplace_back(BlendspaceNode{0.5, 0.0, 0});
      blendspace.nodes.emplace_back(BlendspaceNode{1.0, 0.5, 0});
      blendspace.nodes.emplace_back(BlendspaceNode{0.5, 1.0, 0});
      blendspace.nodes.emplace_back(BlendspaceNode{0.0, 0.5, 0});
      

      blendspace.uv.rows = blendspace.nodes.size();
      blendspace.uv.cols = 2;
      blendspace.uv.data.resize(blendspace.nodes.size() * 2);
      for(int i = 0; i < blendspace.nodes.size(); i++)
      {
            blendspace.uv(i, 0) = blendspace.nodes[i].x;
            blendspace.uv(i, 1) = blendspace.nodes[i].y;
      }
      
      mat blend_mat = init_blend_mat(blendspace);
      std::vector<float> distances(blendspace.nodes.size(), 0);
      unsigned int anim_current_frame = 0;

      std::vector<float> blend_weights(blendspace.nodes.size());

      glm::vec2 input;
      glm::vec2 current_blendspace_pos{0.5f, 0.5f};
      
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

            ThreeWayBlendPose(model, blend_weights, blendspace, ls_bone_transforms, out_pose, bind_pose);
            
            FK(model, out_pose);
            
            DeformMesh(model, out_pose);

            DrawModel(model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            frame = (frame + 1)%(40);

            DrawGrid(10, 1.0f);

            EndMode3D();

            //----STOP DRAWING 3D-----------------
            
            //----DRAW GUI-----------------------
            
            compute_blendspace_pos(GetFrameTime(), input, current_blendspace_pos);
            
            tick_blendspace_gui(blend_space_gui, current_blendspace_pos, blendspace, blend_mat,  distances, blend_weights);
            
            render_anim_params(anims, anim_count, blendspace);

            EndDrawing();

            //----STOP DRAWING GUI----------------
      }
      
      return 0;
}

