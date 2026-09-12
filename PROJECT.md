# SimpleComp product requirements

Repository: https://github.com/IliaMerkurev/SimpleComp

SimpleComp is an open-source Unreal Engine C++ plugin for cinematics, motion design, self-playing scenes, gameplay, and rapid prototyping. Its primary workflow is convenient authoring through Details, Blueprint, and Sequencer.

## Established requirements

- Reusable autonomous components; no mandatory actor base classes or global manager.
- Independence from any game project. Runtime logic remains separate from compiler/editor code.
- Broadly editable user settings by default (`EditAnywhere`, `BlueprintReadWrite`), with `Interp` for suitable animated properties. Uncertainty about future authoring needs is a reason to preserve access.
- Friendly component names, categories, and tooltips; compatible public APIs and reusable animation Data Assets.
- Existing spatial behavior must remain meaningful under transformed parents.

## 0.11 scope

**0.11** follows **0.1**. The owner reports basic functional checks passed and authorizes integration from `codex/0.11` into `master`; production validation remains pending. Unreal descriptor metadata is `VersionName: "0.11"`, integer `Version: 2`.

This iteration reviews all source and relevant descriptors, fixes confirmed identity/lifecycle/timing/spatial defects, and makes localized source-supported optimizations. It covers Stack, curve playback, async/K2 controls, movement, collection, spawning, and engineering guidance. It does not introduce components, modules, frameworks, dependencies, scene reconstruction at arbitrary Sequencer times, or a test harness.

Implementation takes place in the existing plugin checkout. No additional clone, worktree, Unreal project, or parallel plugin installation is required. Delivery is owner-authorized integration through PR #1 against `master`; the checkout remains on `codex/0.11` for owner validation.

## Validation and acceptance

**Agent-run compilation and tests: Not run — explicitly deferred by owner.** No local compilation, UHT/UBT, Unreal execution, tests, cooking/packaging, or benchmarks are authorized for this implementation. The owner has reported basic functional checks, without individual acceptance results. Native compatibility, edge cases, rendering, and performance still require detailed validation. Do not disable normal CI or manually trigger additional workflows.

Use [MANUAL_ACCEPTANCE.md](MANUAL_ACCEPTANCE.md) later in the owner's Unreal project. [STATUS.md](STATUS.md) records findings and limitations; [Linear](https://linear.app/ilia-merkurev/project/simplecomp-0dc908d97ca4), milestone **0.11**, is the implementation/review tracker. No tag or GitHub release is authorized by this integration. Skills remain local ignored tooling; product requirements and behavior contracts remain version-controlled.
