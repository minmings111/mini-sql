#include "storage.h"
#include "repl.h"

int main(void) {
    int status;

    /* 프로그램 시작 후 실제 작업은 REPL 루프가 담당한다. */
    status = run_repl();
    storage_shutdown();
    return status;
}
