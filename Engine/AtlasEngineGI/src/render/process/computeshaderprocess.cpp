#include "computeshaderprocess.h"
#include "scene.h"
#include "quad.h"

/*
 * Utility functions
 * */
static int nextPowerOfTwo(int x) {
  x--;
  x |= x >> 1; // handle 2 bit numbers
  x |= x >> 2; // handle 4 bit numbers
  x |= x >> 4; // handle 8 bit numbers
  x |= x >> 8; // handle 16 bit numbers
  x |= x >> 16; // handle 32 bit numbers
  x++;
  return x;
}

ComputeShaderProcess::ComputeShaderProcess() :
    RenderProcess(0)
{

}

void ComputeShaderProcess::init(const GLuint &width, const GLuint &height)
{
    RenderProcess::init(width, height);

    m_shader = ComputeShader("shaders/computeshader2");

    /*
     * Creating the Texture
     * */
    m_out_texture = Texture(GL_RGBA32F, width, height, GL_RGBA, GL_FLOAT,
                            NULL, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_out_texture2 = Texture(GL_RGBA32F, width, height, GL_RGBA, GL_FLOAT,
                            NULL, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_tex_w = width;
    m_tex_h = height;

    /*
     * Determining the Work Group Size
     * */

    int work_grp_cnt[3], work_grp_inv;
    // Maximum global work group (total work in a dispatch)
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    printf ("max global (total) work group size x:%i y:%i z:%i\n",
    work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    int work_grp_size[3];
    // Maximum local work group (one shader's slice)
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    printf ("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
    work_grp_size[0], work_grp_size[1], work_grp_size[2]);

    // Maximum compute shader invocations (x * y * z)
	//glGetIntegerv(GL_MAX_COMPUTE_LOCAL_INVOCATIONS, &work_grp_inv);
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf ("max computer shader invocations %i\n", work_grp_inv);

    m_out_textures.push_back(&m_out_texture);
    //m_out_textures.push_back(&m_out_texture2);

}

void ComputeShaderProcess::initMenuElement()
{

}

void ComputeShaderProcess::resize(const GLuint &width, const GLuint &height)
{
    RenderProcess::resize(width, height);

    m_out_texture = Texture(GL_RGBA32F, width, height, GL_RGBA, GL_FLOAT,
                            NULL, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);

//    m_out_texture2 = Texture(GL_RGBA32F, width, height, GL_RGBA, GL_FLOAT,
//                            NULL, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
    m_tex_w = width;
    m_tex_h = height;
}

void ComputeShaderProcess::process(const Quad &quad, const Scene &scene, const GLfloat &render_time, const GLboolean (&keys)[1024])
{

    /*
     * Structure input test
     * */
    const int globalWorkGroupSize = m_tex_w*m_tex_h;
    const int localWorkGroupSize = 32;

    GLuint structBuffer;
    glGenBuffers(1, &structBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, structBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, globalWorkGroupSize*sizeof(couleurStruct), NULL, GL_STATIC_DRAW);

    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT; // invalidate makes a ig difference when re-writting

    couleurStruct *coul;
    coul = (couleurStruct *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 3*sizeof(couleurStruct), bufMask);

    coul[0].r = 1.0;
    coul[0].g = 0.0;
    coul[0].b = 0.0;
    coul[0].a = 1.0;

    coul[1].r = 0.0;
    coul[1].g = 1.0;
    coul[1].b = 0.0;
    coul[1].a = 1.0;

    coul[2].r = 0.0;
    coul[2].g = 0.0;
    coul[2].b = 1.0;
    coul[2].a = 1.0;

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, structBuffer);

    m_out_texture.bindImage(0);
    //m_out_texture2.bindImage(1);

    // Launch compute shader
    m_shader.use();

    glDispatchCompute(m_tex_w, m_tex_h, 1);

    // Prevent samplign before all writes to image are done
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}

