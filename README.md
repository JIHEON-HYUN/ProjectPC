# ProjectPC – Paragon Chess

[![Paragon Chess Gameplay](https://img.youtube.com/vi/My3fQt4-K18/hqdefault.jpg)](https://youtu.be/r3RWHsHRCXo)

[이미지 클릭 시 유튜브로 이동합니다]

언리얼 엔진 5.4 기반으로 개발한 멀티플레이 오토 배틀러 게임 프로젝트입니다.  
Paragon Character와 TFT 에셋을 활용해 제작했습니다.

---

## 1. 프로젝트 개요

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
  - 서버 권위 구조의 중앙화된 상점 시스템 구현
  - 서버-클라이언트 복제를 고려한 아이템 및 인벤토리 시스템 구현
  - 오브젝트 풀링 기반 유닛 발사체 시스템 구현

---

## 2. 구현 목표

- 언리얼 엔진의 GAS(Gameplay Ability System) 를 활용한 확장 가능한 캐릭터 능력 시스템 구현
- 서버 권위(Server Authority) 구조 기반으로 데이터 무결성 보장
- 서버-클라이언트 복제를 고려한 네트워크 친화적 시스템 설계
- 오브젝트 풀링을 통한 발사체 시스템 최적화로 GC 호출 빈도 및 부하 감소
- 기능별 컴포넌트 분리 및 데이터 주도 설계를 통한 유지보수성과 확장성 확보

---

## 3. 게임 플로우

- Ip Input Level -> Lobby Level -> Combat Level로 이어지는 선형적 구조
- 각 레벨별 명확한 책임 분리

### 3-1. IP Input Level
<img width="585" height="980" alt="Image" src="https://github.com/user-attachments/assets/2a3d6217-00bb-40f0-94dd-0177bdd6bce5" />

### 3-2. Lobby Level
<img width="1094" height="1058" alt="Image" src="https://github.com/user-attachments/assets/72063f84-fef5-4ad6-b518-90f269bd178e" />

### 3-3. Combat Level
<img width="876" height="1067" alt="Image" src="https://github.com/user-attachments/assets/3c18da4c-ec9d-4683-9f0e-d4cccf6eeca6" />
