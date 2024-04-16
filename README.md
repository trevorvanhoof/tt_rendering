# tt_cpplib
Compact C++ utilities for Windows.

I find myself building similar utilities every project I mess around with, so I decided to start cleaning and collecting utilities.

Disclaimer: 
I am not as versed in C++, and frankly usually care more about building stuff. My applications tend to run on a modern Windows desktop without
doing anything so demanding it really needs highly optimized C++, so much of this code may not be optimized.

Requires C++20 (designated initializes, non-const std::string::data()).

## Modules

### Core

#### CGMath

Math that should match OpenGL conventions.

Should match the output of old style OpenGL with glTranslate, glRotate and glFrustum calls.

You can compare it with that using this to override the state with the CGMath state:

```
glMatrixMode(GL_PROJECTION)
glLoadMatrixf(P.m)

glMatrixMode(GL_MODELVIEW)
glLoadMatrixf((M * V).m)
```

Or use it as a uniform:

```
glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"), 1, false, (M * V * P).m);
```

and with the following vertex shader:

```
#version 410
layout(location = 0) in vec3 inPosition;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(inPosition, 1.0);
}
```

#### Components

This is a basic entity-component system that allows setting up a hierarchy (tree) of transforms,
cameras, and drawables, resulting in an easy way to render hierarchies of 3D models.

#### Files

File IO utilities that avoid having to deal with the horror that is C++ IO.
Using fopen and FILE over ifstream because sometimes SEEK_END on an ifstream yields in numbers != the actual file size and fread with large numbers does not guarantee it will read the requested number of bytes even if the file has that many bytes left.

#### Json5

Header-only json parser depending only on the standard library.
Can parse any istream.

Example usage:
```c++
#include <iostream>

// Include and implement json with wstrings.
#define TT_JSON5_USE_WSTR
#define TT_JSON5_IMPLEMENTATION
#include "tt_json5.h"

// Set a global error handler in case a value can not be found. In this case just trigger a breakpoint.
#include <Windows.h>
TTJson::Value::castErrorHandler = DebugBreak;

int main() {
    // Open a file as UTF8 encoded wifstream.
    TTJson::ifstream_t stream = TTJson::readUtf8("my_data.json");
    // Create a parser and a document.
    TTJson::Parser parser;
    TTJson::Value result;
    // Parse the stream into the document.
    parser.parse(stream, result);
    // Print an element of the document.
    // If castErrorHandler were unset and the document mismatches the expectations, this would silently fail and print an empty string.
    std::wcout << result.asObject().get(L"keywords").asArray()[0].asString().c_str() << std::endl;
    return 0;
}
```

#### Math

Most of this exists in the standard library, but with added support for cgmath vectors.
I.e. min() will perform a component-wise min on the components.
It also contains a mod() that uses glsl-style modulo (creating a continuous sawtooth from negative to positive, no mirror image around 0. fract() also uses this mod.

#### Messages

Utilities for formatting strings, logging, warning and raising errors.

If you are not debugging, every message will be shown as a dialog, we do not use exceptions so a fatal error will likely crash directly after.
If you are debugging, every message will be sent to the output log, warnings and errors will break with __debugbreak().

#### Signals

Signal & slot pattern that does not require code generation.
When you have an object that wants its method to be called, you can add a Lifetime member to it and use the macro `TT_CONNECT0_SELF(signal, Class::callback)`.
The digit in the macro name is the number of arguments the function takes, so use the appropriate macro for the signal.
You can leave out "self" to pass in a pointer to the object owning the message if you don't want to use "this".
You can suffix the macro with _L if you do not wish to use a lifetime, this does imply that you guarantee yourself that the owner of the signal will die before the owner of the callback.
Yoiu can also use std bind manually to call global functions as callbacks: `signal.connect(std::bind(&callback, std::placeholders::_1));`.

#### Strings

String views and other utilities to interact with strings.

#### windont.h

Never include windows.h, always include windont.h. It avoids leaking macros so you can safely call TT::assert and std::min.
Though it may undefine YOUR macros if they clash with the windows macros, you should not be using common english words as macros either.

### Rendering/OpenGL

#### GL

Include tt_gl.h and call loadGLFunctions() to get all OpenGL functions.
Define TT_GL_DBG to make each call also use glGetError and warn about anything that is not GL_NO_ERROR.
The implementation of TT_GL_DBG has as drawback that you can not inline return values.
This is the macro expansion to explain why:

```
// Without TT_GL_DBG this works:
glUniform1f(glGetUniformLocation(program, "test"), 1.0f);
// With TT_GL_DBG it expands to:
glUniform1f(glGetUniformLocation(program, "test"); TT::checkGLErrors();, 1.0f); TT::checkGLErrors();
//                                               ^ Unexpected emicolon
```

So when developing it may be best to leave this ON (as long as performance permits) to ensure
you don't have a lot of work to do once you actually need it.

#### Handles

Some simple concepts have been wrapped with wrapper functions to help using OpeNGL.
Handles are copyable and may be constructed without an OpenGL context, so you have to call alloc() and cleanup() manually.

#### Program manager

This wraps the program (handle), with a little more utility. The ProgramManager singleton is instantiated
automatically and can cache & pool any shader and program that gets requested. Shaders are read with #include support
and non-copyable Program handles are returned for use in a Material.

Remove the NO_QT define at the top of the header to swap for a version that relies on the Qt library (that I am not free to distribute)
for file watching, this results in automatic recompilation of any shader, and relinking of any dependent program, as soon as the file on disk gets altered. 
Great for iterating your shaders.

#### Render concepts

Render concepts simplify object construction and groups logical concepts together.
A mesh wraps the vertex attribute layout (TT::MeshAttrib) and vertex and index buffer data (TT::Buffer).

MeshAttrib has the added benefit that it calls the right variation of glVertexAttrib[I|L]Pointer based on the element data type.
This does mean you can't upload an array of ints and expect it to automatically get converted to floats by the driver.

The Material concept wraps a program and a map of uniform name & value pairs.

### UI

#### Window

An OpenGL capable window that handles windows messages.
Inherit the Window class and implement the on[x]Event(...) methods to respond to keyboard and mouse logic.
There is a very minimal snippet at the top of tt_window.h to set up a windows GUI application using this class.

#### OrbitCameraControl

Something to assist in controlling a camera component with the mouse. Use with a custom tt_window to forward mouse events and easily
render a 3D scene using tt_components and tt_render_concepts.

#### UI

Extremely WIP
