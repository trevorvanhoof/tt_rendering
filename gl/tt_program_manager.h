#pragma once

#include "tt_gl.h"

#define NO_QT

#ifndef NO_QT
#include <QFileSystemWatcher>
#include <QMap>
#include <QStringList>
#include <QString>
#include <QRegularExpression>

namespace TT {
    class Program {
        friend class ProgramManager;
        GLuint handle = 0;
        QStringList filePaths;
        Program(const QStringList& filePaths);
        bool valid();
        GLuint fetch();
        QMap<QString, int> uniformLocations;
        static GLuint active;

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

    public:
        void cleanup();
        void use();
        int uniform(const char* name);
    };

    class ProgramManager : public QObject {
        Q_OBJECT;

        static ProgramManager* singleton;

        QFileSystemWatcher watcher;
        QRegularExpression includes;

        // Each file knows which files to reload when it changes, that is the file itself and any files that (indirectly) include it.
        QMap<QString, QStringList> fileDependents;
        // Each file maps to the final text
        QMap<QString, QString> snippets;
        QMap<QString, QSet<QString>> snippetDependencies;
        // Each shader file maps to a compiled shader
        QMap<QString, GLuint> fileToShader;
        // Each shader file maps to all programs that use that shader
        QMap<QString, QList<Program*>> fileToPrograms;
        // Programs are cached by the files that make up the whole program
        QMap<QStringList, Program*> keyToProgram;

        QSet<QString> watchedFiles;

        ProgramManager();
        ~ProgramManager();
        void invalidate(const QString& filePath);
        void onFileChanged(const QString& filePath);
        void ensureWatched(const QString& filePath);
        GLuint _fetchShader(const QString& filePath);
        Program* _fetchProgram(const QStringList& filePaths);
        QString readWithIncludes(const QString& filePath, QSet<QString>& outDependencies);

        friend class Program;
        static GLuint fetchShader(const QString& filePath);

    public:
        static Program* fetchProgram(const QStringList& filePaths);
    };
}
#else
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "tt_signals.h"
#include "tt_strings.h"

namespace std {
    template <> struct hash<std::vector<std::string>> {
        size_t operator()(const std::vector<std::string>& v) const {
            return std::hash<std::string>()(TT::join(v, ";"));
        }
    };
}

namespace TT {
    class Program {
        friend class ProgramManager;
        GLuint handle = 0;
        std::vector<std::string> filePaths;
        Program(const std::vector<std::string>& filePaths);
        bool valid();
        GLuint fetch();
        std::unordered_map<std::string, int> uniformLocations;
        static GLuint active;

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

    public:
        void cleanup();
        void use();
        int uniform(const char* name);
    };

    class ProgramManager {
        static ProgramManager* singleton;

#ifndef NO_QT
        QFileSystemWatcher watcher;
        QRegularExpression includes;
#endif

        // Each file knows which files to reload when it changes, that is the file itself and any files that (indirectly) include it.
        std::unordered_map<std::string, std::vector<std::string>> fileDependents;
        // Each file maps to the final text
        std::unordered_map<std::string, std::string> snippets;
        std::unordered_map<std::string, std::unordered_set<std::string>> snippetDependencies;
        // Each shader file maps to a compiled shader
        std::unordered_map<std::string, GLuint> fileToShader;
        // Each shader file maps to all programs that use that shader
        std::unordered_map<std::string, std::vector<Program*>> fileToPrograms;
        // Programs are cached by the files that make up the whole program
        std::unordered_map<std::vector<std::string>, Program*> keyToProgram;

        std::unordered_set<std::string> watchedFiles;

        ProgramManager();
        ~ProgramManager();
        void invalidate(const std::string& filePath);
        void onFileChanged(const std::string& filePath);
        void ensureWatched(const std::string& filePath);
        GLuint _fetchShader(const std::string& filePath);
        Program* _fetchProgram(const std::vector<std::string>& filePaths);
        std::string readWithIncludes(const std::string& filePath, std::unordered_set<std::string>& outDependencies);

        friend class Program;
        static GLuint fetchShader(const std::string& filePath);

    public:
        static Program* fetchProgram(const std::vector<std::string>& filePaths);
    };
}
#endif
