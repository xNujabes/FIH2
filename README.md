# FIH2 — Technical Documentation & Project Dossier

---

# 1. Short Technical Documentation

## Overall Project Architecture

**FIH2** is an interactive 3D aquatic sandbox prototype developed in Unreal Engine 5. The project is built around an aquarium ecosystem where players spawn, manipulate, and observe physical actors and autonomous aquatic life in real time. 

The architecture follows a hybrid design combining high-performance native C++ for core gameplay mechanics, AI pathing, physics handling, and interface contracts, alongside Unreal Engine Blueprints and UMG for UI presentation, visual configuration, and Behavior Tree asset orchestration.

```
+-------------------------------------------------------------------------------+
|                               GameModeSandbox                                 |
+---------------------------------------+---------------------------------------+
                                        |
        +-------------------------------+-------------------------------+
        |                                                               |
        v                                                               v
+-----------------------+                               +-----------------------+
|   AMouseController    |                               |    AFishController    |
| (BP_MouseController)  |                               |   (C_FishController)  |
+-----------+-----------+                               +-----------+-----------+
            |                                                       |
            | Uses UPhysicsHandleComponent                          | Runs Behavior Tree
            | & Deprojection Raycasts                               | (BT_Fish / BB_Fish)
            v                                                       v
+-----------------------+       Implements Interfaces   +-----------------------+
|  AInanimate / AFood   |<==============================|         AFish         |
| (PhysicsProxy / Mesh) |   IInteractable / IEatable    | (UFishSteeringMovement|
+-----------------------+                               +-----------------------+
```

### Core Architecture Breakdown:
*   **Game Framework Foundation:** The experience is driven by `GameModeSandbox` (derived from `AGameModeBase`), configured as the primary startup game mode in `DefaultEngine.ini` (`/Game/System/GameModeSandbox.GameModeSandbox_C`). The primary level is `Main.umap`.
*   **Player Input & Perspective:** Player interaction is managed by `AMouseController` (and its Blueprint child `BP_MouseController`), derived from `APlayerController`. Unlike standard character controllers, `AMouseController` directly leverages Unreal Engine's **Enhanced Input System** to orchestrate screen-to-world deprojection, mouse raycasting, and physical object manipulation using a 6-DOF physics handle.
*   **Autonomous Agent Subsystem:** AI agents (`AFish`) are controlled by `AFishController` (derived from `AAIController`). Fish operate in full 3D volumetric space without standard 2D ground NavMesh constraints, navigating via custom 3D wander nodes and whisker-based steering physics components.
*   **Data-Driven Spawning:** Spawning interactive elements is decoupled through Unreal Data Tables (`DT_EntitySpawnable`) and structures (`ST_Entities`), driven by runtime UMG widgets (`WBP_SpawnEntities`, `WBP_SpawnButton`).

---

## Main Classes / Blueprints / Components

### 1. Controllers & Core Gameplay
*   **`AMouseController` / `BP_MouseController` (C++ / Blueprint):**
    *   Subclasses `APlayerController`.
    *   Configures viewport cursor states (`bShowMouseCursor = true`, `bEnableClickEvents = true`, `bEnableMouseOverEvents = true`).
    *   Hosts a `UPhysicsHandleComponent` for grabbing and translating physics primitives.
    *   Binds Enhanced Input actions (`ClickAction`, `01_InputMappingContext`) to handle mouse click, dragging, and release cycles via multi-channel raycasts.
*   **`AFishController` / `C_FishController` (C++ / Blueprint):**
    *   Subclasses `AAIController`.
    *   Manages AI lifecycle and perception via `UAIPerceptionComponent` with an active `UAISenseConfig_Sight` sense (1500 unit sight radius, 180° peripheral angle).
    *   Initializes the blackboard (`BB_Fish`) and behavior tree (`BT_Fish`), managing state switches between idling, food searching, and target tracking.

### 2. Pawns & Actors
*   **`AFish` / `BP_Fish` (C++ / Blueprint):**
    *   Subclasses `APawn` and implements the `IInteractable` interface.
    *   Rooted by a `UStaticMeshComponent` simulating physics and a child `USphereComponent` ("Eat Sphere Collision") for proximity feeding.
    *   Maintains internal metabolic states (`Hunger` clamped between `[0, 3]`, `bIsStimulated`) and notifies dependent systems via the dynamic multicast delegate `FOnHungerChangedDelegate`.
*   **`AInanimate` (C++ Base Class):**
    *   Abstract base class deriving from `AActor` and implementing `IInteractable`.
    *   Provides standardized static mesh integration (`UStaticMeshComponent`) and interaction event foundations for non-living sandbox entities.
*   **`AFood` / `BP_Food` (C++ / Blueprint):**
    *   Subclasses `AInanimate` and implements `IBreakable` and `IEatable`.
    *   Represents consumable entities for fish agents. Overrides `TriggerEat_Implementation` to increment fish hunger and remove itself from the world.
    *   Contains a custom `UBoxComponent` (`FoodPhysicsProxy`) engineered as a physics handle attachment proxy.
*   **`ASand` / `BP_Sand` (C++ / Blueprint):**
    *   Subclasses `AInanimate`. Implements physics simulation on its root mesh to provide physical decor within the aquatic sandbox.
*   **`Tank` (`Tank.uasset`):**
    *   Static geometry and collision volume representing the glass aquarium. Contains the physical and perceptual bounds for water confinement and AI path clamping.
*   **`BP_ActorDelete`:**
    *   Utility trigger actor placed in the sandbox level to recycle and cleanly destroy dragged objects.

### 3. Custom AI & Movement Components
*   **`UFishSteeringMovement` (C++ Component):**
    *   Custom movement component deriving from `UFloatingPawnMovement`.
    *   Executes obstacle avoidance and fluid orientation in 3D space by sampling raycast "whiskers" against `ECC_WorldStatic` geometry.
*   **`UBTT_FindRandomLocation` (C++ Behavior Tree Task):**
    *   Custom `UBTTaskNode` calculating safe 3D search points around the pawn, bounded within the aquarium volume defined by `WaterTankBoxCollisionActor`.
*   **`UBTT_SwimToward` (C++ Behavior Tree Task):**
    *   Latent `UBTTaskNode` with per-instance memory (`FSwimTaskMemory`). Drives pawn movement towards dynamic targets (food actors) or static locations using `UFishSteeringMovement`.

### 4. Interfaces
*   **`IInteractable` (`UInteractable`):** Declares `OnHover`, `OnGrab`, `OnDropped`, and `OnThrown(FVector ThrowVelocity)`.
*   **`IBreakable` (`UBreakable`):** Declares `TriggerBreak(AActor* BreakInstigator, FVector HitLocation, float DamageAmount)`.
*   **`IEatable` (`UEatable`):** Declares `TriggerEat(AActor* EatInstigator, FVector HitLocation, float DamageAmount)`.

---

## Interaction System

The interaction loop is designed around mouse-driven direct physics manipulation, allowing players to click, grab, drag, and release sandbox entities anywhere in the tank.

```
[Player Left Mouse Click]
         │
         ▼
[AMouseController::OnMouseClick()]
         │
         ├─► DeprojectMousePositionToWorld()
         ├─► LineTraceMultiByChannel(ECC_Visibility)
         │
         ▼
[Hit Actor Implements UInteractable?]
         │
         ├─── YES ───► IInteractable::Execute_OnGrab(HitActor)
         │             RootComp = Cast<UPrimitiveComponent>(HitActor->RootComponent)
         │             PhysicsHandle->GrabComponentAtLocation(RootComp, NAME_None, ImpactPoint)
         │             RootComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap)
         │             bIsDragging = true
         │
         └─── NO  ───► Ignore / Log Warning
```

```
[Tick (When bIsDragging == true)]
         │
         ▼
[AMouseController::Tick()]
         │
         ├─► DeprojectMousePositionToWorld()
         ├─► LineTraceMultiByChannel(ECC_Visibility)
         │
         ▼
[PhysicsHandle->SetTargetLocation(TargetPoint)]
```

### Technical Workflow:
1.  **Cursor Deprojection & Multi-Trace:** When `ClickAction` triggers (`OnMouseClick`), the controller deprojects the 2D cursor coordinate to a 3D world origin and direction vector (`DeprojectMousePositionToWorld`). A multi-hit raycast (`LineTraceMultiByChannel`) is fired up to `TraceRange` on `ECC_Visibility`.
2.  **Interface Validation:** The controller iterates over hit results and inspects target actors using Unreal’s interface query `HitActor->Implements<UInteractable>()`.
3.  **Physics Handle Attachment:** Once validated:
    *   The `IInteractable::Execute_OnGrab` interface notification is broadcast to the actor.
    *   The target's root `UPrimitiveComponent` is retrieved.
    *   `PhysicsHandle->GrabComponentAtLocation(RootComp, NAME_None, HitResult.ImpactPoint)` binds the physical actor to the handle.
    *   The actor’s collision response against `ECC_WorldStatic` is temporarily adjusted to `ECR_Overlap` to prevent harsh snagging on aquarium edges while moving.
4.  **Continuous Plane Following:** During `Tick()`, if dragging is active, subsequent cursor deprojections calculate the updated target location in 3D space, calling `PhysicsHandle->SetTargetLocation(...)` to achieve smooth physical interpolation.
5.  **Release & Physics Handoff:** On `OnMouseRelease`, `PhysicsHandle->ReleaseComponent()` is invoked, freeing the body back to full autonomous physics simulation (gravity, buoyancy, and velocity).


---

## Physics Systems

The sandbox leverages a combination of Unreal Engine 5's **Chaos Physics**, physical constraints via **Physics Handles**, and custom kinematic-steering equations for aquatic navigation.

```
                    +------------------------------------+
                    |        Physics Architecture        |
                    +-----------------+------------------+
                                      |
         +----------------------------+----------------------------+
         |                                                         |
         v                                                         v
+-----------------------------+                           +-----------------------------+
|    Rigid Body Simulation    |                           | Custom Fluid AI Steering    |
|       (Chaos Physics)       |                           |  (Raycast Whisker Sensor)   |
+--------------+--------------+                           +--------------+--------------+
               |                                                         |
  +------------+------------+                               +------------+------------+
  |                         |                               |                         |
  v                         v                               v                         v
Food / Sand / Fish       Physics Handle               Forward Obstacle          8-Directional
Primitive Colliders      Kinematic Drag               Detection Trace           Whisker Sampling
(PhysicsActor profile)   & Damping Springs            (300 unit range)          (+/-45°, +/-90°)
```

### 1. Physics Handle Spring Dynamics
Interactive manipulation does not use hard positional teleportation. Instead, `UPhysicsHandleComponent` applies physical linear and angular spring-damper forces to the target body. This allows dragged objects (such as `BP_Food` or `BP_Sand`) to naturally collide with other bodies, slide across geometry, and preserve momentum.

### 2. Physics Proxy Architecture for Chaos Compatibility
Unreal Engine's Chaos Destruction assets (`GeometryCollectionComponent`, e.g., `GC_Food`) present technical constraints when directly attached to standard `UPhysicsHandleComponent` instances in C++. 
To circumvent this, `AFood` implements a **Physics Proxy Pattern**:
*   A dedicated `UBoxComponent` (`FoodPhysicsProxy`) is initialized as the root component with `SetSimulatePhysics(true)` and assigned the collision profile `PhysicsActor`.
*   The proxy acts as the physical surrogate receiving linear physics forces, drag inputs, and collision events, allowing clean detachment, destruction, or mesh swaps when the `IBreakable` or `IEatable` interfaces fire.

### 3. Autonomous 3D Aquatic Steering & Whisker Avoidance
Because aquatic life moves freely through 3D fluid volumes, standard 2D NavMeshes are insufficient. `UFishSteeringMovement` bypasses standard ground walking logic and implements custom kinematic fluid movement:
*   **Forward Sensing:** In each tick, a raycast traces `AvoidanceDistance` (300 units) ahead along the pawn's forward vector.
*   **Multi-Whisker Sensor Array:** When an obstacle is detected, the component queries an array of 8 directional whisker offsets:
    $$\text{Offsets} = \{(\pm 45^\circ, \pm 45^\circ, 0^\circ), (0^\circ, \pm 90^\circ, 0^\circ), (\pm 90^\circ, 0^\circ, 0^\circ)\}$$
*   **Deflection & Interpolation:** The first unobstructed whisker angle is selected as the evasion vector. If all whiskers are occluded, the surface normal of the impact point (`Hit.Normal`) is adopted. The pawn's rotation is then smoothly updated using spherical rotation interpolation (`FMath::RInterpTo`).

---

## Main Technical Choices

### 1. Interface-Driven Architecture (`UInterface`)
*   **Decision:** Rather than casting directly to concrete classes (`AFood`, `AFish`, `ASand`), interaction is decoupled via `IInteractable`, `IBreakable`, and `IEatable`.
*   **Rationale:** Eliminates cyclic compilation dependencies between the player controller, game mode, AI systems, and environment actors. Any new object added to the sandbox can immediately become draggable, destructible, or consumable simply by implementing the corresponding interface.

### 2. Native C++ AI Steering vs. 3D Navigation Plugins
*   **Decision:** Custom `UBTTaskNode` tasks (`UBTT_SwimToward`, `UBTT_FindRandomLocation`) paired with a custom `UFloatingPawnMovement` extension (`UFishSteeringMovement`), rather than importing heavy third-party 3D volumetric pathfinding plugins.
*   **Rationale:** Keeps the project footprint lightweight, reduces third-party plugin dependency risks in UE 5.8, and provides deterministic execution performance with zero runtime navmesh baking overhead.

### 3. Instance Task Memory Optimization
*   **Decision:** `UBTT_SwimToward` allocates per-instance data using Unreal's `GetInstanceMemorySize()` and `CastInstanceNodeMemory<FSwimTaskMemory>`.
*   **Rationale:** Prevents per-tick dynamic memory allocations on the heap and ensures thread-safe, isolated memory storage for multiple fish instances executing the same Behavior Tree simultaneously.

### 4. Data-Driven Entity Management
*   **Decision:** Spawning definitions are abstracted into a Data Table (`DT_EntitySpawnable`) using `ST_Entities`.
*   **Rationale:** Separates code architecture from sandbox content balancing. Game designers can add new spawnable props, adjust icons, or alter classes without touching C++ or Blueprint graph logic.

---
---

# 2. AI Statement of Intent

## AI Statement of Intent

**Project:** FIH2 Sandbox Prototype  
**Author / Developer:** Engineering Team, GTECH MSc1  
**Target Environment:** Unreal Engine 5.8

### Introduction & Context
Throughout the development of the **FIH2** sandbox prototype, artificial intelligence tools were strategically incorporated into the engineering workflow. This statement formally details the specific tools utilized, the exact areas of application, the engineering rationale, the methodologies employed, the tangible benefits realized, and the architectural limitations encountered during development.

---

### 1. The Tools Used

*   **Google Gemini (Advanced):** Utilized for deep analysis of Unreal Engine engine-level source patterns, component lifecycle workflows, and documentation synthesis, also used for the generation of this documentation reviewed by user.

---

### 2. Where AI Was Used in the Project
AI assistance was strictly targeted at technical and algorithmic problem spaces:
*   **Controller Interaction Architecture:** Drafting the mouse deprojection and `UPhysicsHandleComponent` integration within `AMouseController`.
*   **Physics Proxy Pattern Conception:** Resolving attachment incompatibilities between Unreal Engine's Chaos `GeometryCollectionComponent` and `UPhysicsHandleComponent` inside `AFood` via a dedicated `UBoxComponent` physics proxy.
*   **AI Navigation & Vector Math:** Formulating the 3D whisker raycast sensor array and spherical rotation interpolation logic implemented in `UFishSteeringMovement::CalculateAvoidance`.
*   **Behavior Tree Task Scaffolding:** Generating the boilerplate structure for `UBTTaskNode` instance memory (`FSwimTaskMemory`) and latent execution overrides (`ExecuteTask`, `TickTask`) in `UBTT_SwimToward`.

---

### 3. Why It Was Used
Developing an interactive 3D sandbox in modern Unreal Engine involves navigating extensive C++ API surfaces, complex reflection requirements, and specialized physics subsystems. AI was adopted to:
*   Accelerate the prototyping cycle by minimizing time spent searching through thousands of engine header declarations.
*   Rapidly overcome subtle syntax and linking hurdles inherent to Unreal's Enhanced Input, Behavior Tree, and Chaos Physics modules.
*   Explore alternative design patterns for 3D navigation where standard engine 2D navigation meshes were fundamentally inapplicable.
*   Understand issue with my code and how to fix it.

---

### 4. How It Was Used
AI was treated as an on-demand technical pair programmer and architectural advisor. Typical usage patterns included:
*   **Interactive Architecture Brainstorming:** Presenting engine requirements (e.g., "How to drag non-static actors in 3D using mouse cursor deprojection without losing physics collision?") to evaluate candidate solutions.
*   **Targeted Refactoring:** Providing initial C++ function implementations to optimize vector math or eliminate potential null-pointer dereferences in line trace routines.
*   **Compiler & Linker Error Diagnostics:** Submitting verbose Unreal Build Tool (UBT) linker errors (such as missing module dependencies like `GeometryCollectionEngine` or `AIModule`) to rapidly identify missing `Build.cs` inclusions.

---

### 5. Benefits Obtained
*   **Substantial Time Savings:** Reduced engine API lookup and boilerplate authoring time by an estimated 15 to 20 hours.
*   **Robust Architectural Design:** Facilitated the clean separation of concerns through Unreal Interfaces (`IInteractable`, `IBreakable`, `IEatable`), keeping codebase coupling low.
*   **Optimized Algorithmic Solutions:** Produced clean, readable vector avoidance math in `FishSteeringMovement` that ran efficiently without requiring external plugins.

---

### 6. Limitations Encountered
While highly beneficial, AI assistance presented several notable limitations:
*   **Contextual Blindness to Chaos Destruction:** The AI initially recommended directly grabbing a `UGeometryCollectionComponent` with `UPhysicsHandleComponent`, which is unsupported out-of-the-box in Chaos; manual human analysis was required to diagnose the root issue and devise the `PhysicsProxy` surrogate pattern.


---

### Mandatory Closing Declaration
I hereby declare that all AI-assisted code, architectural concepts, and technical implementations in this project have been comprehensively reviewed, manually verified, debugged, and tested within the Unreal Engine 5.8 environment. No AI-generated code was incorporated blindly or without deep inspection. Every system, component, and design choice remains thoroughly understood, validated for runtime stability and performance, and fully explainable by the developer.

Signed: William Pinel

