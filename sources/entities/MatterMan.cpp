#include <entities/MatterMan.h>

constexpr f32 VIEWMODEL_SCALE = 0.01f; // We're doing this because we don't want our viewmodel to clip through stuff

void MatterMan::Spawn(Player* plr) {
    player = plr;
    viewmodel = GetModel("v_matman");
}

// Filter that ignores bodies that can't be held by the matter manipulator
class MatterManObjectLayerFilter : public ObjectLayerFilter
{
public:
    virtual bool ShouldCollide(ObjectLayer inObject) const override
    {
        if (inObject == Layers::NO_GRAB)
            return false;

        return true;
    }
};

void MatterMan::Tick() {
    if (player == nullptr) return; // Nobody picked us up yet

    bool holdingBrake = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Vec3 lookDir = player->cameraMatrix.GetRotation() * Vec3::sAxisZ();

    player->lookLocked = false;

    if (holdingBrake) {
        if (!holdBody.IsInvalid()) {
            Ref shape(body_interface->GetShape(holdBody).GetPtr());

            // Rotation controls
            // You have no idea how long it took me to figure this out
            if (IsKeyDown(KEY_R)) {
                Vector2 mouseDelta = GetMouseDelta();

                // Translate holdMatrixRelative to origin using holdPointRelative
                holdMatrixRelative = holdMatrixRelative * holdPointRelative;

                // Apply the rotation
                holdMatrixRelative = Mat44::sTranslation(holdMatrixRelative.GetTranslation()) * Mat44::sRotation(Quat::sEulerAngles(Vec3(
                    -mouseDelta.y * 0.5f * DEG2RAD,
                    mouseDelta.x * 0.5f * DEG2RAD,
                    0
                )) * holdMatrixRelative.GetRotation().GetQuaternion().Normalized());

                // Translate back to the original position
                holdMatrixRelative = holdMatrixRelative * holdPointRelative.Inversed();

                player->lookLocked = true;
            }

            // Push/pull controls
            if (scrollDirection != 0.0f) {
                Mat44 newHoldMatrixRelative = Mat44::sTranslation(Vec3::sAxisZ() * scrollDirection) * holdMatrixRelative;

                // Cast shape to see if with the new transforms the prop would collide with something
                TraceResult result = TraceShape(
                    shape,
                    body_interface->GetWorldTransform(holdBody),
                    lookDir * scrollDirection,
                    IgnoreSingleBodyFilter(holdBody)
                );

                if (!result.hit)
                    holdMatrixRelative = newHoldMatrixRelative;
            }

            // Apply forces
            Mat44 holdMatrixWorld = player->cameraMatrix * holdMatrixRelative;
            Vec3 cameraPosition = player->cameraMatrix.GetTranslation();

            Vec3 holdDirection = holdMatrixWorld.GetTranslation() - cameraPosition;

            holdPointWorld = body_interface->GetWorldTransform(holdBody) * holdPointRelative;

            Vec3 translationDirection = (holdMatrixWorld.GetTranslation() - body_interface->GetPosition(holdBody));
            Vec3 rotationalDirection = (holdMatrixWorld.GetRotation().GetQuaternion() * body_interface->GetRotation(holdBody).Conjugated()).GetEulerAngles();

            Vec3 velocity = translationDirection.Normalized() * Clamp(translationDirection.Length() * 20, 0.0f, 40.0f);
            Vec3 angularVelocity = rotationalDirection.Normalized() * Clamp(rotationalDirection.Length() * 20, 0.0f, 40.0f);

            body_interface->SetLinearAndAngularVelocity(holdBody, velocity, angularVelocity);
        }
        else {
            TraceResult result = TraceRay(player->cameraMatrix.GetTranslation(), lookDir * 100.0f, IgnoreSingleBodyFilter(player->body->GetID()), MatterManObjectLayerFilter());

            if (result.hit) {
                if (body_interface->GetMotionType(result.bodyId) == EMotionType::Dynamic) {
                    holdBody = result.bodyId;
                    holdPointRelative = body_interface->GetWorldTransform(holdBody).Inversed() * Mat44::sTranslation(result.point);

                    holdMatrixRelative = player->cameraMatrix.Inversed()
                        * body_interface->GetWorldTransform(holdBody);
                }
            }
        }
    }
    else {
        if (!holdBody.IsInvalid())
            holdBody = BodyID();
    }

    player->holdProp = holdBody;
}

void MatterMan::Render() {
    if (player == nullptr) return; // Nobody picked us up yet

    f32 bobIntensity = cos(GetTime() * 8) * 0.2f * (player->velocityState.Get() * Vec3(1, 0, 1)).Length() / PLAYER_MAX_SPEED;

    // Viewmodel sway
    Quat nextcameraRotation = player->cameraMatrix.GetRotation().GetQuaternion();
    Quat deltaRotation = nextcameraRotation * lastCameraRotation.Conjugated();
    f32 deltaAngleY = player->lookAngleY - lastAngleY;
    lastCameraRotation = nextcameraRotation;
    lastAngleY = player->lookAngleY;

    Mat44 viewmodelMatrix = player->cameraMatrix
        * Mat44::sRotation(Quat::sRotation(Vec3::sAxisY(), PI))
        * Mat44::sTranslation({ 0.7f * VIEWMODEL_SCALE, -1.0f * VIEWMODEL_SCALE, (-bobIntensity - 1.9f) * VIEWMODEL_SCALE });

    Vector<2> goal{};
    goal[0] = -deltaRotation.GetY() * 8;
    goal[1] = deltaAngleY * DEG2RAD * 8;

    sway.UpdateWithGoal(goal, GetFrameTime());

    if (sway.position.Length() > 0.0001f) {
        viewmodelMatrix = viewmodelMatrix
            * Mat44::sRotationY(sway.position[0])
            * Mat44::sRotationX(sway.position[1]);
    }

    // Drawing
    Vec3 axis;
    f32 angle;
    viewmodelMatrix.GetRotation().GetQuaternion().GetAxisAngle(axis, angle);
    Vec3 viewmodelPosition = viewmodelMatrix.GetTranslation();

    Vec3 pos = holdPointWorld.GetTranslation();

    DrawCube({ pos.GetX(), pos.GetY(), pos.GetZ() }, 0.1f, 0.1f, 0.1f, raylib::RED);

    viewmodel->Draw(
        { viewmodelPosition.GetX(), viewmodelPosition.GetY(), viewmodelPosition.GetZ() },
        { axis.GetX(), axis.GetY(), axis.GetZ() },
        angle * RAD2DEG, Vector3{ VIEWMODEL_SCALE, VIEWMODEL_SCALE, VIEWMODEL_SCALE }
    );
}