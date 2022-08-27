#ifndef GLB_H_
#define GLB_H_
#include <string>
#include <vector>

class GLB {
public:
  GLB(std::string filePath);

private:
  class Chunk {
  public:
    Chunk(std::ifstream &file);
    const uint32_t chunkLength() { return _chunkLength; }
    const uint32_t chunkType() { return _chunkType; }
    void print();

  private:
    uint32_t _chunkLength;
    uint32_t _chunkType;
    std::vector<unsigned char> chunkData;
  };
  static uint32_t readuint32(std::ifstream &file);
  std::vector<Chunk> chunks;
};

#endif // GLB_H_
