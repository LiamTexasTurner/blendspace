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
                  Vector2 &p,
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

mat init_blend_mat(Blendspace blendspace)
{
      int n_anims = blendspace.nodes.size();

      mat &anim_uv = blendspace.uv;
      
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

void compute_distances(Blendspace blendspace,
                       std::span<float> distance,
                       glm::vec2 current_blendspace_pos)
{
      
      for(int i = 0; i < blendspace.uv.rows; i++)
      {
            distance[i] = 0.0f;
            for(int j = 0; j < blendspace.uv.cols; j++)
            {
                  distance[i] += (current_blendspace_pos[j] - blendspace.uv(i,j)) * (current_blendspace_pos[j] - blendspace.uv(i,j));
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
void compute_blendspace_pos(float dt, glm::vec2 input, glm::vec2 &uv)
{

      glm::vec2 offset(0.5f, 0.5f);
      float delta_x = 0;
      float delta_y = 0;
      input.y = -input.y;
      if(abs(input.x) > 0.1f)
      {
            delta_x = input.x * dt * 4;
            uv.x += delta_x;
      }
      else
      {
            uv.x += (0.5f - uv.x) * (dt * 4);
      }
      
      if(abs(input.y) > 0.1f)
      {
            
            delta_y = input.y * dt * 4;
            uv.y += delta_y;
      }
      else
      {
            uv.y += (0.5f - uv.y) * (dt * 4);
      }

      uv = glm::clamp(uv, 0.0f, 1.0f);
}




static Vector2 window_position = { 10, 10 };
static Vector2 window_size = { 200, 400 };
static bool minimized = false;
static bool moving = false;
static bool resizing = false;
static Vector2 scroll;

static Vector2 window2_position = { 250, 10 };
static Vector2 window2_size = { 200, 400 };
static bool minimized2 = false;
static bool moving2 = false;
static bool resizing2 = false;
static Vector2 scroll2;

void GuiWindowFloating(Vector2 *position, Vector2 *size, bool *minimized, bool *moving, bool *resizing, void (*draw_content)(Vector2, Vector2, ModelAnimation*, BlendspaceNode*, int, bool&), Vector2 content_size, Vector2 *scroll, const char* title, ModelAnimation *anims, int anim_count, bool &close, BlendspaceNode* blendspace_node) {
      #if !defined(RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT)
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24
#endif

      #if !defined(RAYGUI_WINDOW_CLOSEBUTTON_SIZE)
#define RAYGUI_WINDOW_CLOSEBUTTON_SIZE 18
#endif

      int close_title_size_delta_half = (RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT - RAYGUI_WINDOW_CLOSEBUTTON_SIZE) / 2;

      // window movement and resize input and collision check
      if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !*moving && !*resizing) {
            Vector2 mouse_position = GetMousePosition();

            Rectangle title_collision_rect = { position->x, position->y, size->x - (RAYGUI_WINDOW_CLOSEBUTTON_SIZE + close_title_size_delta_half), RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT };
            Rectangle resize_collision_rect = { position->x + size->x - 20, position->y + size->y - 20, 20, 20 };

            if(CheckCollisionPointRec(mouse_position, title_collision_rect)) {
                  *moving = true;
            } else if(!*minimized && CheckCollisionPointRec(mouse_position, resize_collision_rect)) {
                 *resizing = true;
            }
      }

      if(*resizing) {
            Vector2 mouse = GetMousePosition();
            if (mouse.x > position->x)
                  size->x = mouse.x - position->x;
            if (mouse.y > position->y)
                  size->y = mouse.y - position->y;

            // clamp window size to an arbitrary minimum value and the window size as the maximum
            if(size->x < 100) size->x = 100;
            else if(size->x > GetScreenWidth()) size->x = GetScreenWidth();
            if(size->y < 100) size->y = 100;
            else if(size->y > GetScreenHeight()) size->y = GetScreenHeight();

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                  *resizing = false;
            }
      }

      // window and content drawing with scissor and scroll area
      if(*minimized) {
            close = true;
            GuiStatusBar(Rectangle{ position->x, position->y, size->x, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT }, title);

            if (GuiButton(Rectangle{ position->x + size->x - RAYGUI_WINDOW_CLOSEBUTTON_SIZE - close_title_size_delta_half,
                                     position->y + close_title_size_delta_half,
                                     RAYGUI_WINDOW_CLOSEBUTTON_SIZE,
                                     RAYGUI_WINDOW_CLOSEBUTTON_SIZE },
                          "#120#")) {
                                *minimized = false;
                          }

      } else {
            *minimized = GuiWindowBox(Rectangle { position->x, position->y, size->x, size->y }, title);

            // scissor and draw content within a scroll panel
            if(draw_content != NULL) {
                  Rectangle scissor = { 0 };
                  GuiScrollPanel(Rectangle { position->x, position->y + RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT,
                                             size->x, size->y - RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT },
                                 NULL,
                                 Rectangle { position->x, position->y, content_size.x, content_size.y },
                                 scroll,
                                 &scissor);

                  bool require_scissor = size->x < content_size.x || size->y < content_size.y;

                  if(require_scissor) {
                        BeginScissorMode(scissor.x, scissor.y, scissor.width, scissor.height);
                  }

                  draw_content(*position, *scroll, anims, blendspace_node, anim_count, close);

                  if(require_scissor) {
                        EndScissorMode();
                  }
            }

            // draw the resize button/icon
            GuiDrawIcon(71, position->x + size->x - 20, position->y + size->y - 20, 1, WHITE);
      }
}

static void DrawContent(Vector2 position, Vector2 scroll, ModelAnimation *anims, BlendspaceNode *blendspace_node, int anim_count, bool &close) {
      for(int i = 0; i < anim_count; i ++)
      {
            if (GuiButton(Rectangle { position.x + 20 + scroll.x, position.y + 50 + (50 * i) + scroll.y, 100, 25 }, anims[i].name))
            {
                  blendspace_node->id = i;
                  close = true;
            }
      }
}








struct AnimParamGUI
{
      
      Rectangle rec;
      BlendspaceNode *node;
      bool close = false;

      AnimParamGUI(Rectangle in_rec, BlendspaceNode *in_node) : rec(in_rec), node(in_node) {}
};

std::vector<AnimParamGUI> anim_param_guis;

void render_anim_params(ModelAnimation *anims, int anim_count, Blendspace &blendspace)
{
      for (auto it = anim_param_guis.begin(); it != anim_param_guis.end();)
      {
            Vector2 pos{ it->rec.x + 12, it->rec.y + 12 };

            GuiWindowFloating(&pos, &window_size, &minimized,
                              &moving, &resizing, &DrawContent,
                              Vector2{ 140, 320 }, &scroll, anims[it->node->id].name,
                              anims, anim_count, it->close, it->node);
            if (it->close)
            {
                  it = anim_param_guis.erase(it);
                  minimized = false;
            }
            else
            {
                  ++it;
            }
      }
}

void draw_anim_uv(glm::vec2 current_blendspace_pos, Blendspace &blendspace, BlendspaceGui gui_dim, std::vector<float> blend_weights)
{
      for(int i = 0; i < blendspace.uv.rows; i++)
      {
            Vector2 screen_pos;
            uv_to_screen(gui_dim.p00, gui_dim.p10, gui_dim.p01, blendspace.uv(i), &screen_pos);

            Color col = ColorLerp(WHITE, RED, blend_weights[i]);
            DrawCircle(screen_pos.x, screen_pos.y, 5 , col);

            float button_size = 30;
            Rectangle rec{screen_pos.x - 15, screen_pos.y - 15, button_size, button_size};
            if (GuiButton(rec, "#150#"))
            {
                  
                  anim_param_guis.emplace_back(AnimParamGUI{rec, &blendspace.nodes[i]});
                  float x = 0;
            }
      }

      Vector2 blendspace_screen_pos;
      float uv[2];
      uv[0] = current_blendspace_pos.x;
      uv[1] = current_blendspace_pos.y;
      uv_to_screen(gui_dim.p00, gui_dim.p10, gui_dim.p01, uv, &blendspace_screen_pos);
      DrawCircle(blendspace_screen_pos.x, blendspace_screen_pos.y, 10 , WHITE);
}



void tick_blendspace_gui(BlendspaceGui blend_space_gui,
                         glm::vec2 current_blendspace_pos,
                         Blendspace &blendspace,
                         mat blend_mat,
                         std::vector<float> distances,
                         std::vector<float>& out_vec)
{

      Rectangle rec = Rectangle{blend_space_gui.p00.x, blend_space_gui.p00.y,
                                blend_space_gui.width, blend_space_gui.height} ;

      GuiDrawRectangle(rec, 2, GRAY, DARKGRAY);
      
      compute_distances(blendspace, distances, current_blendspace_pos);
      std::vector<float> blend_weights = compute_blend_weights(distances, blend_mat);
      clamp_normalize_blend_weights(blend_weights);

      out_vec = blend_weights;

      draw_anim_uv(current_blendspace_pos, blendspace, blend_space_gui, blend_weights);
      

      for(int i = 0; i < blendspace.nodes.size(); i++)
      {
            DrawText(TextFormat("%.1f\n", blend_weights[i]), 10, 50 + 20 * i, 25, RED);            
      }
}
