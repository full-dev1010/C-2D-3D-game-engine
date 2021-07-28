#include "Precompiled.h"
#include "ALManager.h"
#include "ALSoundNode.h"
#include "Maths/Maths.h"
#include "Graphics/Camera/Camera.h"
#include "Utilities/TimeStep.h"
#include "Scene/Component/SoundComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    namespace Audio
    {
        ALManager::ALManager(int numChannels)
            : m_Context(nullptr)
            , m_Device(nullptr)
            , m_NumChannels(numChannels)
        {
            m_DebugName = "OpenAL Audio";
        }

        ALManager::~ALManager()
        {
            alcDestroyContext(m_Context);
            alcCloseDevice(m_Device);
        }

        void ALManager::OnInit()
        {
            LUMOS_PROFILE_FUNCTION();
            m_Device = alcOpenDevice(nullptr);
            m_Context = alcCreateContext(m_Device, nullptr);

            LUMOS_LOG_INFO("Creating SoundSystem - {0}, Number of devices : ", alcGetString(m_Device, ALC_DEVICE_SPECIFIER), alcGetString(nullptr, ALC_DEVICE_SPECIFIER));

            if(!m_Device)
                LUMOS_LOG_INFO("Failed to create SoundSystem! (No valid device!)");

            alcMakeContextCurrent(m_Context);
            alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
        }

        void ALManager::OnUpdate(const TimeStep& dt, Scene* scene)
        {
            LUMOS_PROFILE_FUNCTION();
            auto& registry = scene->GetRegistry();
            auto listenerView = registry.view<Listener, Maths::Transform>();
            if(!listenerView.empty())
            {
                auto& listenerTransform = registry.get<Maths::Transform>(listenerView.front());
                UpdateListener(listenerTransform);
            }

            auto soundsView = registry.view<SoundComponent, Maths::Transform>();

            for(auto entity : soundsView)
            {
                auto soundNode = soundsView.get<SoundComponent>(entity).GetSoundNode();
                soundNode->SetPosition(soundsView.get<Maths::Transform>(entity).GetWorldPosition());
                soundNode->OnUpdate(dt.GetElapsedMillis());
            }
        }

        void ALManager::UpdateListener(Scene* scene)
        {
            auto& registry = scene->GetRegistry();
            auto listenerView = registry.view<Listener, Maths::Transform>();
            if(!listenerView.empty())
            {
                auto& listenerTransform = registry.get<Maths::Transform>(listenerView.front());
                UpdateListener(listenerTransform);
            }
        }

        //Pass Cameras transform
        void ALManager::UpdateListener(const Maths::Transform& listenerTransform)
        {
            LUMOS_PROFILE_FUNCTION();
            {
                Maths::Vector3 worldPos = listenerTransform.GetWorldPosition();
                Maths::Vector3 velocity = Maths::Vector3(0.0f); //TODO: m_Listener->GetVelocity();

                ALfloat direction[6];

                Maths::Quaternion orientation = listenerTransform.GetWorldOrientation();

                direction[0] = -2 * (orientation.w * orientation.y + orientation.x * orientation.z);
                direction[1] = 2 * (orientation.x * orientation.w - orientation.z * orientation.y);
                direction[2] = 2 * (orientation.x * orientation.x + orientation.y * orientation.y) - 1;
                direction[3] = 2 * (orientation.x * orientation.y - orientation.w * orientation.z);
                direction[4] = 1 - 2 * (orientation.x * orientation.x + orientation.z * orientation.z);
                direction[5] = 2 * (orientation.w * orientation.x + orientation.y * orientation.z);

                alListenerfv(AL_POSITION, reinterpret_cast<float*>(&worldPos));
                alListenerfv(AL_VELOCITY, reinterpret_cast<float*>(&velocity));
                alListenerfv(AL_ORIENTATION, direction);
            }
        }

        void ALManager::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui::TextUnformatted("OpenAL Audio");

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Number Of Audio Sources");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%5.2lu", m_SoundNodes.size());
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Number Of Channels");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::Text("%5.2i", m_NumChannels);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
    }
}
