#include <algorithm>
#include <vector>

struct mat
{
      int rows;
      int cols;
      std::vector<float> data;

      inline float& operator()(int i, int j)
      {
            return data[i * cols + j];
      }
    
      inline const float& operator()(int i, int j) const
      {
            return data[i * cols + j];
      }

      
      inline std::span<float> operator()(int i)
      {
            return std::span<float>(data.data() + i * cols, cols);
      }
      
      inline std::span<const float> operator()(int i) const
      {
            return std::span<const float>(data.data() + i * cols, cols);
      }
};



bool mat_lu_dedcompose_inplace(mat &matrix,
                               std::span<int> row_order,
                               std::span<float> row_scale)
{
      int n = matrix.rows;
      
      //compute scailing for each row
      for(int i = 0; i < n; i++)
      {
            float vmax = 0.0f;
            for(int j = 0; j < n; j++)
            {
                  vmax = maxf(vmax, fabs(matrix(i,j)));
            }

            if(vmax == 0.0) {return false;}

            row_scale[i] = 1.0 / vmax;
      }

      //loop over cols using crouts method
      
      for (int j = 0; j < n; j++)
      {
            for (int i = 0; i < j; i++)
            {
                  float sum = matrix(i, j);
                  for (int k = 0; k < i; k++)
                  {
                        sum -= matrix(i, k) * matrix(k, j);
                  }
                  matrix(i, j) = sum;
            }
        
            // Search Largest Pivot
        
            float vmax = 0.0f;
            int imax = -1;
            for (int i = j; i < n; i++)
            {
                  float sum = matrix(i, j);
                  for (int k = 0; k < j; k++)
                  {
                        sum -= matrix(i, k) * matrix(k, j);
                  }
                  matrix(i, j) = sum;
                  float val = row_scale[i] * fabs(sum);
                  if (val >= vmax)
                  {
                        vmax = val;
                        imax = i;
                  }
            }
        
            if (vmax == 0.0) { return false; }
        
            // Interchange Rows
        
            if (j != imax)
            {
                  for (int k = 0; k < n; k++)
                  {
                        float val = matrix(imax, k);
                        matrix(imax, k) = matrix(j, k);
                        matrix(j, k) = val;
                  }
                  row_scale[imax] = row_scale[j];
            }
        
            // Divide by Pivot
        
            row_order[j] = imax;
        
            if (matrix(j, j) == 0.0) { return false; }

            if (j != n - 1)
            {
                  float val = 1.0 / matrix(j, j);
                  for (int i = j + 1; i < n; i++)
                  {
                        matrix(i, j) *= val;
                  }
            }
      }
    
      return true;
}

void mat_lu_slove_inplace(std::span<float> vector,
                          mat decomp,
                          std::span<int> row_order)
{
      int n = decomp.rows;
      int ii = -1;
    
      // Forward Substitution
    
      for (int i = 0; i < n; i++)
      {
            float sum = vector[row_order[i]];
            vector[row_order[i]] = vector[i];
        
            if (ii != -1)
            {
                  for (int j = ii; j <= i-1; j++)
                  {
                        sum -= decomp(i, j) * vector[j];
                  }
            }
            else if (sum)
            {
                  ii = i;
            }
        
            vector[i] = sum;
      }
    
      // Backward Substitution
    
      for (int i = n - 1; i >= 0; i--)
      {
            float sum = vector[i];
            for (int j = i + 1; j < n; j++)
            {
                  sum -= decomp(i, j) * vector[j];
            }
            vector[i] = sum / decomp(i, i);
      }
}

std::vector<float> mul_vec(mat matrix, std::span<float> vec)
{
      std::vector<float> result(matrix.rows, 0);
      
      for(int i = 0; i < result.size(); i++)
      {
            for(int j = 0; j < vec.size(); j++)
            {
                  result[i] += vec[j] * matrix(i,j);
            }
      }
      return result;
}

mat mat_mul_vec(const mat lhs, std::span<float> rhs)
{

      mat output = {};
      output.rows = lhs.rows;
      output.cols = lhs.cols;
      output.data.resize(output.rows * output.cols, 0);
      
      for (int i = 0; i < output.rows; i++)
      {
            for (int k = 0; k < rhs.size(); k++)
            {
                  for (int j = 0; j < output.cols; j++)
                  {
                        output(i, j) += rhs[j] * lhs(i, k);
                  }
            }
      }

      return output;
}

void print_mat(mat m, std::string str = "")
{
      std::cout << str << std::endl;

      std::cout << std::fixed << std::setprecision(3);

      for(int i = 0; i < m.rows; i++)
      {
            for(int j = 0; j < m.cols; j++)
            {
                  std::cout << std::setw(10) << m(i,j) << " ";
            }
            std::cout << "\n";
      }

      std::cout << "-------------------------------------------\n";

}
