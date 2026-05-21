#define CLAY_IMPLEMENTATION
#include "../libs/clay/clay.h"
#include "../include/ImClay/ImClay.h"
#include "imgui.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace {

ImU32 toImColor(Clay_Color c) {
	return IM_COL32((int)c.r, (int)c.g, (int)c.b, (int)c.a);
}

const ImClay::FontSlot* fontAt(const ImClay::Context::MeasureFonts& measure, int fontId) {
	if (measure.count <= 0) {
		return nullptr;
	}
	const int id = fontId >= 0 && fontId < measure.count ? fontId : 0;
	const ImClay::FontSlot* slot = &measure.slots[id];
	return slot->font ? slot : (measure.slots[0].font ? &measure.slots[0] : nullptr);
}

Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
	Clay_Dimensions size = {0, 0};
	auto* ctx = static_cast<ImClay::Context*>(userData);
	if (!config || !ctx) {
		return size;
	}
	const ImClay::FontSlot* slot = fontAt(ctx->measure, config->fontId);
	if (!slot || !slot->font) {
		return size;
	}
	const float fontSize = config->fontSize > 0 ? (float)config->fontSize : slot->size;
	float maxW = 0.f;
	float lineW = 0.f;
	float lineH = fontSize;
	for (int i = 0; i < text.length; ++i) {
		if (text.chars[i] == '\n') {
			maxW = std::max(maxW, lineW);
			lineW = 0.f;
			lineH += fontSize;
			continue;
		}
		char buf[2] = {text.chars[i], 0};
		lineW += slot->font->CalcTextSizeA(fontSize, FLT_MAX, config->letterSpacing, buf).x;
	}
	maxW = std::max(maxW, lineW);
	size.width = maxW;
	size.height = lineH;
	return size;
}

void bindMeasureText(ImClay::Context& ctx) {
	Clay_SetMeasureTextFunction(measureText, &ctx);
}

}

namespace ImClay {

void HandleClayError(Clay_ErrorData e) {
	if (e.errorText.chars) {
		fprintf(stderr, "ImClay: %.*s\n", e.errorText.length, e.errorText.chars);
	}
}

void SetMeasureFonts(Context& ctx, const FontSlot* fonts, int n) {
	ctx.measure.count = n > Context::MeasureFonts::kMaxFonts ? Context::MeasureFonts::kMaxFonts : n;
	for (int i = 0; i < ctx.measure.count; ++i) {
		ctx.measure.slots[i] = fonts[i];
	}
	bindMeasureText(ctx);
}

bool Init(Context& ctx, int w, int h, size_t maxEl) {
	if (ctx.initialized) {
		Shutdown(ctx);
	}
	ctx.arenaBytes = Clay_MinMemorySize();
	ctx.arenaMemory = malloc(ctx.arenaBytes);
	if (!ctx.arenaMemory) {
		return false;
	}
	Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(ctx.arenaBytes, ctx.arenaMemory);
	Clay_Initialize(arena, (Clay_Dimensions){(float)w, (float)h}, (Clay_ErrorHandler){HandleClayError});
	Clay_SetMaxElementCount(maxEl);
	ctx.initialized = true;
	bindMeasureText(ctx);
	SetLayoutDimensions(ctx, w, h);
	return true;
}

void Shutdown(Context& ctx) {
	if (ctx.arenaMemory) {
		free(ctx.arenaMemory);
	}
	ctx.arenaMemory = nullptr;
	ctx.arenaBytes = 0;
	ctx.initialized = false;
	ctx.measure.count = 0;
}

void SetLayoutDimensions(Context&, int w, int h) {
	Clay_SetLayoutDimensions((Clay_Dimensions){(float)w, (float)h});
}

void SetPointerState(Context&, float x, float y, bool down) {
	Clay_SetPointerState((Clay_Vector2){x, y}, down);
}

void UpdateScrollContainers(Context&, bool drag, float wx, float wy, float dt) {
	Clay_UpdateScrollContainers(drag, (Clay_Vector2){wx, wy}, dt);
}

void BeginLayout(Context&) {
	Clay_BeginLayout();
}

Clay_RenderCommandArray EndLayout(Context&, float dt) {
	return Clay_EndLayout(dt);
}

void Render(const Clay_RenderCommandArray& cmds, ImDrawList* dl, const FontSlot* fonts, int n, ImVec2 offset) {
	if (!dl || !fonts || n <= 0) {
		return;
	}

	Context::MeasureFonts drawFonts;
	drawFonts.count = n > Context::MeasureFonts::kMaxFonts ? Context::MeasureFonts::kMaxFonts : n;
	for (int i = 0; i < drawFonts.count; ++i) {
		drawFonts.slots[i] = fonts[i];
	}

	static std::vector<char> buf;
	int clips = 0;
	for (int32_t i = 0; i < cmds.length; ++i) {
		Clay_RenderCommand* c = Clay_RenderCommandArray_Get((Clay_RenderCommandArray*)&cmds, i);
		if (!c) {
			continue;
		}
		ImVec2 p0(c->boundingBox.x + offset.x, c->boundingBox.y + offset.y);
		ImVec2 p1(p0.x + c->boundingBox.width, p0.y + c->boundingBox.height);

		switch (c->commandType) {
		case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
			const auto& r = c->renderData.rectangle;
			const float rad = std::max({r.cornerRadius.topLeft, r.cornerRadius.topRight,
										r.cornerRadius.bottomLeft, r.cornerRadius.bottomRight});
			dl->AddRectFilled(p0, p1, toImColor(r.backgroundColor), rad);
			break;
		}
		case CLAY_RENDER_COMMAND_TYPE_TEXT: {
			const auto& t = c->renderData.text;
			if (t.stringContents.length <= 0) {
				break;
			}
			const FontSlot* slot = fontAt(drawFonts, t.fontId);
			if (!slot || !slot->font) {
				break;
			}
			buf.resize((size_t)t.stringContents.length + 1);
			memcpy(buf.data(), t.stringContents.chars, (size_t)t.stringContents.length);
			buf[(size_t)t.stringContents.length] = 0;
			const float fontSize = t.fontSize > 0 ? (float)t.fontSize : slot->size;
			dl->AddText(slot->font, fontSize, p0, toImColor(t.textColor), buf.data());
			break;
		}
		case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
			const auto& im = c->renderData.image;
			if (!im.imageData) {
				break;
			}
			ImU32 tint = toImColor(im.backgroundColor);
			if (im.backgroundColor.a == 0 && im.backgroundColor.r == 0) {
				tint = IM_COL32_WHITE;
			}
			const float rad = std::max({im.cornerRadius.topLeft, im.cornerRadius.topRight,
										im.cornerRadius.bottomLeft, im.cornerRadius.bottomRight});
			dl->AddImageRounded((ImTextureID)im.imageData, p0, p1, ImVec2(0, 0), ImVec2(1, 1), tint, rad);
			break;
		}
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
			dl->PushClipRect(p0, p1, true);
			++clips;
			break;
		case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
			if (clips > 0) {
				dl->PopClipRect();
				--clips;
			}
			break;
		default:
			break;
		}
	}
}

} // namespace ImClay
