# ProjectPC – Paragon Chess

[![Paragon Chess Gameplay](https://img.youtube.com/vi/My3fQt4-K18/hqdefault.jpg)](https://youtu.be/r3RWHsHRCXo)

[이미지 클릭 시 유튜브로 이동합니다]
<br /><br />

언리얼 엔진 5.4 기반으로 개발한 멀티플레이 오토 배틀러 게임 프로젝트입니다.  
Paragon Character와 TFT 에셋을 활용해 제작했습니다.
<br /><br />

---

## 📋 목차
- [프로젝트 개요](#-프로젝트-개요)
- [구현 목표](#-구현-목표)
- [프로젝트 구조](#-프로젝트-구조)
- [게임 플로우](#-게임-플로우)
- [핵심 아키텍처](#-핵심-아키텍처)
- [기술 문서 링크](#-기술-문서-링크)
<br />

---

## ✅ 프로젝트 개요

- **프로젝트명** : ProjectPC – Paragon Chess
- **플랫폼** : PC (Windows)
- **개발 기간** : 2025.08.25 ~ 2025.11.10 (총 11주)
- **참여 인원** : 3인 개발 (프로그래머 3인)

### 기술 스택
- Unreal Engine 5.4
- Gameplay Ability System (GAS)
- Dedicated Server

### 협업 도구
- Notion  
  - [Project PC Team Notion](https://www.notion.so/4-5-2541f50237ec80e698e7fdd128df643a?pvs=21)
- SourceTree

### 요구사항
- 게임 플레이
  - 다수의 플레이어가 동시에 참여하는 멀티플레이 환경 지원
  - 유닛 구매, 배치, 전투로 이어지는 오토배틀러 핵심 루프 구현
  - 유닛별 고유 능력 및 시너지 시스템
- 네트워크
  - 모든 중요 게임 로직은 서버에서 처리 (서버 권위 구조)
  - 클라이언트는 시각적 피드백만 처리하여 네트워크 복제 비용 최소화
  - 서버-클라이언트 간 데이터 동기화 신뢰성 보장
- 성능
  - 다수의 객체가 동시에 생성/소멸되는 환경에서의 프레임 안정성 확보
  - 위젯 재사용을 통한 GC 부하 최소화

### 담당 구현 내용
  - GAS(Gameplay Ability System) 기반 플레이어 캐릭터 구현
  - 플레이어 전원이 공유하는 중앙화된 상점 시스템 구현
  - 아이템 베이스 및 인벤토리 시스템 구현
  - 유닛 발사체 시스템 구현
<br />

---

## 🎯 구현 목표

- 언리얼 엔진의 GAS(Gameplay Ability System) 를 활용한 확장 가능한 캐릭터 능력 시스템 구현
- 서버 권위(Server Authority) 구조 기반으로 데이터 무결성 보장
- 서버-클라이언트 복제를 고려한 네트워크 친화적 시스템 설계
- 오브젝트 풀링을 통한 발사체 시스템 최적화로 GC 호출 빈도 및 부하 감소
- 기능별 컴포넌트 분리를 통한 유지보수성과 확장성 확보
<br />

---

## 📁 프로젝트 구조

### 디렉토리 구조
```
ProjectPC/Source/ProjectPC/
├── Public/                  # 헤더 파일 (.h)
│   ├── AbilitySystem/       # GAS (Gameplay Ability System)
│   │   ├── Player/          # GA, GE, AttributeSet, ExecutionCalculation
│   │   └── Unit/            # GA(Synergy 포함), GC, AttributeSet, EffectSpec
│   ├── AI/                  # AI Task
│   ├── Animation/           # 플레이어/유닛 애니메이션, Notify
│   ├── Character/           # 플레이어(카메라), 유닛, 투사체
│   ├── Component/
│   ├── Controller/          # 플레이어/유닛 컨트롤러
│   ├── DataAsset/           # 아이템, 플레이어, 유닛, 시너지 등 데이터 에셋
│   ├── DynamicUIActor/
│   ├── GameFramework/       # GameMode, GameState, GameInstance, Subsystem, Save 등
│   ├── Item/
│   ├── Shop/
│   ├── Synergy/
│   ├── UI/                  # Board, Shop, Lobby, StartMenu, 유닛 UI 등
│   └── Utility/
└── Private/                 # 구현 파일 (.cpp) — Public과 동일 구조
```
- 기능 단위로 묶어 폴더링

### 에셋 관리 방식
- Content/CustomAsset/ Git 추적에서 제외<br />
[`.gitignore`](https://github.com/JIHEON-HYUN/ProjectPC/blob/7380437ab0c8f925d149dafb2bea589abc7a3b91/.gitignore#L10)
- 용량을 크게 차지하는 에셋들은 일괄적으로 CustomAsset폴더에 보관
- 프로젝트 진행 중, 추가/수정된 에셋은 공유 폴더에 업로드 후 issue 공유
- 레포지토리 용량 절감, 바이너리 에셋 충돌로 인한 Merge 오류 방지
<img width="847" height="692" alt="Image" src="https://github.com/user-attachments/assets/969cc06a-c488-4b10-88a3-7d7c20b75a0b" />

### 커밋 메시지 Rule
<img width="841" height="527" alt="image" src="https://github.com/user-attachments/assets/87157168-7bee-446a-9a22-177d450e94aa" />
<br /><br />

---

## 🎮 게임 플로우

- Ip Input Level -> Lobby Level -> Combat Level로 이어지는 선형적 구조
- 각 레벨별 명확한 책임 분리

### IP Input Level
<img width="585" height="980" alt="Image" src="https://github.com/user-attachments/assets/2a3d6217-00bb-40f0-94dd-0177bdd6bce5" />

### Lobby Level
<img width="1094" height="1058" alt="Image" src="https://github.com/user-attachments/assets/72063f84-fef5-4ad6-b518-90f269bd178e" />

### Combat Level
<img width="876" height="1067" alt="Image" src="https://github.com/user-attachments/assets/3c18da4c-ec9d-4683-9f0e-d4cccf6eeca6" />
<br /><br />

---

## 🏗️ 핵심 아키텍처

### 컴포넌트 기반 설계
- 거대한 단일 클래스 대신 기능 단위로 컴포넌트 분리, 단일 책임 원칙(SRP) 준수
  - APCCombatGameState에서 상점 로직을 직접 구현하지 않고, UPCShopManager 액터 컴포넌트를 소유하는 형태로 설계<br />
  [`PCCombatGameState.cpp`](https://github.com/JIHEON-HYUN/ProjectPC/blob/7dcfa34c676d92760706365baff8c930cd668282/Source/ProjectPC/Public/GameFramework/GameState/PCCombatGameState.h#L544-L546)
  - APCPlayerState도 마찬가지로 인벤토리 기능을 직접 구현하는 대신 UPCPlayerInventory 컴포넌트를 소유<br />
  [`PCPlayerState.cpp`](https://github.com/JIHEON-HYUN/ProjectPC/blob/261cef11a9cf20bde3d161e2398b19f86305d480/Source/ProjectPC/Public/GameFramework/PlayerState/PCPlayerState.h#L180-L182)
  - 시스템 간 결합도를 낮춰 유지보수성과 재사용성 극대화
 
### GAS 기반 캐릭터 설계
- 플레이어의 스탯을 AttributeSet으로 관리, 플레이어의 행동을 UGameplayAbility(GA) 클래스로 객체화
  - AttributeSet은 서버에서만 변경되고 클라이언트에 복제되어 데이터 무결성 보장
  - AttributeSet 변경 시 Delegate를 통해 UI에 자동 반영 (Observer Pattern)
  [`PCPlayerOverheadWidget.cpp`](https://github.com/JIHEON-HYUN/ProjectPC/blob/a4326be146b22b76ecb296ec11c181065a9ba6f9/Source/ProjectPC/Private/UI/PlayerMainWidget/PCPlayerOverheadWidget.cpp#L68-L81)
  - 플레이어가 상점 기능 이용 시 APCPlayerState나 UPCShopManager와 직접 결합하지 않고 GA를 통한 요청만 수행하여 결합도 감소
  - GA 활성화는 서버 권위에서 이루어지며, 이후 변동사항은 각 클라에 복제됨 (서버 권위 구조의 중앙화된 상점 시스템)
  <img width="2349" height="1122" alt="image" src="https://github.com/user-attachments/assets/fed2e7ba-463d-4255-8048-dbfd79cac0fb" /><br />
  1. UI 상호작용 (경험치 구매 버튼 클릭)
  https://github.com/JIHEON-HYUN/ProjectPC/blob/a4326be146b22b76ecb296ec11c181065a9ba6f9/Source/ProjectPC/Private/UI/Shop/PCShopWidget.cpp#L171-L177
  2. 서버 RPC 요청
  https://github.com/JIHEON-HYUN/ProjectPC/blob/a4326be146b22b76ecb296ec11c181065a9ba6f9/Source/ProjectPC/Private/Controller/Player/PCCombatPlayerController.cpp#L358-L365
  3. GA 활성화 요청
  https://github.com/JIHEON-HYUN/ProjectPC/blob/a4326be146b22b76ecb296ec11c181065a9ba6f9/Source/ProjectPC/Private/Controller/Player/PCCombatPlayerController.cpp#L432-L443
 
### 서버-클라이언트 복제를 고려한 인벤토리 시스템
- 아이템은 데이터값만을 가진 구조체<br />
[`PCItemData.h`](https://github.com/JIHEON-HYUN/ProjectPC/blob/87c75d6c143ec71204c575602ed0a7b4d49c2714/Source/ProjectPC/Public/Item/PCItemData.h#L21-L35)
- 플레이어가 가진 아이템 목록은 해당 아이템의 ItemTag(FGameplayTag) 기반으로 관리<br />
[`PCPlayerInventory.h`](https://github.com/JIHEON-HYUN/ProjectPC/blob/3516a3ae62e8751f261f08edd9bb3df6d6d67e84/Source/ProjectPC/Public/Item/PCPlayerInventory.h#L28-L30)
- 아이템 데이터는 UPCItemManagerSubsystem에서 FGameplayTag 기반으로 Get<br />
[`PCItemManagerSubsystem.cpp`](https://github.com/JIHEON-HYUN/ProjectPC/blob/3516a3ae62e8751f261f08edd9bb3df6d6d67e84/Source/ProjectPC/Private/GameFramework/WorldSubsystem/PCItemManagerSubsystem.cpp#L28-L31)
  - 실제로 복제되는 데이터는 FGameplayTag 배열로 제한하여 메모리 사용량 절감
  - 아이콘 이미지 같은 에셋은 TSoftObjectPtr로 관리, 필요한 시점에만 비동기 로드하여 로딩 스파이크 방지
 
### 발사체 오브젝트 풀링
- 오브젝트 풀링을 통한 다수의 발사체 관리
- Queue 자료구조를 활용하여 미리 생성해둔 발사체 객체를 순환 재사용<br />
[`PCProjectilePoolSubsystem.h`](https://github.com/JIHEON-HYUN/ProjectPC/blob/9d37bd716b56dc5b2598187a4d272927483070d7/Source/ProjectPC/Public/GameFramework/WorldSubsystem/PCProjectilePoolSubsystem.h#L26)
[`PCProjectilePoolSubsystem.cpp`](https://github.com/JIHEON-HYUN/ProjectPC/blob/9d37bd716b56dc5b2598187a4d272927483070d7/Source/ProjectPC/Private/GameFramework/WorldSubsystem/PCProjectilePoolSubsystem.cpp#L10-L26)
  - 전투 중 발사체가 반복 생성/소멸하여 발생하는 GC 부하를 줄임
  - Stack 대비 객체를 균등하게 순환 사용하여, 미초기화 상태에서 재사용 위험 방지
  - Array 대비 삽입/꺼내기 모두 O(1)로 발사체 사용 시 발생하는 연산 비용 절감
<br />

---

## 🔗 기술 문서 링크
- [Google Drive](https://drive.google.com/file/d/1kzrute3VOy9PtB188sV5Atnsk53Go186/view?usp=sharing)
