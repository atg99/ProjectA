# Incident Report: The "Replication Was Off" Catastrophe

**Date:** 2025-11-20
**Subject:** Missing `bReplicates = true` in `ATGContainerComponent` and Actor
**Severity:** Critical (but embarrassing)

## Summary of the Incident
After extensive investigation into why `InitContainerItem` was not showing items on the client, involving deep dives into `FFastArraySerializer`, `MarkArrayDirty`, and `MarkItemDirty`, the root cause was identified as the most fundamental requirement for networking in Unreal Engine: **Replication was simply turned off.**

## The "Stupidity" Analysis
This error falls into the category of "Is the power cord plugged in?" mistakes. It is deceptively common yet devastatingly effective at wasting time because one assumes the basics are covered while debugging complex systems.

### Why it happened
1.  **Assumption of Competence:** We assumed the container Actor and Component were already set up for networking because the code contained `GetLifetimeReplicatedProps` and `DOREPLIFETIME`.
2.  **The Code Deception:** The presence of replication code (macros, functions) does *not* automatically enable replication. It only defines *how* to replicate *if* replication is active.
3.  **Constructor Blindness:** The constructor `UATGContainerComponent::UATGContainerComponent()` was missing the magic line:
    ```cpp
    SetIsReplicatedByDefault(true);
    ```

### Impact
-   **Time Lost:** Approximately 1 hour of debugging.
-   **Mental Anguish:** High.
-   **Code Churn:** Unnecessary debug logs added and removed.

## Corrective Actions
1.  **Immediate Fix:** Added `SetIsReplicatedByDefault(true);` to the component constructor.
2.  **Process Improvement:** When debugging "variable not updating on client", Step 0 should always be: "Is the Actor/Component actually replicating?"
    -   Check `IsReplicating()` on the Actor.
    -   Check `GetIsReplicated()` on the Component.
    -   Check the "Replicates" checkbox in Blueprint.

## Conclusion
While technically a "stupid" mistake, it is a rite of passage for every Unreal Engine network programmer. The system is now functioning correctly.

> [!TIP]
> Always check the "Replicates" checkbox first. Always.
