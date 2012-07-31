#ifndef CLIENT_VIEW_TEXTURE_MANAGER_H
#define CLIENT_VIEW_TEXTURE_MANAGER_H

#include <map>
#include <string>
#include <GL/gl.h>

#include "common/google.h"

//! TextureManager loads bitmaps to opengl
class TextureManager
{
public:
  TextureManager();

  GLuint LoadImage(const std::string&);
  GLuint GetTexture(const std::string&);
  void FreeTextures();

private:
  DISALLOW_COPY_AND_ASSIGN(TextureManager);
  std::map<std::string, GLuint> textures_;
};

#endif

