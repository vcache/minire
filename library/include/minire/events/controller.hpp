#pragma once

#include <minire/events/controller/general.hpp>
#include <minire/events/controller/sprites.hpp>
#include <minire/events/controller/labels.hpp>
#include <minire/events/controller/text-input.hpp>
#include <minire/events/controller/scene.hpp>

#include <variant>

namespace minire::events
{
    using Controller = std::variant<controller::Quit,
                                    controller::SetMouseMode,
                                    controller::DebugDrawsUpdate,
                                    controller::SetInstrumentation,
                                    controller::NewResourceLayer,
                                    controller::DisposeResourceLayer,
                                    controller::ContentManagerCleanup,
                                    controller::CreateVertexBuffer,
                                    controller::DisposeVertexBuffer,

                                    controller::CreateSprite,
                                    controller::ResizeSprite,
                                    controller::MoveSprite,
                                    controller::SetSpriteArea,
                                    controller::SetSpriteClippingWindow,
                                    controller::SetSpriteVisible,
                                    controller::SetSpriteZOrder,
                                    controller::RemoveSprite,
                                    controller::BulkSetSpriteZOrders,

                                    controller::CreateLabel,
                                    controller::MoveLabel,
                                    controller::SetLabelVisible,
                                    controller::SetLabelText,
                                    controller::SetLabelFontFace,
                                    controller::SetLabelClippingWindow,
                                    controller::SetLabelZOrder,
                                    controller::RemoveLabel,
                                    controller::BulkSetLabelZOrders,

                                    controller::StartTextInput,
                                    controller::StopTextInput,

                                    controller::SceneReset,
                                    controller::SceneDispose,
                                    controller::SceneActivateCamera,
                                    controller::SceneSetAmbientLight,
                                    controller::SceneNewNode,
                                    controller::SceneNewFromSource,
                                    controller::SceneNewMesh,
                                    controller::SceneNewDirectionalLight,
                                    controller::SceneNewPointLight,
                                    controller::SceneNewPerspectiveCamera,
                                    controller::SceneNewOrthographicCamera,
                                    controller::SceneNewBillboard,
                                    controller::SceneSetParent,
                                    controller::SceneSetVisibility,
                                    controller::SceneSetTransform,
                                    controller::SceneSetDirectionalLight,
                                    controller::SceneSetPointLight,
                                    controller::SceneSetPerspectiveCamera,
                                    controller::SceneSetOrthographicCamera,
                                    controller::SceneSetMeshEmissiveFactor,
                                    controller::SceneSetMeshSkin,
                                    controller::SceneNewAnimationSet,
                                    controller::ScenePlayAnimation,
                                    controller::SceneStopAnimation,
                                    controller::SceneInlineAnimation,
                                    controller::SetRayCaster>;
}
