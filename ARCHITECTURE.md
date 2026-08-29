# Architecture: SimpleComp Toolkit

## 🧠 Philosophy
**SimpleComp** is a collection of autonomous, reusable Actor Components.
- **Not a Framework**: No mandatory base classes, no global managers, no hidden dependencies.
- **Atomic**: Each component solves exactly one problem.
- **Standalone**: Components must work in isolation.
- **Cross-System Isolation**: Functional systems (Movement, Animation, Spawning) remain strictly decoupled.

## 🧱 Core Principles
1.  **Professional Presence**: Components look like first-class Unreal features via `DisplayName` metadata.
2.  **Cinematic-Ready**: High-level support for `Sequencer` via `interp` specifiers.
3.  **High Flexibility**: Default to `EditAnywhere` and `BlueprintReadWrite` for ease of use.
4.  **Runtime-Safe**: Use `ensure()` for critical owner validation and `BlueprintPure` for state queries.

## 🛠 Naming Convention (Strict)
- **Class Prefix**: `SC` (e.g., `USCRotationComponent`).
- **File Names**: Match class names (e.g., `SCRotationComponent.h`).

## 🧱 Folder Structure
- `.../Public/Core/`: Shared types and base definitions (`SCTypes.h`).
- `.../Public/Components/Movement/`: Translation and rotation logic.
- `.../Public/Components/Spawning/`: Actor lifecycle and spawning logic.
- `.../Public/Components/Animation/`: Technical curve-based animation system.

## � Component & Class Registry

### 🏃 Movement System (`.../Components/Movement/`)
| Class Name | File | Purpose | Responsibilities |
| :--- | :--- | :--- | :--- |
| `USCRotationComponent` | `SCRotationComponent.h` | Parametric Rotation | Applies procedural rotation in world/local space. |
| `USCSphereRollComponent` | `SCSphereRollComponent.h` | Rolling Physics | Visual-only sphere rolling based on movement delta. |
| `USCWheelComponent` | `SCWheelComponent.h` | Wheel Logic | Base logic for individual wheel behavior. |
| `USCFollowConstraint` | `SCFollowConstraint.h` | Relative Distance | Maintains distance to target with smoothed tracking. |

### 🎭 Animation System (`.../Components/Animation/`)
| Class Name | File | Purpose | Responsibilities |
| :--- | :--- | :--- | :--- |
| `USCCurveAnimComponent` | `SCCurveAnimComponent.h` | Runtime Engine | Evaluates curve sequences and triggers notifies. |
| `USCAnimSequence` | `SCAnimSequence.h` | Data Asset | Stores curve tracks (Scale/Rot/Pos) and notify markers. |
| `USCAnimAsyncAction` | `SCAnimAsyncAction.h` | Logic Proxy | Manages K2Node state and delegate routing. |
| `UK2Node_PlaySCAnimation`| `UK2Node_PlaySCAnimation.h`| BP Node | Compiler-time node for dynamic notify execution pins. |

### 🏗 Spawning System (`.../Components/Spawning/`)
| Class Name | File | Purpose | Responsibilities |
| :--- | :--- | :--- | :--- |
| `USCSpawnerComponent` | `SCSpawnerComponent.h` | Universal Spawner | Handles actor lifecycle, quantity control, and offsets. |
| `USCStackComponent` | `SCStackComponent.h` | Resource Stack | HISM-based grid stack with slot reservation state machine (`Free → Reserved → Filled`), editor viewport preview toggle (`bShowPreview`), timer-driven scale-in animation, and `Explode()` burst. |
| `USCCollectorComponent` | `SCCollectorComponent.h` | Resource Collector | Dynamically spawned Sphere/Box trigger that detects `ISCCollectableInterface` Actors, requests a stack slot, disables resource collision, and calls `InitFlight`. |

### 🔌 Interfaces (`.../Core/Interfaces/`)
| Interface | File | Purpose | Methods |
| :--- | :--- | :--- | :--- |
| `ISCMessageInterface` | `SCMessageInterface.h` | Universal Messaging | `OnReceiveSCMessage(Payload)` — generic event-driven messaging between components. |
| `ISCCollectableInterface` | `SCCollectableInterface.h` | Collection Protocol | `InitFlight(TargetStack, SlotID)` — implemented by resource Actors. Triggers flight logic and must call `TargetStack->ConfirmArrival(SlotID)` upon arrival. |

## 🧪 Implementation Checklist
- [ ] `UCLASS` has `meta = (DisplayName = "Friendly Name")`.
- [ ] Driveable properties use `interp` and `BlueprintReadWrite`.
- [ ] Categories follow `SimpleComp|[Feature]` format.
- [ ] Engineering Safety: Use `ensure()` for critical owner validation. Mark getters as `BlueprintPure`.
- [ ] Modern C++: Use `TObjectPtr` for UObject pointers and `const&` for complex parameters.
- [ ] Clean Code: Implementation bodies must be free of logic comments; rely on code clarity and tooltips.
