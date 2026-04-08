# Stack / Heap / Static Memory를 이 프로젝트로 이해하기

이 문서는 C를 처음 배우는 사람이 자주 헷갈리는 개념인 `stack`, `heap`, `static memory`를 이 프로젝트 코드와 연결해서 설명하기 위한 학습용 문서이다.

![MiniSQL memory map](/home/leeminjeong/workspace/c_project/my_study/assets/minisql-memory-map.svg)

핵심 질문은 이것이다.

- 함수 안에 선언한 변수는 어디에 저장되는가?
- 전역 상수나 문자열은 어디에 있는가?
- `malloc()`을 안 쓰고도 프로그램이 돌아가는 이유는 무엇인가?

## 아주 짧게 요약
- `stack`: 함수가 실행될 때 잠깐 생기는 지역 변수 공간
- `heap`: 필요할 때 직접 동적 할당해서 쓰는 공간
- `static memory`: 프로그램 시작부터 끝까지 유지되는 전역 데이터나 문자열, 정적 데이터 영역

이 프로젝트는 1차 구현 기준으로 `heap`을 거의 쓰지 않고, 주로 `stack`과 `static memory`를 사용한다.

## 1. Stack이란?
Stack은 보통 함수가 호출될 때 만들어지는 지역 변수 공간이라고 이해하면 된다.

예를 들어 [repl.c](/home/leeminjeong/workspace/c_project/my_study/src/repl.c)의 `run_repl()` 안에는 이런 변수가 있다.

- `char line[MAX_INPUT_LEN];`
- `char buffer[MAX_INPUT_LEN];`
- `int collecting = 0;`

이런 변수들은 `run_repl()` 함수가 실행되는 동안 stack에 놓인다.

즉:

- 함수 시작 -> 지역 변수 생성
- 함수 종료 -> 지역 변수 사라짐

이라고 이해하면 된다.

## 2. 이 프로젝트에서 stack에 올라가는 대표 예시
### [main.c](/home/leeminjeong/workspace/c_project/my_study/src/main.c)
- `main()` 자체는 지역 변수가 거의 없다.

### [repl.c](/home/leeminjeong/workspace/c_project/my_study/src/repl.c)
- `line`
- `buffer`
- `collecting`
- `command`, `parse_status`, `exec_status` 같은 지역 변수

### [executor.c](/home/leeminjeong/workspace/c_project/my_study/src/executor.c)
- `rows`
- `row_count`
- `matched_count`
- `values`
- `FILE *fp`

### [storage.c](/home/leeminjeong/workspace/c_project/my_study/src/storage.c)
- `buffer`
- `count`
- `column`
- `index`

즉 이 프로젝트의 대부분의 작업용 데이터는 stack에 잠깐 만들어졌다가 함수가 끝나면 사라진다.

## 3. Heap이란?
Heap은 프로그램이 실행 중에 필요할 때 직접 메모리를 요청해서 쓰는 공간이다.

보통 C에서는:

```c
malloc(...)
calloc(...)
realloc(...)
free(...)
```

같은 함수를 써서 heap을 다룬다.

예를 들어:

```c
char *name = malloc(100);
```

이건 "문자열 100바이트를 heap에 달라"는 뜻이다.

그리고 다 쓴 뒤에는:

```c
free(name);
```

으로 해제해야 한다.

## 4. 그런데 우리 프로젝트는 왜 heap을 거의 안 쓰나?
이 프로젝트는 1차 구현에서 구조를 단순하게 가져가기 위해, 크기가 미리 정해진 배열을 많이 사용한다.

예:

- `char line[MAX_INPUT_LEN];`
- `char rows[MAX_INPUT_LEN][MAX_INPUT_LEN];`
- `char values[USER_COLUMN_COUNT][128];`

즉 "필요한 최대 크기를 미리 정해놓고 stack 배열로 처리"하는 방식이다.

장점:

- `malloc()` / `free()`를 몰라도 구현 가능
- 메모리 해제 실수 위험이 줄어듦
- 초보자가 읽기 쉬움

단점:

- 최대 크기를 넘는 큰 입력은 유연하게 처리하기 어려움
- 메모리를 더 똑똑하게 쓰는 구조는 아님

그래서 지금 프로젝트는 학습용 1차 구현에 맞게 `heap 없이도 동작하도록 단순화`된 설계라고 보면 된다.

## 5. Static memory란?
Static memory는 프로그램이 시작될 때부터 끝날 때까지 유지되는 데이터 영역이라고 생각하면 된다.

대표적으로 이런 것들이 들어간다.

- 전역 변수
- `static` 전역/함수 내부 변수
- 문자열 리터럴
- 상수 데이터

예를 들어 코드에 이런 문자열이 있으면:

```c
"Inserted 1 row"
"Error: missing semicolon"
"users"
```

이런 문자열 리터럴은 보통 static한 영역에 놓인다.

또 [constants.h](/home/leeminjeong/workspace/c_project/my_study/include/constants.h)의 매크로를 통해 참조되는 프롬프트 문자열이나 파일 경로도 프로그램 전반에서 같은 의미로 사용된다.

## 6. 이 프로젝트에서 static memory로 생각하면 좋은 것들
- `"MiniSQL> "` 같은 프롬프트 문자열
- `"Error: ..."` 형태의 고정 오류 메시지
- `"users"` 같은 하드코딩 테이블명
- 코드 안에 직접 적힌 문자열 리터럴들

이 값들은 어떤 함수가 끝났다고 없어지는 게 아니라, 프로그램 실행 중 계속 참조될 수 있다.

## 7. 왜 지역 배열은 stack이고, 문자열 리터럴은 static인가?
예를 들어:

```c
char line[MAX_INPUT_LEN];
puts("Inserted 1 row");
```

이 경우:

- `line`은 함수 안 지역 변수이므로 stack
- `"Inserted 1 row"`는 코드에 박혀 있는 문자열 리터럴이므로 static memory

같은 코드 안에 있어도 저장되는 메모리 영역이 다르다.

## 8. 파일은 memory가 아니라 디스크에 있다
중요한 점 하나 더 있다.

[users.csv](/home/leeminjeong/workspace/c_project/my_study/data/users.csv)는 stack도 아니고 heap도 아니고 static memory도 아니다.  
이 파일은 SSD/HDD 같은 저장장치에 존재한다.

단, 파일을 읽는 순간:

- 한 줄이 `buffer` 같은 배열에 들어오면 stack으로 올라오고
- CPU는 그 stack 데이터를 처리한다.

즉:

- 파일 자체는 디스크
- 읽어온 데이터는 메모리

라고 구분해야 한다.

## 9. 함수 호출과 stack의 관계
함수가 호출되면 stack frame이라는 실행 공간이 생긴다고 설명하기도 한다.

예를 들어:

1. `main()`
2. `run_repl()`
3. `process_input_line()`
4. `parse_command()`

순서로 호출되면, 각 함수마다 자기 지역 변수 공간이 생긴다.

그리고 함수가 끝나면 그 함수의 지역 변수 공간은 사라진다.

그래서 어떤 함수 안의 지역 배열 주소를 바깥에서 오래 들고 있으면 위험할 수 있다.

이 프로젝트에서는 그런 위험한 패턴을 피하기 위해:

- 구조체를 값처럼 넘기거나
- 호출 중에만 쓸 배열을 지역 변수로 두고
- 파일 저장 전에는 필요한 값을 복사해 사용하는 편이다.

## 10. 왜 heap을 아직 안 쓰는 게 좋은 선택인가?
너처럼 C를 이제 막 배우는 단계에서는 heap까지 한 번에 들어오면 난이도가 훨씬 올라간다.

왜냐하면 heap을 쓰기 시작하면:

- `malloc()` 성공 여부 확인
- `free()` 해제 타이밍
- 메모리 누수
- 이중 해제
- dangling pointer

같은 문제까지 같이 봐야 하기 때문이다.

그래서 지금 프로젝트는:

- 먼저 흐름 이해
- 파싱과 실행 이해
- 파일 입출력 이해

를 우선하고, 메모리 관리 난이도는 일부러 낮춘 구조라고 볼 수 있다.

## 11. 나중에 heap을 쓰게 된다면 어디가 바뀔까?
나중에 확장하면 이런 부분에서 heap이 등장할 수 있다.

- 매우 긴 입력 문자열 동적 확장
- 행 개수를 모를 때 동적 배열 사용
- 여러 테이블을 위한 유연한 스키마 구조
- CSV 컬럼 값을 동적으로 저장

즉 지금은 고정 배열 기반이지만, 나중에 더 큰 프로그램으로 가면 heap을 도입할 가능성이 높다.

## 12. 이 프로젝트를 메모리 관점에서 한 문장으로 말하면
이 프로젝트는 실행 중 필요한 대부분의 데이터를 stack의 고정 배열로 처리하고, 고정 문자열은 static memory에 두며, 영구 데이터는 SSD/HDD의 CSV 파일에 저장하는 단순한 구조의 MiniSQL 처리기이다.

## 13. 발표 때 이렇게 설명해도 좋다
"현재 구현은 초보자 학습과 최소 구현을 우선했기 때문에, 동적 메모리 할당보다는 stack 기반 고정 배열을 많이 사용했습니다. 그래서 메모리 관리가 단순하고 코드 흐름을 읽기 쉽습니다. 반면 실제 사용자 데이터는 RAM이 아니라 CSV 파일로 디스크에 저장되므로 프로그램 종료 후에도 유지됩니다."
