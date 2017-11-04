/**
 * EGL windows desktop implementation.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) since 2014 Norbert Nopper
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

#include "egl_internal.h"

const char * DummyClassName = "EGLDummyWindow";

static LRESULT CALLBACK __DummyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

EGLBoolean __internalInit(NativeLocalStorage* nativeLocalStorage)
{
	HDC hdc = nativeLocalStorage->hdc;
	HGLRC ctx = nativeLocalStorage->ctx;

	if (!nativeLocalStorage)
	{
		return EGL_FALSE;
	}

	if (hdc && ctx)
	{
		return EGL_TRUE;
	}

	if (hdc || ctx)
	{
		return EGL_FALSE;
	}

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));

	wc.lpfnWndProc = __DummyWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = DummyClassName;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(0, DummyClassName, "", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);

	if (!hwnd)
	{
		return EGL_FALSE;
	}

	hdc = GetDC(hwnd);

	if (!hdc)
	{
		DestroyWindow(hwnd);
		return EGL_FALSE;
	}

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	const EGLint pixelFormat = ChoosePixelFormat(hdc, &pfd);

	if (pixelFormat == 0 || !SetPixelFormat(hdc, pixelFormat, &pfd))
	{
		ReleaseDC(0, hdc);
		DestroyWindow(hwnd);
		return EGL_FALSE;
	}

	ctx = wglCreateContext(hdc);
	if (!ctx)
	{
		ReleaseDC(0, hdc);
		DestroyWindow(hwnd);
		return EGL_FALSE;
	}

	if (!wglMakeCurrent(hdc, ctx))
	{
		wglDeleteContext(ctx);

		ReleaseDC(0, hdc);
		DestroyWindow(hwnd);
		return EGL_FALSE;
	}

	glewExperimental = GL_TRUE;
	if (glewInit() != GL_NO_ERROR)
	{
		wglMakeCurrent(0, 0);
		wglDeleteContext(ctx);
		ReleaseDC(0, hdc);
		DestroyWindow(hwnd);
		return EGL_FALSE;
	}

	nativeLocalStorage->hwnd = hwnd;
	nativeLocalStorage->hdc = hdc;
	nativeLocalStorage->ctx = ctx;

	return EGL_TRUE;
}

EGLBoolean __internalTerminate(NativeLocalStorage* nativeLocalStorage)
{
	if (!nativeLocalStorage)
	{
		return EGL_FALSE;
	}

	wglMakeCurrent(0, 0);

	HDC hdc = nativeLocalStorage->hdc;
	HGLRC ctx = nativeLocalStorage->ctx;
	HWND hwnd = nativeLocalStorage->hwnd;

	if (ctx)
		wglDeleteContext(ctx);

	if (hdc)
		ReleaseDC(0, hdc);

	if (hwnd)
		DestroyWindow(hwnd);

	nativeLocalStorage->hdc = 0;
	nativeLocalStorage->ctx = 0;
	nativeLocalStorage->hwnd = 0;


	UnregisterClass(DummyClassName, NULL);

	return EGL_TRUE;
}

EGLBoolean __deleteContext(const EGLDisplayImpl* walkerDpy, NativeContext* nativeContext)
{
	if (!walkerDpy || !nativeContext)
	{
		return EGL_FALSE;
	}

	EGLBoolean res = wglDeleteContext(nativeContext->ctx);
	nativeContext->ctx = 0;
	return res;
}

EGLBoolean __processAttribList(EGLint* target_attrib_list, const EGLint* attrib_list, EGLint* error)
{
	if (!target_attrib_list || !attrib_list || !error)
	{
		return EGL_FALSE;
	}

	EGLint _WGL_CONTEXT_MAJOR_VERSION_ARB = 0;
	EGLint _WGL_CONTEXT_MINOR_VERSION_ARB = 0;
	EGLint _WGL_CONTEXT_LAYER_PLANE_ARB = 0;
	EGLint _WGL_CONTEXT_FLAGS_ARB = 0;
	EGLint _WGL_CONTEXT_PROFILE_MASK_ARB = 0;
	EGLint _WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB = 0;

	EGLint attribListIndex = 0;

	while (attrib_list[attribListIndex] != EGL_NONE)
	{
		EGLint value = attrib_list[attribListIndex + 1];

		switch (attrib_list[attribListIndex])
		{
		case EGL_CONTEXT_MAJOR_VERSION:
		{
			if (value < 1)
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}

			_WGL_CONTEXT_MAJOR_VERSION_ARB = value;
		}
		break;
		case EGL_CONTEXT_MINOR_VERSION:
		{
			if (value < 0)
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}

			_WGL_CONTEXT_MINOR_VERSION_ARB = value;
		}
		break;
		case EGL_CONTEXT_OPENGL_PROFILE_MASK:
		{
			if (value == EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT)
			{
				_WGL_CONTEXT_PROFILE_MASK_ARB = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
			}
			else if (value == EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT)
			{
				_WGL_CONTEXT_PROFILE_MASK_ARB = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
			}
			else
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}
		}
		break;
		case EGL_CONTEXT_OPENGL_DEBUG:
		{
			if (value == EGL_TRUE)
			{
				_WGL_CONTEXT_FLAGS_ARB |= WGL_CONTEXT_DEBUG_BIT_ARB;
			}
			else if (value == EGL_FALSE)
			{
				_WGL_CONTEXT_FLAGS_ARB &= ~WGL_CONTEXT_DEBUG_BIT_ARB;
			}
			else
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}
		}
		break;
		case EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE:
		{
			if (value == EGL_TRUE)
			{
				_WGL_CONTEXT_FLAGS_ARB |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
			}
			else if (value == EGL_FALSE)
			{
				_WGL_CONTEXT_FLAGS_ARB &= ~WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
			}
			else
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}
		}
		break;
		case EGL_CONTEXT_OPENGL_ROBUST_ACCESS:
		{
			if (value == EGL_TRUE)
			{
				_WGL_CONTEXT_FLAGS_ARB |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
			}
			else if (value == EGL_FALSE)
			{
				_WGL_CONTEXT_FLAGS_ARB &= ~WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
			}
			else
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}
		}
		break;
		case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY:
		{
			if (value == EGL_NO_RESET_NOTIFICATION)
			{
				_WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB = WGL_NO_RESET_NOTIFICATION_ARB;
			}
			else if (value == EGL_LOSE_CONTEXT_ON_RESET)
			{
				_WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB = WGL_LOSE_CONTEXT_ON_RESET_ARB;
			}
			else
			{
				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}
		}
		break;
		default:
		{
			*error = EGL_BAD_ATTRIBUTE;

			return EGL_FALSE;
		}
		break;
		}

		attribListIndex += 2;

		// More than 14 entries can not exist.
		if (attribListIndex >= 7 * 2)
		{
			*error = EGL_BAD_ATTRIBUTE;

			return EGL_FALSE;
		}
	}

	EGLint curr_attrib_list[] = 
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, _WGL_CONTEXT_MAJOR_VERSION_ARB,
		WGL_CONTEXT_MINOR_VERSION_ARB, _WGL_CONTEXT_MINOR_VERSION_ARB,
		WGL_CONTEXT_LAYER_PLANE_ARB, _WGL_CONTEXT_LAYER_PLANE_ARB,
		WGL_CONTEXT_FLAGS_ARB, _WGL_CONTEXT_FLAGS_ARB,
		WGL_CONTEXT_PROFILE_MASK_ARB, _WGL_CONTEXT_PROFILE_MASK_ARB,
		WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, _WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
		0
	};

	EGLint template_attrib_list[20];
	EGLint * curr_template_attrib_list = template_attrib_list;

	for (int i = 0; i < 6; ++i)
	{
		const int num = i * 2;
		if (curr_attrib_list[num + 1])
		{
			*curr_template_attrib_list = curr_attrib_list[num];
			++curr_template_attrib_list;
			*curr_template_attrib_list = curr_attrib_list[num + 1];
			++curr_template_attrib_list;
		}
	}

	*curr_template_attrib_list = 0;
		
	memcpy(target_attrib_list, template_attrib_list, CONTEXT_ATTRIB_LIST_SIZE * sizeof(EGLint));

	return EGL_TRUE;
}

EGLBoolean __createWindowSurface(EGLSurfaceImpl* newSurface, EGLNativeWindowType win, const EGLint *attrib_list, const EGLDisplayImpl* walkerDpy, const EGLConfigImpl* walkerConfig, EGLint* error)
{
	if (!newSurface || !walkerDpy || !walkerConfig || !error)
	{
		return EGL_FALSE;
	}

	HDC hdc = GetDC(win);

	if (!hdc)
	{
		*error = EGL_BAD_NATIVE_WINDOW;

		return EGL_FALSE;
	}

	// FIXME Check more values.
	EGLint template_attrib_list[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_COLOR_BITS_ARB, 32,
			WGL_RED_BITS_EXT, 8,
			WGL_GREEN_BITS_EXT, 8,
			WGL_BLUE_BITS_EXT, 8,
			WGL_ALPHA_BITS_EXT, 8,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_SAMPLE_BUFFERS_ARB, 0,
			WGL_SAMPLES_ARB, 0,
			0
	};

	if (attrib_list)
	{
		EGLint indexAttribList = 0;

		while (attrib_list[indexAttribList] != EGL_NONE)
		{
			EGLint value = attrib_list[indexAttribList + 1];

			switch (attrib_list[indexAttribList])
			{
			case EGL_GL_COLORSPACE:
			{
				if (value == EGL_GL_COLORSPACE_LINEAR)
				{
					// Do nothing.
				}
				else if (value == EGL_GL_COLORSPACE_SRGB)
				{
					ReleaseDC(win, hdc);

					*error = EGL_BAD_MATCH;

					return EGL_FALSE;
				}
				else
				{
					ReleaseDC(win, hdc);

					*error = EGL_BAD_ATTRIBUTE;

					return EGL_FALSE;
				}
			}
			break;
			case EGL_RENDER_BUFFER:
			{
				if (value == EGL_SINGLE_BUFFER)
				{
					template_attrib_list[7] = GL_FALSE;
				}
				else if (value == EGL_BACK_BUFFER)
				{
					template_attrib_list[7] = GL_TRUE;
				}
				else
				{
					ReleaseDC(win, hdc);

					*error = EGL_BAD_ATTRIBUTE;

					return EGL_FALSE;
				}
			}
			break;
			case EGL_VG_ALPHA_FORMAT:
			{
				ReleaseDC(win, hdc);

				*error = EGL_BAD_MATCH;

				return EGL_FALSE;
			}
			break;
			case EGL_VG_COLORSPACE:
			{
				ReleaseDC(win, hdc);

				*error = EGL_BAD_MATCH;

				return EGL_FALSE;
			}
			break;
			}

			indexAttribList += 2;

			// More than 4 entries can not exist.
			if (indexAttribList >= 4 * 2)
			{
				ReleaseDC(win, hdc);

				*error = EGL_BAD_ATTRIBUTE;

				return EGL_FALSE;
			}
		}
	}

	// Create out of EGL configuration an array of WGL configuration and use it.
	// see https://www.opengl.org/registry/specs/ARB/wgl_pixel_format.txt

	template_attrib_list[9] = walkerConfig->bufferSize;
	template_attrib_list[11] = walkerConfig->redSize;
	template_attrib_list[13] = walkerConfig->blueSize;
	template_attrib_list[15] = walkerConfig->greenSize;
	template_attrib_list[17] = walkerConfig->alphaSize;
	template_attrib_list[19] = walkerConfig->depthSize;
	template_attrib_list[21] = walkerConfig->stencilSize;
	template_attrib_list[23] = walkerConfig->sampleBuffers;
	template_attrib_list[25] = walkerConfig->samples;

	//

	UINT wgl_max_formats = 1;
	INT wgl_formats;
	UINT wgl_num_formats;

	if (wglChoosePixelFormatARB(hdc, template_attrib_list, 0, wgl_max_formats, &wgl_formats, &wgl_num_formats) == GL_FALSE)
	{
		ReleaseDC(win, hdc);

		*error = EGL_BAD_MATCH;

		return EGL_FALSE;
	}

	if (wgl_num_formats == 0)
	{
		ReleaseDC(win, hdc);

		*error = EGL_BAD_MATCH;

		return EGL_FALSE;
	}

	PIXELFORMATDESCRIPTOR pfd;

	if (!DescribePixelFormat(hdc, wgl_formats, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
	{
		ReleaseDC(win, hdc);

		*error = EGL_BAD_MATCH;

		return EGL_FALSE;
	}

	if (!SetPixelFormat(hdc, wgl_formats, &pfd))
	{
		ReleaseDC(win, hdc);

		*error = EGL_BAD_MATCH;

		return EGL_FALSE;
	}

	newSurface->drawToWindow = EGL_TRUE;
	newSurface->drawToPixmap = EGL_FALSE;
	newSurface->drawToPixmap = EGL_FALSE;
	newSurface->doubleBuffer = (EGLBoolean)template_attrib_list[7];
	newSurface->configId = wgl_formats;

	newSurface->initialized = EGL_TRUE;
	newSurface->destroy = EGL_FALSE;
	newSurface->win = win;
	newSurface->nativeSurface.hdc = hdc;

	return EGL_TRUE;
}

EGLBoolean __destroySurface(EGLNativeWindowType win, NativeSurface* nativeSurface)
{
	if (!nativeSurface)
	{
		return EGL_FALSE;
	}

	ReleaseDC(win, nativeSurface->hdc);
	nativeSurface->hdc = 0;

	return EGL_TRUE;
}

__eglMustCastToProperFunctionPointerType __getProcAddress(const char *procname)
{
	return (__eglMustCastToProperFunctionPointerType)wglGetProcAddress(procname);
}

GLboolean __GetPixelFormatAttrib(HDC hdc, int pixelFormat, int attrib, int * value)
{
	return wglGetPixelFormatAttribivARB(hdc, pixelFormat, 0, 1, &attrib, value);
}

EGLBoolean __initialize(EGLDisplayImpl* walkerDpy, const NativeLocalStorage* nativeLocalStorage, EGLint* error)
{
	if (!walkerDpy || !nativeLocalStorage || !error)
	{
		return EGL_FALSE;
	}

	// Create configuration list.

	EGLConfigImpl* lastConfig = 0;
	HDC hdc = nativeLocalStorage->hdc;
	EGLint numberPixelFormats = 0;

	if (!__GetPixelFormatAttrib(hdc, 1, WGL_NUMBER_PIXEL_FORMATS_ARB, &numberPixelFormats))
	{
		*error = EGL_NOT_INITIALIZED;
		return EGL_FALSE;
	}

	for (EGLint currentPixelFormat = 1; currentPixelFormat <= numberPixelFormats; currentPixelFormat++)
	{
		EGLint value;

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_SUPPORT_OPENGL_ARB, &value))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!value)
		{
			continue;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_PIXEL_TYPE_ARB, &value))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (value != WGL_TYPE_RGBA_ARB)
		{
			continue;
		}

		EGLConfigImpl tmpConfig;
		_eglInternalSetDefaultConfig(&tmpConfig);
		tmpConfig.next = 0;

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_DRAW_TO_WINDOW_ARB, &tmpConfig.drawToWindow))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_DRAW_TO_BITMAP_ARB, &tmpConfig.drawToPixmap))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_DOUBLE_BUFFER_ARB, &tmpConfig.doubleBuffer))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		tmpConfig.drawToPBuffer = EGL_FALSE;
		tmpConfig.conformant = EGL_OPENGL_BIT;
		tmpConfig.renderableType = EGL_OPENGL_BIT;
		tmpConfig.surfaceType = 0;

		if (tmpConfig.drawToWindow)
			tmpConfig.surfaceType |= EGL_WINDOW_BIT;

		if (tmpConfig.drawToPixmap)
			tmpConfig.surfaceType |= EGL_PIXMAP_BIT;

		if (tmpConfig.drawToPBuffer)
			tmpConfig.surfaceType |= EGL_PBUFFER_BIT;

		tmpConfig.colorBufferType = EGL_RGB_BUFFER;
		tmpConfig.configId = currentPixelFormat;

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_COLOR_BITS_ARB, &tmpConfig.bufferSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_RED_BITS_ARB, &tmpConfig.redSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_GREEN_BITS_ARB, &tmpConfig.greenSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_BLUE_BITS_ARB, &tmpConfig.blueSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_ALPHA_BITS_ARB, &tmpConfig.alphaSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_DEPTH_BITS_ARB, &tmpConfig.depthSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_STENCIL_BITS_ARB, &tmpConfig.stencilSize))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_SAMPLE_BUFFERS_ARB, &tmpConfig.sampleBuffers))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_SAMPLES_ARB, &tmpConfig.samples))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_BIND_TO_TEXTURE_RGB_ARB, &tmpConfig.bindToTextureRGB))
		{
			tmpConfig.bindToTextureRGB = tmpConfig.bindToTextureRGB ? EGL_TRUE : EGL_FALSE;
		}

		if (__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_BIND_TO_TEXTURE_RGBA_ARB, &tmpConfig.bindToTextureRGBA))
		{
			tmpConfig.bindToTextureRGBA = tmpConfig.bindToTextureRGBA ? EGL_TRUE : EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_MAX_PBUFFER_PIXELS_ARB, &tmpConfig.maxPBufferPixels))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_MAX_PBUFFER_WIDTH_ARB, &tmpConfig.maxPBufferWidth))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_MAX_PBUFFER_HEIGHT_ARB, &tmpConfig.maxPBufferHeight))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_TRANSPARENT_ARB, &tmpConfig.transparentType))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}
		tmpConfig.transparentType = tmpConfig.transparentType ? EGL_TRANSPARENT_RGB : EGL_NONE;

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_TRANSPARENT_RED_VALUE_ARB, &tmpConfig.transparentRedValue))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_TRANSPARENT_GREEN_VALUE_ARB, &tmpConfig.transparentGreenValue))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		if (!__GetPixelFormatAttrib(hdc, currentPixelFormat, WGL_TRANSPARENT_BLUE_VALUE_ARB, &tmpConfig.transparentBlueValue))
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		EGLConfigImpl * newConfig = (EGLConfigImpl*)malloc(sizeof(EGLConfigImpl));
		if (!newConfig)
		{
			*error = EGL_NOT_INITIALIZED;
			return EGL_FALSE;
		}

		*newConfig = tmpConfig;

		// Store in the same order as received.
		if (lastConfig != 0)
			lastConfig->next = newConfig;
		else
			walkerDpy->rootConfig = newConfig;

		lastConfig = newConfig;

		// FIXME: Query and save more values.
	}

	return EGL_TRUE;
}

EGLBoolean __createContext(NativeContext* nativeContext, const EGLDisplayImpl* walkerDpy, const NativeSurface* nativeSurface, const NativeContext* sharedNativeSurfaceContainer, const EGLint* attribList)
{
	if (!walkerDpy || !nativeContext || !nativeSurface)
	{
		return EGL_FALSE;
	}

	nativeContext->ctx = wglCreateContextAttribsARB(nativeSurface->hdc, sharedNativeSurfaceContainer ? sharedNativeSurfaceContainer->ctx : 0, attribList);
	return nativeContext->ctx != 0;
}

EGLBoolean __makeCurrent(const EGLDisplayImpl* walkerDpy, const NativeSurface* nativeSurface, const NativeContext* nativeContext)
{
	if (!walkerDpy || !nativeSurface || !nativeContext)
	{
		return EGL_FALSE;
	}

	return (EGLBoolean)wglMakeCurrent(nativeSurface->hdc, nativeContext->ctx);
}

EGLBoolean __swapBuffers(const EGLDisplayImpl* walkerDpy, const EGLSurfaceImpl* walkerSurface)
{
	if (!walkerDpy || !walkerSurface)
	{
		return EGL_FALSE;
	}

	return (EGLBoolean)SwapBuffers(walkerSurface->nativeSurface.hdc);
}

EGLBoolean __swapInterval(const EGLDisplayImpl* walkerDpy, EGLint interval)
{
	if (!walkerDpy)
	{
		return EGL_FALSE;
	}

	return (EGLBoolean)wglSwapIntervalEXT(interval);
}
