#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/matrix.hpp"
#include "raylib.h"
#include <cstdlib>
#include <span>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "cgltf.c"

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
                       std::span<float> weights,
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

      for(int i = 0; i < 3; i ++)
      {
            if(weights[i] == 0.0f)
            {
                  continue;
            }
            for(int j = 0; j < model.boneCount; j++)
            {
                  glm::vec3 translation =  in_poses[i * model.boneCount + j].translation;
                  OutPose[j].translation += weights[i] * translation;

      
                  glm::quat rotation =  in_poses[i * model.boneCount + j].rotation;
                  glm::quat q = glm::inverse(RefPose[j].rotation) * rotation;
                  if (glm::dot(OutPose[j].rotation, q) < 0.0f) q = -q;
                  OutPose[j].rotation += weights[i] * q;
            }  
      }

      for(int j= 0; j< model.boneCount; j++)
      {
            OutPose[j].rotation = glm::normalize(RefPose[j].rotation * OutPose[j].rotation);    
      }
}



std::vector<BoneTransform> bone_ms_to_ls(Model model, const std::span<BoneTransform> pose)
{
      std::vector<BoneTransform> out_pose (model.boneCount * 3);

      out_pose[0].translation = glm::vec3(0);
      out_pose[0].rotation    = glm::quat(1,0,0,0);  
      out_pose[0].scale       = glm::vec3(1);

      for(int i = 0; i < 3; i ++)
      {
            for(int j = 1; j < model.boneCount; j++)
            {
                  int parent_index = model.bones[j].parent;
            
                  glm::vec3 parent_position = pose[i * model.boneCount + parent_index].translation;
                  glm::quat parent_rotation = pose[i * model.boneCount + parent_index].rotation;
                  glm::mat4 parent_mat = glm::translate(glm::mat4(1.0f), parent_position) *
			                       glm::mat4_cast(parent_rotation);


            
                  glm::mat4 parent_mat_inv = glm::inverse(parent_mat);

                  glm::vec3 ls_pos = pose[i * model.boneCount + j].translation - parent_position;
                  ls_pos = glm::inverse(parent_rotation) * ls_pos;

            
            
                  glm::quat ls_rot = glm::inverse(parent_rotation) * pose[i * model.boneCount + j].rotation;
           
                  out_pose[i * model.boneCount + j].translation = ls_pos;
                  out_pose[i * model.boneCount + j].rotation = ls_rot;

                  // glm::vec4 ls_pos4 = glm::inverse(parent_mat) * glm::vec4(pose[i * model.boneCount + i].translation, 1.0f);
                  // out_pose[i * model.boneCount + i].translation = glm::vec3(ls_pos4.x, ls_pos4.y, ls_pos4.z);
            }      
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
int FindBoneIndex(const cgltf_skin* skin, const cgltf_node* node)
{
      for (int i = 0; i < (int)skin->joints_count; ++i)
      {
            if (skin->joints[i] == node) return i;
      }
    
      return -1;
}


void LoadAnimation(const cgltf_data* data, int animIndex, const cgltf_skin* skin, Animation* out)
{
      const cgltf_animation* anim = &data->animations[animIndex];

      out->boneCount = (int)skin->joints_count;
      out->bones = (BoneAnim*)RL_CALLOC(out->boneCount, sizeof(BoneAnim));

      for (int c = 0; c < (int)anim->channels_count; ++c)
      {
            const cgltf_animation_channel* ch = &anim->channels[c];
            const cgltf_animation_sampler* s  = ch->sampler;

            int bone = FindBoneIndex(skin, ch->target_node);
            if (bone < 0) continue; // channel targets a node not in this skin

            int keyCount = (int)s->input->count;

            AnimTrack* track = NULL;
            int components = 0;

            switch (ch->target_path)
            {
                  case cgltf_animation_path_type_translation:
                        track = &out->bones[bone].translation;
                        components = 3;
                        break;

                  case cgltf_animation_path_type_rotation:
                        track = &out->bones[bone].rotation;
                        components = 4;
                        break;

                  case cgltf_animation_path_type_scale:
                        track = &out->bones[bone].scale;
                        components = 3;
                        break;

                  default:
                        continue;
            }

            track->keyCount = keyCount;
            track->components = components;

            // ---- copy times ----
            track->times = (float*)RL_MALLOC(sizeof(float) * keyCount);
            for (int k = 0; k < keyCount; ++k)
                  cgltf_accessor_read_float(s->input, k, &track->times[k], 1);

            // ---- copy values ----
            size_t elemSize = (components == 4) ? (4*sizeof(float)) : (3*sizeof(float));
            track->vec = (float*)RL_MALLOC(elemSize * keyCount);

            for (int k = 0; k < keyCount; ++k)
            {
                  cgltf_accessor_read_float(
                                            s->output,
                                            k,
                                            (float*)track->vec + k * components,
                                            components
                                            );
            }

            // ---- update duration ----
            float lastT = track->times[keyCount - 1];
            if (lastT > out->duration) out->duration = lastT;
      }

}
Animation* LoadAnimDeep(const char* FileName, int *animCount)
{
      Animation* Anims = NULL;

      int dataSize = 0;
      unsigned char *fileData = LoadFileData(FileName, &dataSize);

      cgltf_options options = { 0 };
      cgltf_data *data = NULL;
      cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

      if (result != cgltf_result_success)
      {
            TraceLog(LOG_INFO, "failed loading anim");

      }
      else
      {
            TraceLog(LOG_INFO, "LoadedAnim");
      }

      result = cgltf_load_buffers(&options, data, FileName);

      if (result != cgltf_result_success)
      {
            TRACELOG(LOG_INFO, "MODEL: [%s] Failed to load animation buffers", fileName);
      }
    

      if (result == cgltf_result_success)
      {
            if (data->skins_count > 0)
            {
                  cgltf_skin skin = data->skins[0];
                  int AnimCount = data->animations_count;
                  *animCount = AnimCount;
                  Anims = (Animation*)malloc(AnimCount*sizeof(Animation));
                  for(int i = 0; i < AnimCount; i ++)
                  {
	                  const cgltf_animation* anim = &data->animations[i];

	                  Anims[i].boneCount = skin.joints_count;
	                  Anims[i].bones = (BoneAnim*)malloc(Anims[i].boneCount*sizeof(BoneAnim));
	                  Anims[i].channelCount = (int)anim->channels_count;

	                  for (int c = 0; c < (int)anim->channels_count; ++c)
	                  {
	                        const cgltf_animation_channel* ch = &anim->channels[c];
	                        const cgltf_animation_sampler* s  = ch->sampler;

	                        int bone = FindBoneIndex(&skin, ch->target_node);
	                        if (bone < 0) continue; 

	                        int keyCount = (int)s->input->count;

	                        AnimTrack* track = NULL;
	                        int components = 0;

	                        switch (ch->target_path)
	                        {
	                              case cgltf_animation_path_type_translation:
                                          track = &Anims[i].bones[bone].translation;
                                          components = 3;
                                          break;

	                              case cgltf_animation_path_type_rotation:
                                          track = &Anims[i].bones[bone].rotation;
                                          components = 4;
                                          break;

	                              case cgltf_animation_path_type_scale:
                                          track = &Anims[i].bones[bone].scale;
                                          components = 3;
                                          break;

	                              default:
                                          continue;
	                        }

	                        track->keyCount = keyCount;
	                        track->components = components;


	                        track->times = (float*)malloc(sizeof(float) * keyCount);
	                        for (int k = 0; k < keyCount; ++k)
	                              cgltf_accessor_read_float(s->input, k, &track->times[k], 1);

	                        size_t elemSize = (components == 4) ? (4*sizeof(float)) : (3*sizeof(float));
	                        track->vec = (float*)malloc(elemSize * keyCount);

	                        for (int k = 0; k < keyCount; ++k)
	                        {
	                              cgltf_accessor_read_float(
                                                              s->output,
                                                              k,
                                                              (float*)track->vec + k * components,
                                                              components
                                                              );
	                        }
	                        float lastT = track->times[keyCount - 1];
	                        if (lastT > Anims[i].duration) Anims[i].duration = lastT;
	                  }
                  }      
            }
      }
      cgltf_free(data);
      return Anims;
}
static BoneInfo *LoadBoneInfoGLTF(cgltf_skin skin, int *boneCount)
{
    *boneCount = (int)skin.joints_count;
      BoneInfo *bones = (BoneInfo*)RL_MALLOC(skin.joints_count*sizeof(BoneInfo));

    for (unsigned int i = 0; i < skin.joints_count; i++)
    {
        cgltf_node node = *skin.joints[i];
        if (node.name != NULL)
        {
            strncpy(bones[i].name, node.name, sizeof(bones[i].name));
            bones[i].name[sizeof(bones[i].name) - 1] = '\0';
        }

        // Find parent bone index
        int parentIndex = -1;

        for (unsigned int j = 0; j < skin.joints_count; j++)
        {
            if (skin.joints[j] == node.parent)
            {
                parentIndex = (int)j;
                break;
            }
        }

        bones[i].parent = parentIndex;
    }

    return bones;
}
static glm::vec3 GetBoneTranslationAtTime(AnimTrack* TranslationTrack, float t)
{

      if(TranslationTrack->keyCount <= 2)
      {
            return  glm::vec3(TranslationTrack->vec[0], TranslationTrack->vec[1], TranslationTrack->vec[2]);
      }

      int k = 0;
      float t0 = TranslationTrack->times[k];
      float t1 = TranslationTrack->times[k + 1];

      float denom = (t1 - t0);
      float a = (denom > 0.0f) ? (t - t0) / denom : 0.0f;

      int i0 = k * 3;
      int i1 = (k + 1) * 3;

      glm::vec3 v0(TranslationTrack->vec[i0 + 0], TranslationTrack->vec[i0 + 1], TranslationTrack->vec[i0 + 2]);
      glm::vec3 v1(TranslationTrack->vec[i1 + 0], TranslationTrack->vec[i1 + 1], TranslationTrack->vec[i1 + 2]);

      return v0 + a * (v1 - v0);
}



static glm::quat GetBoneRotationAtTime(AnimTrack* RotationTrack, float t)
{

      int CurrentFrame = 0;
      float t0 = RotationTrack->times[CurrentFrame];
      float t1 = RotationTrack->times[CurrentFrame + 1];

      float denom = (t1 - t0);
      float SlerpAlpha = (denom > 0.0f) ? (t - t0) / denom : 0.0f;
  
      int i = CurrentFrame * 4;
      glm::quat q0 = glm::quat(RotationTrack->vec[i + 3],RotationTrack->vec[i + 0],RotationTrack->vec[i + 1],RotationTrack->vec[i + 2]);
      i += 4;
      glm::quat q1 = glm::quat(RotationTrack->vec[i + 3],RotationTrack->vec[i + 0],RotationTrack->vec[i + 1],RotationTrack->vec[i + 2]);  

      if (glm::dot(q0, q1) < 0.0f)
      {
            q1 = -q1;
      }   

      glm::quat q = glm::slerp(q0, q1, SlerpAlpha);
      return q;
  
}

