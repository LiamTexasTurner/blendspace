
//p00 -------- p10
// |            |
// |     p      |
// |            |
//p01 -------- p11

#include "raylib.h"
#include <vector>
struct BlendspaceGui
{
      Vector2 p00;
      Vector2 p10;
      Vector2 p01;
      float width;
      float height;
};

template<typename T, glm::length_t C, glm::length_t R>
void print_glm_mat(const glm::mat<C, R, T>& m, std::string str = "")
{
      std::cout << str << std::endl;

      std::cout << std::fixed << std::setprecision(3);

      for(int i = 0; i < R; i++)          // rows
      {
            for(int j = 0; j < C; j++)    // cols
            {
                  std::cout << std::setw(10) << m[j][i] << " ";
            }
            std::cout << "\n";
      }
      std::cout << "-------------------------------------------\n";
}

bool screen_to_uv(BlendspaceGui gui_dim,
                  Vector2  &p,
                  std::span<float> outUV)
{
      glm::mat2 A = glm::mat2(0.0f);

      A[0] = glm::vec2(gui_dim.p10.x - gui_dim.p00.x, 0);
      A[1] = glm::vec2(0, gui_dim.p01.y - gui_dim.p00.y);
      
      glm::vec2 b = glm::vec2(p.x - gui_dim.p00.x, p.y - gui_dim.p00.y);

      glm::vec2 uv = glm::inverse(A) * b;

      outUV[0] = uv[0];
      outUV[1] = uv[1];
      
      return false;
}

void uv_to_screen(const Vector2 p00,
                                const Vector2 p10,
                                const Vector2 p01,
                                std::span<float> uv,
                                Vector2* outP)
{
      glm::mat2 A;
      A[0] = glm::vec2(p10.x - p00.x, p10.y - p00.y);
      A[1] = glm::vec2(p01.x - p00.x, p01.y - p00.y);

      glm::vec2 local = A * glm::vec2(uv[0], uv[1]);
      glm::vec2 world = local + glm::vec2(p00.x, p00.y);

      outP->x = world[0];
      outP->y = world[1];
}

mat init_blend_mat(mat anim_uv)
{
      int n_anims = anim_uv.rows;
      
      mat distances_mat = {};
      distances_mat.rows = n_anims;
      distances_mat.cols = n_anims;
      distances_mat.data.resize(distances_mat.rows * distances_mat.cols);

      mat blend_mat = {};
      blend_mat.rows = n_anims;
      blend_mat.cols = n_anims;
      blend_mat.data.resize(blend_mat.rows * blend_mat.cols);
      
      for(int i = 0; i < n_anims; i++)
      {
            for(int j = 0; j < n_anims; j++)
            {
                  distances_mat(i,j) = 0.0f;
                  for(int k = 0; k < anim_uv.cols; k++)
                  {
                        distances_mat(i,j) += (anim_uv(i,k) - anim_uv(j,k)) * (anim_uv(i,k) - anim_uv(j,k));
                  }
                  distances_mat(i,j) = sqrt(distances_mat(i,j));
            }
      }

      for(int i = 0; i < n_anims; i++)
      {
            distances_mat(i, i)-= 1e-4f;
      }
      
      std::vector<int> row_order(anim_uv.rows);
      std::vector<float> row_scale(anim_uv.rows);
      
      bool success = mat_lu_dedcompose_inplace(distances_mat, row_order, row_scale);
      assert(success);
      
      for(int i = 0; i < n_anims; i++)
      {
            for(int j = 0; j < n_anims; j++)
            {
                  blend_mat(i,j) = i == j ? 1.0f : 0.0f;
            }
      }

      for(int i = 0; i < n_anims; i++)
      {
            mat_lu_slove_inplace(blend_mat(i), distances_mat, row_order);
      }
      
      return blend_mat;
}

void compute_distances(mat &mat,
                       std::span<float> distance,
                       std::span<float> uv)
{
      for(int i = 0; i < mat.rows; i++)
      {
            distance[i] = 0.0f;
            for(int j = 0; j < mat.cols; j++)
            {
                  distance[i] += (uv[j] - mat(i,j)) * (uv[j] - mat(i,j));
            }
            distance[i] = sqrt(distance[i]);
      }
}

std::vector<float> compute_blend_weights(std::span<float> distances,
                                         mat blend_mat)
{
      assert(distances.size() == blend_mat.rows);
      return mul_vec(blend_mat, distances);
}

void clamp_normalize_blend_weights(std::span<float> weights)
{
      float total_blend_weights = 0.0f;
      for(int i = 0; i < weights.size(); i++)
      {
            weights[i] = maxf(weights[i], 0.0f);
            total_blend_weights += weights[i];
      }
      for(int i = 0; i < weights.size(); i++)
      {
            weights[i] /= total_blend_weights;
      }
}

void draw_anim_uv(mat anim_uv_coord, BlendspaceGui gui_dim, std::vector<float> blend_weights)
{
      for(int i = 0; i < anim_uv_coord.rows; i++)
      {
            Vector2 screen_pos;
            uv_to_screen(gui_dim.p00, gui_dim.p10, gui_dim.p01, anim_uv_coord(i), &screen_pos);

            Color col = ColorLerp(WHITE, RED, blend_weights[i]);
            DrawCircle(screen_pos.x, screen_pos.y, 5 , col);
      }
}

void draw_blendspace_gui(BlendspaceGui blend_space_gui,
                         Vector2 mouse_pos,
                         mat anim_uv_coords,
                         mat blend_mat,
                         std::vector<float> distances,
                         std::vector<float>& out_vec)
{

      Rectangle rec = Rectangle{blend_space_gui.p00.x, blend_space_gui.p00.y,
                                blend_space_gui.width, blend_space_gui.height} ;

      GuiDrawRectangle(rec, 2, GRAY, DARKGRAY);
      
      float out_uv_custom[2];

      screen_to_uv(blend_space_gui, mouse_pos, out_uv_custom);
            
      compute_distances(anim_uv_coords, distances, out_uv_custom);
      std::vector<float> blend_weights = compute_blend_weights(distances, blend_mat);
      clamp_normalize_blend_weights(blend_weights);

      out_vec = blend_weights;

      draw_anim_uv(anim_uv_coords, blend_space_gui, blend_weights);
      
}
