# Architecture: SimpleComp Toolkit

## 🧠 Philosophy
**SimpleComp** is a collection of autonomous, reusable Actor Components.
- **Not a Framework**: No mandatory base classes, no global managers, no hidden dependencies.
- **Atomic**: Each component solves exactly one problem.
- **Standalone**: Components must work in isolation.

## 🧱 Core Principles
1.  **Runtime-Friendly**: Logic should work in Game, Editor, and Sequencer (Cinematics).
2.  **Blueprint-Accessible**: All logic must be exposed to Blueprints.
3.  **No Coupled Dependencies**: Component A should not require Component B to function.

## 🛠 Naming Convention (Strict)
- **Class Prefix**: `SC` (e.g., `USCRotationComponent`).
- **File Names**: Match class names (e.g., `SCRotationComponent.h`).
- **Folder Structure**:
    - `Source/SimpleComp/Public/Components/Movement/`
    - `Source/SimpleComp/Public/Components/Spawning/`

## 📦 Component Registry (Indexed)
| Class Name | Type | Purpose |
| :--- | :--- | :--- |
| `USCRotationComponent` | Movement | Rotates actors/meshes parametrically. |
| `USCSphereRollComponent` | Movement | Rolls spheres based on delta location (Visual-only physics). |
| `USCWheelComponent` | Movement | Wheel logic (Base for vehicles/carts). |
| `USCSpawnerComponent` | Spawning | Universal actor spawner with transform control. |

## 🧪 Implementation Guidelines
- **Category**: All components must appear under `Components` -> `SimpleComp`.
- **Tooltips**: All properties must have `/** Javadoc style */` comments.
- **Construction**: Avoid heavy logic in constructors. Use `BeginPlay`.
