glm::vec2 tick_input(float dt)
{
      glm::vec2 input = glm::vec2(0.0f);
      if(IsKeyPressed(KEY_Q))
      {
            CloseWindow();
      }
      glm::vec2 input_vectors(0.0f);
      if(IsKeyDown(KEY_W))
      {
            input_vectors += glm::vec2(0.0f, 1.0f);
      }
      if(IsKeyDown(KEY_D))
      {
            input_vectors += glm::vec2(1.0f, 0.0f);
      }
      if(IsKeyDown(KEY_S))
      {
            input_vectors += glm::vec2(0.0f, -1.0f);
      }
      if(IsKeyDown(KEY_A))
      {
            input_vectors += glm::vec2(-1.0f, 0.0f);
      }

      if(glm::length(input_vectors) > 0.5f)
      {
            input = glm::normalize(input_vectors);;
      }
      else
      {
            input = glm::vec2(0.0f);
      }
      return input;
}
