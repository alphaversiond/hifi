
//
//  glewand.h
//  libraries/gl/src/gl
//
//  Created by Cristian Duarte & Gabriel Calero on 10/3/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef __glewand_h__
#define __glewand_h__

#ifndef __gl31_h_
#include <GLES3/gl31.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GLEWAPI extern
#define GLEW_GET_FUN(x) x
#define GLEW_FUN_EXPORT GLEWAPI
#define GLEWAPIENTRY

// gl2ext.h 40
#ifndef GL_APIENTRYP
#define GL_APIENTRYP GL_APIENTRY*
#endif



// gl2ext.h 229
#ifndef GL_OES_depth32
#define GL_OES_depth32 1
#define GL_DEPTH_COMPONENT32_OES          0x81A7
#endif /* GL_OES_depth32 */

// gl2ext.h 765
#ifndef GL_EXT_disjoint_timer_query
#define GL_EXT_disjoint_timer_query 1
#define GL_QUERY_COUNTER_BITS_EXT         0x8864
#define GL_CURRENT_QUERY_EXT              0x8865
#define GL_QUERY_RESULT_EXT               0x8866
#define GL_QUERY_RESULT_AVAILABLE_EXT     0x8867
#define GL_TIME_ELAPSED_EXT               0x88BF
#define GL_TIMESTAMP_EXT                  0x8E28
#define GL_GPU_DISJOINT_EXT               0x8FBB
typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (GL_APIENTRYP PFNGLISQUERYEXTPROC) (GLuint id);
typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLQUERYCOUNTEREXTPROC) (GLuint id, GLenum target);
typedef void (GL_APIENTRYP PFNGLGETQUERYIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUIVEXTPROC) (GLuint id, GLenum pname, GLuint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTI64VEXTPROC) (GLuint id, GLenum pname, GLint64 *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64 *params);

// gl2ext.h 866
#ifndef GL_EXT_draw_instanced
#define GL_EXT_draw_instanced 1
typedef void (GL_APIENTRYP PFNGLDRAWARRAYSINSTANCEDEXTPROC) (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSINSTANCEDEXTPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
#ifdef GL_GLEXT_PROTOTYPES
//L_APICALL void GL_APIENTRY glDrawArraysInstancedEXT (GLenum mode, GLint start, GLsizei count, GLsizei primcount);
//GL_APICALL void GL_APIENTRY glDrawElementsInstancedEXT (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
#endif
#endif /* GL_EXT_draw_instanced */

// gl2ext.h 876
#ifndef GL_EXT_geometry_shader
#define GL_EXT_geometry_shader 1
#define GL_GEOMETRY_SHADER_EXT            0x8DD9
#define GL_GEOMETRY_SHADER_BIT_EXT        0x00000004
#define GL_GEOMETRY_LINKED_VERTICES_OUT_EXT 0x8916
#define GL_GEOMETRY_LINKED_INPUT_TYPE_EXT 0x8917
#define GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT 0x8918
#define GL_GEOMETRY_SHADER_INVOCATIONS_EXT 0x887F
#define GL_LAYER_PROVOKING_VERTEX_EXT     0x825E
#define GL_LINES_ADJACENCY_EXT            0x000A
#define GL_LINE_STRIP_ADJACENCY_EXT       0x000B
#define GL_TRIANGLES_ADJACENCY_EXT        0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT   0x000D
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT 0x8A2C
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8A32
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT 0x8DE1
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT 0x8E5A
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT 0x8C29
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT 0x92CF
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT 0x92D5
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT 0x90CD
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT 0x90D7
#define GL_FIRST_VERTEX_CONVENTION_EXT    0x8E4D
#define GL_LAST_VERTEX_CONVENTION_EXT     0x8E4E
#define GL_UNDEFINED_VERTEX_EXT           0x8260
#define GL_PRIMITIVES_GENERATED_EXT       0x8C87
#define GL_FRAMEBUFFER_DEFAULT_LAYERS_EXT 0x9312
#define GL_MAX_FRAMEBUFFER_LAYERS_EXT     0x9317
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT 0x8DA8
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT 0x8DA7
#define GL_REFERENCED_BY_GEOMETRY_SHADER_EXT 0x9309
typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTUREEXTPROC) (GLenum target, GLenum attachment, GLuint texture, GLint level);
#ifdef GL_GLEXT_PROTOTYPES
//GL_APICALL void GL_APIENTRY glFramebufferTextureEXT (GLenum target, GLenum attachment, GLuint texture, GLint level);
#endif
#endif /* GL_EXT_geometry_shader */


#define glQueryCounterEXT GLEW_GET_FUN(__glewQueryCounterEXT)
#define glGetQueryObjectui64vEXT GLEW_GET_FUN(__glewGetQueryObjectui64vEXT)
#define glTexBufferEXT GLEW_GET_FUN(__glTexBufferEXT)

#define glDrawArraysInstancedEXT GLEW_GET_FUN(__glewDrawArraysInstancedEXT)
#define glFrameBufferTextureEXT GLEW_GET_FUN(__glewFramebufferTextureEXT)

#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glGenQueriesEXT (GLsizei n, GLuint *ids);
GL_APICALL void GL_APIENTRY glDeleteQueriesEXT (GLsizei n, const GLuint *ids);
GL_APICALL GLboolean GL_APIENTRY glIsQueryEXT (GLuint id);
GL_APICALL void GL_APIENTRY glBeginQueryEXT (GLenum target, GLuint id);
GL_APICALL void GL_APIENTRY glEndQueryEXT (GLenum target);
//GL_APICALL void GL_APIENTRY glQueryCounterEXT (GLuint id, GLenum target);
GL_APICALL void GL_APIENTRY glGetQueryivEXT (GLenum target, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glGetQueryObjectivEXT (GLuint id, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glGetQueryObjectuivEXT (GLuint id, GLenum pname, GLuint *params);
GL_APICALL void GL_APIENTRY glGetQueryObjecti64vEXT (GLuint id, GLenum pname, GLint64 *params);
//GL_APICALL void GL_APIENTRY glGetQueryObjectui64vEXT (GLuint id, GLenum pname, GLuint64 *params);
#endif
#endif /* GL_EXT_disjoint_timer_query */

// gl2ext.h 1047
#ifndef GL_EXT_sRGB_write_control
#define GL_EXT_sRGB_write_control 1
#define GL_FRAMEBUFFER_SRGB_EXT           0x8DB9
#endif /* GL_EXT_sRGB_write_control */

// gl2ext.h 1239
#ifndef GL_EXT_texture_border_clamp
#define GL_EXT_texture_border_clamp 1
#define GL_TEXTURE_BORDER_COLOR_EXT       0x1004
#define GL_CLAMP_TO_BORDER_EXT            0x812D
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERIIVEXTPROC) (GLenum target, GLenum pname, const GLint *params);
typedef void (GL_APIENTRYP PFNGLTEXPARAMETERIUIVEXTPROC) (GLenum target, GLenum pname, const GLuint *params);
typedef void (GL_APIENTRYP PFNGLGETTEXPARAMETERIIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETTEXPARAMETERIUIVEXTPROC) (GLenum target, GLenum pname, GLuint *params);
typedef void (GL_APIENTRYP PFNGLSAMPLERPARAMETERIIVEXTPROC) (GLuint sampler, GLenum pname, const GLint *param);
typedef void (GL_APIENTRYP PFNGLSAMPLERPARAMETERIUIVEXTPROC) (GLuint sampler, GLenum pname, const GLuint *param);
typedef void (GL_APIENTRYP PFNGLGETSAMPLERPARAMETERIIVEXTPROC) (GLuint sampler, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETSAMPLERPARAMETERIUIVEXTPROC) (GLuint sampler, GLenum pname, GLuint *params);
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glTexParameterIivEXT (GLenum target, GLenum pname, const GLint *params);
GL_APICALL void GL_APIENTRY glTexParameterIuivEXT (GLenum target, GLenum pname, const GLuint *params);
GL_APICALL void GL_APIENTRY glGetTexParameterIivEXT (GLenum target, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glGetTexParameterIuivEXT (GLenum target, GLenum pname, GLuint *params);
GL_APICALL void GL_APIENTRY glSamplerParameterIivEXT (GLuint sampler, GLenum pname, const GLint *param);
GL_APICALL void GL_APIENTRY glSamplerParameterIuivEXT (GLuint sampler, GLenum pname, const GLuint *param);
GL_APICALL void GL_APIENTRY glGetSamplerParameterIivEXT (GLuint sampler, GLenum pname, GLint *params);
GL_APICALL void GL_APIENTRY glGetSamplerParameterIuivEXT (GLuint sampler, GLenum pname, GLuint *params);
#endif
#endif /* GL_EXT_texture_border_clamp */

// gl2ext.h 1263
#ifndef GL_EXT_texture_buffer
#define GL_EXT_texture_buffer 1
#define GL_TEXTURE_BUFFER_EXT             0x8C2A
#define GL_TEXTURE_BUFFER_BINDING_EXT     0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_EXT    0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_EXT     0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT 0x8C2D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT 0x919F
#define GL_SAMPLER_BUFFER_EXT             0x8DC2
#define GL_INT_SAMPLER_BUFFER_EXT         0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT 0x8DD8
#define GL_IMAGE_BUFFER_EXT               0x9051
#define GL_INT_IMAGE_BUFFER_EXT           0x905C
#define GL_UNSIGNED_INT_IMAGE_BUFFER_EXT  0x9067
#define GL_TEXTURE_BUFFER_OFFSET_EXT      0x919D
#define GL_TEXTURE_BUFFER_SIZE_EXT        0x919E
typedef void (GL_APIENTRYP PFNGLTEXBUFFEREXTPROC) (GLenum target, GLenum internalformat, GLuint buffer);
typedef void (GL_APIENTRYP PFNGLTEXBUFFERRANGEEXTPROC) (GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
#ifdef GL_GLEXT_PROTOTYPES
//GL_APICALL void GL_APIENTRY glTexBufferEXT (GLenum target, GLenum internalformat, GLuint buffer);
GL_APICALL void GL_APIENTRY glTexBufferRangeEXT (GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
#endif
#endif /* GL_EXT_texture_buffer */

// gl2ext.h 1312
#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif /* GL_EXT_texture_filter_anisotropic */


GLEW_FUN_EXPORT PFNGLQUERYCOUNTEREXTPROC __glewQueryCounterEXT;
GLEW_FUN_EXPORT PFNGLGETQUERYOBJECTUI64VEXTPROC __glewGetQueryObjectui64vEXT;
GLEW_FUN_EXPORT PFNGLTEXBUFFEREXTPROC __glTexBufferEXT;

GLEW_FUN_EXPORT PFNGLDRAWARRAYSINSTANCEDEXTPROC __glewDrawArraysInstancedEXT;
GLEW_FUN_EXPORT PFNGLFRAMEBUFFERTEXTUREEXTPROC __glewFrameBufferTextureEXT;

GLEWAPI GLenum GLEWAPIENTRY glewInit (void);

/* error codes */
#define GLEW_OK 0
#define GLEW_NO_ERROR 0
#define GLEW_ERROR_NO_GL_VERSION 1  /* missing GL version */


#ifdef __cplusplus
}
#endif


#endif // __glewand_h__