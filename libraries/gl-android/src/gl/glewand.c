//
//  glewand.c
//  libraries/gl/src/gl
//
//  Created by Cristian Duarte & Gabriel Calero on 10/3/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "glewand.h"
#include <EGL/egl.h>

#define glewGetProcAddress(name) eglGetProcAddress(name)

PFNGLQUERYCOUNTEREXTPROC __glewQueryCounterEXT = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC __glewGetQueryObjectui64vEXT = NULL;
PFNGLTEXBUFFEREXTPROC __glTexBufferEXT = NULL;
PFNGLDRAWARRAYSINSTANCEDEXTPROC __glewDrawArraysInstancedEXT;
PFNGLFRAMEBUFFERTEXTUREEXTPROC __glewFramebufferTextureEXT;

int GLEWAPIENTRY glewInit (void)
{
  int res = 100;
  GLboolean r = GL_FALSE;

  r = ((glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC)glewGetProcAddress("glQueryCounterEXT")) == NULL) || r;
  if (glQueryCounterEXT == NULL) {
    res += 1;
  }
  r = ((glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)glewGetProcAddress("glGetQueryObjectui64vEXT")) == NULL) || r;
  if (glGetQueryObjectui64vEXT == NULL) {
    res += 2;
  }
  r = ((glTexBufferEXT = (PFNGLTEXBUFFEREXTPROC)glewGetProcAddress("glTexBufferEXT")) == NULL) || r;
  if (glTexBufferEXT == NULL) {
    res += 4;
  }
  r = ((glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXTPROC) glewGetProcAddress("glDrawArraysInstancedEXT")) == NULL) || r;
  if (glDrawArraysInstancedEXT == NULL) {
    res += 8;
  }
  r = ((glFramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXTPROC) glewGetProcAddress("glFramebufferTextureEXT")) == NULL) || r;
  if (glFramebufferTextureEXT == NULL) {
    res += 16;
  }
	return res;
}