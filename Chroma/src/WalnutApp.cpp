#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Timer.h"

#include "Renderer.h"
#include "Camera.h"
#include "Shapes.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
    ExampleLayer()
        : m_Camera(45.0f, 0.1f, 100.0f)
    {
        CreateMaterials();
        CreateScene();
    }

    void CreateMaterials()
    {
        // Floor material
        Material& floorMaterial = m_Scene.Materials.emplace_back();
        floorMaterial.Albedo = { 0.9f, 0.9f, 0.9f };     // White
        floorMaterial.Roughness = 0.8f;
        floorMaterial.Metallic = 0.0f;
        floorMaterial.ReflectionStrength = 0.05f;

        // Glass material
        Material& glassMaterial = m_Scene.Materials.emplace_back();
        glassMaterial.Albedo = { 0.9f, 0.9f, 1.0f };     // Very slight blue tint
        glassMaterial.Roughness = 0.0f;                  // Perfectly smooth
        glassMaterial.Metallic = 0.0f;
        glassMaterial.ReflectionStrength = 0.3f;
        glassMaterial.ReflectionTint = { 0.95f, 0.95f, 1.0f };
        glassMaterial.Transparency = 0.95f;              // High transparency
        glassMaterial.IndexOfRefraction = 1.52f;         // Glass IOR

        // Red box material
        Material& redMaterial = m_Scene.Materials.emplace_back();
        redMaterial.Albedo = { 0.9f, 0.1f, 0.1f };       // Red
        redMaterial.Roughness = 0.1f;                    // Smooth
        redMaterial.Metallic = 1.0f;
        redMaterial.ReflectionStrength = 0.8f;

        // Green box material
        Material& greenMaterial = m_Scene.Materials.emplace_back();
        greenMaterial.Albedo = { 0.1f, 0.9f, 0.1f };     // Green
        greenMaterial.Roughness = 0.4f;                  // Moderate roughness
        greenMaterial.Metallic = 0.0f;
        greenMaterial.ReflectionStrength = 0.2f;

        // Light source material
        Material& lightMaterial = m_Scene.Materials.emplace_back();
        lightMaterial.EmissionColor = { 1.0f, 0.9f, 0.7f }; // Warm white
        lightMaterial.EmissionPower = 25.0f;                // Bright light
    }

    void CreateScene()
    {
        // Add a plane as a floor
        Shapes::AddPlane(m_Scene, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0);

        // Add the glass sphere
        Shapes::AddSphere(m_Scene, glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 1);

        // Add a red box
        Shapes::AddCube(m_Scene, glm::vec3(3.0f, 1.0f, 0.0f), 1.0f, 2);

        // Add a green box
        Shapes::AddCube(m_Scene, glm::vec3(-3.0f, 0.5f, 0.0f), 1.0f, 3);

        // Add a green pyramid
        Shapes::AddPyramid(m_Scene, glm::vec3(0.0f, 0.0f, -2.0f), 2.0f, 2.0f, 3);

        // Add a light source
        Shapes::AddSphere(m_Scene, glm::vec3(0.0f, 5.0f, 0.0f), 0.5f, 4);
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

        // Sphere section
        if (ImGui::CollapsingHeader("Spheres"))
        {
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

            // Add new sphere button
            if (ImGui::Button("Add Sphere"))
            {
                Shapes::AddSphere(m_Scene, glm::vec3(0.0f), 0.5f, 0);
                m_Renderer.ResetFrameIndex();
            }
        }

        // Plane section
        if (ImGui::CollapsingHeader("Planes"))
        {
            for (size_t i = 0; i < m_Scene.Planes.size(); i++)
            {
                ImGui::PushID(i + 1000); // Offset to avoid ID conflicts

                Plane& plane = m_Scene.Planes[i];
                ImGui::DragFloat3("Normal", glm::value_ptr(plane.Normal), 0.1f);
                ImGui::DragFloat("Distance", &plane.Distance, 0.1f);
                ImGui::DragInt("Material", &plane.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);

                ImGui::Separator();

                ImGui::PopID();
            }

            // Add new plane button
            if (ImGui::Button("Add Plane"))
            {
                Shapes::AddPlane(m_Scene, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0);
                m_Renderer.ResetFrameIndex();
            }
        }

        // Box section
        if (ImGui::CollapsingHeader("Boxes"))
        {
            for (size_t i = 0; i < m_Scene.Boxes.size(); i++)
            {
                ImGui::PushID(i + 2000); // Offset to avoid ID conflicts

                Box& box = m_Scene.Boxes[i];
                ImGui::DragFloat3("Min", glm::value_ptr(box.Min), 0.1f);
                ImGui::DragFloat3("Max", glm::value_ptr(box.Max), 0.1f);
                ImGui::DragInt("Material", &box.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);

                ImGui::Separator();

                ImGui::PopID();
            }

            // Add new box button
            if (ImGui::Button("Add Box"))
            {
                Shapes::AddCube(m_Scene, glm::vec3(0.0f), 1.0f, 0);
                m_Renderer.ResetFrameIndex();
            }
        }

        // Triangle section
        if (ImGui::CollapsingHeader("Triangles"))
        {
            for (size_t i = 0; i < m_Scene.Triangles.size(); i++)
            {
                ImGui::PushID(i + 3000); // Offset to avoid ID conflicts

                Triangle& triangle = m_Scene.Triangles[i];
                ImGui::DragFloat3("Vertex 0", glm::value_ptr(triangle.v0), 0.1f);
                ImGui::DragFloat3("Vertex 1", glm::value_ptr(triangle.v1), 0.1f);
                ImGui::DragFloat3("Vertex 2", glm::value_ptr(triangle.v2), 0.1f);
                ImGui::DragInt("Material", &triangle.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);

                ImGui::Separator();

                ImGui::PopID();
            }

            // Add new triangle button
            if (ImGui::Button("Add Triangle"))
            {
                Triangle triangle(
                    glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                triangle.MaterialIndex = 0;
                m_Scene.Triangles.push_back(triangle);
                m_Renderer.ResetFrameIndex();
            }

            // Add pyramid button (convenience)
            if (ImGui::Button("Add Pyramid"))
            {
                Shapes::AddPyramid(m_Scene, glm::vec3(0.0f), 1.0f, 1.0f, 0);
                m_Renderer.ResetFrameIndex();
            }
        }

        // Material section
        for (size_t i = 0; i < m_Scene.Materials.size(); i++)
        {
            ImGui::PushID(i + 4000); // Offset to avoid ID conflicts

            Material& material = m_Scene.Materials[i];

            if (ImGui::TreeNode(("Material " + std::to_string(i)).c_str()))
            {
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

                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        // Add new material button
        if (ImGui::Button("Add Material"))
        {
            Material material;
            material.Albedo = { 0.8f, 0.8f, 0.8f };
            material.Roughness = 0.5f;
            material.Metallic = 0.0f;
            m_Scene.Materials.push_back(material);
            m_Renderer.ResetFrameIndex();
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