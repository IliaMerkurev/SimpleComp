---
name: simplecomp-rules
description: Apply SimpleComp's cinematic-first API, autonomous-component architecture, and stable ticket contracts when reviewing or changing this plugin.
---

# SimpleComp project rules

Read [AGENTS.md](../../../AGENTS.md), [PROJECT.md](../../../PROJECT.md), and [ARCHITECTURE.md](../../../ARCHITECTURE.md). Use the [generic Unreal skill](../unreal-cpp-developer/SKILL.md) for engine mechanics. **These project-specific rules take precedence over generic skill defaults for SimpleComp's API philosophy.** Explicit owner instructions remain authoritative.

## Cinematics and accessible settings

SimpleComp serves cinematics, motion design, self-playing scenes, gameplay, and prototyping. Convenient Details, Blueprint, and Sequencer access is a product requirement.

- Default user-facing configuration to `EditAnywhere` and `BlueprintReadWrite`. When it is unclear whether a parameter will be animated, preserve access; do not predict the owner's workflow by restricting it.
- Add the `Interp` **UPROPERTY specifier** to suitable settings intended for Sequencer, and verify supported track types later in native validation. Do not put `Interp` inside `meta` or equate the specifier with complete Sequencer support.
- Handle supported runtime edits from all write paths. Keep settings editable even when cache/geometry invalidation needs implementation work. Document synchronization latency and provide immediate refresh where it matters.
- Physical indices, tickets, caches, timers, and playback bookkeeping are internal state, not user settings. Read-only query outputs are appropriate.
- Preserve friendly component names, `SimpleComp|Feature` categories, and useful tooltips. Preserve public names and serialized fields; document unavoidable behavior changes.

## Architecture and contracts

Keep autonomous reusable components without mandatory actor base classes or global managers. The plugin must not depend on any game project. Runtime `SimpleComp` and compiler/editor `SimpleCompEditor` remain separate; explicit Stack/Collector cooperation within Spawning is expected. Use interfaces and delegates at external integration boundaries.

Preserve existing Local/World functionality and account for transformed parents. Do not add a space selector to every component where its behavior already has a specific coordinate contract.

For Stack, a stable TicketID identifies a resource/reservation throughout its lifetime; a physical slot/HISM index can change. Queues, animations, and delayed spawn work must retain tickets and resolve indices when executed. Removal must invalidate the old identity before events. Resizing and failed conversion must preserve resources and reservations unless the owner explicitly authorizes a different contract.

For curve animation, playback seconds and sequence sample seconds are distinct. Evaluate the pose once, then dispatch traversed events with reentrancy checks. Keep reusable sequence Data Assets independent of any playback instance. Consult the architecture document for endpoint, seek, and completion contracts.

## Documentation and scope

Keep architecture accurate when classes or contracts change. Comments explaining invariants, math, workarounds, and rationale are allowed. Avoid unrelated frameworks, components, dependency additions, or speculative optimizations.

Use the owner's version numbering exactly. The accepted version and pending implementation are different states; branch preparation is not release acceptance. Validation, release actions, and delivery follow the current task's explicit authorization.
