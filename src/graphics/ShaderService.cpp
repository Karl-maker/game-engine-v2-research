#include "graphics/ShaderService.h"

// Author: Karl-Johan Bailey

#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_set>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace graphics {

namespace {

static std::string injectDefineAfterVersion(const std::string& src, const char* defineLine) {
  const std::string versionPrefix = "#version";
  const auto versionPos = src.find(versionPrefix);
  if (versionPos == std::string::npos) {
    return std::string(defineLine) + "\n" + src;
  }
  const auto lineEnd = src.find('\n', versionPos);
  if (lineEnd == std::string::npos) {
    return src + "\n" + defineLine + "\n";
  }
  std::string out;
  out.reserve(src.size() + std::strlen(defineLine) + 2);
  out.append(src.data(), lineEnd + 1);
  out.append(defineLine);
  out.push_back('\n');
  out.append(src.data() + lineEnd + 1, src.size() - (lineEnd + 1));
  return out;
}

static std::string resolveExistingPath(const std::string& path) {
  std::ifstream f(path);
  if (f.good()) return path;

  std::string candidate = path;
  for (int i = 0; i < 6; ++i) {
    candidate = "../" + candidate;
    std::ifstream pf(candidate);
    if (pf.good()) return candidate;
  }

  return {};
}

}  // namespace

ShaderService::~ShaderService() { clear(); }

void ShaderService::clear() {
  for (auto& [_, p] : m_programs) {
    destroyProgram(p);
  }
  m_programs.clear();
}

std::string ShaderService::readTextFile(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return {};
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

bool ShaderService::fileExists(const std::string& path) {
  std::ifstream f(path);
  return f.good();
}

std::string ShaderService::readShaderSourceWithIncludes(const std::string& path) {
  std::unordered_set<std::string> seen;
  std::function<std::string(const std::string&)> expand = [&](const std::string& p) -> std::string {
    if (seen.count(p)) {
      std::cerr << "Shader include cycle detected at: " << p << "\n";
      return {};
    }
    seen.insert(p);

    const std::string src = readTextFile(p);
    if (src.empty()) return {};

    std::string dir;
    if (auto slash = p.find_last_of("/\\"); slash != std::string::npos) {
      dir = p.substr(0, slash + 1);
    }

    std::stringstream in(src);
    std::stringstream out;
    std::string line;
    while (std::getline(in, line)) {
      const std::string includePrefix = "#include \"";
      if (line.rfind(includePrefix, 0) == 0) {
        const auto endQuote = line.find("\"", includePrefix.size());
        if (endQuote != std::string::npos) {
          const std::string includeRaw = line.substr(includePrefix.size(), endQuote - includePrefix.size());
          const bool absoluteLike = (!includeRaw.empty() && (includeRaw[0] == '/' || includeRaw[0] == '\\'));
          const std::string includePath = absoluteLike ? includeRaw : (dir + includeRaw);
          out << expand(includePath) << "\n";
          continue;
        }
      }
      out << line << "\n";
    }
    return out.str();
  };
  return expand(path);
}

std::uint32_t ShaderService::compileShader(std::uint32_t type, const std::string& source, std::string* outError) {
  const GLuint id = glCreateShader(type);
  const char* src = source.c_str();
  const GLint len = static_cast<GLint>(source.size());
  glShaderSource(id, 1, &src, &len);
  glCompileShader(id);

  GLint ok = 0;
  glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
  if (ok == GL_TRUE) return id;

  GLint logLen = 0;
  glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
  std::string log;
  log.resize(static_cast<std::size_t>(std::max(0, logLen)));
  if (logLen > 0) {
    glGetShaderInfoLog(id, logLen, nullptr, log.data());
  }
  glDeleteShader(id);
  if (outError) *outError = std::move(log);
  return 0;
}

ShaderService::Program ShaderService::linkProgram(std::uint32_t vsId,
                                                  std::uint32_t fsId,
                                                  std::uint32_t tcsId,
                                                  std::uint32_t tesId,
                                                  std::string* outError) {
  Program p{};
  p.vsId = vsId;
  p.fsId = fsId;
  p.tcsId = tcsId;
  p.tesId = tesId;
  p.hasTessellation = (tcsId != 0 && tesId != 0);

  p.programId = glCreateProgram();
  glAttachShader(p.programId, vsId);
  glAttachShader(p.programId, fsId);
  if (p.hasTessellation) {
    glAttachShader(p.programId, tcsId);
    glAttachShader(p.programId, tesId);
  }
  glLinkProgram(p.programId);

  GLint ok = 0;
  glGetProgramiv(p.programId, GL_LINK_STATUS, &ok);
  if (ok == GL_TRUE) return p;

  GLint logLen = 0;
  glGetProgramiv(p.programId, GL_INFO_LOG_LENGTH, &logLen);
  std::string log;
  log.resize(static_cast<std::size_t>(std::max(0, logLen)));
  if (logLen > 0) {
    glGetProgramInfoLog(p.programId, logLen, nullptr, log.data());
  }
  destroyProgram(p);
  if (outError) *outError = std::move(log);
  return {};
}

void ShaderService::destroyProgram(Program& p) {
  if (p.programId) glDeleteProgram(p.programId);
  if (p.vsId) glDeleteShader(p.vsId);
  if (p.fsId) glDeleteShader(p.fsId);
  if (p.tcsId) glDeleteShader(p.tcsId);
  if (p.tesId) glDeleteShader(p.tesId);
  p = {};
}

const ShaderService::Program* ShaderService::getOrCreate(const std::string& shaderKey) {
  auto it = m_programs.find(shaderKey);
  if (it != m_programs.end()) return &it->second;

  const std::string vsPathRaw = shaderKey + ".vert.glsl";
  const std::string fsPathRaw = shaderKey + ".frag.glsl";
  const std::string tcsPathRaw = shaderKey + ".tesc.glsl";
  const std::string tesPathRaw = shaderKey + ".tese.glsl";

  const std::string vsPath = resolveExistingPath(vsPathRaw);
  const std::string fsPath = resolveExistingPath(fsPathRaw);
  const std::string tcsPath = resolveExistingPath(tcsPathRaw);
  const std::string tesPath = resolveExistingPath(tesPathRaw);

  const std::string vsSrc = vsPath.empty() ? std::string{} : readShaderSourceWithIncludes(vsPath);
  const std::string fsSrc = fsPath.empty() ? std::string{} : readShaderSourceWithIncludes(fsPath);
  if (vsSrc.empty() || fsSrc.empty()) {
    std::cerr << "Shader files missing for key \"" << shaderKey << "\"\n";
    m_programs.emplace(shaderKey, Program{});
    return &m_programs.find(shaderKey)->second;
  }

  const bool hasTessFiles = !tcsPath.empty() && !tesPath.empty();

  auto tryBuild = [&](bool withTess) -> Program {
    const bool doTess = withTess && hasTessFiles;

    std::string vsFinal = vsSrc;
    std::string fsFinal = fsSrc;
    std::string tcsFinal;
    std::string tesFinal;

    if (doTess) {
      tcsFinal = readShaderSourceWithIncludes(tcsPath);
      tesFinal = readShaderSourceWithIncludes(tesPath);
      if (tcsFinal.empty() || tesFinal.empty()) return {};

      constexpr const char* kTessDefine = "#define HAS_TESSELLATION 1";
      vsFinal = injectDefineAfterVersion(vsFinal, kTessDefine);
      fsFinal = injectDefineAfterVersion(fsFinal, kTessDefine);
      tcsFinal = injectDefineAfterVersion(tcsFinal, kTessDefine);
      tesFinal = injectDefineAfterVersion(tesFinal, kTessDefine);
    }

    std::string err;
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vsFinal, &err);
    if (!vs) {
      std::cerr << "Vertex shader compile failed (" << vsPath << "):\n" << err << "\n";
      return {};
    }

    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsFinal, &err);
    if (!fs) {
      std::cerr << "Fragment shader compile failed (" << fsPath << "):\n" << err << "\n";
      glDeleteShader(vs);
      return {};
    }

    GLuint tcs = 0;
    GLuint tes = 0;
    if (doTess) {
      tcs = compileShader(GL_TESS_CONTROL_SHADER, tcsFinal, &err);
      if (!tcs) {
        std::cerr << "Tess control shader compile failed (" << tcsPath << "):\n" << err << "\n";
        glDeleteShader(vs);
        glDeleteShader(fs);
        return {};
      }
      tes = compileShader(GL_TESS_EVALUATION_SHADER, tesFinal, &err);
      if (!tes) {
        std::cerr << "Tess eval shader compile failed (" << tesPath << "):\n" << err << "\n";
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteShader(tcs);
        return {};
      }
    }

    Program p = linkProgram(vs, fs, tcs, tes, &err);
    if (!p.programId) {
      std::cerr << "Program link failed (" << shaderKey << "):\n" << err << "\n";
      glDeleteShader(vs);
      glDeleteShader(fs);
      if (tcs) glDeleteShader(tcs);
      if (tes) glDeleteShader(tes);
      return {};
    }

    return p;
  };

  Program p{};
  if (hasTessFiles) {
    p = tryBuild(true);
    if (!p.programId) {
      std::cerr << "Falling back to non-tess program for \"" << shaderKey << "\"\n";
      p = tryBuild(false);
    }
  } else {
    p = tryBuild(false);
  }

  auto [insIt, _] = m_programs.emplace(shaderKey, p);
  return &insIt->second;
}

}  // namespace graphics
