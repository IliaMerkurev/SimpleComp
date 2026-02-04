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
    - `.../Public/Core/`: Shared types and base enums.
    - `.../Public/Components/Movement/`: Movement-related logic.
    - `.../Public/Components/Spawning/`: Spawning-related logic.
    - `.../Public/Components/Animation/`: Animation and curve-based logic.

## 💎 Core Types
To ensure consistency across the toolkit, shared data structures are centralized in `SCTypes.h`.

| Type | Nature | Description |
| :--- | :--- | :--- |
| `ESCAxisMode` | Enum | Standardizes per-axis behavior: `Free`, `Limited`, or `Locked`. |
| `ESCRotationMode` | Enum | Logic for rotation calculation (Target, Velocity, Forward Delta, etc). |
| `FSCAxisSettings` | Struct | Universal configuration for a single axis (Mode + Min/Max range). |

## 📦 Component Registry (Indexed)
| Class Name | Type | Purpose |
| :--- | :--- | :--- |
| `USCRotationComponent` | Movement | Rotates actors/meshes parametrically. |
| `USCSphereRollComponent` | Movement | Rolls spheres based on delta location (Visual-only physics). |
| `USCWheelComponent` | Movement | Wheel logic (Base for vehicles/carts). |
| `USCFollowConstraintComponent` | Movement | Constrains actor distance to a target with smoothed rotation. |
| `USCSpawnerComponent` | Spawning | Universal actor spawner with transform control. |
| `USCCurveAnimComponent` | Animation | Plays curve-based animations with notifies. |

## 🎭 Animation System
The animation system uses a hybrid approach to provide professional Blueprint usability:
- **USCAnimSequence**: Data Assets containing Float/Vector curve tracks and event notifies.
- **USCCurveAnimComponent**: The runtime engine that evaluates curves and broadcasts events.
- **UK2Node_PlaySCAnimation**: A professional-grade Blueprint node that:
    - Automatically discovers notifies in the assigned sequence.
    - Dynamically generates execution pins for each notify.
    - Uses an internal `USCAnimAsyncAction` proxy to handle asynchronous state and multi-pin output.

## 🧪 Implementation Guidelines
- **Category**: All components must appear under `Components` -> `SimpleComp`.
- **Tooltips**: All properties must have `/** Javadoc style */` comments.
- **Construction**: Avoid heavy logic in constructors. Use `BeginPlay`.
