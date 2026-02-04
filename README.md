# SimpleComp

**Simple Gameplay Components Toolkit for Unreal Engine**

SimpleComp is an open-source plugin for Unreal Engine featuring a collection of simple, universal, and reusable Actor Components designed for prototyping, self-playing scenes, cinematics, and real game projects.

## 🎯 Goals
- Reduce implementation time for common logic.
- Replace fragile Blueprint solutions with stable C++ components.
- Provide convenient tools for motion design and gameplay.
- Maintain maximum integration simplicity.
- **Deliver a Premium Experience**: Professional Editor names and full Sequencer compatibility.

## 🧩 Philosophy
SimpleComp is a collection of autonomous components, not a rigid framework.
- No mandatory base classes.
- No hidden dependencies.
- No global managers.
- Each component solves one task and works in isolation.
- **Zero-Coupling**: Systems (Animation, Movement, Spawning) are strictly decoupled for maximum modularity.

## 📌 Included Components

Detailed technical information, directory structure, and shared types are documented in [ARCHITECTURE.md](ARCHITECTURE.md).

## 🛠 Naming & Style
- All classes use the `SC` prefix (e.g., `USCRotationComponent`).
- PascalCase naming convention (no underscores in class names).
- Components use `/** Javadoc style */` comments for Editor tooltips.

## 📜 License
Licensed under the **MIT License**. Free for commercial use, modification, and distribution.
