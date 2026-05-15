# ProjectA

Unreal Engine 5.7 기반 멀티플레이 액션 RPG 프로토타입입니다.
C++ 중심으로 인벤토리/장비, GAS 전투, 백엔드 연동, 실시간 통신, 런타임 슬라이싱, 탄환 매니저를 구현했습니다.

![ProjectA demo](./Document/ProjectADemoResize.gif)

## Demo Flow

5분 데모 기준 플레이 흐름입니다.

1. 로그인 및 로비 진입
2. 아이템 획득
3. Tetris-style 그리드 인벤토리 배치
4. 무기 장착 및 슬롯 전환
5. GAS 기반 전투
6. 인벤토리/창고 저장 및 로드
7. 마켓/상점 데이터 연동

## Core Contributions

### 1. Grid Inventory & Equipment

가변 크기 아이템을 2D 그리드에 배치하는 인벤토리 시스템을 구현했습니다.

- `TArray<FInventoryEntry>` 기반 Sparse 2D Grid 구조
- `X`, `Y`, `Width`, `Height`, `bRotated`를 이용한 아이템 점유 영역 표현
- `CanPlaceRect` 기반 사각형 충돌 검사
- `FindFirstFit` 기반 빈 공간 탐색
- 동일 아이템 스택 병합 및 수량 분할
- `FFastArraySerializer` 기반 인벤토리 델타 복제

대표 코드:

- `Plugins/ATGGridInventory/Source/ATGGridInventory/Public/InventoryTypes.h`
- `Plugins/ATGGridInventory/Source/ATGGridInventory/Private/InventoryTypes.cpp`
- `Plugins/ATGGridInventory/Source/ATGGridInventory/Private/ATGInventoryComponent.cpp`
- `Source/ProjectA/Private/ATGPlayerEquipComponent.cpp`

### 2. GAS Combat

Gameplay Ability System을 사용해 무기 기반 전투와 콤보 입력 흐름을 구현했습니다.

- `UAbilitySystemComponent`와 `UCharacterAttributeSet` 기반 스탯 관리
- 무기 장착 시 Ability와 GameplayEffect 갱신
- `AbilityTask_WaitMeleeTargetData` 커스텀 태스크 구현
- 입력 타이밍 기반 근접 콤보 처리
- GameplayTag 기반 이벤트/데미지 처리

대표 코드:

- `Source/ProjectA/Private/GAS/GA_MeleeCombo.cpp`
- `Source/ProjectA/Private/GAS/AbilityTask_WaitMeleeTargetData.cpp`
- `Source/ProjectA/Private/GAS/CharacterAttributeSet.cpp`
- `Source/ProjectA/Private/ATGPlayerCharacter.cpp`

### 3. Backend-linked Persistence & Market

게임 서버와 별도 백엔드 서버를 연동해 계정, 인벤토리, 창고, 마켓 데이터를 처리했습니다.

- HTTP/REST API 기반 로그인, 회원가입, 저장, 로드
- TCP 소켓 기반 실시간 메시지 수신
- FlatBuffers 기반 패킷 직렬화
- 인벤토리/창고 데이터를 JSON으로 변환해 서버와 동기화
- 마켓 등록, 구매, 판매, 조회 요청 처리

대표 코드:

- `Source/ProjectA/Private/Title/NetworkGameInstanceSubsystem.cpp`
- `Source/ProjectA/Private/Lobby/MarketSubsystem.cpp`
- `Source/ProjectA/Private/Utils/ATGSerializationLibrary.cpp`

### 4. Runtime Slicing & Bullet Manager

전투 표현과 성능 최적화를 위해 런타임 메시 처리와 경량 탄환 시뮬레이션을 구현했습니다.

- GeometryScript / ProceduralMesh 기반 런타임 절단 처리
- 절단된 메시의 스키닝 갱신
- 탄환을 개별 Actor가 아닌 `FBullet` 구조체 배열로 관리
- 중앙 Tick 루프에서 탄도, 중력, 항력, 충돌 계산

대표 코드:

- `Source/ProjectA/Private/SliceSystemComponent.cpp`
- `Source/ProjectA/Private/Utils/SliceUtils.cpp`
- `Source/ProjectA/Private/Weapon/BulletManager.cpp`

## Technical Highlights

| Area | Implementation |
| --- | --- |
| Inventory | Sparse 2D Grid, First-Fit placement, stack merge, FastArray replication |
| Equipment | Replicated slot state, weapon actor spawn/attach, input mapping change |
| Combat | GAS Ability, GameplayEffect, GameplayTag, custom AbilityTask |
| Networking | UE Replication, Iris enabled, HTTP, TCP socket worker |
| Backend Data | JSON serialization, inventory/stash persistence, market API |
| Performance | Bullet struct simulation, reduced projectile actor overhead |
| Runtime Mesh | GeometryScript, ProceduralMesh, skinning buffer update |

## Design Notes

### Why `TArray` for Inventory?

The inventory stores only existing items, not every grid cell.
This keeps the structure simple and works well with Unreal replication.

Trade-off:

- Good: simple iteration, serialization-friendly, Blueprint-friendly, compatible with `FFastArraySerializer`
- Bad: ID lookup and placement checks are linear
- Possible improvement: add a transient `TMap<int32, int32>` index cache or occupancy grid for faster lookup

### Why not Replicated `TMap` for Equipment?

Unreal's default property replication does not support `TMap` cleanly.
For replicated equipment state, a `TArray` slot table is safer. Since equipment slots are few, linear search with `FindByPredicate` is acceptable.

## Tech Stack

- Unreal Engine 5.7
- C++
- Gameplay Ability System
- Enhanced Input
- UE Replication / Iris
- GeometryScript
- ProceduralMeshComponent
- HTTP / TCP Socket
- FlatBuffers
- MySQL-backed external API
- Blueprint UI integration

## Project Scope

This project focuses on technical gameplay systems rather than final commercial content volume.

The main goal was to prove:

- network-aware gameplay system design
- data-driven item and equipment handling
- backend-linked persistence
- GAS-based combat extensibility
- runtime mesh and lightweight simulation experiments

## Build

Required environment:

- Unreal Engine 5.7
- Windows
- Visual Studio with C++ toolchain
- OnlineSubsystemSteam enabled for Steam session testing

Open `ProjectA.uproject`, generate project files if needed, then build `ProjectAEditor`.

## Current Cleanup Targets

These are known polish tasks before public portfolio submission:

- remove unused Tick functions
- reduce debug logs and DrawDebug calls
- replace temporary return values in equipment helper functions
- add null checks around equipment slot lookup
- split deeper technical notes into `Document/` pages
