# SQL Processor Study Project

이 저장소는 C 언어로 구현하는 파일 기반 SQL 처리기 과제를 위한 작업 공간이다.
지금 단계에서는 코드를 먼저 작성하기보다, 구현 기준이 되는 문서를 `docs/` 아래에 정리해두고 그 문서를 바탕으로 이후 구현을 진행한다.

## 문서 안내
- [문서 인덱스](/home/leeminjeong/workspace/c_project/my_study/docs/README.md)
- [프로젝트 개요와 범위](/home/leeminjeong/workspace/c_project/my_study/docs/01-project-overview.md)
- [MiniSQL 문법 기초](/home/leeminjeong/workspace/c_project/my_study/docs/02-sql-basics.md)
- [처리 흐름 설계](/home/leeminjeong/workspace/c_project/my_study/docs/03-processing-flow.md)
- [저장 포맷 설계](/home/leeminjeong/workspace/c_project/my_study/docs/04-storage-design.md)
- [구현 계획](/home/leeminjeong/workspace/c_project/my_study/docs/05-implementation-plan.md)
- [하드코딩 테이블 정의](/home/leeminjeong/workspace/c_project/my_study/docs/06-users-table.md)
- [파일 구조와 함수 설계](/home/leeminjeong/workspace/c_project/my_study/docs/07-file-structure-and-functions.md)
- [MiniSQL 입력 규칙](/home/leeminjeong/workspace/c_project/my_study/docs/08-minisql-input-rules.md)
- [지원/미지원 문법 요약](/home/leeminjeong/workspace/c_project/my_study/docs/09-supported-grammar.md)

## 현재 방향
- 최소 구현을 우선한다.
- `INSERT`, `SELECT`만 지원한다.
- `MiniSQL> ` 프롬프트를 표시하는 인터랙티브 CLI로 동작한다.
- 사용자는 MiniSQL 문장을 한 줄에 하나씩 반복 입력할 수 있다.
- 1차 구현에서는 하드코딩된 `users` 테이블 하나만 지원한다.
- 데이터는 `data/users.csv` 파일에 CSV 형식으로 저장한다.

## 다음 단계
문서 내용이 충분히 합의되면, 그다음에만 폴더 구조와 C 코드 구현을 시작한다.
