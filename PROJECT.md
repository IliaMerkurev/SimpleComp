# SimpleComp product requirements

Repository: https://github.com/IliaMerkurev/SimpleComp

SimpleComp is an open-source Unreal Engine C++ plugin for cinematics, motion design, self-playing scenes, gameplay, and rapid prototyping. Its primary workflow is convenient authoring through Details, Blueprint, and Sequencer.

## Established requirements

- Reusable autonomous components; no mandatory actor base classes or global manager.
- Independence from any game project. Runtime logic remains separate from compiler/editor code.
- Broadly editable user settings by default (`EditAnywhere`, `BlueprintReadWrite`), with `Interp` for suitable animated properties. Uncertainty about future authoring needs is a reason to preserve access.
- Friendly component names, categories, and tooltips; compatible public APIs and reusable animation Data Assets.
- Existing spatial behavior must remain meaningful under transformed parents.

## Pending 0.11 scope

The accepted product is **0.1**. **0.11** is a pending implementation on `codex/0.11`, based on `master`. Unreal descriptor metadata is `VersionName: "0.11"`, integer `Version: 2`.

This iteration reviews all source and relevant descriptors, fixes confirmed identity/lifecycle/timing/spatial defects, and makes localized source-supported optimizations. It covers Stack, curve playback, async/K2 controls, movement, collection, spawning, and engineering guidance. It does not introduce components, modules, frameworks, dependencies, scene reconstruction at arbitrary Sequencer times, or a test harness.

Implementation takes place in the existing plugin checkout. No additional clone, worktree, Unreal project, or parallel plugin installation is required. Delivery is a Draft PR against `master`; the checkout remains on `codex/0.11` for owner validation.

## Validation and acceptance

**Not run — explicitly deferred by owner.** No local compilation, UHT/UBT, Unreal execution, tests, cooking/packaging, or benchmarks are authorized for this implementation. Native Blueprint compiler behavior, runtime behavior, rendering, and performance remain unverified. Do not disable normal CI or manually trigger additional workflows.

Use [MANUAL_ACCEPTANCE.md](MANUAL_ACCEPTANCE.md) later in the owner's Unreal project. [STATUS.md](STATUS.md) records findings and limitations; [Linear](https://linear.app/ilia-merkurev/project/simplecomp-0dc908d97ca4), milestone **0.11**, is the implementation/review tracker. Release acceptance remains the owner's decision.
