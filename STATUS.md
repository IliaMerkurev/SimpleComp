# SimpleComp 0.11 implementation status

Version **0.11** follows **0.1**. The owner reports basic functional checks passed and authorizes integration into master; production validation remains pending. Implementation branch `codex/0.11`, based on `master` at `19c66128d45a8f08a34177055b0b3bd10b936b1d`.

**Agent-run compilation and tests: Not run — explicitly deferred by owner.** No Unreal execution, UHT/UBT, automation, benchmark, or test harness was used. Evidence is source inspection, installed-engine header inspection, algorithm reasoning, and Git diff review only. Individual acceptance results have not been recorded; continue detailed owner validation using [MANUAL_ACCEPTANCE.md](MANUAL_ACCEPTANCE.md).

## Review coverage and disposition

All **32 files** under Source were read: Animation (6), Movement (8), Spawning (6), shared types/interfaces (4), runtime/editor module headers/implementations/build rules (6), and K2 node (2). README, architecture, descriptor, filter configuration, ignore rules, and both local skills were also reviewed. Coverage does not imply every file needed changes.

| Area | Confirmed findings addressed |
| --- | --- |
| Stack | Ticket/index confusion in pending fill; index-bound animation callbacks; wrong OnSlotFilled IDs; stale visibility callbacks; repeated fill after reuse/extraction; random-scale loss; destructive runtime resize; hidden destination transforms; failed-spawn resource loss; callback reentrancy and teardown |
| Curve playback | Duplicate pose/notify evaluation; inconsistent retiming; forward/reverse multi-loop boundaries; skipped zero markers; completion disabling chained playback; stale work after event-handler state changes; Local/World baselines; paused/stopped seeking |
| Async/K2 | Registered unused control proxies; accumulated subscriptions; unwanted activation rewind; incomplete teardown; Real pins without Float/Double subtype; duplicate/colliding notify pins; shared time outputs; sequence Details edits |
| Movement | Sphere world-axis/relative-quaternion mismatch; invalid rolling radius; Wheel initial orientation and stationary steering; FollowConstraint ignored location limits and zero-smoothness mismatch; Rotation static-target fallback and stale displacement |
| Collector/Spawner | Raw shape/flow edits not updating dependent state; initial overlap handling; same-collector duplicate reservations; destroyed flight cleanup; flow timer overlap; callback destruction/cancellation during spawn and message dispatch; invalid timing/weight handling |
| Modules/guidance | Unused runtime Slate dependencies; public BlueprintGraph dependency; Interp placement; ensure semantics; ownership/loading choices; Tick capability versus activation; hard-reference blanket prohibition; blanket logic-comment ban; correct skill discovery and separation of generic guidance from project rules |

The earlier suggestion to narrow user settings was **not adopted**: it conflicts with the cinematic authoring workflow. Hard references, broad editability, and explicit Stack/Collector cooperation are not defects. Default subobject registration was not treated as universally broken; Unreal can register actor-owned nested components through its lifecycle. Native lifecycle validation remains necessary.

## Compatibility and conservative decisions

- Public reflected names remain; `Explode` remains a deprecated wrapper. Additive APIs are `RefreshSettings` on Stack/Collector and `HasTicket` on Stack. Internal bookkeeping is not exposed as editable settings.
- OnSlotFilled now reports its promised ticket. Existing graphs that compensated for the old index bug need review. OnActorSpawned SlotID remains a physical index.
- Arrival now honors scale-animation behavior. Extraction/destinations use full per-element target scale. Random variation follows identity instead of physical index. Inertia uses component space and implicit spring integration, with TiltScale applied consistently.
- Resizing cannot discard occupied slots/reservations. Failed spawning retains resources and reports failure; a completed wave cycle can leave failed elements in place.
- Explicit seek evaluates pose without notifies or Finished. Endpoint notifies and completion chaining follow [ARCHITECTURE.md](ARCHITECTURE.md).
- K2 numeric pins now use Double subtypes. Existing Blueprint nodes may require reconstruction/recompilation; exact serialized-asset migration behavior is pending native validation. Runtime-only sequence overrides cannot add compiled notify outputs.
- FollowConstraint Limited location values now mean target-relative world-axis offsets. Explicit limits take precedence over incompatible rope settings. Zero smoothing follows the existing instant-turn tooltip.
- Raw idle settings use bounded synchronization (0.1 seconds) without an always-on expensive Tick. Spawner flow edits restart the active schedule from the edit; completed/stopped schedules do not restart merely because a property changed.
- The descriptor is normalized from UTF-16 to UTF-8 for readable version metadata. VersionName is exactly 0.11 and numeric Version is 2.

## Optimizations and deferred work

Expected benefits are **unmeasured**: one scale timer per Stack instead of one per element; average constant-time ticket lookup; one render-dirty notification per scale update batch; batched compaction transforms; single curve pose evaluation; no traversal loop when no notify listeners exist; no unused registered control proxies; cached settings fingerprint; squared movement threshold comparisons; removal of unused runtime UI dependencies.

BoundsScale = 10000 remains as the existing all-hidden HISM workaround until culling is validated. Further HISM changes, persistent curve/table caches (mutable Data Assets need invalidation), weighted-class caching (runtime edits need invalidation), and performance tuning need profiling/visual evidence. Notify dispatch is proportional to the number of crossed loops when listeners exist; very large but representable advances can be expensive.

Vector-curve tracks and CustomFloat output were advertised more broadly than implemented. Their public types remain compatible and tooltips now describe the limitation; adding those features is outside this iteration. Full arbitrary-time scene reconstruction and cross-collector global resource arbitration are also outside scope.

No product decision blocks the conservative implementation. Detailed owner review is still needed for non-destructive resize/failure behavior, seek/event boundaries, FollowConstraint offset semantics, randomization/arrival visuals, and flow-edit restart behavior. Alternative destructive rebuild, notify-on-seek, or retry policy requires a separate explicit decision.

## Tracking and handoff

Project: [SimpleComp](https://linear.app/ilia-merkurev/project/simplecomp-0dc908d97ca4), milestone **0.11**. Implementation areas: [ILI-46](https://linear.app/ilia-merkurev/issue/ILI-46), [ILI-47](https://linear.app/ilia-merkurev/issue/ILI-47), [ILI-48](https://linear.app/ilia-merkurev/issue/ILI-48), [ILI-49](https://linear.app/ilia-merkurev/issue/ILI-49), [ILI-50](https://linear.app/ilia-merkurev/issue/ILI-50), [ILI-51](https://linear.app/ilia-merkurev/issue/ILI-51). Linear remains the task tracker; implemented issues await review/native validation, not Done.

The original checkout was clean at preparation and had no local-only commits. Existing ignored build artifacts remain untouched. The work stays in the existing plugin directory on `codex/0.11`; no additional working copy was created. Delivery details and any available remote CI results are recorded in [PR #1](https://github.com/IliaMerkurev/SimpleComp/pull/1) and the final handoff. The owner has authorized merging after basic checks. No release, tag, or branch-protection change is authorized.

The current skills remain locally in `.agents/skills`, excluded from Git. Legacy `.agent` pointers are removed. A squash merge keeps skill-containing development commits out of master's new history; the development branch retains its existing history. Stable product and architecture rules remain version-controlled.
