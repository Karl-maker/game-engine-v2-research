// Author: Karl-Johan Bailey

#include "ecs/services/SvgDrawService.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace ecs::services {

namespace {

static std::string readFileToString(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return {};
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::string trim(std::string s) {
  auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
  while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
  return s;
}

static std::optional<std::string> findAttrValue(const std::string& text, const char* key) {
  const std::string needle = std::string(key) + "=\"";
  const std::size_t start = text.find(needle);
  if (start == std::string::npos) return std::nullopt;
  const std::size_t valueStart = start + needle.size();
  const std::size_t end = text.find('"', valueStart);
  if (end == std::string::npos) return std::nullopt;
  return text.substr(valueStart, end - valueStart);
}

static float parseLeadingFloat(const std::string& s) {
  std::string t;
  t.reserve(s.size());
  for (char c : s) {
    if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+') {
      t.push_back(c);
    } else {
      break;
    }
  }
  if (t.empty()) return 0.0f;
  try {
    return static_cast<float>(std::stod(t));
  } catch (...) {
    return 0.0f;
  }
}

static bool parseViewBox(const std::string& viewBox, float& outW, float& outH) {
  std::istringstream ss(viewBox);
  float minX = 0.0f, minY = 0.0f, w = 0.0f, h = 0.0f;
  ss >> minX >> minY >> w >> h;
  (void)minX;
  (void)minY;
  if (!(w > 0.0f) || !(h > 0.0f)) return false;
  outW = w;
  outH = h;
  return true;
}

static SvgAssetInfo inspectUncached(const std::string& svgPath) {
  SvgAssetInfo info;
  info.path = svgPath;

  const std::string text = readFileToString(svgPath);
  if (text.empty()) return info;

  float w = 0.0f;
  float h = 0.0f;
  if (const auto vw = findAttrValue(text, "width")) w = parseLeadingFloat(*vw);
  if (const auto vh = findAttrValue(text, "height")) h = parseLeadingFloat(*vh);

  // Prefer viewBox if width/height are missing or unusable.
  if (!(w > 0.0f) || !(h > 0.0f)) {
    if (const auto vb = findAttrValue(text, "viewBox")) {
      float vbW = 0.0f, vbH = 0.0f;
      if (parseViewBox(trim(*vb), vbW, vbH)) {
        w = vbW;
        h = vbH;
      }
    }
  }

  if (w > 0.0f && h > 0.0f) {
    info.width = w;
    info.height = h;
    info.aspect = w / h;
    info.valid = true;
  }
  return info;
}

static bool hasSuffixCaseInsensitive(const std::string& s, const char* suffix) {
  const std::string suf(suffix ? suffix : "");
  if (suf.empty() || s.size() < suf.size()) return false;
  const std::size_t start = s.size() - suf.size();
  for (std::size_t i = 0; i < suf.size(); ++i) {
    const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(s[start + i])));
    const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(suf[i])));
    if (a != b) return false;
  }
  return true;
}

static std::string hexHash(std::size_t v) {
  std::ostringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << static_cast<unsigned long long>(v);
  return ss.str();
}

static std::string resolveAssetPath(const std::string& path) {
  if (path.empty()) return path;

  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path input(path);
  if (input.is_absolute() && fs::exists(input, ec) && !ec) return input.string();
  if (fs::exists(input, ec) && !ec) return fs::absolute(input, ec).string();

  fs::path probe = fs::current_path(ec);
  for (int i = 0; i < 8 && !probe.empty(); ++i) {
    const fs::path candidate = probe / path;
    if (fs::exists(candidate, ec) && !ec) return fs::absolute(candidate, ec).string();
    const fs::path parent = probe.parent_path();
    if (parent == probe) break;
    probe = parent;
  }
  return path;
}

static std::string rasterizeSvgToCachedPng(const std::string& svgPath, int rasterHeightPx) {
  const std::string resolvedPath = resolveAssetPath(svgPath);
  if (!hasSuffixCaseInsensitive(resolvedPath, ".svg")) return resolvedPath;
  rasterHeightPx = std::max(8, rasterHeightPx);

  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path cacheDir = fs::temp_directory_path(ec) / "duppy_svg_cache";
  if (!ec) fs::create_directories(cacheDir, ec);

  const auto mtime = fs::last_write_time(fs::path(resolvedPath), ec);
  const auto mtimeCount = ec ? 0ULL : static_cast<unsigned long long>(mtime.time_since_epoch().count());

  const std::string key = resolvedPath + "|" + std::to_string(rasterHeightPx) + "|" + std::to_string(mtimeCount);
  const std::size_t h = std::hash<std::string>{}(key);
  const std::string hash = hexHash(h);

  const fs::path cachedSvg = cacheDir / (hash + ".svg");
  const fs::path cachedPng = cacheDir / (hash + ".svg.png");  // qlmanage output naming

  if (fs::exists(cachedPng, ec) && !ec) return cachedPng.string();

  // Copy the SVG into the cache so the generated PNG name is stable.
  fs::copy_file(fs::path(resolvedPath), cachedSvg, fs::copy_options::overwrite_existing, ec);
  if (ec) return resolvedPath;

  // Use QuickLook to rasterize the SVG.
  // Note: qlmanage writes `<input>.png` into the output dir.
  std::ostringstream cmd;
  cmd << "qlmanage -t -s " << rasterHeightPx << " -o " << cacheDir.string() << " " << cachedSvg.string() << " >/dev/null 2>&1";
  const int rc = std::system(cmd.str().c_str());
  (void)rc;

  if (fs::exists(cachedPng, ec) && !ec) return cachedPng.string();
  return resolvedPath;
}

}  // namespace

SvgAssetInfo SvgDrawService::inspect(const std::string& svgPath) {
  static std::unordered_map<std::string, SvgAssetInfo> cache;
  const std::string resolvedPath = resolveAssetPath(svgPath);
  auto it = cache.find(resolvedPath);
  if (it != cache.end()) return it->second;
  SvgAssetInfo info = inspectUncached(resolvedPath);
  cache.emplace(resolvedPath, info);
  return info;
}

render::AssetRef SvgDrawService::textureRef(const std::string& svgPath, int rasterHeightPx) {
  render::AssetRef ref;
  ref.enabled = true;
  ref.key = rasterizeSvgToCachedPng(svgPath, rasterHeightPx);
  ref.id = 0;
  return ref;
}

SvgSizeResult SvgDrawService::computeSize(const std::string& svgPath, const SvgSizeRequest& req) {
  SvgSizeResult out;
  const SvgAssetInfo info = inspect(svgPath);
  if (!info.valid) return out;

  const float aspect = info.aspect > 0.0f ? info.aspect : 1.0f;
  out.aspect = aspect;
  out.valid = true;

  // Screen-space.
  float pxH = info.height;
  if (req.targetHeightPx) pxH = std::max(1.0f, *req.targetHeightPx);
  pxH = std::max(1.0f, pxH * std::max(0.0f, req.scale));
  out.sizePx = {pxH * aspect, pxH};

  // World-space.
  float mH = 0.25f;  // a conservative default if no explicit request is given
  if (req.targetHeightMeters) mH = std::max(0.001f, *req.targetHeightMeters);
  mH = std::max(0.001f, mH * std::max(0.0f, req.scale));
  out.sizeMeters = {mH * aspect, mH};

  return out;
}

}  // namespace ecs::services
