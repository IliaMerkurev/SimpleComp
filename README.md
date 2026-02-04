# SimpleComp

**Simple Gameplay Components Toolkit for Unreal Engine**

SimpleComp is an open-source plugin for Unreal Engine featuring a collection of simple, universal, and reusable Actor Components designed for prototyping, self-playing scenes, cinematics, and real game projects.

## 🎯 Goals
- Reduce implementation time for common logic.
- Replace fragile Blueprint solutions with stable C++ components.
- Provide convenient tools for motion design and gameplay.
- Maintain maximum integration simplicity.

## 🧩 Philosophy
SimpleComp is a collection of autonomous components, not a rigid framework.
- No mandatory base classes.
- No hidden dependencies.
- No global managers.
- Each component solves one task and works in isolation.

## 📌 Included Components

### Movement Components
- **SCRotationComponent**: Parametric rotation for actors or meshes.
- **SCFollowConstraintComponent**: Constrains actor distance to a target with smoothed rotation.
- **SCSphereRollComponent**: Sphere rolling based on delta location (ideal for motion design without complex physics).
- **SCWheelComponent**: Base component for wheels and transport movement.

### Spawning Components
- **SCSpawnerComponent**: Universal actor spawner with transform control.

---

Detailed technical information, directory structure, and shared types are documented in [ARCHITECTURE.md](ARCHITECTURE.md).

## 🛠 Naming & Style
- All classes use the `SC` prefix (e.g., `USCRotationComponent`).
- PascalCase naming convention (no underscores in class names).
- Components use `/** Javadoc style */` comments for Editor tooltips.

## 📜 License
Licensed under the **MIT License**. Free for commercial use, modification, and distribution.
