#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/matrix.hpp"
#include "raylib.h"
#include <span>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
struct AnimTrack
{
  float* times;
  float* vec;     
  int    keyCount;
  int    components;
  int cursor;
};

struct BoneAnim
{
  AnimTrack translation;
  AnimTrack rotation;
  AnimTrack scale;
};

struct Animation
{
  BoneAnim* bones;   
  int boneCount;
  int channelCount;
  float duration;
}; 

struct BoneTransform
{
      glm::vec3 translation = glm::vec3(0);
      glm::quat rotation    = glm::quat(0,0,0,0);  
      glm::vec3 scale       = glm::vec3(1);
};

void ThreeWayBlendPose(Model& model,
                       int num_poses,
                       std::span<BoneTransform> in_poses,
		           std::span<BoneTransform> OutPose,
                       std::span<BoneTransform> RefPose)
{

      for(int i = 0; i < model.boneCount; i ++)
      {
            OutPose[i].translation = glm::vec3(0.0f, 0.0f, 0.0f);
            OutPose[i].rotation = glm::quat(0.0f,0.0f,0.0f,0.0f);
            OutPose[i].scale = glm::vec3(1.0f);
      }

      int i = 0;
      
      for(int j = 0; j < model.boneCount; j++)
      {
            glm::vec3 translation =  in_poses[j].translation;
                  
            OutPose[j].translation = translation;
            
            glm::quat rotation =  in_poses[j].rotation;
                  
            glm::quat q = glm::inverse(RefPose[j].rotation) * rotation;
            if (glm::dot(OutPose[j].rotation, q) < 0.0f) q = -q;
            OutPose[i].rotation = q;
            
      }

      
      
      
      for(int j= 0; j< model.boneCount; j++)
      {
            OutPose[j].rotation = glm::normalize(RefPose[j].rotation * OutPose[j].rotation);    
      }
}

std::vector<BoneTransform> bone_ms_to_ls(Model model, std::span<BoneTransform> pose)
{
      std::vector<BoneTransform> out_pose (model.boneCount);

      out_pose[0].translation = glm::vec3(0);
      out_pose[0].rotation    = glm::quat(0,0,0,0);  
      out_pose[0].scale       = glm::vec3(1);
      
      for(int i = 1; i < model.boneCount; i++)
      {
            int parent_index = model.bones[i].parent;
            
            glm::vec3 parent_position = pose[parent_index].translation;
            glm::quat parent_rotation = pose[parent_index].rotation;
            glm::mat4 parent_mat = glm::translate(glm::mat4(1.0f), parent_position) *
			                 glm::mat4_cast(parent_rotation);


            
            glm::mat4 parent_mat_inv = glm::inverse(parent_mat);

            glm::vec3 ls_pos = glm::inverse(parent_rotation) * (pose[i].translation - parent_position);
            glm::quat ls_rot =  pose[i].rotation * glm::inverse(parent_rotation);
           
            out_pose[i].translation = glm::vec3(ls_pos.x, ls_pos.y, ls_pos.z);
            out_pose[i].rotation = ls_rot;
      }
      
      return out_pose;
}

std::vector<BoneTransform> bone_ls_to_ms(Model model, std::span<BoneTransform> pose)
{
      std::vector<BoneTransform> out_pose (model.boneCount);

      out_pose[0].translation = glm::vec3(0);
      out_pose[0].rotation    = glm::quat(1,0,0,0);  
      out_pose[0].scale       = glm::vec3(1);
      
      for(int i = 1; i < model.boneCount; i++)
      {
            int parent_index = model.bones[i].parent;
            
            glm::vec3 parent_position = pose[parent_index].translation;
            glm::quat parent_rotation = pose[parent_index].rotation;
            glm::mat4 parent_mat = glm::translate(glm::mat4(1.0f), parent_position) *
			                 glm::mat4_cast(parent_rotation);

            glm::vec3 ls_pos = parent_rotation * pose[i].translation + parent_position;
            
            glm::quat ls_rot = parent_rotation * pose[i].rotation;
           
            pose[i].translation = glm::vec3(ls_pos.x, ls_pos.y, ls_pos.z);
            pose[i].rotation = ls_rot;
            out_pose[i].translation = pose[i].translation;
            out_pose[i].rotation = pose[i].rotation;
      }
      
      return out_pose;
}

void FK(Model model, std::span<BoneTransform> out_pose)
{
      for(int i = 1; i < model.boneCount; i ++)
      {
            int parent_index = model.bones[i].parent;
    
            glm::vec3 parent_position = out_pose[parent_index].translation;
            glm::quat parent_rotation = out_pose[parent_index].rotation;
            glm::mat4 parent_mat = glm::translate(glm::mat4(1.0f), parent_position) *
			                 glm::mat4_cast(parent_rotation);

            glm::vec3 local_position = out_pose[i].translation;
            glm::vec3 ls_to_ms_position = (parent_rotation * local_position) + parent_position;

            glm::quat local_rotation = out_pose[i].rotation;
            glm::quat ls_to_ms_rotation = parent_rotation * local_rotation;

            out_pose[i].translation = ls_to_ms_position;
            out_pose[i].rotation = ls_to_ms_rotation;
      }
}
void DeformMesh(Model model, std::span<BoneTransform> pose)
{
      for (int i = 0; i < model.meshCount; i ++)
      {
            Mesh mesh = model.meshes[i];
            glm::vec3 animVertex = glm::vec3(0.f);
            int boneId = 0;
            int boneCounter = 0;
            float boneWeight = 0.0f;
            const int vValues = mesh.vertexCount*3;
            for (int vCounter = 0; vCounter < vValues; vCounter += 3)
            {
                  mesh.animVertices[vCounter] = 0;
                  mesh.animVertices[vCounter + 1] = 0;
                  mesh.animVertices[vCounter + 2] = 0;
                  for (int j = 0; j < 4; j++, boneCounter++)
                  {
	                  boneWeight = mesh.boneWeights[boneCounter];
	                  boneId = mesh.boneIds[boneCounter];

	                  if(boneWeight == 0.0f)
	                  {
	                        continue;
	                  }

	                  glm::vec3 bone_bind_position = RayVec3ToGLM(model.bindPose[boneId].translation);
	                  glm::quat bone_bind_rotation = RayQuatToGLM(model.bindPose[boneId].rotation);

	                  glm::vec3 bone_anim_position = pose[boneId].translation;
	                  glm::quat bone_anim_rotation = pose[boneId].rotation;
	
	                  glm::vec3 vertex_pos = glm::vec3(mesh.vertices[vCounter], mesh.vertices[vCounter + 1], mesh.vertices[vCounter + 2]);


	                  vertex_pos = glm::inverse(bone_bind_rotation) * (vertex_pos - bone_bind_position);
	                  vertex_pos = (bone_anim_rotation * vertex_pos) + bone_anim_position;
	
	                  mesh.animVertices[vCounter]	+= vertex_pos.x * boneWeight;
                        mesh.animVertices[vCounter+1]	+= vertex_pos.y * boneWeight;
                        mesh.animVertices[vCounter+2]	+= vertex_pos.z * boneWeight;	

                  }
            }
            rlUpdateVertexBuffer(mesh.vboId[0], mesh.animVertices, mesh.vertexCount*3*sizeof(float), 0);
      }
}
