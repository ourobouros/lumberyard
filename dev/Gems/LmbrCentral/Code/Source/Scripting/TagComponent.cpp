/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "LmbrCentral_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "TagComponent.h"

namespace LmbrCentral
{
    // BehaviorContext TagComponentNotificationsBus forwarder
    class BehaviorTagComponentNotificationsBusHandler : public TagComponentNotificationsBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTagComponentNotificationsBusHandler,"{7AEDC591-41AB-4E3B-87D2-03346154279D}",AZ::SystemAllocator,
            OnTagAdded, OnTagRemoved);

        void OnTagAdded(const Tag& tag) override
        {
            Call(FN_OnTagAdded, tag);
        }

        void OnTagRemoved(const Tag& tag) override
        {
            Call(FN_OnTagRemoved, tag);
        }
    };

    class BehaviorTagGlobalNotificationBusHandler : public TagGlobalNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTagGlobalNotificationBusHandler, "{87E9363C-C346-4A1E-BCDA-37C0504B1985}", AZ::SystemAllocator,
            OnEntityTagAdded, OnEntityTagRemoved);

        void OnEntityTagAdded(const AZ::EntityId& entityId) override
        {
            Call(FN_OnEntityTagAdded, entityId);
        }

        void OnEntityTagRemoved(const AZ::EntityId& entityId) override
        {
            Call(FN_OnEntityTagRemoved, entityId);
        }
    };

    //=========================================================================
    // Component Descriptor
    //=========================================================================
    void TagComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TagComponent, AZ::Component>()
                ->Version(1)
                ->Field("Tags", &TagComponent::m_tags);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<TagComponentRequestBus>("TagComponentRequestBus")
                ->Event("HasTag", &TagComponentRequestBus::Events::HasTag)
                ->Event("AddTag", &TagComponentRequestBus::Events::AddTag)
                ->Event("RemoveTag", &TagComponentRequestBus::Events::RemoveTag)
                ;

            behaviorContext->EBus<TagGlobalRequestBus>("TagGlobalRequestBus")
                ->Event("RequestTaggedEntities", &TagGlobalRequestBus::Events::RequestTaggedEntities)
                ;
            
            behaviorContext->EBus<TagComponentNotificationsBus>("TagComponentNotificationsBus")
                ->Handler<BehaviorTagComponentNotificationsBusHandler>()
                ;

            behaviorContext->EBus<TagGlobalNotificationBus>("TagGlobalNotificationBus")
                ->Handler<BehaviorTagGlobalNotificationBusHandler>()
                ;
        }
    }

    //=========================================================================
    // AZ::Component
    //=========================================================================
    void TagComponent::Activate()
    {
        for (const Tag& tag : m_tags)
        {
            TagGlobalRequestBus::MultiHandler::BusConnect(tag);
            EBUS_EVENT_ID(tag, TagGlobalNotificationBus, OnEntityTagAdded, GetEntityId());
        }
        TagComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void TagComponent::Deactivate()
    { 
        TagComponentRequestBus::Handler::BusDisconnect();
        for (const Tag& tag : m_tags)
        {
            TagGlobalRequestBus::MultiHandler::BusDisconnect(tag);
            EBUS_EVENT_ID(tag, TagGlobalNotificationBus, OnEntityTagRemoved, GetEntityId());
        }
    }

    //=========================================================================
    // EditorTagComponent friend will call this
    //=========================================================================
    void TagComponent::EditorSetTags(Tags&& editorTagList)
    {
        m_tags = AZStd::move(editorTagList);
    }

    //=========================================================================
    // TagRequestBus
    //=========================================================================
    bool TagComponent::HasTag(const Tag& tag)
    {
        return m_tags.find(tag) != m_tags.end();
    }

    void TagComponent::AddTag(const Tag& tag)
    {
        if (m_tags.insert(tag).second)
        {
            EBUS_EVENT_ID(GetEntityId(), TagComponentNotificationsBus, OnTagAdded, tag);
            EBUS_EVENT_ID(tag, TagGlobalNotificationBus, OnEntityTagAdded, GetEntityId());
            TagGlobalRequestBus::MultiHandler::BusConnect(tag);
        }
    }

    void TagComponent::AddTags(const Tags& tags)
    {
        for (const Tag& tag : tags)
        {
            AddTag(tag);
        }
    }

    void TagComponent::RemoveTag(const Tag& tag)
    {
        if (m_tags.erase(tag) > 0)
        {
            EBUS_EVENT_ID(GetEntityId(), TagComponentNotificationsBus, OnTagRemoved, tag);
            EBUS_EVENT_ID(tag, TagGlobalNotificationBus, OnEntityTagRemoved, GetEntityId());
            TagGlobalRequestBus::MultiHandler::BusDisconnect(tag);
        }
    }

    void TagComponent::RemoveTags(const Tags& tags)
    {
        for (const Tag& tag : tags)
        {
            RemoveTag(tag);
        }
    }

} // namespace LmbrCentral
