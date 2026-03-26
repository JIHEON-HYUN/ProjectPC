# ProjectPC – Paragon Chess

[![Paragon Chess Gameplay](https://img.youtube.com/vi/My3fQt4-K18/hqdefault.jpg)](https://youtu.be/r3RWHsHRCXo)

[이미지 클릭 시 유튜브로 이동합니다]

언리얼 엔진 5.4 기반으로 개발한 멀티플레이 오토 배틀러 게임 프로젝트입니다.  
Paragon Character와 TFT 에셋을 활용해 제작했습니다.

---

## 1. 프로젝트 개요

- **프로젝트명**: ProjectPC – Paragon Chess
- **개발 기간**: 2025.08.25 ~ 2025.11.10 (총 11주)
- **참여 인원**: 3인 개발
- **담당 구현 내용**
  - GAS(Gameplay Ability System) 기반 플레이어 캐릭터 구현
  - 서버 권위 구조의 중앙화된 상점 시스템 구현
  - 서버-클라이언트 복제를 고려한 아이템 및 인벤토리 시스템 구현
  - 오브젝트 풀링 기반 유닛 발사체 시스템 구현

### 기술 스택
- Unreal Engine 5.4
- Gameplay Ability System (GAS)
- Dedicated Server

### 협업 도구
- Notion  
  - [Project PC Team Notion](https://www.notion.so/4-5-2541f50237ec80e698e7fdd128df643a?pvs=21)
- SourceTree

---

## 2. 프로젝트 목표

- 다양한 Paragon Character를 활용하여 많은 종류의 유닛을 구현
- 45종 유닛, 16개 시너지, 65종 아이템 능력치를 데이터 기반 구조로 설계
- Template AnimBP 기반 구조를 통해 유닛 개발 속도 향상

---

## 3. 요구사항

- 팀원들이 직관적으로 사용할 수 있는 유닛 시스템 설계
- 내부 로직 이해 없이도 명확한 함수 호출만으로 기능 동작
- 재사용 가능한 데이터 중심 설계로 개발 효율 극대화
