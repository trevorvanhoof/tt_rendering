#include "tt_program_manager.h"
#include "tt_messages.h"
#include "tt_files.h"
#include <sstream>

#ifndef NO_QT
#include <QFile.h>
#include <QTextStream.h>

namespace {
    GLint getProgrami(GLuint program, GLenum query) {
        GLint v;
        glGetProgramiv(program, query, &v);
        return v;
    }

    QString getProgramInfoLog(GLuint program) {
        GLsizei infoLogSize = getProgrami(program, GL_INFO_LOG_LENGTH);
        GLchar* infoLog = new GLchar[infoLogSize];
        glGetProgramInfoLog(program, infoLogSize, &infoLogSize, infoLog);
        QString result = QString::fromLocal8Bit(infoLog, infoLogSize);
        delete[] infoLog;
        return result;
    }

    GLint getShaderi(GLuint shader, GLenum query) {
        GLint v;
        glGetShaderiv(shader, query, &v);
        return v;
    }

    QString getShaderInfoLog(GLuint shader) {
        GLsizei infoLogSize = getShaderi(shader, GL_INFO_LOG_LENGTH);
        GLchar* infoLog = new GLchar[infoLogSize];
        glGetShaderInfoLog(shader, infoLogSize, &infoLogSize, infoLog);
        QString result = QString::fromLocal8Bit(infoLog, infoLogSize);
        delete[] infoLog;
        return result;
    }

    GLuint compileShader(const QString& code, GLenum mode) {
        GLuint shader = glCreateShader(mode);
        QByteArray codeA = code.toLocal8Bit();
        const char* codeAptr = codeA.data();
        GLsizei length = codeA.size();
        glShaderSource(shader, 1, &codeAptr, &length);
        glCompileShader(shader);
        if (getShaderi(shader, GL_COMPILE_STATUS) == GL_FALSE) {
            QByteArray r = getShaderInfoLog(shader).toLocal8Bit();
            TT::Error("%s\n", r.data());
        }
        return shader;
    }
}

namespace TT {
    ProgramManager::ProgramManager() {
        const char* includePattern = R"(#include \"(.*?(?<!\\))\"|\/\/[^\r\n]*?(?:[\r\n]|$)|\/\*.*?\*\/)";
        includes = QRegularExpression(includePattern, QRegularExpression::DotMatchesEverythingOption);
        QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, this, &ProgramManager::onFileChanged);
    }

    void ProgramManager::invalidate(const QString& filePath) {
        if (snippets.contains(filePath))
            snippets.remove(filePath);

        if (fileToShader.contains(filePath)) {
            glDeleteShader(fileToShader.find(filePath).value());
            fileToShader.remove(filePath);
        }

        if (fileToPrograms.contains(filePath)) {
            for (Program* program : fileToPrograms.find(filePath).value())
                program->cleanup();
        }

        if (fileDependents.contains(filePath)) {
            for (const QString& dependent : fileDependents.find(filePath).value()) {
                if (dependent != filePath)
                    invalidate(dependent);
            }
            fileDependents.remove(filePath);
        }
    }

    ProgramManager::~ProgramManager() {
        for (Program* handle : keyToProgram.values()) {
            handle->cleanup();
        }
    }

    void ProgramManager::onFileChanged(const QString& filePath) {
        watchedFiles.remove(filePath);
        invalidate(filePath);
        ensureWatched(filePath);
    }

    void ProgramManager::ensureWatched(const QString& filePath) {
        if (watchedFiles.contains(filePath))
            return;
        watchedFiles.insert(filePath);
        watcher.addPath(filePath);
    }

    GLuint ProgramManager::_fetchShader(const QString& filePath) {
        if (fileToShader.contains(filePath))
            return fileToShader.find(filePath).value();
        QSet<QString> dependencies;
        QString code = readWithIncludes(filePath, dependencies);
        QStringList parts = filePath.split('.');
        QString identifier = parts[parts.length() - 2];
        GLenum mode;
        if (identifier == "vert")
            mode = GL_VERTEX_SHADER;
        else if (identifier == "frag")
            mode = GL_FRAGMENT_SHADER;
        else if (identifier == "geom")
            mode = GL_GEOMETRY_SHADER;
        else
            mode = GL_COMPUTE_SHADER;
        GLuint shader = compileShader(code, mode);
        for (const QString& dependency : dependencies)
            ensureWatched(dependency);
        fileToShader[filePath] = shader;
        return shader;
    }

    Program* ProgramManager::_fetchProgram(const QStringList& filePaths) {
        if (keyToProgram.contains(filePaths))
            return keyToProgram.find(filePaths).value();
        Program* p = new Program(filePaths);
        for (QString filePath : filePaths) {
            if (!fileToPrograms.contains(filePath))
                fileToPrograms.insert(filePath, { p });
            else
                fileToPrograms.find(filePath).value().append(p);
        }
        keyToProgram[filePaths] = p;
        return p;
    }

    QString ProgramManager::readWithIncludes(const QString& filePath, QSet<QString>& outDependencies) {
        // Return the contents of this file if it is known
        if (snippets.contains(filePath))
            return snippets.find(filePath).value();

        // Read this file
        outDependencies.insert(filePath);

        if (!fileDependents.contains(filePath))
            fileDependents.insert(filePath, {});
        fileDependents.find(filePath).value().append(filePath);

        QFile fh(filePath);
        fh.open(QFile::ReadOnly | QFile::Text);
        QTextStream fhStream(&fh);
        QString code = fhStream.readAll();

        // Parse #include statements that are not commented out
        // Accumulate snippets
        QList<QString> sections;
        int cursor = 0;
        for (const auto& match : includes.globalMatch(code)) {
            QStringList texts = match.capturedTexts();
            if (texts.length() != 2)
                continue;

            // Add snippet between previous include and this include
            sections.append(code.sliced(cursor, match.capturedStart() - cursor));
            cursor = match.capturedEnd();

            // Add snippet from include path
            QSet<QString> dependencies;
            QString incCode = readWithIncludes(texts[1], dependencies);
            for (const QString& dependency : dependencies)
                if (!fileDependents.contains(dependency))
                    fileDependents.insert(dependency, { filePath });
                else
                    fileDependents.find(dependency).value().append(filePath);
            outDependencies += dependencies;
            sections.append(incCode);
        }

        // Add trailing code
        sections.append(code.sliced(cursor));

        // Return code and dependencies
        code = sections.join("");

        // Cache
        snippets[filePath] = code;
        return code;
    }

    GLuint ProgramManager::fetchShader(const QString& filePath) { return singleton->_fetchShader(filePath); }

    Program* ProgramManager::fetchProgram(const QStringList& filePaths) {
        return singleton->_fetchProgram(filePaths);
    }

    ProgramManager* ProgramManager::singleton = new ProgramManager;

    Program::Program(const QStringList& filePaths) :
        filePaths(filePaths) {
    }

    void Program::cleanup() {
        if (handle != 0) {
            glDeleteProgram(handle);
            handle = 0;
        }
    }

    bool Program::valid() {
        return handle != 0;
    }

    GLuint Program::fetch() {
        if (valid())
            return handle;
        handle = glCreateProgram();
        for (const QString& filePath : filePaths) {
            GLuint shader = ProgramManager::fetchShader(filePath);
            glAttachShader(handle, shader);
        }
        glLinkProgram(handle);
        if (getProgrami(handle, GL_LINK_STATUS) == GL_FALSE) {
            auto b = getProgramInfoLog(handle).toLocal8Bit();
            TT::Error("%s\n", b.data());
        }
        glValidateProgram(handle);
        if (getProgrami(handle, GL_VALIDATE_STATUS) == GL_FALSE) {
            auto b = getProgramInfoLog(handle).toLocal8Bit();
            TT::Error("%s\n", b.data());
        }
        return handle;
    }

    void Program::use() {
        fetch();
        if (active == handle)
            return;
        active = handle;
        glUseProgram(handle);
    }

    int Program::uniform(const char* name) {
        const auto& it = uniformLocations.find(name);
        if (it != uniformLocations.end())
            return it.value();
        int v = glGetUniformLocation(fetch(), name);
        uniformLocations.insert(name, v);
        return v;
    }

    GLuint Program::active = 0;
}
#else
namespace {
    GLint getProgrami(GLuint program, GLenum query) {
        GLint v;
        glGetProgramiv(program, query, &v);
        return v;
    }

    std::string getProgramInfoLog(GLuint program) {
        GLsizei infoLogSize = getProgrami(program, GL_INFO_LOG_LENGTH);
        GLchar* infoLog = new GLchar[infoLogSize];
        glGetProgramInfoLog(program, infoLogSize, &infoLogSize, infoLog);
        std::string result(infoLog, infoLogSize);
        delete[] infoLog;
        return result;
    }

    GLint getShaderi(GLuint shader, GLenum query) {
        GLint v;
        glGetShaderiv(shader, query, &v);
        return v;
    }

    std::string getShaderInfoLog(GLuint shader) {
        GLsizei infoLogSize = getShaderi(shader, GL_INFO_LOG_LENGTH);
        GLchar* infoLog = new GLchar[infoLogSize];
        glGetShaderInfoLog(shader, infoLogSize, &infoLogSize, infoLog);
        std::string result(infoLog, infoLogSize);
        delete[] infoLog;
        return result;
    }

    GLuint compileShader(const std::string& code, GLenum mode) {
        GLuint shader = glCreateShader(mode);
        const char* codeAptr = code.data();
        GLsizei length = (GLsizei)code.size();
        glShaderSource(shader, 1, &codeAptr, &length);
        glCompileShader(shader);
        if (getShaderi(shader, GL_COMPILE_STATUS) == GL_FALSE) {
            std::string r = getShaderInfoLog(shader);
            TT::error("%s\n", r.data());
        }
        return shader;
    }
}

namespace TT {

    ProgramManager::ProgramManager() {
        const char* includePattern = R"(#include \"(.*?(?<!\\))\"|\/\/[^\r\n]*?(?:[\r\n]|$)|\/\*.*?\*\/)";
#ifndef NO_QT
        includes = QRegularExpression(includePattern, QRegularExpression::DotMatchesEverythingOption);
        QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, this, &ProgramManager::onFileChanged);
#endif
    }

    void ProgramManager::invalidate(const std::string& filePath) {
        if (snippets.find(filePath) != snippets.end())
            snippets.erase(filePath);

        if (fileToShader.find(filePath) != fileToShader.end()) {
            glDeleteShader(fileToShader.find(filePath)->second);
            fileToShader.erase(filePath);
        }

        if (fileToPrograms.find(filePath) != fileToPrograms.end()) {
            for (Program* program : fileToPrograms.find(filePath)->second)
                program->cleanup();
        }

        if (fileDependents.find(filePath) != fileDependents.end()) {
            for (const std::string& dependent : fileDependents.find(filePath)->second) {
                if (dependent != filePath)
                    invalidate(dependent);
            }
            fileDependents.erase(filePath);
        }
    }

    ProgramManager::~ProgramManager() {
        for (const auto& pair : keyToProgram) {
            pair.second->cleanup();
        }
    }

    void ProgramManager::onFileChanged(const std::string& filePath) {
        watchedFiles.erase(filePath);
        invalidate(filePath);
        ensureWatched(filePath);
    }

    void ProgramManager::ensureWatched(const std::string& filePath) {
        if (watchedFiles.find(filePath) != watchedFiles.end())
            return;
        watchedFiles.insert(filePath);
#ifndef NO_QT
        watcher.addPath(filePath);
#endif
    }

    GLuint ProgramManager::_fetchShader(const std::string& filePath) {
        if (fileToShader.find(filePath) != fileToShader.end())
            return fileToShader.find(filePath)->second;
        std::unordered_set<std::string> dependencies;
        std::string code = readWithIncludes(filePath, dependencies);
        std::vector<std::string> parts = split(filePath, ".");
        std::string identifier = parts[parts.size() - 2];
        GLenum mode;
        if (identifier == "vert")
            mode = GL_VERTEX_SHADER;
        else if (identifier == "frag")
            mode = GL_FRAGMENT_SHADER;
        else if (identifier == "geom")
            mode = GL_GEOMETRY_SHADER;
        else
            mode = GL_COMPUTE_SHADER;
        GLuint shader = compileShader(code, mode);
        for (const std::string& dependency : dependencies)
            ensureWatched(dependency);
        fileToShader[filePath] = shader;
        return shader;
    }

    Program* ProgramManager::_fetchProgram(const std::vector<std::string>& filePaths) {
        if (keyToProgram.find(filePaths) != keyToProgram.end())
            return keyToProgram.find(filePaths)->second;
        Program* p = new Program(filePaths);
        for (std::string filePath : filePaths) {
            if (fileToPrograms.find(filePath) == fileToPrograms.end())
                fileToPrograms.insert_or_assign(filePath, std::vector<Program*> { p });
            else
                fileToPrograms.find(filePath)->second.push_back(p);
        }
        keyToProgram[filePaths] = p;
        return p;
    }

    std::string ProgramManager::readWithIncludes(const std::string& filePath, std::unordered_set<std::string>& outDependencies) {
        // Return the contents of this file if it is known
        if (snippets.find(filePath) != snippets.end())
            return snippets.find(filePath)->second;

        // Read this file
        outDependencies.insert(filePath);

        fileDependents[filePath].push_back(filePath);

        std::string code = readAllBytes(filePath.c_str());

        std::vector<std::string> sections;
        size_t cursor = 0;
        size_t cursor2 = 0;
        while (cursor < code.size()) {
            if (
                code[cursor] == '/' &&
                code[cursor + 1] == '*'
                ) {
                size_t end = code.find("*/", cursor + 2);
                cursor = end + 2;
            }
            else if (
                code[cursor] == '/' &&
                code[cursor + 1] == '/')
            {
                size_t start = cursor;
                cursor += 2;
                while (code[cursor] != '\n' && code[cursor] != '\r' && cursor < code.size())
                    ++cursor;
            } else if (code.compare(cursor, 10, "#include \"") == 0) {
                size_t end = code.find('"', cursor + 10) + 1;
                std::string includeName = code.substr(cursor + 10, end - cursor - 11);

                // Add snippet between previous include and this include
                sections.push_back(code.substr(cursor2, cursor - cursor2));
                cursor = end;
                cursor2 = end;

                // Add snippet from include path
                std::unordered_set<std::string> dependencies;
                std::string incCode = readWithIncludes(includeName, dependencies);
                for (const std::string& dependency : dependencies) {
                    fileDependents[dependency].push_back(filePath);
                    outDependencies.insert(dependency);
                }
                sections.emplace_back(incCode);
            } else {
                ++cursor;
            }
        }

        // Add trailing code
        sections.push_back(code.substr(cursor2));

        // Return code and dependencies
        code = join(sections, "");
        // info(code.data());

        // Cache
        snippets[filePath] = code;
        return code;
    }

    GLuint ProgramManager::fetchShader(const std::string& filePath) { return singleton->_fetchShader(filePath); }

    Program* ProgramManager::fetchProgram(const std::vector<std::string>& filePaths) {
        return singleton->_fetchProgram(filePaths);
    }

    ProgramManager* ProgramManager::singleton = new ProgramManager;

    Program::Program(const std::vector<std::string>& filePaths) :
        filePaths(filePaths) {
    }

    void Program::cleanup() {
        if (handle != 0) {
            glDeleteProgram(handle);
            handle = 0;
        }
    }

    bool Program::valid() {
        return handle != 0;
    }

    GLuint Program::fetch() {
        if (valid())
            return handle;
        handle = glCreateProgram();
        for (const std::string& filePath : filePaths) {
            GLuint shader = ProgramManager::fetchShader(filePath);
            glAttachShader(handle, shader);
        }
        glLinkProgram(handle);
        if (getProgrami(handle, GL_LINK_STATUS) == GL_FALSE) {
            auto b = getProgramInfoLog(handle);
            error("%s\n", b.data());
        }
        glValidateProgram(handle);
        if (getProgrami(handle, GL_VALIDATE_STATUS) == GL_FALSE) {
            auto b = getProgramInfoLog(handle);
            error("%s\n", b.data());
        }
        return handle;
    }

    void Program::use() {
        fetch();
        if (active == handle)
            return;
        active = handle;
        glUseProgram(handle);
    }

    int Program::uniform(const char* name) {
        const auto& it = uniformLocations.find(name);
        if (it != uniformLocations.end())
            return it->second;
        int v = glGetUniformLocation(fetch(), name);
        uniformLocations.insert_or_assign(name, v);
        return v;
    }

    GLuint Program::active = 0;
}
#endif
