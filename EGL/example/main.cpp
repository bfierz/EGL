/*
 * The MIT License (MIT)
 * 
 * Copyright (c) since 2017 Basil Fierz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <EGL/egl.h>
#include <GL/glew.h>
#include <GL/gl.h>

#include <iostream>

#ifdef WIN32
#include <windows.h>

EGLNativeWindowType createNativeWindow()
{
	auto hInstance = GetModuleHandle(nullptr);

	WNDCLASS wc;
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC) DefWindowProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor			= LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground	= nullptr;
	wc.lpszMenuName		= nullptr;
	wc.lpszClassName	= "OpenGL";

	if (RegisterClass(&wc))
	{
		DWORD dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle=WS_OVERLAPPEDWINDOW;

		HWND hWnd = CreateWindowEx(dwExStyle, "OpenGL", "",
		                           dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
		                           0, 0, 300, 300,
		                           nullptr, nullptr, hInstance, nullptr);

		if (hWnd)
			ShowWindow(hWnd,SW_SHOW);
		return hWnd;
	}

	return 0;
}

void sleep(int ms)
{
	Sleep(ms);
}

#endif

constexpr EGLint config_attribute[] = 
{
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_NONE
};

constexpr EGLint context_attribute[] =
{
	EGL_CONTEXT_MAJOR_VERSION, 4,
	EGL_CONTEXT_MINOR_VERSION, 0,
	EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
	EGL_NONE
};


// Force the use of the NVIDIA GPU in an Optimus system
//extern "C"
//{
//	_declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
//}

const char* profileType()
{
	GLint profile;
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);

	if (profile == GL_CONTEXT_CORE_PROFILE_BIT)
		return "Core";
	else if (profile == GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
		return "Compatibility";
	else
		return "Invalid";
}

int main(int argc, char ** argv)
{
	// Use the OpenGL API
	eglBindAPI(EGL_OPENGL_API);

	// Initialize the default display
	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint major, minor;
	if (!eglInitialize(display, &major, &minor))
	{
		std::cerr << "Could not initialize EGL" << std::endl;
		return -1;
	}

	// Find a matching frame buffer configuration
	EGLConfig config;
	EGLint num_config;
	eglChooseConfig(display, config_attribute, &config, 1, &num_config);

	// Create the OpenGL context with the requested configuration
	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribute);

	// Create a native window with an EGL surface
	EGLNativeWindowType native_window = createNativeWindow();
	EGLSurface surface = eglCreateWindowSurface(display, config, native_window, nullptr);

	// Use the created context for rendering
	if (!eglMakeCurrent(display, surface, surface, context))
	{
		std::cerr << "Could not make context current" << std::endl;
		return -1;
	}
	
	// Initialize glew
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cerr << "Error initializing GLEW: " << glewGetErrorString(err) << std::endl;
		return -1;
	}
	
	std::cout << "Status: Using EGL:      " << major << "." << minor << std::endl;
	std::cout << "Status: Using OpenGL:   " << glGetString(GL_VERSION) << std::endl;
	std::cout << "Status:       Vendor:   " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Status:       Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Status:       Profile:  " << profileType() << std::endl;
	std::cout << "Status:       Shading:  " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Status: Using GLEW:     " << glewGetString(GLEW_VERSION) << std::endl;

	glClearColor(1.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	eglSwapBuffers(display, surface);

	sleep(1000);
	return 0;
}