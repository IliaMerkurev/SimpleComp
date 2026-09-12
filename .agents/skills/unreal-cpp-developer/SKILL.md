---
name: unreal-cpp-developer
description: Review and implement Unreal Engine C++ components, reflected APIs, UObject lifecycles, and runtime/editor module boundaries.
---

# Unreal C++ development

Apply repository instructions and product requirements before these generic defaults. Inspect the installed engine headers when a reflection, compiler, or lifecycle API is uncertain. Keep plugin code independent of game-project modules.

## Reflection and accessibility

Choose editability from the product workflow. `EditAnywhere` controls Details access; `BlueprintReadWrite` controls Blueprint access; `Interp` is a **UPROPERTY specifier**, not metadata. Supported Sequencer tracks still depend on the property type and engine integration.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "Animation")
float Speed = 1.0f;
```

Retain public API and serialized property names when possible. Use compatible wrappers or deprecation paths for necessary changes. User settings and internal runtime state need different exposure. Document units, coordinate spaces, defaults, limits, and side effects with Unreal tooltips.

A direct Blueprint/Sequencer property write need not call a setter or `PostEditChangeProperty`. Design cache invalidation and Tick activation around the actual write paths. Prefer explicit refresh hooks, existing active updates, or inexpensive bounded-frequency synchronization when setters alone cannot cover editable state.

## Ownership, loading, and lifetime

- Use reflected `TObjectPtr<T>` members for strong UObject references that should participate in GC. `TObjectPtr` alone, in an unreflected structure, is not a GC root.
- Use `TWeakObjectPtr<T>` for observation without keeping the object alive; revalidate before use.
- Use `TSoftObjectPtr`/`TSoftClassPtr` when deferred loading is required. Handle loading and failure explicitly. Hard references and `TSubclassOf` are appropriate for required, already-loaded assets or immediate spawning.
- Raw pointers and references are appropriate for non-owning local variables and parameters. Choose value versus `const&` according to size, semantics, and reflection requirements.
- Remove subscriptions and cancel timers at the relevant lifecycle boundaries. Async proxies should register only while they own an active operation and unregister on cancellation, replacement, completion, or destruction.
- Treat delegates, Blueprint events, actor spawning, and transform-induced overlaps as reentrant. Commit internal state before dispatch and revalidate identity/lifetime afterward. Do not retain container references across user callbacks.

Gameplay UObject and actor operations normally run on the game thread. Workers may process isolated data or use explicitly thread-safe engine APIs. Do not assume every UObject operation or GC implementation is single-threaded; follow the specific API's contract. Validate object lifetime when dispatching worker results back.

## Tick, safety, and math

`bCanEverTick` declares capability. Use `bStartWithTickEnabled = false` and `SetComponentTickEnabled` to suspend a component that may tick later. Setting capability false is appropriate only when no Tick is needed. Timers also have scheduling and lifetime costs; do not replace many simple updates with many independent timers without a reason.

`ensure` is a non-fatal diagnostic: the expression is still evaluated in Shipping, while reporting depends on build configuration. It does not replace control flow:

```cpp
if (!ensure(IsValid(Target)))
{
    return;
}
```

`check` is generally disabled in Shipping unless configured otherwise. Do not rely on assertion side effects. Guard divisors and finite values at runtime; editor clamp metadata does not validate arbitrary C++/Blueprint writes. State coordinate spaces explicitly, and compose transforms in Unreal's order.

## Maintainability and validation

Use Unreal containers and naming, IWYU, generated headers last, and correct public/private module dependencies. Keep runtime code free of editor/compiler dependencies. Reserve storage when useful, avoid duplicate evaluation and unnecessary transform writes, and cache only with a clear invalidation policy. Do not claim a performance bottleneck or improvement is measured without profiling evidence.

Use braces for control flow and keep changes locally readable. Comments explaining invariants, mathematics, engine workarounds, ownership, or design decisions are welcome. Avoid comments that merely repeat the code. Constructors establish defaults and default subobjects; world-dependent work belongs in lifecycle hooks.

Choose validation appropriate to the task and obey explicit execution restrictions. Record deferred compilation, native editor checks, and runtime validation honestly; source reasoning alone does not verify them.
