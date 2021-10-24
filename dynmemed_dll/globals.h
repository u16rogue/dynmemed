#pragma once

#include <Windows.h>
#include <string>

#define APP_NAME "dynmemed"

#define PREFER_WIDTH 800
#define PREFER_HEIGHT 600

namespace globals
{
	inline HMODULE dll_handle = nullptr;
	inline std::string  custom_time = "10:00";
	inline std::string  custom_date = { __DATE__[4], __DATE__[5], '-', __DATE__[0], __DATE__[1], __DATE__[2], '-', __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10], 0x0 };
	inline std::int32_t custom_score = 8;
	inline int          custom_progress = 100;
}

// this is dumb
namespace dyned
{
	inline HWND hwnd_bg                        = nullptr; // BackgroundClass
	inline HWND hwnd_engine                    = nullptr; // dynedEngineWindowClass
	inline void *string_formatter              = nullptr; // string formatting function
	inline char **pbuffer_study_detail_full    = nullptr;
	inline char **pbuffer_study_detail_summary = nullptr;
}