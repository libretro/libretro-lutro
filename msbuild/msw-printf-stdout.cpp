
#if PLATFORM_MSW
#define NOMINMAX
#include <Windows.h>
#include "fi-printf-redirect.h"

#include <ios>
#include <io.h>
#include <fcntl.h>

#include <algorithm>

bool msw_IsDebuggerPresent()
{
	return ::IsDebuggerPresent() == TRUE;
}

void msw_OutputDebugString(const char* fmt)
{
	::OutputDebugStringA(fmt);
}

void msw_OutputDebugStringV(const char* fmt, va_list args)
{
	// vsnprintf has what is arguably unexpected behavior: it returns the length of the formatted string,
	// not the # of chars written.  It also doesn't null-terminate strings if it runs out of buffer space.

	char buf[2048];
	int sizeof_buf = sizeof(buf);
	int len = vsnprintf(buf, sizeof_buf, fmt, args);
	buf[sizeof_buf-1] = '\0';		// in case vsnprintf overflowed.

	::OutputDebugStringA(buf);

	if (len >= sizeof_buf) {
		// Things piped to stderr shouldn't be excessively verbose anyway
		::OutputDebugStringA("\n");
		::OutputDebugStringA("(*OutputDebugStringA*) previous message was truncated, see stderr console for full content");
	}
}

#if REDEFINE_PRINTF
#	undef printf
#	undef vfprintf
#	undef fprintf
#	undef puts
#	undef fputs
#   undef fwrite

extern "C" {
int _fi_redirect_printf(const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	auto result = _fi_redirect_vfprintf(stdout, fmt, argptr);
	va_end(argptr);
	return result;
}

int _fi_redirect_fprintf(FILE* handle, const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	auto result = _fi_redirect_vfprintf(handle, fmt, argptr);
	va_end(argptr);
	return result;
}

int _fi_redirect_vfprintf(FILE* handle, const char* fmt, va_list args)
{
	int result;
	if (1) {
		// flush stdout before writing stderr, otherwise the context of stderr will be misleading.
		if (handle == stderr) {
			fflush(stdout);
		}

		va_list argptr;
		va_copy(argptr, args);
		result = vfprintf(handle, fmt, argptr);
		va_end(argptr);
	}

	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			va_list argptr;
			va_copy(argptr, args);
			msw_OutputDebugStringV(fmt, argptr);
			va_end(argptr);
		}
	}
	return result;
}

int _fi_redirect_puts(char const* buffer) {
	// since puts appends a newline and OutputDebugString doesn't have a formatted version,
	// it's easier to just fall back on our printf implementation.
	return _fi_redirect_printf("%s\n", buffer);
}

int _fi_redirect_fputs(char const* buffer, FILE* handle) {
	int result = fputs(buffer, handle);
	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			msw_OutputDebugString(buffer);
		}
	}
	return result;
}

intmax_t _fi_redirect_fwrite(void const* buffer, size_t size, size_t nelem, FILE* handle)
{
	auto result = fwrite(buffer, size, nelem, handle);
	if (handle == stdout || handle == stderr)
	{
		if (msw_IsDebuggerPresent()) {
			char mess[1024];
			auto nsize = size * nelem;
			auto* chbuf = (const char*)buffer;
			while(nsize) {
				auto chunksize = std::min(nsize, 1023ULL);
				memcpy(mess, chbuf, chunksize);
				mess[chunksize] = 0;
				msw_OutputDebugString(mess);
				nsize -= chunksize;
				chbuf += chunksize;
			}
		}
	}
	return result;
}
}
#endif
#endif
