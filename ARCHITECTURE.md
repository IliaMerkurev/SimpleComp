# SimpleComp architecture

SimpleComp contains autonomous components with explicit cooperation where useful. Stack and Collector cooperate within Spawning; resource actors implement an interface. Movement and Animation do not require Spawning. No game-project module, mandatory actor base class, or global manager is required.

## Modules and source registry

Public API lives under `Source/<Module>/Public`; implementation lives under `Private` with matching subdirectories.

| Area | Classes and responsibility |
| --- | --- |
| Runtime module | `FSimpleCompModule`; Core, CoreUObject, Engine dependencies |
| Movement | `USCRotationComponent`: target/velocity/displacement/constant rotation; `USCSphereRollComponent`: visual rolling; `USCWheelComponent`: rolling and steering; `USCFollowConstraintComponent`: rope and axis constraints on the owner actor |
| Animation | `USCAnimSequence`: reusable tracks/notifies; `USCCurveAnimComponent`: playback and pose evaluation; `USCAnimAsyncAction`: subscription lifetime for K2 playback |
| Spawning | `USCStackComponent`: HISM grid, reservations, filling, deformation, extraction/conversion; `USCCollectorComponent`: overlaps and flight assignment; `USCSpawnerComponent`: weighted bursts, flow, messages, and physics launch |
| Core | `SCTypes.h`: shared enums/settings; `ISCCollectableInterface::InitFlight`: resource flight handoff; `ISCMessageInterface::OnReceiveSCMessage`: generic actor messages |
| Compiler module | `FSimpleCompEditorModule`, `UK2Node_PlaySCAnimation`; `UncookedOnly` module expands the node into runtime calls/delegates. `BlueprintGraph` is a public dependency because the public node derives from `UK2Node`. Runtime has no compiler dependency. |

## Stack identity and lifetime

`Free -> Reserved -> Filled` describes slot occupancy. `RequestSlot` returns a monotonically allocated **TicketID**, unique for that component's lifetime. `INDEX_NONE` means allocation failed. Tickets are not recycled; allocation fails if the integer identity space is exhausted.

A **physical index** addresses `SlotStatuses` and its corresponding HISM instance. Extraction and shrinking can change that index. A ticket-to-index map resolves its current location. Pending fills, animations, and spawn waves store tickets. `OnSlotFilled` reports the ticket; the existing `OnActorSpawned` argument `SlotID` remains the physical index at successful removal. `HasTicket` queries whether a reservation or filled resource still owns a ticket.

`ConfirmArrival` transitions a reservation into a filled, scale-animating element. One component timer advances all active scale animations using elapsed world time. Completion records are detached before Blueprint events run and each ticket is rechecked before dispatch. Removing an element cancels its animation. Extraction compacts slot data and rebuilds the lookup; unrelated animations and callbacks keep their identities.

Random rotation/scale use the seed and live ticket, retaining variation through compaction. Editing random settings intentionally changes that variation. `GetSlotWorldTransform` and extraction return the full target transform, including random scale, instead of the near-zero visibility scale of a reservation or unfinished animation.

`SetFillLevel` reconciles against actual occupancy and supports repeated requests after extraction. It cancels only its own pending fill reservations. External flight reservations count toward the target and are not discarded when the level decreases; occupancy can therefore exceed the target until those reservations are resolved.

With `bEnableFillAnimation`, changes to FillLevel start all newly needed slots together, including calls through `SetFillLevel`. Sequencer supplies the fill timing; `FillStaggerDelay` does not add another queue. Individual elements still grow over `ScaleAnimationDuration`. With fill monitoring disabled, `SetFillLevel` retains staggered filling. Enabling monitoring drains/reconciles any outstanding stagger reservations on the next settings update.

Scale-animation batches and full transform refreshes request an asynchronous HISM tree rebuild. Game-world scale-only updates can otherwise retain cluster bounds from hidden instances; marking render state dirty alone does not refresh that hierarchy. The owner confirmed that the reported fill-timing and visibility regressions are resolved in their scene; broader validation remains separate.

Successful actor conversion removes the resource before actor-message/Blueprint callbacks. Failed spawning retains the resource and ticket. Completion means the scheduled waves were processed, not that every spawn succeeded. A conversion snapshots currently filled tickets; later arrivals are not added to those waves. An actor currently being constructed for conversion cannot also be extracted by a reentrant callback.

Runtime resizing preserves tickets, reservations, scale progress, and queued operations. A smaller layout compacts occupied slots if they fit. Capacity below occupancy, or integer-overflowing capacity, is rejected and the last applied dimensions are restored. Mesh/settings edits preserve occupancy. There is no destructive rebuild API in 0.11.

`EndPlay` and destruction stop spawn/fill/animation/settings timers. Revision checks prevent callbacks from continuing an obsolete spawn or fill operation.

## Mutable settings and geometry

Stack has Tick capability but enables per-frame work only for deformation or animated FillLevel monitoring. Raw runtime edits synchronize every 0.1 seconds while otherwise idle; `RefreshSettings` applies them immediately. Active Tick also synchronizes settings. A geometry settings fingerprint gates geometry updates; moving manual-curve controls are evaluated during active Tick. Collision remains editable. FillLevel uses the `Interp` UPROPERTY specifier.

Manual curves and inertia operate in Stack component local space. Inertia derives acceleration from component world displacement, converts it to local axes, and uses an implicit damped spring step. `TiltScale` applies to both modes. These visible changes require comparison against existing scenes.

Collector retains both default shape components and switches the active collision volume. Raw shape edits synchronize every 0.1 seconds; `RefreshSettings` applies them immediately. Initial overlaps are inspected after BeginPlay ordering settles. A local weak actor/ticket map avoids duplicate assignments by the same collector and releases reservations whose actor has been destroyed. Cross-collector exclusivity remains the resource actor's responsibility: `InitFlight` must handle flight state, confirm arrival, release cancelled reservations, and validate its target. Destroying a collector does not revoke living resources' assigned flights.

Spawner clears old flow/duration/repeat timers on restart and guards work after actor callbacks. Volume, class, and launch settings apply to subsequent bursts. Raw flow mode/timing edits restart an active schedule within 0.1 seconds; call `Spawn` to restart immediately. Non-positive/non-finite flow intervals produce one burst per active phase. The settings timer stops with the schedule. Actor callbacks may stop/restart spawning or destroy an actor; obsolete work does not continue.

## Animation time and events

Playback position and `CurrentTime` are **playback seconds**. A positive sequence `DefaultDuration` is the sample-time extent; otherwise it is inferred from curve ranges. Conversion is `sample = playback / PlaybackDuration * sequence extent`. Editing positive playback duration during active playback preserves normalized progress. Negative `PlayRate` reverses traversal relative to the selected direction.

Each update computes its final pose once, dispatches crossed notifies in traversal order, then Update, then Finished at a non-looping endpoint. Notify outputs observe that frame's final position. A callback that stops, seeks, pauses, restarts, or replaces playback aborts remaining old dispatch. Finished disables the completed playback **before** calling handlers, allowing a handler to start another animation.

- Forward intervals exclude their starting time and include their ending time; reverse intervals follow the same rule in the opposite direction.
- Starting from an endpoint includes its markers on the first non-zero advance. Zero rate does not consume the boundary event.
- Loop crossings dispatch the departing endpoint then the entering endpoint (end then zero forward; zero then end reverse), once each per crossing, including exact boundary landings. Multiple loops are traversed in order. No-notify playback avoids this loop. Unrepresentable traversal increments stop playback with a diagnostic instead of hanging.
- Equal-time markers retain authored order. Empty-name/out-of-range markers are ignored.
- `SetPlaybackPosition` evaluates playback seconds even while paused/stopped. It dispatches no traversal notifies, Update, or Finished and does not reconstruct prior scene state.
- Stop retains position and pose. PlayFromStart and ReverseFromEnd restart; K2 Play continues. ReverseFromCurrent toggles direction without changing position. Pause/Resume retain the playback subscription.

Initial Local and World transforms are captured separately on initial use/BeginPlay. Additive tracks use the corresponding baseline; Local transforms follow transformed parents. Float curves drive scalar axes; CurveTables use X/Y/Z rows. The broad `UCurveBase` property and `CustomFloat` enum remain for compatibility; vector-curve evaluation and custom-float output are not implemented.

## Async and K2

Proxy creation/activation does not start playback or register dormant controls. A playback command registers its proxy with the game instance and subscribes once; replacement, Stop, completion, EndPlay, or destruction detaches it. The component reference is weak; the sequence reference is strong. Pause/Resume/Stop command proxies never acquire unused registrations.

The node uses explicit UE5 Double subtypes for Real pins and shared temporary time outputs assigned by each event. Unique notify names produce one execution pin each. Names colliding with built-in pins receive an internal `SCNotify:` prefix while retaining their friendly name. Pins come from the selected author-time sequence; a runtime-only sequence cannot manufacture new compiled outputs. Native reconstruction, saved-asset compatibility, and compiler expansion remain pending validation.

## Movement contracts

- SphereRoll converts the world rolling axis to relative rotation space. Its initial relative pose remains the return target.
- Wheel preserves its initial relative orientation and lets steering converge while stationary. Disabling steering returns yaw toward zero.
- Rolling radii are world-space centimeters. Non-positive/non-finite values suspend rolling without accumulating a deferred movement jump.
- Rotation's null target uses `TargetLocationOffset` as a static world location. Constant rotation remains local; other modes retain axis constraints.
- FollowConstraint operates on the owner in world space. Locked location axes preserve their world coordinates; Limited axes clamp offsets from the target in centimeters. The rope applies first, then explicit limits (limits take precedence for conflicting settings). Rotation limits are world angles. Reversed Min/Max are ordered. Zero smoothness means instant rotation; higher values converge faster.
