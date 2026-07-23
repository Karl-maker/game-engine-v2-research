#include "assets/MeshAssetService.h"

// Author: Karl-Johan Bailey

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace assets {

namespace {

struct Json final {
  enum class Type { Null, Bool, Number, String, Array, Object };
  Type type = Type::Null;
  bool b = false;
  double n = 0.0;
  std::string s;
  std::vector<Json> a;
  std::map<std::string, Json> o;

  const Json& at(const char* key) const {
    static const Json empty;
    if (type != Type::Object) return empty;
    const auto it = o.find(key);
    return it == o.end() ? empty : it->second;
  }
  const Json& at(std::size_t i) const {
    static const Json empty;
    return (type == Type::Array && i < a.size()) ? a[i] : empty;
  }
  bool isObject() const { return type == Type::Object; }
  bool isArray() const { return type == Type::Array; }
  std::string stringOr(std::string def = {}) const { return type == Type::String ? s : def; }
  int intOr(int def = 0) const { return type == Type::Number ? static_cast<int>(n) : def; }
  float floatOr(float def = 0.0f) const { return type == Type::Number ? static_cast<float>(n) : def; }
};

class JsonParser final {
 public:
  explicit JsonParser(std::string text) : m_text(std::move(text)) {}

  bool parse(Json& out) {
    skipWs();
    out = parseValue();
    skipWs();
    return !m_failed && m_pos == m_text.size();
  }

 private:
  Json parseValue() {
    skipWs();
    if (peek() == '{') return parseObject();
    if (peek() == '[') return parseArray();
    if (peek() == '"') return parseString();
    if (peek() == 't') {
      Json v;
      v.type = Json::Type::Bool;
      v.b = true;
      return parseLiteral("true", v);
    }
    if (peek() == 'f') {
      Json v;
      v.type = Json::Type::Bool;
      v.b = false;
      return parseLiteral("false", v);
    }
    if (peek() == 'n') return parseLiteral("null", Json{});
    return parseNumber();
  }

  Json parseObject() {
    Json out;
    out.type = Json::Type::Object;
    consume('{');
    skipWs();
    if (match('}')) return out;
    while (!m_failed) {
      Json key = parseString();
      skipWs();
      consume(':');
      out.o[key.s] = parseValue();
      skipWs();
      if (match('}')) break;
      consume(',');
    }
    return out;
  }

  Json parseArray() {
    Json out;
    out.type = Json::Type::Array;
    consume('[');
    skipWs();
    if (match(']')) return out;
    while (!m_failed) {
      out.a.push_back(parseValue());
      skipWs();
      if (match(']')) break;
      consume(',');
    }
    return out;
  }

  Json parseString() {
    Json out;
    out.type = Json::Type::String;
    if (!consume('"')) return out;
    while (m_pos < m_text.size()) {
      char c = m_text[m_pos++];
      if (c == '"') return out;
      if (c == '\\' && m_pos < m_text.size()) {
        const char e = m_text[m_pos++];
        if (e == '"' || e == '\\' || e == '/') out.s.push_back(e);
        else if (e == 'n') out.s.push_back('\n');
        else if (e == 'r') out.s.push_back('\r');
        else if (e == 't') out.s.push_back('\t');
        else if (e == 'b') out.s.push_back('\b');
        else if (e == 'f') out.s.push_back('\f');
      } else {
        out.s.push_back(c);
      }
    }
    m_failed = true;
    return out;
  }

  Json parseNumber() {
    Json out;
    out.type = Json::Type::Number;
    const std::size_t start = m_pos;
    if (peek() == '-') ++m_pos;
    while (std::isdigit(peek())) ++m_pos;
    if (peek() == '.') {
      ++m_pos;
      while (std::isdigit(peek())) ++m_pos;
    }
    if (peek() == 'e' || peek() == 'E') {
      ++m_pos;
      if (peek() == '+' || peek() == '-') ++m_pos;
      while (std::isdigit(peek())) ++m_pos;
    }
    try {
      out.n = std::stod(m_text.substr(start, m_pos - start));
    } catch (...) {
      m_failed = true;
    }
    return out;
  }

  Json parseLiteral(const char* lit, Json value) {
    const std::size_t len = std::strlen(lit);
    if (m_text.compare(m_pos, len, lit) != 0) {
      m_failed = true;
      return {};
    }
    m_pos += len;
    return value;
  }

  char peek() const { return m_pos < m_text.size() ? m_text[m_pos] : '\0'; }
  bool consume(char c) {
    skipWs();
    if (peek() != c) {
      m_failed = true;
      return false;
    }
    ++m_pos;
    return true;
  }
  bool match(char c) {
    skipWs();
    if (peek() != c) return false;
    ++m_pos;
    return true;
  }
  void skipWs() {
    while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos]))) ++m_pos;
  }

  std::string m_text;
  std::size_t m_pos = 0;
  bool m_failed = false;
};

std::string readText(const std::string& path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

bool readBytes(const std::string& path, std::vector<std::uint8_t>& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) return false;
  f.seekg(0, std::ios::end);
  const auto size = f.tellg();
  f.seekg(0, std::ios::beg);
  out.resize(static_cast<std::size_t>(size));
  if (!out.empty()) f.read(reinterpret_cast<char*>(out.data()), size);
  return true;
}

std::string dirName(const std::string& path) {
  const auto slash = path.find_last_of("/\\");
  return slash == std::string::npos ? std::string{} : path.substr(0, slash + 1);
}

std::string lowerExt(const std::string& path) {
  const auto dot = path.find_last_of('.');
  std::string ext = dot == std::string::npos ? std::string{} : path.substr(dot);
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

std::uint32_t readU32(const std::uint8_t* p, int componentType) {
  if (componentType == 5125) {
    std::uint32_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
  }
  if (componentType == 5123) {
    std::uint16_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
  }
  if (componentType == 5121) return *p;
  return 0;
}

float readFloat(const std::uint8_t* p) {
  float v = 0.0f;
  std::memcpy(&v, p, sizeof(v));
  return v;
}

int componentSize(int componentType) {
  switch (componentType) {
    case 5120:
    case 5121: return 1;
    case 5122:
    case 5123: return 2;
    case 5125:
    case 5126: return 4;
    default: return 4;
  }
}

int elementCount(const std::string& type) {
  if (type == "SCALAR") return 1;
  if (type == "VEC2") return 2;
  if (type == "VEC3") return 3;
  if (type == "VEC4") return 4;
  if (type == "MAT4") return 16;
  return 1;
}

class GltfMeshLoader final : public IMeshAssetLoader {
 public:
  bool load(const std::string& path, LoadedMeshAsset& out, std::string& error) const override {
    Json root;
    if (!JsonParser(readText(path)).parse(root)) {
      error = "Failed to parse glTF JSON";
      return false;
    }

    const std::string baseDir = dirName(path);
    const Json& buffers = root.at("buffers");
    if (!buffers.isArray() || buffers.a.empty()) {
      error = "glTF has no buffers";
      return false;
    }

    std::vector<std::uint8_t> bin;
    if (!readBytes(baseDir + buffers.at(static_cast<std::size_t>(0)).at("uri").stringOr(), bin)) {
      error = "Failed to read glTF buffer";
      return false;
    }

    out = {};
    out.id = path;
    out.sourcePath = path;
    out.boundsMin = {999999.0f, 999999.0f, 999999.0f};
    out.boundsMax = {-999999.0f, -999999.0f, -999999.0f};

    readMaterials(root, baseDir, out);
    readSkeleton(root, bin, out);
    readAnimations(root, bin, out);
    readMeshes(root, bin, out);

    if (out.subMeshes.empty()) {
      error = "glTF contained no renderable primitives";
      return false;
    }
    return true;
  }

 private:
  struct AccessorView final {
    const std::uint8_t* data = nullptr;
    int count = 0;
    int componentType = 5126;
    int elementCount = 1;
    int stride = 0;
  };

  static AccessorView accessor(const Json& root, const std::vector<std::uint8_t>& bin, int accessorIndex) {
    const Json& a = root.at("accessors").at(static_cast<std::size_t>(accessorIndex));
    const Json& bv = root.at("bufferViews").at(static_cast<std::size_t>(a.at("bufferView").intOr()));
    const int componentType = a.at("componentType").intOr(5126);
    const int elems = elementCount(a.at("type").stringOr("SCALAR"));
    const int byteOffset = bv.at("byteOffset").intOr() + a.at("byteOffset").intOr();
    const int stride = bv.at("byteStride").intOr(componentSize(componentType) * elems);
    AccessorView out;
    out.data = byteOffset >= 0 && static_cast<std::size_t>(byteOffset) < bin.size() ? bin.data() + byteOffset : nullptr;
    out.count = a.at("count").intOr();
    out.componentType = componentType;
    out.elementCount = elems;
    out.stride = stride;
    return out;
  }

  static std::vector<float> readFloatAccessor(const Json& root, const std::vector<std::uint8_t>& bin, int idx) {
    if (idx < 0) return {};
    const auto v = accessor(root, bin, idx);
    std::vector<float> out;
    if (!v.data || v.componentType != 5126) return out;
    out.resize(static_cast<std::size_t>(v.count * v.elementCount));
    for (int i = 0; i < v.count; ++i) {
      for (int c = 0; c < v.elementCount; ++c) {
        out[static_cast<std::size_t>(i * v.elementCount + c)] = readFloat(v.data + i * v.stride + c * sizeof(float));
      }
    }
    return out;
  }

  static std::vector<math::Mat4> readMat4Accessor(const Json& root, const std::vector<std::uint8_t>& bin, int idx) {
    const auto values = readFloatAccessor(root, bin, idx);
    std::vector<math::Mat4> out;
    if (values.empty()) return out;
    const std::size_t count = values.size() / 16;
    out.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
      for (int c = 0; c < 16; ++c) {
        out[i].m[c] = values[i * 16 + static_cast<std::size_t>(c)];
      }
    }
    return out;
  }

  static std::vector<std::uint32_t> readIndexAccessor(const Json& root, const std::vector<std::uint8_t>& bin, int idx) {
    if (idx < 0) return {};
    const auto v = accessor(root, bin, idx);
    std::vector<std::uint32_t> out;
    if (!v.data) return out;
    out.resize(static_cast<std::size_t>(v.count * v.elementCount));
    const int compSize = componentSize(v.componentType);
    for (int i = 0; i < v.count; ++i) {
      for (int c = 0; c < v.elementCount; ++c) {
        out[static_cast<std::size_t>(i * v.elementCount + c)] = readU32(v.data + i * v.stride + c * compSize, v.componentType);
      }
    }
    return out;
  }

  static int attrAccessor(const Json& primitive, const char* name) {
    const Json& attrs = primitive.at("attributes");
    if (!attrs.isObject()) return -1;
    return attrs.at(name).intOr(-1);
  }

  static std::string textureImagePath(const Json& root, const std::string& baseDir, int textureIndex) {
    if (textureIndex < 0) return {};
    const int imageIndex = root.at("textures").at(static_cast<std::size_t>(textureIndex)).at("source").intOr(-1);
    if (imageIndex < 0) return {};
    const std::string uri = root.at("images").at(static_cast<std::size_t>(imageIndex)).at("uri").stringOr();
    return uri.empty() ? std::string{} : baseDir + uri;
  }

  static void readMaterials(const Json& root, const std::string& baseDir, LoadedMeshAsset& out) {
    const Json& materials = root.at("materials");
    if (!materials.isArray()) return;
    for (const auto& jm : materials.a) {
      MeshMaterial m;
      m.name = jm.at("name").stringOr();
      const Json& pbr = jm.at("pbrMetallicRoughness");
      m.baseColorTexture = textureImagePath(root, baseDir, pbr.at("baseColorTexture").at("index").intOr(-1));
      m.metallicRoughnessTexture =
          textureImagePath(root, baseDir, pbr.at("metallicRoughnessTexture").at("index").intOr(-1));
      m.normalTexture = textureImagePath(root, baseDir, jm.at("normalTexture").at("index").intOr(-1));
      const Json& color = pbr.at("baseColorFactor");
      if (color.isArray() && color.a.size() >= 3) {
        m.baseColorFactor = {color.at(static_cast<std::size_t>(0)).floatOr(1.0f),
                             color.at(static_cast<std::size_t>(1)).floatOr(1.0f),
                             color.at(static_cast<std::size_t>(2)).floatOr(1.0f)};
      }
      out.materials.push_back(std::move(m));
    }
  }

  static math::Vec3 readVec3(const Json& node, const char* key, const math::Vec3& def) {
    const Json& value = node.at(key);
    if (!value.isArray() || value.a.size() < 3) return def;
    return {value.at(static_cast<std::size_t>(0)).floatOr(def.x),
            value.at(static_cast<std::size_t>(1)).floatOr(def.y),
            value.at(static_cast<std::size_t>(2)).floatOr(def.z)};
  }

  static math::Quat readQuat(const Json& node, const char* key, const math::Quat& def) {
    const Json& value = node.at(key);
    if (!value.isArray() || value.a.size() < 4) return def;
    return {value.at(static_cast<std::size_t>(0)).floatOr(def.x),
            value.at(static_cast<std::size_t>(1)).floatOr(def.y),
            value.at(static_cast<std::size_t>(2)).floatOr(def.z),
            value.at(static_cast<std::size_t>(3)).floatOr(def.w)};
  }

  static math::Mat4 readNodeLocalTransform(const Json& node) {
    const Json& matrix = node.at("matrix");
    if (matrix.isArray() && matrix.a.size() >= 16) {
      math::Mat4 out{};
      for (int i = 0; i < 16; ++i) {
        out.m[i] = matrix.at(static_cast<std::size_t>(i)).floatOr(out.m[i]);
      }
      return out;
    }
    const math::Vec3 t = readVec3(node, "translation", {0.0f, 0.0f, 0.0f});
    const math::Quat r = readQuat(node, "rotation", {});
    const math::Vec3 s = readVec3(node, "scale", {1.0f, 1.0f, 1.0f});
    return math::compose(t, r, s);
  }

  static void readSkeleton(const Json& root, const std::vector<std::uint8_t>& bin, LoadedMeshAsset& out) {
    const Json& skins = root.at("skins");
    if (!skins.isArray() || skins.a.empty()) return;

    const Json& skin = skins.at(static_cast<std::size_t>(0));
    out.skeleton.id = out.id + "#skin0";
    const Json& joints = skin.at("joints");
    if (!joints.isArray()) return;

    std::unordered_map<int, int> nodeToBone;
    for (std::size_t i = 0; i < joints.a.size(); ++i) {
      const int nodeIndex = joints.at(i).intOr(-1);
      nodeToBone[nodeIndex] = static_cast<int>(i);
      LoadedSkeleton::Bone b;
      b.nodeIndex = nodeIndex;
      b.name = root.at("nodes").at(static_cast<std::size_t>(nodeIndex)).at("name").stringOr("bone");
      b.localBindTransform = readNodeLocalTransform(root.at("nodes").at(static_cast<std::size_t>(nodeIndex)));
      out.skeleton.bones.push_back(std::move(b));
    }

    const Json& nodes = root.at("nodes");
    for (std::size_t parentNode = 0; parentNode < nodes.a.size(); ++parentNode) {
      const auto parentIt = nodeToBone.find(static_cast<int>(parentNode));
      const Json& children = nodes.at(parentNode).at("children");
      if (!children.isArray()) continue;
      for (const auto& child : children.a) {
        const auto childIt = nodeToBone.find(child.intOr(-1));
        if (childIt != nodeToBone.end()) {
          out.skeleton.bones[static_cast<std::size_t>(childIt->second)].parentIndex =
              parentIt == nodeToBone.end() ? -1 : parentIt->second;
        }
      }
    }

    const int rootNode = skin.at("skeleton").intOr(-1);
    const auto rootIt = nodeToBone.find(rootNode);
    out.skeleton.rootBone = rootIt == nodeToBone.end() ? 0 : rootIt->second;

    const auto inverseBinds = readMat4Accessor(root, bin, skin.at("inverseBindMatrices").intOr(-1));
    for (std::size_t i = 0; i < out.skeleton.bones.size() && i < inverseBinds.size(); ++i) {
      out.skeleton.bones[i].inverseBindMatrix = inverseBinds[i];
    }
  }

  static void readAnimations(const Json& root, const std::vector<std::uint8_t>& bin, LoadedMeshAsset& out) {
    const Json& animations = root.at("animations");
    if (!animations.isArray()) return;

    std::unordered_map<int, int> nodeToBone;
    for (std::size_t i = 0; i < out.skeleton.bones.size(); ++i) {
      nodeToBone[out.skeleton.bones[i].nodeIndex] = static_cast<int>(i);
    }

    for (const auto& ja : animations.a) {
      LoadedAnimation clip;
      clip.name = ja.at("name").stringOr("Animation");
      const Json& samplers = ja.at("samplers");
      if (samplers.isArray()) {
        for (const auto& s : samplers.a) {
          const int input = s.at("input").intOr(-1);
          if (input >= 0) {
            const Json& max = root.at("accessors").at(static_cast<std::size_t>(input)).at("max");
            if (max.isArray() && !max.a.empty()) {
              clip.durationSeconds = std::max(clip.durationSeconds, max.at(static_cast<std::size_t>(0)).floatOr());
            }
          }
        }
      }
      const Json& channels = ja.at("channels");
      if (channels.isArray() && samplers.isArray()) {
        for (const auto& jc : channels.a) {
          const int samplerIndex = jc.at("sampler").intOr(-1);
          if (samplerIndex < 0 || static_cast<std::size_t>(samplerIndex) >= samplers.a.size()) continue;
          const Json& target = jc.at("target");
          const auto boneIt = nodeToBone.find(target.at("node").intOr(-1));
          if (boneIt == nodeToBone.end()) continue;

          const Json& sampler = samplers.at(static_cast<std::size_t>(samplerIndex));
          const auto times = readFloatAccessor(root, bin, sampler.at("input").intOr(-1));
          const auto values = readFloatAccessor(root, bin, sampler.at("output").intOr(-1));
          if (times.empty() || values.empty()) continue;

          LoadedAnimation::Channel channel;
          channel.boneIndex = boneIt->second;
          channel.times = times;

          const std::string path = target.at("path").stringOr();
          if (path == "rotation") {
            channel.path = LoadedAnimation::Path::Rotation;
            const std::size_t count = values.size() / 4;
            channel.quatValues.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
              channel.quatValues.push_back({values[i * 4 + 0], values[i * 4 + 1], values[i * 4 + 2], values[i * 4 + 3]});
            }
          } else if (path == "scale") {
            channel.path = LoadedAnimation::Path::Scale;
            const std::size_t count = values.size() / 3;
            channel.vec3Values.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
              channel.vec3Values.push_back({values[i * 3 + 0], values[i * 3 + 1], values[i * 3 + 2]});
            }
          } else {
            channel.path = LoadedAnimation::Path::Translation;
            const std::size_t count = values.size() / 3;
            channel.vec3Values.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
              channel.vec3Values.push_back({values[i * 3 + 0], values[i * 3 + 1], values[i * 3 + 2]});
            }
          }

          if (!channel.times.empty()) {
            clip.durationSeconds = std::max(clip.durationSeconds, channel.times.back());
            clip.channels.push_back(std::move(channel));
          }
        }
      }
      out.animations.push_back(std::move(clip));
    }
  }

  static void readMeshes(const Json& root, const std::vector<std::uint8_t>& bin, LoadedMeshAsset& out) {
    const Json& meshes = root.at("meshes");
    if (!meshes.isArray()) return;

    for (std::size_t meshIndex = 0; meshIndex < meshes.a.size(); ++meshIndex) {
      const Json& jm = meshes.at(meshIndex);
      const Json& primitives = jm.at("primitives");
      if (!primitives.isArray()) continue;
      for (std::size_t primIndex = 0; primIndex < primitives.a.size(); ++primIndex) {
        const Json& jp = primitives.at(primIndex);
        const int posIdx = attrAccessor(jp, "POSITION");
        if (posIdx < 0) continue;

        const auto positions = readFloatAccessor(root, bin, posIdx);
        const auto normals = readFloatAccessor(root, bin, attrAccessor(jp, "NORMAL"));
        const auto texcoords = readFloatAccessor(root, bin, attrAccessor(jp, "TEXCOORD_0"));
        const auto joints = readIndexAccessor(root, bin, attrAccessor(jp, "JOINTS_0"));
        const auto weights = readFloatAccessor(root, bin, attrAccessor(jp, "WEIGHTS_0"));
        auto indices = readIndexAccessor(root, bin, jp.at("indices").intOr(-1));
        const std::size_t vertexCount = positions.size() / 3;
        if (indices.empty()) {
          indices.resize(vertexCount);
          for (std::size_t i = 0; i < vertexCount; ++i) indices[i] = static_cast<std::uint32_t>(i);
        }

        LoadedSubMesh sm;
        sm.name = jm.at("name").stringOr("mesh") + ":" + std::to_string(primIndex);
        sm.materialIndex = static_cast<std::uint32_t>(std::max(0, jp.at("material").intOr(0)));
        sm.indices = std::move(indices);
        sm.vertices.resize(vertexCount);

        for (std::size_t i = 0; i < vertexCount; ++i) {
          auto& v = sm.vertices[i];
          v.px = positions[i * 3 + 0];
          v.py = positions[i * 3 + 1];
          v.pz = positions[i * 3 + 2];
          if (normals.size() >= (i + 1) * 3) {
            v.nx = normals[i * 3 + 0];
            v.ny = normals[i * 3 + 1];
            v.nz = normals[i * 3 + 2];
          }
          if (texcoords.size() >= (i + 1) * 2) {
            v.u = texcoords[i * 2 + 0];
            v.v = texcoords[i * 2 + 1];
          }
          for (int k = 0; k < 4; ++k) {
            if (joints.size() >= i * 4 + static_cast<std::size_t>(k) + 1) v.joints[k] = static_cast<std::uint16_t>(joints[i * 4 + k]);
            if (weights.size() >= i * 4 + static_cast<std::size_t>(k) + 1) v.weights[k] = weights[i * 4 + k];
          }
          out.boundsMin.x = std::min(out.boundsMin.x, v.px);
          out.boundsMin.y = std::min(out.boundsMin.y, v.py);
          out.boundsMin.z = std::min(out.boundsMin.z, v.pz);
          out.boundsMax.x = std::max(out.boundsMax.x, v.px);
          out.boundsMax.y = std::max(out.boundsMax.y, v.py);
          out.boundsMax.z = std::max(out.boundsMax.z, v.pz);
        }
        out.subMeshes.push_back(std::move(sm));
      }
    }
  }
};

}  // namespace

MeshAssetService::MeshAssetService() { registerLoader(".gltf", makeGltfMeshLoader()); }

MeshAssetService::~MeshAssetService() { stop(); }

void MeshAssetService::start() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_running) return;
  m_running = true;
  m_worker = std::thread([this] { workerMain(); });
}

void MeshAssetService::stop() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) return;
    m_running = false;
  }
  if (m_worker.joinable()) m_worker.join();
}

void MeshAssetService::registerLoader(std::string extension, std::unique_ptr<IMeshAssetLoader> loader) {
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  m_loaders[std::move(extension)] = std::move(loader);
}

MeshAssetService::Status MeshAssetService::request(const std::string& path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto& e = m_entries[path];
  if (e.state == State::Unloaded) {
    e.state = State::Queued;
    m_queue.push_back(path);
  }
  return {e.state, e.error};
}

std::optional<LoadedMeshAsset> MeshAssetService::takeReady(const std::string& path) {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_entries.find(path);
  if (it == m_entries.end() || it->second.state != State::Ready || !it->second.ready) return std::nullopt;
  return it->second.ready;
}

MeshAssetService::Status MeshAssetService::status(const std::string& path) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_entries.find(path);
  if (it == m_entries.end()) return {};
  return {it->second.state, it->second.error};
}

void MeshAssetService::workerMain() {
  for (;;) {
    std::string job;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (!m_running) break;
      if (!m_queue.empty()) {
        job = std::move(m_queue.back());
        m_queue.pop_back();
        m_entries[job].state = State::Loading;
      }
    }
    if (job.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      continue;
    }

    LoadedMeshAsset loaded;
    std::string error;
    const IMeshAssetLoader* loader = loaderFor(job);
    const bool ok = loader && loader->load(job, loaded, error);
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto& e = m_entries[job];
      if (ok) {
        e.ready = std::move(loaded);
        e.state = State::Ready;
        e.error.clear();
      } else {
        e.state = State::Failed;
        e.error = loader ? error : "No mesh loader for extension";
      }
    }
  }
}

const IMeshAssetLoader* MeshAssetService::loaderFor(const std::string& path) const {
  const auto it = m_loaders.find(lowerExt(path));
  return it == m_loaders.end() ? nullptr : it->second.get();
}

std::unique_ptr<IMeshAssetLoader> makeGltfMeshLoader() { return std::make_unique<GltfMeshLoader>(); }

}  // namespace assets
