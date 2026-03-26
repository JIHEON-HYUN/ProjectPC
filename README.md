# ProjectPC – Paragon Chess

[![Paragon Chess Gameplay](https://img.youtube.com/vi/My3fQt4-K18/hqdefault.jpg)](https://youtu.be/r3RWHsHRCXo)

[이미지 클릭 시 유튜브로 이동합니다]

언리얼 엔진 5.4 기반으로 개발한 멀티플레이 오토 배틀러 게임 프로젝트입니다.  
Paragon Character와 TFT 에셋을 활용해 제작했습니다.

### 기술 문서 링크
- [Google Drive](https://drive.google.com/file/d/1kzrute3VOy9PtB188sV5Atnsk53Go186/view?usp=sharing)

---

## ✅ 프로젝트 개요

- **프로젝트명** : ProjectPC – Paragon Chess
- **개발 기간** : 2025.08.25 ~ 2025.11.10 (총 11주)
- **참여 인원** : 3인 개발

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

---

## 🎯 구현 목표

- 언리얼 엔진의 GAS(Gameplay Ability System) 를 활용한 확장 가능한 캐릭터 능력 시스템 구현
- 서버 권위(Server Authority) 구조 기반으로 데이터 무결성 보장
- 서버-클라이언트 복제를 고려한 네트워크 친화적 시스템 설계
- 오브젝트 풀링을 통한 발사체 시스템 최적화로 GC 호출 빈도 및 부하 감소
- 기능별 컴포넌트 분리를 통한 유지보수성과 확장성 확보

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

---

## 🏗️ 핵심 아키텍처

### 컴포넌트 기반 설계
<img width="5552" height="2596" alt="Image" src="https://github.com/user-attachments/assets/c7e289f4-a1dc-40a0-8160-6a8a19cad8db" />
- 거대한 단일 클래스 대신 기능 단위로 컴포넌트 분리, 단일 책임 원칙(SRP) 준수
  - APCPlayerState에서 인벤토리 기능을 직접 구현하지 않고, UPCPlayerInventory의 컴포넌트를 소유하는 형태로 설계
  - APCCombatGameState 역시 상점 로직을 UPCShopManager 컴포넌트로 분리
  - 시스템 간 결합도를 낮춰 유지보수성과 재사용성 극대화
 
### GAS 기반 캐릭터 설계
- 플레이어의 스탯을 AttributeSet으로 관리, 플레이어의 행동을 UGameplayAbility(GA) 클래스로 객체화
  - AttributeSet은 서버에서만 변경되고 클라이언트에 복제되어 데이터 무결성 보장
  - AttributeSet 변경 시 Delegate를 통해 UI에 자동 반영 (Observer Pattern)
  - 플레이어가 상점 기능 이용 시 APCCombatGameState 와 직접 결합하지 않고 GA를 통한 요청만 수행하여 결합도 감소

### 서버 권위 구조의 중앙화된 상점 시스템
- 모든 상점 로직은 서버에서만 처리
  - 상점 데이터는 APCCombatGameState가 소유하는 액터 컴포넌트 APCShopManager에서 관리
  - UI는 Delegate에 바인딩만 할 뿐 강한 참조 없이 상태 변화에 반응 (Observer Pattern)
 
### 서버-클라이언트 복제를 고려한 인벤토리 시스템
- 플레이어가 가진 아이템 목록은 FGameplayTag 기반으로 관리
- 아이템 데이터는 UPCItemManagerSubsystem에서 FGameplayTag 기반으로 Get
  - 실제로 복제되는 데이터는 FGameplayTag로 제한하여 메모리 사용량 절감
  - 아이콘 이미지 같은 에셋은 TSoftObjectPtr로 관리, 필요한 시점에만 비동기 로드하여 로딩 스파이크 방지
 
### 발사체 오브젝트 풀링
- 오브젝트 풀링을 통한 다수의 발사체 관리
  - 전투 중 발사체가 반복 생성/소멸하여 발생하는 GC 부하를 줄임
- Queue 자료구조를 활용하여 미리 생성해둔 발사체 객체를 순환 재사용
  - Stack 대비 객체를 균등하게 순환 사용하여, 미초기화 상태에서 재사용 위험 방지
  - Array 대비 삽입/꺼내기 모두 O(1)로 발사체 사용 시 발생하는 연산 비용 절감
