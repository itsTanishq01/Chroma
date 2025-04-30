#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Renderer.h"
#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer()
		: m_Camera(45.0f, 0.1f, 100.0f)
	{
		// Create materials
		// Floor material (large sphere)
		Material& floorMaterial = m_Scene.Materials.emplace_back();
		floorMaterial.Albedo = { 0.5f, 0.5f, 0.5f };      // Medium gray
		floorMaterial.Roughness = 0.2f;
		floorMaterial.Metallic = 0.0f;
		floorMaterial.ReflectionStrength = 0.1f;

		// Glass material (transparent sphere)
		Material& glassMaterial = m_Scene.Materials.emplace_back();
		glassMaterial.Albedo = { 0.9f, 0.9f, 1.0f };      // Very slight blue tint
		glassMaterial.Roughness = 0.0f;                   // Perfectly smooth
		glassMaterial.Metallic = 0.0f;
		glassMaterial.ReflectionStrength = 0.3f;
		glassMaterial.ReflectionTint = { 0.95f, 0.95f, 1.0f };
		glassMaterial.Transparency = 0.95f;               // High transparency
		glassMaterial.IndexOfRefraction = 1.52f;          // Glass IOR

		// Light source material
		Material& lightMaterial = m_Scene.Materials.emplace_back();
		lightMaterial.EmissionColor = { 1.0f, 0.9f, 0.7f }; // Warm white
		lightMaterial.EmissionPower = 25.0f;                // Bright light

		// Big sphere as floor
		{
			Sphere sphere;
			sphere.Position = { 0.0f, -10.0f, 0.0f };     // Center below the scene
			sphere.Radius = 10.0f;                        // Large radius to act as a floor
			sphere.MaterialIndex = 0;                     // Floor material
			m_Scene.Spheres.push_back(sphere);
		}

		// Small transparent sphere above
		{
			Sphere sphere;
			sphere.Position = { 0.0f, 1.0f, 0.0f };       // Floating above the "floor" sphere
			sphere.Radius = 1.0f;                         // Small sphere
			sphere.MaterialIndex = 1;                     // Glass material
			m_Scene.Spheres.push_back(sphere);
		}

		// Light sphere
		{
			Sphere sphere;
			sphere.Position = { 4.0f, 5.0f, -2.0f };      // Above and to the side
			sphere.Radius = 0.5f;                         // Small light source
			sphere.MaterialIndex = 2;                     // Light material
			m_Scene.Spheres.push_back(sphere);
		}
	}
	virtual void OnUpdate(float ts) override
	{
		if (m_Camera.OnUpdate(ts))
			m_Renderer.ResetFrameIndex();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
		ImGui::Checkbox("SlowRandom", &m_Renderer.GetSettings().SlowRandom);

		ImGui::SliderInt("Anti-aliasing", &m_Renderer.GetSettings().SamplesPerPixel, 1, 16);

		if (ImGui::Button("Reset"))
			m_Renderer.ResetFrameIndex();

		ImGui::End();

		ImGui::Begin("Scene");
		for (size_t i = 0; i < m_Scene.Spheres.size(); i++)
		{
			ImGui::PushID(i);

			Sphere& sphere = m_Scene.Spheres[i];
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::DragInt("Material", &sphere.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);

			ImGui::Separator();

			ImGui::PopID();
		}

		for (size_t i = 0; i < m_Scene.Materials.size(); i++)
		{
			ImGui::PushID(i);

			Material& material = m_Scene.Materials[i];
			ImGui::ColorEdit3("Albedo", glm::value_ptr(material.Albedo));
			ImGui::DragFloat("Roughness", &material.Roughness, 0.05f, 0.0f, 1.0f);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.05f, 0.0f, 1.0f);

			// Add these new controls
			ImGui::DragFloat("Reflection Strength", &material.ReflectionStrength, 0.05f, 0.0f, 1.0f);
			ImGui::ColorEdit3("Reflection Tint", glm::value_ptr(material.ReflectionTint));

			ImGui::ColorEdit3("Emission Color", glm::value_ptr(material.EmissionColor));
			ImGui::DragFloat("Emission Power", &material.EmissionPower, 0.05f, 0.0f, FLT_MAX);

			ImGui::DragFloat("Transparency", &material.Transparency, 0.05f, 0.0f, 1.0f);
			ImGui::DragFloat("Index of Refraction", &material.IndexOfRefraction, 0.05f, 1.0f, 3.0f);

			ImGui::Separator();

			ImGui::PopID();
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if (image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() },
				ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}

	void Render()
	{
		Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		m_LastRenderTime = timer.ElapsedMillis();
	}
private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	float m_LastRenderTime = 0.0f;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					app->Close();
				}
				ImGui::EndMenu();
			}
		});
	return app;
}