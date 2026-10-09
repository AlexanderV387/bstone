/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2024 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#ifndef _WIN32

#include "bstone_shared_library.h"

#ifndef __3DS__
#include <dlfcn.h>
#endif

#include "bstone_exception.h"

namespace bstone {

#ifdef __3DS__

// No shared libraries on the 3DS: OpenAL, OpenGL and Vulkan are never found
// and BStone falls back to its own mixer and the software renderer.

void SharedLibraryHandleDeleter::operator()(void*) const noexcept {}

bool SharedLibrary::try_open(const char*)
{
	close();
	return false;
}

void* SharedLibrary::find_symbol(const char*) const noexcept
{
	return nullptr;
}

#else

void SharedLibraryHandleDeleter::operator()(void* handle) const noexcept
{
	dlclose(handle);
}

// ==========================================================================

bool SharedLibrary::try_open(const char* file_path)
{
	close();

	auto handle = SharedLibraryHandleUPtr{dlopen(file_path, RTLD_NOW | RTLD_LOCAL)};

	if (handle == nullptr)
	{
		return false;
	}

	handle_.swap(handle);
	return true;
}

void* SharedLibrary::find_symbol(const char* symbol_name) const noexcept
{
	return dlsym(handle_.get(), symbol_name);
}

#endif // __3DS__

} // namespace bstone

#endif // _WIN32
