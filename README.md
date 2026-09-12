# SimpleComp

**Reusable cinematic and gameplay components for Unreal Engine**

SimpleComp is an open-source C++ plugin containing simple, autonomous Actor Components for cinematics, motion design, self-playing scenes, gameplay, and rapid prototyping. It reduces the time spent rebuilding common logic in Blueprint while keeping that logic easy to configure and reuse.

## Philosophy

SimpleComp is a collection of components, not a framework. There are no mandatory actor base classes, global managers, or dependencies on a particular game project. Each component has a focused responsibility; components cooperate explicitly where useful, such as Stack and Collector.

Convenient authoring comes first. Friendly names, categories, and tooltips make components approachable in Details. User-facing settings remain broadly editable through Details and Blueprint (`EditAnywhere`, `BlueprintReadWrite`), with `Interp` on suitable properties for Sequencer. Cinematic needs are difficult to predict in advance, so access is preserved instead of restricting settings to a presumed gameplay workflow.

Movement and animation can be used independently of spawning. Animation sequences are reusable Data Assets, and existing Local/World behavior supports authoring under transformed parents.

## Components

| Component | Purpose |
| --- | --- |
| Rotation | Constant rotation, target-facing rotation, and orientation from velocity or displacement. |
| Sphere Roll | Visual rolling from movement, with a return-to-initial-pose option. |
| Wheel | Visual wheel rolling and steering. |
| Follow Constraint | Follow behavior with rope and per-axis location/rotation constraints. |
| Curve Animation | Transform animation from reusable curve sequence Data Assets, with playback controls and named notifies. |
| Stack | Instanced resource layouts with filling, reservations, deformation, extraction, and conversion into actors. |
| Collector | Overlap-based resource collection and flight handoff to a Stack. |
| Spawner | Weighted actor spawning, bursts, flow scheduling, messages, and physics launch. |

## Getting started

1. Place this repository in your Unreal project's `Plugins/SimpleComp` directory.
2. Build the plugin for your engine version and enable SimpleComp in the project's Plugins settings. The development environment for 0.11 is Unreal Engine 5.6; other engine versions have not been established as supported by this review.
3. Add the relevant component to an actor or Blueprint and configure its settings in Details. Use Blueprint controls and suitable Sequencer properties for your scene.
4. For curve animation, create an `SCAnimSequence` Data Asset, configure its tracks and notifies, and assign it to a Curve Animation component. Float curves and CurveTable X/Y/Z rows drive the implemented tracks; vector-curve evaluation and CustomFloat output are not implemented.
5. For collection, implement the collectable interface on resource actors. Their flight logic is responsible for confirming arrival or cancelling the assigned Stack reservation.

See [ARCHITECTURE.md](ARCHITECTURE.md) for class names, interfaces, coordinate spaces, and detailed behavior contracts.

## Version 0.11

0.11 follows 0.1 using SimpleComp's own incremental numbering. This update focuses on reliability and localized code optimization while preserving autonomous components and broad authoring access.

- **Stack:** stable tickets through reuse and compaction, safer concurrent arrivals and extraction, preserved random scale, non-destructive resizing, and resource retention when actor spawning fails.
- **Curve animation:** consistent retiming, forward/reverse loop and notify boundaries, safe completion chaining, and corrected Local/World transforms.
- **Blueprint controls:** improved async proxy and delegate cleanup, continuing playback without unwanted rewinds, and corrected K2 numeric/notify pins.
- **Movement:** corrected rolling under rotated parents, invalid-radius guards, preserved wheel orientation, and consistent Follow Constraint limits and zero smoothing.
- **Collector and Spawner:** runtime settings synchronization, safer cancellation, and protection against stale work after callbacks.

Stack scale animation now shares one timer, tickets use a lookup map, and redundant pose/render updates have been reduced. Expected benefits are unmeasured; no performance benchmark was run.

### Upgrading existing scenes

`OnSlotFilled` now returns the promised stable ticket; review graphs that compensated for its previous physical-index behavior. `OnActorSpawned.SlotID` remains a physical index. Existing animation nodes may need reconstruction and Blueprint recompilation after the numeric-pin corrections.

Explicit animation seeks update the pose without firing notifies or Finished. Follow Constraint Limited locations are target-relative offsets in world axes, and zero rotation smoothing is instant. Stack resize rejects capacities below current occupancy; failed conversion retains the resource. Idle raw Stack/Collector settings synchronize within 0.1 seconds, or immediately through `RefreshSettings`. See [STATUS.md](STATUS.md) for further compatibility notes and known limitations.

### Validation

The owner reports that basic functional checks passed and has authorized integration into `master`. Production use and the detailed acceptance scenarios remain pending. Agent-run compilation and tests: **Not run — explicitly deferred by owner.** This update does not claim production validation or measured performance gains.

The [manual acceptance checklist](MANUAL_ACCEPTANCE.md) covers scene checks and later dedicated validation. [PROJECT.md](PROJECT.md) records the product requirements.

## License

Licensed under the **MIT License**. Free for commercial use, modification, and distribution.
