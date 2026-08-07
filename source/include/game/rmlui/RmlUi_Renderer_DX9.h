#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <d3d9.h>

// DirectX 9 render interface for RmlUi (basic geometry + scissor + textures).
class RenderInterface_DX9 : public Rml::RenderInterface {
public:
	RenderInterface_DX9();
	~RenderInterface_DX9() override;

	void SetDevice(IDirect3DDevice9* device);
	void SetViewport(int width, int height);

	void BeginFrame();
	void EndFrame();

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

private:
	struct GeometryView {
		Rml::Span<const Rml::Vertex> vertices;
		Rml::Span<const int> indices;
	};

	struct DX9Vertex {
		float x, y, z;
		DWORD color;
		float u, v;
	};

	void ApplyTransform(Rml::Vector2f translation);
	static DWORD ToD3DColor(const Rml::ColourbPremultiplied& colour);

	IDirect3DDevice9* m_device = nullptr;
	int m_width = 0;
	int m_height = 0;
	bool m_hasTransform = false;
	Rml::Matrix4f m_transform = Rml::Matrix4f::Identity();
};
