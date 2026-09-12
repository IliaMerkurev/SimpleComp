# Engineering rules

Read [PROJECT.md](PROJECT.md) for product requirements, [ARCHITECTURE.md](ARCHITECTURE.md) for behavior contracts, and [STATUS.md](STATUS.md) for the current validation boundary.

- Use [.agents/skills/simplecomp-rules/SKILL.md](.agents/skills/simplecomp-rules/SKILL.md) and [.agents/skills/unreal-cpp-developer/SKILL.md](.agents/skills/unreal-cpp-developer/SKILL.md). SimpleComp rules govern its cinematic API; generic skill defaults must not narrow user-facing editability. Legacy `.agent` entrypoints forward to these canonical files.
- Write code, tooltips, engineering documentation, issues, commits, and PRs in English. Owner-facing reports may be in Russian.
- Keep autonomous components and game-project independence. Preserve public names, Blueprint assets, Details access, and existing Local/World behavior. Internal caches, tickets, and timers are implementation details.
- Preserve existing local work. Inspect Git state before branch operations; do not reset, automatically stash, or commit unrelated files. Follow the owner's selected checkout and branch strategy.
- SimpleComp uses project-specific incremental versions: the accepted version is **0.1** and the pending implementation is **0.11**. Do not substitute semantic version numbering. Keep Unreal's numeric `Version` an integer. A branch or Draft PR does not accept a release; tags, releases, and merging require owner authorization.
- Validate according to the active task's authorization. The 0.11 implementation explicitly defers all compilation, editor execution, tests, and benchmarks to the owner. Source review cannot establish runtime correctness or measured performance.
- Explain invariants, mathematics, lifecycle decisions, and engine workarounds in comments when useful. Do not remove meaningful comments to satisfy a generic style rule.
