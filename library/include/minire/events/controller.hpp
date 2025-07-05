#pragma once

#include <minire/events/controller/general.hpp>
#include <minire/events/controller/sprites.hpp>
#include <minire/events/controller/labels.hpp>
#include <minire/events/controller/text-input.hpp>
#include <minire/events/controller/scene.hpp>

#include <minire/utils/safe-queue.hpp>

#include <variant>

namespace minire::events
{
    using Controller = std::variant<controller::Quit,
                                    controller::NewEpoch,
                                    controller::MouseGrab,
                                    controller::DebugDrawsUpdate,

                                    controller::CreateSprite,
                                    controller::CreateNinePatch,
                                    controller::ResizeNinePatch,
                                    controller::MoveSprite,
                                    controller::VisibleSprite,
                                    controller::RemoveSprite,
                                    controller::BulkSetSpriteZOrders,

                                    controller::CreateLabel,
                                    controller::ResizeLabel,
                                    controller::MoveLabel,
                                    controller::SetCharLabel,
                                    controller::SetSymbolLabel,
                                    controller::UnsetCharLabel,
                                    controller::SetStringLabel,
                                    controller::SetLabelCursor,
                                    controller::UnsetLabelCursor,
                                    controller::SetLabelVisible,
                                    controller::SetLabelFonts,
                                    controller::RemoveLabel,
                                    controller::BulkSetLabelZOrders,

                                    controller::StartTextInput,
                                    controller::StopTextInput,

                                    controller::SceneReset,
                                    controller::SceneEmergeModel,
                                    controller::SceneEmergePointLight,
                                    controller::SceneUnmergeModel,
                                    controller::SceneUnmergePointLight,
                                    controller::SceneUpdateFpsCamera,
                                    controller::SceneUpdateModel,
                                    controller::SceneUpdateLight,
                                    controller::SceneSetSelectedModels>;

    using ControllerQueue = utils::SafeQueue<Controller, true>;
}
