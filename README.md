# SQL Processor Study Project

이 프로젝트는 C 언어로 구현한 파일 기반 `MiniSQL` 처리기이다.
현재 버전은 `MiniSQL> ` 프롬프트를 제공하는 인터랙티브 CLI 형태로 동작하며, 하드코딩된 `users` 테이블 하나에 대해 `INSERT`와 `SELECT`를 수행한다.

## 현재 구현 상태
- 인터랙티브 REPL 스타일 CLI 지원
- `exit`, `quit`, EOF(`Ctrl + D`) 종료 지원
- `INSERT INTO users VALUES (...)`
- `SELECT * FROM users;`
- `SELECT * FROM users WHERE id = ...;`
- 데이터 저장 위치: [users.csv](/home/leeminjeong/workspace/c_project/my_study/data/users.csv)
- 저장 포맷: CSV
- 문자열 컬럼은 CSV 저장 시 큰따옴표로 저장

## 빌드 방법
```bash
make
```

## 실행 방법
```bash
./sql_processor
```

실행 예시:
```text
$ ./sql_processor
MiniSQL> SELECT * FROM users;
MiniSQL> INSERT INTO users VALUES (
...> 15, "demo15", "Demo User",
...> 24, "010-1515-1515", "demo15@example.com"
...> );
MiniSQL> SELECT * FROM users WHERE id = 15;
MiniSQL> exit
```

## 프로젝트 구조
- [docs](/home/leeminjeong/workspace/c_project/my_study/docs/README.md): 설계 및 기술 문서
- [include](/home/leeminjeong/workspace/c_project/my_study/include/constants.h): 헤더 파일
- [src](/home/leeminjeong/workspace/c_project/my_study/src/main.c): C 소스 파일
- [data](/home/leeminjeong/workspace/c_project/my_study/data/users.csv): CSV 데이터 파일

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

## 1차 구현 범위 요약
- `users` 테이블 하나만 지원
- `INSERT`, `SELECT`만 지원
- `WHERE`는 단일 조건만 지원
- 1차 구현의 대표 조건은 `id = 값`
- `UPDATE`, `DELETE`, `CREATE TABLE`, 복합 조건은 아직 지원하지 않음
