# SimpleComp

**Reusable cinematic and gameplay components for Unreal Engine**

Accepted version: **0.1**. This branch contains the pending **0.11** implementation, awaiting owner validation; it is not an accepted release.

Start with [PROJECT.md](PROJECT.md), [ARCHITECTURE.md](ARCHITECTURE.md), and the [0.11 status](STATUS.md). Compilation and tests: **Not run — explicitly deferred by owner.** The [manual acceptance checklist](MANUAL_ACCEPTANCE.md) is ready for later Unreal validation.

SimpleComp is an open-source plugin for Unreal Engine featuring a collection of simple, universal, and reusable Actor Components designed for prototyping, self-playing scenes, cinematics, and real game projects.

## 🎯 Goals
- Reduce implementation time for common logic.
- Replace fragile Blueprint solutions with stable C++ components.
- Provide convenient tools for motion design and gameplay.
- Maintain maximum integration simplicity.
- **Deliver a Premium Experience**: Friendly Editor names, broad editable settings, and appropriate Sequencer support. Native validation remains necessary.

## 🧩 Philosophy
SimpleComp is a collection of autonomous components, not a rigid framework.
- No mandatory base classes.
- No hidden dependencies.
- No global managers.
- Each component solves one task and works in isolation.
- Movement and Animation do not depend on Spawning. Stack and Collector cooperate explicitly through stable tickets and the collection interface.
- Default user settings remain `EditAnywhere` and `BlueprintReadWrite`; suitable animated properties use the `Interp` UPROPERTY specifier.

## 📌 Included Components

Detailed technical information, directory structure, and shared types are documented in [ARCHITECTURE.md](ARCHITECTURE.md).

## 🛠 Naming & Style
- All classes use the `SC` prefix (e.g., `USCRotationComponent`).
- PascalCase naming convention (no underscores in class names).
- Components use `/** Javadoc style */` comments for Editor tooltips.

## 📜 License
Licensed under the **MIT License**. Free for commercial use, modification, and distribution.
