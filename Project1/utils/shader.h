#pragma once

#include "utils.h"

class DefineList : public std::map<std::string, std::string>
{
public:
    DefineList& add(const std::string& name, const std::string& val = "") { (*this)[name] = val; return *this; }
    DefineList& remove(const std::string& name) { (*this).erase(name); return *this; }
    DefineList& add(const DefineList& dl) { for (const auto& p : dl) add(p.first, p.second); return *this; }
    DefineList& remove(const DefineList& dl) { for (const auto& p : dl) remove(p.first); return *this; }
    std::string toDefineString() {
        std::string defStr = "";
        for (const auto& p : *this) {
            defStr += "#define " + p.first + " " + p.second + "\n";
        }
        return defStr;
    }

    DefineList() = default;
    DefineList(std::initializer_list<std::pair<const std::string, std::string>> il) : std::map<std::string, std::string>(il) {}
};

class Shader {
public:
    uint ID = 0;
    bool has_gshader = false;
    std::vector<std::string> paths;
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    std::string vertHead = _glsl_version;
    std::string fragHead = _glsl_version;
    std::string geometryHead = _glsl_version;
    std::vector<DefineList> defs;
    std::string vertDefs;
    std::string fragDefs;
    std::string geometryDefs;

    enum class UniformType {
        Undefined = 0,
        Bool,
        Int,
        Float, 
        Vec2,
        Vec3, 
        Vec4,
        Mat2,
        Mat3,
        Mat4,
        Texture
    };
    struct UniformBuffer {
        GLint location;
        UniformType type;
        void* value;
    };
    mutable std::unordered_map<std::string, UniformBuffer> uBuffers;

    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        uBuffers.clear();
        has_gshader = geometryPath ? true : false;
        if (has_gshader) {
            paths.resize(3);
            defs.resize(3);
        }
        else {
            paths.resize(2);
            defs.resize(2);
        }
        paths[VSHADER] = vertexPath;
        paths[FSHADER] = fragmentPath;
        readFile(vertexCode, vertexPath);
        readFile(fragmentCode, fragmentPath);
        vertHead += preProcess(vertexCode);
        fragHead += preProcess(fragmentCode);
        if (has_gshader) {
            paths[GSHADER] = geometryPath;
            readFile(geometryCode, geometryPath);
            geometryHead += preProcess(geometryCode);
        }
    }
    /** If there are "//++`<path>`" in first several lines in shader file, 
    *  it will be regard as an include command, equals to #include <path>.
    */
    std::string preProcess(std::string code) {
        int pos = 0, line;
        std::string head;
        while (line = code.find('\n', pos)) {
            std::string subs = code.substr(pos, 5);
            if (subs == "//++`") {
                std::string ncode;
                readFile(ncode, code.substr(pos + 5, line - pos - 6).c_str());
                head += preProcess(ncode) + '\n';
                head += ncode;
                pos = line + 1;
            }
            else {
                break;
            }
        }
        return head;
    }
    void setDefineList(uint shader, DefineList ndef) {
        defs[shader] = ndef;
    }
    void addDef(uint shader, const std::string& name, const std::string& val = "") {
        defs[shader].add(name, val);
    }
    void removeDef(uint shader, const std::string& name) {
        defs[shader].remove(name);
    }
    void createProgram() {
        if (ID != 0)
            glDeleteProgram(ID);
        std::string vCodeWithHead = (vertHead + vertexCode);
        std::string fCodeWithHead = (fragHead + fragmentCode);
        std::string gCodeWithHead;
        const char* vShaderCode = vCodeWithHead.c_str();
        const char* fShaderCode = fCodeWithHead.c_str();
        uint vertex, fragment, geometry;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        if (has_gshader)
        {
            gCodeWithHead = (geometryHead + geometryCode);
            const char* gShaderCode = gCodeWithHead.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (has_gshader)
            glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (has_gshader)
            glDeleteShader(geometry);
    }
    void reload() {
        uBuffers.clear();
        readFile(vertexCode, paths[VSHADER].c_str());
        readFile(fragmentCode, paths[FSHADER].c_str());
        vertDefs = defs[VSHADER].toDefineString();
        fragDefs = defs[FSHADER].toDefineString();
        vertHead = _glsl_version + vertDefs + preProcess(vertexCode);
        fragHead = _glsl_version + fragDefs + preProcess(fragmentCode);
        if (has_gshader) {
            readFile(geometryCode, paths[GSHADER].c_str());
            geometryDefs = defs[GSHADER].toDefineString();
            geometryHead = _glsl_version + geometryDefs + preProcess(geometryCode);
        }
        createProgram();
    }
private:
    void readFile(std::string& str_in, const char* path) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            file.open(path);
            std::stringstream fstream;
            fstream << file.rdbuf();
            file.close();
            str_in = fstream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
    }
    void checkCompileErrors(GLuint shader, std::string type){
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }

public:
    // activate the shader
    // ------------------------------------------------------------------------
    void use(){
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    UniformBuffer& getUniform(const std::string& name) const {
        if(uBuffers.find(name) != uBuffers.end())
            return uBuffers[name];
        uBuffers[name] = { glGetUniformLocation(ID, name.c_str()), UniformType::Undefined };
        return uBuffers[name];
    }
    void setBool(const std::string& name, bool value) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Bool)
            uniform.value = new bool;
        else if (*((bool*)uniform.value) == value)
            return;
        uniform.type = UniformType::Bool;
        *((bool*)uniform.value) = value;
        glUniform1i(uniform.location, (int)value);
    }
    void setInt(const std::string& name, int value) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Int)
            uniform.value = new int;
        else if (*((int*)uniform.value) == value)
            return;
        uniform.type = UniformType::Int;
        *((int*)uniform.value) = value;
        glUniform1i(uniform.location, value);
    }
    void setFloat(const std::string& name, float value) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Float)
            uniform.value = new float;
        else if (*((float*)uniform.value) == value)
            return;
        uniform.type = UniformType::Float;
        *((float*)uniform.value) = value;
        glUniform1f(uniform.location, value);
    }
    void setVec2(const std::string& name, const glm::vec2& value) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Vec2)
            uniform.value = new glm::vec2;
        else if (*((glm::vec2*)uniform.value) == value)
            return;
        uniform.type = UniformType::Vec2;
        *((glm::vec2*)uniform.value) = value;
        glUniform2fv(uniform.location, 1, &value[0]);
    }
    void setVec3(const std::string& name, const glm::vec3& value) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Vec3)
            uniform.value = new glm::vec3;
        else if (*((glm::vec3*)uniform.value) == value)
            return;
        uniform.type = UniformType::Vec3;
        *((glm::vec3*)uniform.value) = value;
        glUniform3fv(uniform.location, 1, &value[0]);
    }
    void setVec4(const std::string& name, const glm::vec4& value) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Vec4)
            uniform.value = new glm::vec4;
        else if (*((glm::vec4*)uniform.value) == value)
            return;
        uniform.type = UniformType::Vec4;
        *((glm::vec4*)uniform.value) = value;
        glUniform4fv(uniform.location, 1, &value[0]);
    }
    void setMat2(const std::string& name, const glm::mat2& mat) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Mat2)
            uniform.value = new glm::mat2;
        else if (*((glm::mat2*)uniform.value) == mat)
            return;
        uniform.type = UniformType::Mat2;
        *((glm::mat2*)uniform.value) = mat;
        glUniformMatrix2fv(uniform.location, 1, GL_FALSE, &mat[0][0]);
    }
    void setMat3(const std::string& name, const glm::mat3& mat) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Mat3)
            uniform.value = new glm::mat3;
        else if (*((glm::mat3*)uniform.value) == mat)
            return;
        uniform.type = UniformType::Mat3;
        *((glm::mat3*)uniform.value) = mat;
        glUniformMatrix3fv(uniform.location, 1, GL_FALSE, &mat[0][0]);
    }
    void setMat4(const std::string& name, const glm::mat4& mat) const{
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Mat4)
            uniform.value = new glm::mat4;
        else if (*((glm::mat4*)uniform.value) == mat)
            return;
        uniform.type = UniformType::Mat4;
        *((glm::mat4*)uniform.value) = mat;
        glUniformMatrix4fv(uniform.location, 1, GL_FALSE, &mat[0][0]);
    }
    void setTextureSource(const std::string& name, int index, GLuint texture_id, GLenum target = GL_TEXTURE_2D) const{
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(target, texture_id);
        glActiveTexture(GL_TEXTURE0);
        UniformBuffer& uniform = getUniform(name);
        if (uniform.type != UniformType::Texture)
            uniform.value = new int;
        else if (*((int*)uniform.value) == index)
            return;
        uniform.type = UniformType::Texture;
        *((int*)uniform.value) = index;
        glUniform1i(uniform.location, index);
    }
};

class ShaderManager {
public:
    enum class Status : int {
        Loaded,
        Compiled
    };
    std::vector<Shader*> shaders;
    std::vector<Status> status;

    ShaderManager() {};
    void init() {
        uint shader_amount = _shader_paths.size() / 3;
        shaders.resize(shader_amount);
        status.resize(shader_amount);
        for (uint i = 0; i < shader_amount; i++) {
            shaders[i] = new Shader(_shader_paths[i * 3], _shader_paths[i * 3 + 1], _shader_paths[i * 3 + 2]);
            status[i] = Status::Loaded;
        }
    }
    Shader* getShader(uint index) {
        if (status[index] == Status::Compiled) {
            return shaders[index];
        }
        else if (status[index] == Status::Loaded) {
            shaders[index]->createProgram();
            status[index] = Status::Compiled;
            return shaders[index];
        }
    }
    void reload(int index = -1) {
        if (index == -1) {
            for (uint i = 0; i < shaders.size(); i++) {
                shaders[i]->reload();
                status[i] = Status::Loaded;
            }
        }else{
            shaders[index]->reload();
            status[index] = Status::Loaded;
        }
    }
};
// All shaders are accessible with ShaderManager
ShaderManager sManager;