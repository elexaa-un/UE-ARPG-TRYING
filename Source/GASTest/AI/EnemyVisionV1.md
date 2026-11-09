# Enemy Vision V1 setup and acceptance checks

## Files

Created: `AI/Observation/EnemyObservation.h`, `AI/Perception/EnemyVisionComponent.h`,
`AI/Perception/EnemyVisionComponent.cpp`, `AI/Tests/EnemyVisionTests.cpp`, and this guide.
Generated validation log: `AI/Tests/EnemyVisionAutomation.log`.
Modified: `EnemyCharacter.h`, `EnemyCharacter.cpp`.
No Build.cs changes: existing Core, CoreUObject and Engine dependencies suffice.
No changes to GAS initialization, player code, lock-on or combat base.

## Verified results (2026-09-27, Unreal Engine 5.8.1)

GASTestEditor Win64 Development compiled and linked successfully with
`-ModuleWithSuffix=GASTest,9273`. A suffixed DLL was necessary because the open
editor held the ordinary project DLL locked. The existing editor session has not
been verified to have loaded the new code. For normal use, save and close the
editor, build GASTestEditor normally, then reopen the project.

Headless engine automation `GASTest.AI.EnemyVision.Acceptance` returned Success
and the process exited with code 0. The test uses a temporary game world, real
query collision shapes, Visibility traces, and the component's actual timer.
It covers all ten core scenarios below plus LOS disabled, target switching with
rate reset, Actor Tag filtering, rear-angle wrapping, and VisionPoint offset/rotation/mesh-independence regression checks. It does not change maps
or Blueprint assets. Test code is guarded by WITH_DEV_AUTOMATION_TESTS.

The actual player Blueprint Actor Tag, level collision configuration, Blueprint
inspection, and rendered debug drawing still require the PIE checks below.

Run again through Session Frontend > Automation > GASTest.AI.EnemyVision.Acceptance,
or launch UnrealEditor-Cmd with your project and these arguments:

```text
-ExecCmds="Automation RunTests GASTest.AI.EnemyVision.Acceptance" -TestExit="Automation Test Queue Empty" -unattended -NullRHI -nosound -nop4
```

## Blueprint and collision setup

### VisionPoint alignment

In BP_EnemyCharacter, select the inherited VisionPoint scene component. Adjust its
Relative Location to the head/eyes and its Relative Rotation so local +X points
out of the face and local +Y points right. It attaches to the capsule/root, not
the skeletal mesh or a bone/socket. No mesh-axis correction is hardcoded.

The point's world location is used for overlap search, LOS origin, range estimates
and debug drawing. Its world Forward/Right define FOV, relative yaw/direction and
the debug fan. EyeOffset is ignored while the point is valid; without a valid
point, the original owner-location + rotated EyeOffset and owner-yaw axes apply.
This also supports the existing generic-actor fallback tests.

After a normal build and editor restart, verify in PIE that rotating only
VisionPoint rotates the debug fan and changes the detected bearing, while rotating
only the Mesh leaves perception unchanged. Moving VisionPoint should move the ray
origin and change the eye-to-target distance. The automated test also exercises
nonzero point pitch/yaw/roll, an offset origin, right-side bearing and rear rejection.

After building, restart the editor so existing enemy Blueprint subclasses load
the new native default subobject. Open the enemy Blueprint and select its inherited
EnemyVisionComponent. Do not add another copy manually. Default settings are
1500 cm range, 100 degree horizontal FOV, 0.1 second interval, LOS enabled, and
VisionPoint at relative location (0, 0, 70) with zero relative rotation.

On the player Blueprint, Class Defaults > Actor > Tags, add `Player`.
This is an Actor Tag, not a component tag or GAS GameplayTag. Existing placed
instances with overridden tags also need the tag. No player C++ class is referenced.

The player's capsule must have collision queries enabled (Query Only or Query and
Physics) and object type Pawn. Empty TargetObjectTypes defaults to Pawn; configure
the array if your target uses a different object type. Generate Overlap Events is
not required for this explicit overlap query. Walls need query collision with a
simple collision shape and must Block Visibility. A target's own capsule/mesh
blocking Visibility is accepted as visible. A clear trace that hits nothing is
also accepted. The owner is ignored. Attached actors remain separate occluders.

Read Get Enemy Vision Component > Get Latest Observation > Break Enemy Observation
in Blueprint. The snapshot is local to each world instance; it is not replicated.
Green marks the selected visible target; red ends at an occluding hit; cyan is
forward; yellow shows horizontal FOV boundaries and range arc. Drawing expires
after one sampling interval. Disable bDrawVisionDebug/bEnableVisionLogs as needed.
Output Log category is LogEnemyVision. Invisible observations only log on loss.

## Semantics

Range and distance use the eye-to-actor-center vector, in centimeters. Horizontal
bearing uses VisionPoint world Forward/Right axes projected into its local horizontal plane. Pitch/roll edits rotate that plane and the debug FOV together.
There is no vertical FOV gate. A directly vertical/coincident target has zero
horizontal direction and yaw. Confidence is the equally weighted normalized
distance and centering score; LOS gates visibility when enabled.

Rates use actual elapsed game time and consecutive samples of the same visible
actor. They include enemy movement/rotation. Acquisition, switching and reacquisition
start with zero rates. Lost sight resets all fields except the current timestamp;
no last-seen data is retained. A runtime interval edit takes effect after the next
scheduled sample; intervals below 0.01 seconds are clamped.

## Runtime acceptance checklist (requires PIE)

Use a stationary enemy with yaw 0 (forward +X, right +Y), default settings, no
obstructions, and one tagged player. Positions below are relative to the enemy
actor origin; keep the player's center at the enemy's center height. Wait at least
0.2 seconds after each placement and inspect the snapshot, debug lines and log.
For rates, move continuously for several samples rather than inspecting only a
teleport or a stopped actor.

1. At (600, 0, 0): visible, yaw approximately 0, direction X positive, distance
   approximately 604 cm (70 cm eye offset), confidence positive, green line.
2. At (600, 300, 0): visible, yaw approximately +26.6 degrees, direction Y positive.
3. At (600, -300, 0): visible, yaw approximately -26.6 degrees, direction Y negative.
4. Move from (800, 0, 0) toward (400, 0, 0): distance decreases, closing speed positive.
5. Reverse that movement: distance increases, closing speed negative.
6. Strafe across the front at X=600: yaw changes and angular velocity is nonzero;
   motion toward +Y gives positive angular velocity with a stationary enemy.
7. At (-600, 0, 0): invisible; direction, yaw, distance, confidence and rates zero.
8. At (600, 0, 0), place a Visibility-blocking wall across the eye-to-player line:
   invisible and a red line to the wall. Disable LOS temporarily: visible again.
9. Restore LOS and remove the wall; at (1600, 0, 0): invisible.
10. Return to (600, 0, 0): visible without memory; first visible sample has zero
    rates. Continue moving to see rates resume on the next sample.

Additional boundary checks:

- Put a second tagged Pawn closer and more centered: it wins; switching has zero
  rates. Remove its tag or destroy it: the remaining visible target is selected.
- With FOV 360, move around the rear from +179 to -179 degrees: the angular delta
  is approximately +2 degrees divided by elapsed time, not -358 degrees.
- Disable collision queries on the target: it disappears from candidate search.
- Change interval at runtime: sampling adopts it after the pending sample.
- Stop PIE/destroy the enemy: its timer is cleared.

These PIE checks are a manual acceptance procedure, not a claim of executed tests.
