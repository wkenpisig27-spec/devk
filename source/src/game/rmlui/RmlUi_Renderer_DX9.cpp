#include "rmlui/RmlUi_Renderer_DX9.h"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <d3dx9.h>
#include <cstring>
#include <memory>
#include <vector>

#pragma pack(push, 1)
struct TGAHeader {
	char idLength;
	char colourMapType;
	char dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char bitsPerPixel;
	char imageDescriptor;
};
#pragma pack(pop)

namespace {
constexpr DWORD kFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
}

RenderInterface_DX9::RenderInterface_DX9() = default;

RenderInterface_DX9::~RenderInterface_DX9() = default;

void RenderInterface_DX9::SetDevice(IDirect3DDevice9* device) {
	m_device = device;
}

void RenderInterface_DX9::SetViewport(int width, int height) {
	m_width = width;
	m_height = height;
}

void RenderInterface_DX9::BeginFrame() {
	if (!m_device || m_width <= 0 || m_height <= 0)
		return;

	m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
	m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
	m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	m_device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

	m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	m_device->SetFVF(kFVF);

	D3DXMATRIX identity;
	D3DXMatrixIdentity(&identity);
	m_device->SetTransform(D3DTS_WORLD, &identity);
	m_device->SetTransform(D3DTS_VIEW, &identity);

	// Top-left origin orthographic projection matching RmlUi layout space.
	D3DXMATRIX proj;
	D3DXMatrixOrthoOffCenterLH(&proj, 0.0f, (float)m_width, (float)m_height, 0.0f, -10000.0f, 10000.0f);
	m_device->SetTransform(D3DTS_PROJECTION, &proj);

	m_hasTransform = false;
	m_transform = Rml::Matrix4f::Identity();
}

void RenderInterface_DX9::EndFrame() {
	if (!m_device)
		return;
	m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	m_device->SetTexture(0, nullptr);
}

DWORD RenderInterface_DX9::ToD3DColor(const Rml::ColourbPremultiplied& colour) {
	return D3DCOLOR_ARGB(colour.alpha, colour.red, colour.green, colour.blue);
}

void RenderInterface_DX9::ApplyTransform(Rml::Vector2f translation) {
	D3DXMATRIX world;
	D3DXMatrixTranslation(&world, translation.x, translation.y, 0.0f);

	if (m_hasTransform) {
		D3DXMATRIX transform;
		// RmlUi Matrix4f is column-major by default; D3DXMATRIX is row-major.
		const float* src = m_transform.data();
		transform._11 = src[0];
		transform._12 = src[4];
		transform._13 = src[8];
		transform._14 = src[12];
		transform._21 = src[1];
		transform._22 = src[5];
		transform._23 = src[9];
		transform._24 = src[13];
		transform._31 = src[2];
		transform._32 = src[6];
		transform._33 = src[10];
		transform._34 = src[14];
		transform._41 = src[3];
		transform._42 = src[7];
		transform._43 = src[11];
		transform._44 = src[15];

		D3DXMATRIX combined;
		D3DXMatrixMultiply(&combined, &world, &transform);
		m_device->SetTransform(D3DTS_WORLD, &combined);
	} else {
		m_device->SetTransform(D3DTS_WORLD, &world);
	}
}

Rml::CompiledGeometryHandle RenderInterface_DX9::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
	return reinterpret_cast<Rml::CompiledGeometryHandle>(new GeometryView{vertices, indices});
}

void RenderInterface_DX9::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
	delete reinterpret_cast<GeometryView*>(geometry);
}

void RenderInterface_DX9::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) {
	if (!m_device || !handle)
		return;

	const GeometryView* geometry = reinterpret_cast<GeometryView*>(handle);
	const size_t vertex_count = geometry->vertices.size();
	const size_t index_count = geometry->indices.size();
	if (vertex_count == 0 || index_count == 0)
		return;

	std::vector<DX9Vertex> verts(vertex_count);
	for (size_t i = 0; i < vertex_count; ++i) {
		const Rml::Vertex& v = geometry->vertices[i];
		verts[i].x = v.position.x;
		verts[i].y = v.position.y;
		verts[i].z = 0.0f;
		verts[i].color = ToD3DColor(v.colour);
		verts[i].u = v.tex_coord.x;
		verts[i].v = v.tex_coord.y;
	}

	ApplyTransform(translation);

	if (texture) {
		m_device->SetTexture(0, reinterpret_cast<IDirect3DTexture9*>(texture));
		m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	} else {
		m_device->SetTexture(0, nullptr);
		m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	}

	m_device->DrawIndexedPrimitiveUP(
		D3DPT_TRIANGLELIST,
		0,
		(UINT)vertex_count,
		(UINT)(index_count / 3),
		geometry->indices.data(),
		D3DFMT_INDEX32,
		verts.data(),
		sizeof(DX9Vertex));
}

void RenderInterface_DX9::EnableScissorRegion(bool enable) {
	if (!m_device)
		return;
	m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, enable ? TRUE : FALSE);
}

void RenderInterface_DX9::SetScissorRegion(Rml::Rectanglei region) {
	if (!m_device)
		return;

	RECT rect;
	rect.left = region.Left();
	rect.top = region.Top();
	rect.right = region.Right();
	rect.bottom = region.Bottom();
	m_device->SetScissorRect(&rect);
}

void RenderInterface_DX9::SetTransform(const Rml::Matrix4f* transform) {
	if (transform) {
		m_hasTransform = true;
		m_transform = *transform;
	} else {
		m_hasTransform = false;
		m_transform = Rml::Matrix4f::Identity();
	}
}

Rml::TextureHandle RenderInterface_DX9::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
		return 0;

	file_interface->Seek(file_handle, 0, SEEK_END);
	size_t buffer_size = file_interface->Tell(file_handle);
	file_interface->Seek(file_handle, 0, SEEK_SET);

	if (buffer_size <= sizeof(TGAHeader)) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Texture file size is smaller than TGAHeader, file is not a valid TGA image.");
		file_interface->Close(file_handle);
		return 0;
	}

	using Rml::byte;
	std::unique_ptr<byte[]> buffer(new byte[buffer_size]);
	file_interface->Read(buffer.get(), buffer_size, file_handle);
	file_interface->Close(file_handle);

	TGAHeader header;
	memcpy(&header, buffer.get(), sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	const size_t image_size = (size_t)header.width * (size_t)header.height * 4;

	if (header.dataType != 2) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		return 0;
	}
	if (color_mode < 3) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		return 0;
	}

	const byte* image_src = buffer.get() + sizeof(TGAHeader);
	std::unique_ptr<byte[]> image_dest_buffer(new byte[image_size]);
	byte* image_dest = image_dest_buffer.get();
	const bool top_to_bottom_order = ((header.imageDescriptor & 32) != 0);

	for (long y = 0; y < header.height; y++) {
		long read_index = y * header.width * color_mode;
		long write_index = top_to_bottom_order ? (y * header.width * 4) : ((header.height - y - 1) * header.width * 4);
		for (long x = 0; x < header.width; x++) {
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4) {
				const byte alpha = image_src[read_index + 3];
				for (size_t j = 0; j < 3; j++)
					image_dest[write_index + j] = byte((image_dest[write_index + j] * alpha) / 255);
				image_dest[write_index + 3] = alpha;
			} else {
				image_dest[write_index + 3] = 255;
			}

			write_index += 4;
			read_index += color_mode;
		}
	}

	texture_dimensions.x = header.width;
	texture_dimensions.y = header.height;
	return GenerateTexture({image_dest, image_size}, texture_dimensions);
}

Rml::TextureHandle RenderInterface_DX9::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
	if (!m_device || !source.data() || source_dimensions.x <= 0 || source_dimensions.y <= 0)
		return 0;

	IDirect3DTexture9* texture = nullptr;
	if (FAILED(m_device->CreateTexture(source_dimensions.x, source_dimensions.y, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, nullptr))) {
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create DX9 texture.");
		return 0;
	}

	D3DLOCKED_RECT locked;
	if (FAILED(texture->LockRect(0, &locked, nullptr, 0))) {
		texture->Release();
		return 0;
	}

	// Convert RGBA (RmlUi) -> BGRA (D3D A8R8G8B8 memory layout).
	// Do not flip Y: RmlUi glyph UVs already match a top-down buffer upload on D3D.
	auto* dest_rows = static_cast<unsigned char*>(locked.pBits);
	for (int y = 0; y < source_dimensions.y; ++y) {
		const Rml::byte* src = source.data() + (size_t)y * (size_t)source_dimensions.x * 4;
		auto* dest = dest_rows + y * locked.Pitch;
		for (int x = 0; x < source_dimensions.x; ++x) {
			const Rml::byte r = src[0];
			const Rml::byte g = src[1];
			const Rml::byte b = src[2];
			const Rml::byte a = src[3];
			dest[0] = b;
			dest[1] = g;
			dest[2] = r;
			dest[3] = a;
			src += 4;
			dest += 4;
		}
	}

	texture->UnlockRect(0);
	return reinterpret_cast<Rml::TextureHandle>(texture);
}

void RenderInterface_DX9::ReleaseTexture(Rml::TextureHandle texture_handle) {
	if (!texture_handle)
		return;
	reinterpret_cast<IDirect3DTexture9*>(texture_handle)->Release();
}
