inline glm::quat RayQuatToGLM(const Quaternion& q)
{
    return glm::quat(q.w, q.x, q.y, q.z);
}
inline glm::vec3 RayVec3ToGLM(Vector3 vec)
{
  return glm::vec3(vec.x, vec.y, vec.z);
}
Vector3 GLMVec3ToRayVec3(glm::vec3& vec)
{
  return Vector3{vec.x, vec.y, vec.z};
}

static inline glm::quat quat_abs(glm::quat x)
{
    return x.w < 0.0 ? -x : x;
}


