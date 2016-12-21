#include "GLShaders.h"

#include "GLLogging.h"

namespace gl {


#ifdef SEPARATE_PROGRAM
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject, GLuint &programObject) {
#else
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject) {
#endif
    qDebug() << "[CRASH] bool compileShader ... shaderSrc "; // << shaderSource.c_str();
    if (shaderSource.empty()) {
        qCDebug(glLogging) << "GLShader::compileShader - no GLSL shader source code ? so failed to create";
        return false;
    }
    qDebug() << "[CRASH] bool compileShader ... shaderDomain " << shaderDomain;

    // Create the shader object
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        qCDebug(glLogging) << "GLShader::compileShader - failed to create the gl shader object";
        return false;
    }

    // Assign the source
    const int NUM_SOURCE_STRINGS = 2;
    const GLchar* srcstr[] = { defines.c_str(), shaderSource.c_str() };
    qDebug() << "[CRASH] compileShader ... defines " << defines.c_str();

    glShaderSource(glshader, NUM_SOURCE_STRINGS, srcstr, NULL);

    qDebug() << "[CRASH] compileShader ... glShaderSource " << NUM_SOURCE_STRINGS << " " << defines.length() << " src " << shaderSource.length();
    static int shaderCnt = 0;
    shaderCnt++;
    if (123 == shaderCnt) {
        qDebug() << "This must be the one that fails " << shaderSource.c_str();
    }
    // Compile !
    glCompileShader(glshader);

    qDebug() << "[CRASH] compileShader ... compiled! ";

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    qDebug() << "[CRASH] compileShader ... GL_COMPILE_STATUS! ";

    // if compilation fails
    if (!compiled) {

        qDebug() << "[CRASH] compileShader ... !Compiled ";

        // save the source code to a temp file so we can debug easily
        /*
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << srcstr[0];
        filestream << srcstr[1];
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);


        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << std::string(temp);
        filestream.close();
        }
        */

        qCWarning(glLogging) << "GLShader::compileShader - failed to compile the gl shader object:";
        for (auto s : srcstr) {
            qCWarning(glLogging) << s;
        }
        qCWarning(glLogging) << "GLShader::compileShader - errors:";
        qCWarning(glLogging) << temp;
        delete[] temp;

        glDeleteShader(glshader);
        return false;
    }

    qDebug() << "[CRASH] compileShader ... here ";

#ifdef SEPARATE_PROGRAM
    GLuint glprogram = 0;
    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(glLogging) << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return false;
    }

    qDebug() << "[CRASH] compileShader ... here 2";

    glProgramParameteri(glprogram, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(glprogram, glshader);
    glLinkProgram(glprogram);
    qDebug() << "[CRASH] compileShader ... linking program";

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
    qDebug() << "[CRASH] compileShader ... linking failed";
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(glLogging) << "GLShader::compileShader -  failed to LINK the gl program object :";
        qCDebug(glLogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << String(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader(glshader);
        glDeleteProgram(glprogram);
        return false;
    }
    programObject = glprogram;
#endif
    shaderObject = glshader;
    qDebug() << "[CRASH] compileShader ... END";
    return true;
}

GLuint compileProgram(const std::vector<GLuint>& glshaders) {
    // A brand new program:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(glLogging) << "GLShader::compileProgram - failed to create the gl program object";
        return 0;
    }

    // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
    // Create the program from the sub shaders
    for (auto so : glshaders) {
        glAttachShader(glprogram, so);
    }

    // Link!
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(glLogging) << "GLShader::compileProgram -  failed to LINK the gl program object :";
        qCDebug(glLogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << std::string(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteProgram(glprogram);
        return 0;
    }

    return glprogram;
}

}
