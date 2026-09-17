# Exception API 语法改进指南

这份文档用于指导现有 `setjmp`/`longjmp` 异常接口的语法重构。当前阶段暂不处理多线程，目标是让接口：

- 更接近普通 C 控制流，阅读和格式化更自然；
- 减少宏名称冲突；
- 不让调用方依赖 `Except_Frame` 等内部细节；
- 明确 `return`、`break`、`continue`、`goto` 和 `FINALLY` 的边界；
- 保留“通过异常对象地址判断异常类型”的现有设计。

建议最终形成下面这样的调用方式：

```c
static EXCEPT_DEFINE(Foo, "Foo exception");

int run(void) {
    int result = 0;

    EXCEPT_TRY {
        EXCEPT_RAISE(Foo);

    } EXCEPT_CATCH(Foo, caught) {
        fprintf(stderr, "%s at %s:%d\n",
                caught->exception->reason,
                caught->file,
                caught->line);
        result = -1;

    } EXCEPT_CATCH_ANY(caught) {
        log_unknown_exception(caught);
        EXCEPT_RETHROW();

    } EXCEPT_FINALLY {
        cleanup();

    } EXCEPT_END;

    return result;
}
```

## 第一步：先写清楚语义契约

不要先改宏。先确定并记录接口保证什么、不保证什么，否则实现很容易在边界行为上反复修改。

建议采用以下契约：

1. `EXCEPT_RAISE(Foo)` 使用 `Foo` 对象的地址标识异常类型。
2. `EXCEPT_CATCH(Foo, caught)` 只捕获类型为 `Foo` 的异常。
3. 多个 `EXCEPT_CATCH` 按书写顺序匹配，最多执行一个。
4. `EXCEPT_CATCH_ANY(caught)` 只处理之前没有匹配的异常。
5. 未处理的异常会在 `EXCEPT_END` 处自动向外传播。
6. `EXCEPT_RETHROW()` 保留原始抛出位置，而不是把 catch 位置当作新位置。
7. `EXCEPT_FINALLY` 在以下路径执行：
   - TRY 主体正常结束；
   - TRY 主体抛出异常并被当前层捕获；
   - TRY 主体抛出异常但当前层没有捕获。
8. 当前实现不保证 catch 中再次抛出异常时，本层 `FINALLY` 仍然执行。若保留这一行为，必须明确写进文档。
9. 不允许从 TRY/CATCH/FINALLY 中直接使用普通 `return`、`break`、`continue` 或跳出结构的 `goto`。
10. 暂不提供通用的 `EXCEPT_RETURN`。函数返回值先写入局部变量，在 `EXCEPT_END` 后统一 `return`。

第 8 条尤其重要：当前结构中的异常 frame 在第一次抛出时已经出栈，因此 catch 中再次 `RAISE` 或 `RETHROW` 会直接跳到外层，无法返回本层继续执行 `FINALLY`。

## 第二步：统一公共 API 的命名

把过于通用的宏名替换为带 `EXCEPT_` 前缀的名字。

| 现有名字 | 建议名字 | 说明 |
| --- | --- | --- |
| `TRY` | `EXCEPT_TRY` | 避免污染通用命名空间 |
| `EXCEPT(e)` | `EXCEPT_CATCH(e, info)` | 表达真实的 catch 语义，并绑定异常信息 |
| `ELSE` | `EXCEPT_CATCH_ANY(info)` | 明确表示捕获任意剩余异常 |
| `FINALLY` | `EXCEPT_FINALLY` | 保留原语义 |
| `END_TRY` | `EXCEPT_END` | 与其他名字保持一致 |
| `RAISE(e)` | `EXCEPT_RAISE(e)` | 统一前缀 |
| `RERAISE` | `EXCEPT_RETHROW()` | 使用函数式宏形式，调用意图更明显 |
| `RETURN` | 删除 | 无法安全覆盖嵌套 TRY 和所有控制流位置 |

迁移期间如果确实需要兼容旧代码，可以临时提供旧名字的别名，但建议通过一个显式选项开启：

```c
#ifdef EXCEPT_ENABLE_LEGACY_NAMES
#define TRY       EXCEPT_TRY
#define FINALLY   EXCEPT_FINALLY
#define END_TRY   EXCEPT_END
#define RAISE(e)  EXCEPT_RAISE(e)
#endif
```

不要默认启用这些别名，否则命名冲突问题仍然存在。

## 第三步：让调用方显式书写大括号

把当前这种形式：

```c
TRY
    work();
EXCEPT(Foo)
    recover();
END_TRY;
```

改成：

```c
EXCEPT_TRY {
    work();
} EXCEPT_CATCH(Foo, caught) {
    recover(caught);
} EXCEPT_END;
```

需要完成的修改：

1. `EXCEPT_TRY` 只展开到 `if (state == entered)`，不替调用方打开 TRY 主体的大括号。
2. `EXCEPT_CATCH` 展开成一个只执行一次的分支，并把 `Except_Info` 指针绑定到调用方指定的变量名。
3. `EXCEPT_END` 负责结束最外层 `do { ... } while (0)`。
4. 用 clang-format 检查嵌套 TRY 的缩进是否稳定。
5. 故意漏写一个 `}`，确认编译错误仍能定位到附近代码，而不是完全落到宏定义内部。

## 第四步：把异常信息和内部 frame 分开

调用方真正需要的信息通常只有三项：

```c
typedef struct Except_Info {
    const Exception *exception;
    const char *file;
    int line;
} Except_Info;
```

`Except_Frame` 仍然可以包含 `prev` 和 `jmp_buf`，但 handler 只接触 `const Except_Info *`：

```c
EXCEPT_CATCH(Foo, caught) {
    printf("%s:%d: %s\n",
           caught->file,
           caught->line,
           caught->exception->reason);
}
```

需要完成的修改：

1. 新增 `Except_Info`。
2. 在 `Except_Frame` 中放入一个 `Except_Info info` 成员。
3. `Except_raise` 将异常类型、文件和行号写入 `frame->info`。
4. catch 宏只暴露 `&frame->info`，不要暴露整个 frame。
5. handler 得到只读指针，避免调用方篡改传播状态。

这一步并没有完全隐藏 `Except_Frame` 的定义，因为宏仍然需要在栈上创建它。可以在注释中把它标记为内部结构；如果未来希望彻底隐藏，需要改用函数回调、动态分配 frame 或者编译器扩展。

## 第五步：规范异常的声明和定义

增加两个辅助宏：

```c
#define EXCEPT_DECLARE(name_) \
    extern const Exception name_

#define EXCEPT_DEFINE(name_, reason_) \
    const Exception name_ = { .reason = (reason_) }
```

公共异常的写法：

```c
/* parser.h */
EXCEPT_DECLARE(Parse_Error);

/* parser.c */
EXCEPT_DEFINE(Parse_Error, "parse error");
```

文件内部异常的写法：

```c
static EXCEPT_DEFINE(Internal_Error, "internal error");
```

需要约定异常对象在程序使用期间始终有效。因此，不建议把自动局部异常对象传给 `EXCEPT_RAISE`；优先使用静态存储期对象。

## 第六步：移除 `RETURN`，收紧控制流

`RETURN` 看起来像普通返回，但它只能尝试弹出一个异常 frame：

- 在 catch 或 finally 中，当前 frame 可能已经被弹出，再弹一次会影响调用者；
- 在同一个函数的嵌套 TRY 中，只弹一个 frame 又不够；
- 普通 `break`、`continue` 和 `goto` 也可能绕过 `EXCEPT_END`。

因此建议删除它，统一改为单出口：

```c
int load(void) {
    int result = 0;

    EXCEPT_TRY {
        result = load_impl();
    } EXCEPT_CATCH(Io_Error, caught) {
        report(caught);
        result = -1;
    } EXCEPT_END;

    return result;
}
```

如果函数需要在多个位置决定结果，可以只设置 `result` 和状态标记，让控制流自然走到 `EXCEPT_END`。

不要急着增加 `EXCEPT_LEAVE` 或新的返回宏。先验证实际调用代码是否真的需要；控制流宏越多，嵌套语义越难解释。

## 第七步：实现明确的重新抛出操作

把对象式宏：

```c
RERAISE;
```

改成函数调用外观：

```c
EXCEPT_RETHROW();
```

重新抛出必须使用当前 frame 保存的原始信息：

```c
Except_raise(except__frame.info.exception,
             except__frame.info.file,
             except__frame.info.line)
```

不要改用 `__FILE__` 和 `__LINE__`，否则错误日志只会显示 rethrow 的位置，丢失第一次抛出的来源。

另外要明确：`EXCEPT_RETHROW()` 只能在正在处理异常的 catch 或由异常路径进入的 finally 中使用。在正常路径的 finally 中调用它没有有效的当前异常。

## 第八步：加强 `Except_raise` 的接口边界

建议同步完成以下小改动：

1. 把函数声明为 `_Noreturn`，表达它不会正常返回。
2. 显式检查异常指针是否为 `NULL`，不要依赖可能被 `NDEBUG` 关闭的断言。
3. 未捕获异常的输出始终包含换行。
4. 暂时继续使用 `abort()`；可配置的 uncaught handler 可以作为后续功能。

示例声明：

```c
_Noreturn void Except_raise(const Exception *exception,
                            const char *file,
                            int line);
```

## 第九步：记录 `setjmp`/`longjmp` 的固有限制

即使宏语法变得更漂亮，底层仍然是 `longjmp`，必须在文档中说明：

1. TRY 所在函数中，在 `setjmp` 之后被修改的非 `volatile` 自动变量，在 `longjmp` 返回后可能具有不确定值。
2. `longjmp` 不会像 C++ 异常一样自动释放资源。
3. 动态内存、文件、锁等资源必须通过清理策略显式释放。
4. 不要把即将被 `longjmp` 跳过的函数局部变量地址作为异常 payload。
5. 不要跨线程传播异常。

例如下面的 `value` 不能可靠读取：

```c
int value = 0;

EXCEPT_TRY {
    value = 42;
    EXCEPT_RAISE(Foo);
} EXCEPT_CATCH(Foo, caught) {
    printf("%d\n", value); /* 可能是不确定值 */
} EXCEPT_END;
```

如果 catch 后确实需要这个值，应重新组织数据生命周期，或者谨慎使用 `volatile`，而不是假定优化级别较低时的表现就是语言保证。

## 第十步：按顺序补齐测试

建议每完成一个语法步骤就增加对应测试，不要最后一次性迁移。

最低测试清单：

- [ ] TRY 正常结束。
- [ ] 一个异常被准确类型的 catch 捕获。
- [ ] 第一个 catch 不匹配、第二个 catch 匹配。
- [ ] `CATCH_ANY` 捕获未匹配异常。
- [ ] 已匹配后不会继续执行后续 catch。
- [ ] 未匹配异常自动传播到外层。
- [ ] `EXCEPT_RETHROW()` 保留原始文件和行号。
- [ ] FINALLY 在正常路径执行。
- [ ] FINALLY 在已捕获异常路径执行。
- [ ] FINALLY 在未捕获异常传播前执行。
- [ ] 未捕获到最外层时进程以 `SIGABRT` 结束。
- [ ] 向 `Except_raise` 传入 `NULL` 时确定性地终止，而不是空指针崩溃。
- [ ] 两层嵌套 TRY 的异常栈在结束后恢复为空。

至少使用以下编译方式验证：

```sh
cc -std=c11 -O0 -Wall -Wextra -Wpedantic \
  except.c test_except.c -o test_except

cc -std=c11 -O2 -Wall -Wextra -Wpedantic \
  except.c test_except.c -o test_except
```

如果编译器支持，再增加 AddressSanitizer 和 UndefinedBehaviorSanitizer。

## 推荐实施顺序

按照下面顺序修改，可以让每一步都保持相对容易验证：

1. 新增 `Except_Info`，但暂时不改调用语法。
2. 增加 `EXCEPT_RAISE`、`EXCEPT_RETHROW()` 和异常声明宏。
3. 实现显式大括号版本的 `EXCEPT_TRY`/`EXCEPT_CATCH`/`EXCEPT_END`。
4. 用 `EXCEPT_CATCH_ANY` 替换 `ELSE`。
5. 删除 `RETURN`，迁移为 `EXCEPT_END` 后统一返回。
6. 加入 `EXCEPT_FINALLY`，并用测试固定其语义。
7. 给 `Except_raise` 增加 `_Noreturn` 和空指针检查。
8. 补齐嵌套、传播、rethrow 和 finally 测试。
9. 删除旧宏；如果需要过渡，则放到默认关闭的兼容选项中。
10. 最后再考虑 uncaught handler、payload 和线程局部异常栈。

<details>
<summary><strong>展开查看参考实现</strong></summary>

下面是一份用于学习和验证 API 语法的最小参考实现。它保持现有的单线程假设，也保留了 `setjmp`/`longjmp` 本身的可移植性限制。

### `except.h`

```c
#ifndef EXCEPT_INCLUDED
#define EXCEPT_INCLUDED

#include <setjmp.h>
#include <stddef.h>

typedef struct Exception {
    const char *reason;
} Exception;

typedef struct Except_Info {
    const Exception *exception;
    const char *file;
    int line;
} Except_Info;

/* 宏实现所需的内部结构；调用方不应直接访问。 */
typedef struct Except_Frame Except_Frame;
struct Except_Frame {
    Except_Frame *prev;
    jmp_buf env;
    Except_Info info;
};

enum {
    EXCEPT_STATE_ENTERED = 0,
    EXCEPT_STATE_RAISED,
    EXCEPT_STATE_HANDLED,
    EXCEPT_STATE_FINALIZED
};

extern Except_Frame *Except_stack;

_Noreturn void Except_raise(const Exception *exception,
                            const char *file,
                            int line);

#define EXCEPT_DECLARE(name_) \
    extern const Exception name_

#define EXCEPT_DEFINE(name_, reason_) \
    const Exception name_ = { .reason = (reason_) }

#define EXCEPT_RAISE(exception_) \
    Except_raise(&(exception_), __FILE__, __LINE__)

#define EXCEPT_RETHROW()                                      \
    Except_raise(except__frame.info.exception,                \
                 except__frame.info.file,                     \
                 except__frame.info.line)

#define EXCEPT_TRY                                             \
    do {                                                       \
        volatile int except__state;                            \
        Except_Frame except__frame;                            \
        except__frame.prev = Except_stack;                     \
        except__frame.info = (Except_Info){0};                 \
        Except_stack = &except__frame;                         \
        except__state = setjmp(except__frame.env);             \
        if (except__state == EXCEPT_STATE_ENTERED)

#define EXCEPT_CATCH(exception_, info_)                        \
        if (except__state == EXCEPT_STATE_ENTERED)             \
            Except_stack = except__frame.prev;                 \
        if (except__state == EXCEPT_STATE_RAISED &&            \
            except__frame.info.exception == &(exception_))     \
            for (const Except_Info *info_ =                    \
                     (except__state = EXCEPT_STATE_HANDLED,    \
                      &except__frame.info);                     \
                 info_ != NULL;                                \
                 info_ = NULL)

#define EXCEPT_CATCH_ANY(info_)                                \
        if (except__state == EXCEPT_STATE_ENTERED)             \
            Except_stack = except__frame.prev;                 \
        if (except__state == EXCEPT_STATE_RAISED)              \
            for (const Except_Info *info_ =                    \
                     (except__state = EXCEPT_STATE_HANDLED,    \
                      &except__frame.info);                     \
                 info_ != NULL;                                \
                 info_ = NULL)

#define EXCEPT_FINALLY                                         \
        if (except__state == EXCEPT_STATE_ENTERED)             \
            Except_stack = except__frame.prev;                 \
        if (except__state == EXCEPT_STATE_ENTERED)             \
            except__state = EXCEPT_STATE_FINALIZED;

#define EXCEPT_END                                             \
        if (except__state == EXCEPT_STATE_ENTERED)             \
            Except_stack = except__frame.prev;                 \
        if (except__state == EXCEPT_STATE_RAISED)              \
            EXCEPT_RETHROW();                                  \
    } while (0)

#endif
```

### `except.c`

```c
#include "except.h"

#include <stdio.h>
#include <stdlib.h>

Except_Frame *Except_stack = NULL;

_Noreturn void Except_raise(const Exception *exception,
                            const char *file,
                            int line) {
    Except_Frame *frame = Except_stack;

    if (exception == NULL) {
        fputs("Invalid NULL exception\n", stderr);
        abort();
    }

    if (frame == NULL) {
        fprintf(stderr,
                "Uncaught exception: %s",
                exception->reason != NULL
                    ? exception->reason
                    : "<unnamed>");

        if (file != NULL && line > 0)
            fprintf(stderr, " at %s:%d", file, line);

        fputc('\n', stderr);
        fflush(stderr);
        abort();
    }

    frame->info.exception = exception;
    frame->info.file = file;
    frame->info.line = line;

    Except_stack = frame->prev;
    longjmp(frame->env, EXCEPT_STATE_RAISED);
}
```

### 使用示例

```c
#include <stdio.h>

#include "except.h"

static EXCEPT_DEFINE(Foo, "Foo exception");
static EXCEPT_DEFINE(Bar, "Bar exception");

static int example(int raise_bar) {
    int result = 0;

    EXCEPT_TRY {
        if (raise_bar)
            EXCEPT_RAISE(Bar);

        EXCEPT_RAISE(Foo);

    } EXCEPT_CATCH(Foo, caught) {
        fprintf(stderr,
                "caught %s at %s:%d\n",
                caught->exception->reason,
                caught->file,
                caught->line);
        result = 1;

    } EXCEPT_CATCH_ANY(caught) {
        fprintf(stderr,
                "caught other exception: %s\n",
                caught->exception->reason);
        result = 2;

    } EXCEPT_FINALLY {
        puts("cleanup");

    } EXCEPT_END;

    return result;
}

int main(void) {
    printf("result = %d\n", example(0));
    printf("result = %d\n", example(1));
    return 0;
}
```

</details>

## 完成标准

满足下面几点后，可以认为这次语法重构已经完成：

- 调用代码不再使用无前缀的 `TRY`、`ELSE`、`RETURN` 等宏；
- 调用方显式书写所有代码块的大括号；
- catch 能得到只读的 `Except_Info`，但不接触内部 frame；
- 未匹配异常、rethrow 和 finally 的行为都有测试固定；
- 文档明确禁止绕过 `EXCEPT_END` 的控制流；
- `-O0` 和 `-O2` 下测试均通过，并且没有新增编译警告。
