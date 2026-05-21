#pragma once

#include <cstddef>
#include <cstdint>

#include "imgui.h"

struct Clay_RenderCommandArray;
typedef struct Clay_ErrorData Clay_ErrorData;

namespace ImClay {

struct FontSlot {
	ImFont* font = nullptr;
	float size = 16.f;
};

struct Context {
	void* arenaMemory = nullptr;
	size_t arenaBytes = 0;
	bool initialized = false;

	struct MeasureFonts {
		static constexpr int kMaxFonts = 16;
		FontSlot slots[kMaxFonts]{};
		int count = 0;
	} measure;
};

bool Init(Context& ctx, int width, int height, size_t maxElements = 8192);
void Shutdown(Context& ctx);
void SetLayoutDimensions(Context& ctx, int width, int height);
void SetPointerState(Context& ctx, float x, float y, bool pointerDown);
void UpdateScrollContainers(Context& ctx, bool enableDragScroll, float wheelX, float wheelY, float deltaTime);
void BeginLayout(Context& ctx);
Clay_RenderCommandArray EndLayout(Context& ctx, float deltaTime);
void Render(const Clay_RenderCommandArray& commands, ImDrawList* drawList, const FontSlot* fonts, int fontCount, ImVec2 offset = ImVec2(0, 0));
void SetMeasureFonts(Context& ctx, const FontSlot* fonts, int fontCount);
void HandleClayError(Clay_ErrorData errorData);

} // namespace ImClay
