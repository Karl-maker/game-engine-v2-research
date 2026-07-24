#include "ecs/services/FileChunkSource.h"

// Author: Karl-Johan Bailey

#include "data/JsonUtil.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace ecs::services {

namespace {

std::string readTextFile(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) return {};
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

bool readCoord(const data::JsonValue& v, ChunkCoord& out) {
  if (const auto* a = v.tryArray()) {
    if (a->size() < 2) return false;
    int x = 0, y = 0;
    if (!data::readInt((*a)[0], x) || !data::readInt((*a)[1], y)) return false;
    out = {x, y};
    return true;
  }
  if (const auto* o = v.tryObject()) {
    int x = 0, y = 0;
    const data::JsonValue* jx = data::getObjectKey(*o, "x");
    const data::JsonValue* jy = data::getObjectKey(*o, "y");
    if (!jx || !jy) return false;
    if (!data::readInt(*jx, x) || !data::readInt(*jy, y)) return false;
    out = {x, y};
    return true;
  }
  return false;
}

}  // namespace

FileChunkSource::FileChunkSource(std::string path) : m_path(std::move(path)) { (void)reload(); }

bool FileChunkSource::reload() {
  m_chunks.clear();
  if (m_path.empty()) return false;

  const std::string text = readTextFile(m_path);
  if (text.empty()) {
    std::cerr << "[chunks] failed to read chunk config: " << m_path << "\n";
    return false;
  }

  auto parsed = data::parseJson(text);
  if (!parsed.ok) {
    std::cerr << "[chunks] JSON parse error at " << parsed.errorOffset << " in " << m_path << ": " << parsed.error << "\n";
    return false;
  }

  const auto* root = parsed.value.tryObject();
  if (!root) {
    std::cerr << "[chunks] root must be an object: " << m_path << "\n";
    return false;
  }

  const data::JsonValue* chunksV = data::getObjectKey(*root, "chunks");
  const auto* chunksArr = chunksV ? chunksV->tryArray() : nullptr;
  if (!chunksArr) {
    std::cerr << "[chunks] missing \"chunks\" array: " << m_path << "\n";
    return false;
  }

  for (const auto& c : *chunksArr) {
    const auto* obj = c.tryObject();
    if (!obj) continue;

    ChunkCoord coord{};
    if (const auto* coordV = data::getObjectKey(*obj, "coord")) {
      if (!readCoord(*coordV, coord)) continue;
    } else {
      const data::JsonValue* xV = data::getObjectKey(*obj, "x");
      const data::JsonValue* yV = data::getObjectKey(*obj, "y");
      int x = 0, y = 0;
      if (!xV || !yV) continue;
      if (!data::readInt(*xV, x) || !data::readInt(*yV, y)) continue;
      coord = {x, y};
    }

    ChunkDefinition def{};
    def.coord = coord;

    if (const auto* entsV = data::getObjectKey(*obj, "entities")) {
      if (const auto* ents = entsV->tryArray()) {
        def.entities.reserve(ents->size());
        for (const auto& e : *ents) {
          const auto* eobj = e.tryObject();
          if (!eobj) continue;
          ChunkEntitySpawn spawn{};
          spawn.factory = data::getStringOr(*eobj, "factory", "");
          if (spawn.factory.empty()) continue;
          if (const auto* cfg = data::getObjectKey(*eobj, "config")) {
            spawn.config = *cfg;
          } else {
            spawn.config = data::JsonValue(data::JsonValue::Object{});
          }
          def.entities.push_back(std::move(spawn));
        }
      }
    }

    m_chunks.emplace(coord, std::move(def));
  }

  return true;
}

std::optional<ChunkDefinition> FileChunkSource::loadChunk(const ChunkCoord& coord) {
  auto it = m_chunks.find(coord);
  if (it == m_chunks.end()) return std::nullopt;
  return it->second;
}

}  // namespace ecs::services

