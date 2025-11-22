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
                                    controller::SetSpriteVisible,
                                    controller::SetSpriteZOrder,
                                    controller::RemoveSprite,
                                    controller::BulkSetSpriteZOrders,

                                    controller::CreateLabel,
                                    controller::MoveLabel,
                                    controller::SetLabelVisible,
                                    controller::SetLabelText,
                                    controller::SetLabelFontFace,
                                    controller::SetLabelClipping,
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
                                    controller::SceneNewPointLight,
                                    controller::SceneNewPerspectiveCamera,
                                    controller::SceneNewOrthographicCamera,
                                    controller::SceneNewBillboard,
                                    controller::SceneSetParent,
                                    controller::SceneSetVisibility,
                                    controller::SceneSetTransform,
                                    controller::SceneSetPointLight,
                                    controller::SceneSetPerspectiveCamera,
                                    controller::SceneSetOrthographicCamera,
                                    controller::SceneSetMeshEmissiveFactor,
                                    controller::SceneNewAnimationSet,
                                    controller::ScenePlayAnimation,
                                    controller::SceneStopAnimation,
                                    controller::SetRayCaster>;
}
