# Error & Result (Non-exception Error Handling)

CCSDSPack uses a lightweight **error-first** pattern instead of C++ exceptions.  
Core operations return a `Result<T>`: either a valid value (`T`) **or** an `Error` describing what went wrong. This makes control flow explicit, friendly to embedded targets, and easy to wire into CLIs and CI.

---

## Why this pattern?

- **Deterministic control flow:** No hidden stack unwinding—callers *must* check results.
- **Low overhead:** No exception machinery; plays well with `-fno-exceptions` builds.
- **Great for tools & CI:** Natural mapping to exit codes and human-readable messages.
- **API clarity:** Public interfaces document failure modes via typed error codes.

---

## What is `Error`?

A compact object describing a failure.

| Field        | Type        | Meaning                                                       |
|--------------|-------------|---------------------------------------------------------------|
| `code`       | `int` / enum| Machine-readable reason for failure (stable across releases). |
| `message`    | `string`    | Human-readable explanation (actionable, short, no stack).     |

> The **code** is for branching/automation; the **message** is for logs and users.

---

## What is `Result<T>`?

A discriminated container: **exactly one** of `{ value<T>, Error }`.

| Property / Method     | Meaning                                                                 |
|-----------------------|-------------------------------------------------------------------------|
| `has_value()`         | `true` iff operation succeeded.                                         |
| `value()`             | The produced `T` (only if `has_value() == true`).                       |
| `error()`             | The `Error` (only if `has_value() == false`).                           |
| *(bool)*              | Often implemented as `has_value()` to enable concise checks.           |

> Callers always check `has_value()` (or the bool conversion) and branch accordingly.

---

## Error Codes

Below is the canonical set of error codes from `inc/Result.h` (`enum ErrorCode : uint8_t`).

| Code name                        | Value | Where it appears                        | Meaning / When raised                                                             |
|----------------------------------|:-----:|------------------------------------------|------------------------------------------------------------------------------------|
| `NONE`                           | 0     | All modules                              | No error (success).                                                                |
| `UNKNOWN_ERROR`                  | 1     | Anywhere                                 | Unspecified failure; unexpected condition without a specific mapped code.          |
| `NO_DATA`                        | 2     | Manager, Decoder                         | Operation requires data but buffers/packets are empty.                             |
| `INVALID_DATA`                   | 3     | Parser, Manager                          | Data content is invalid or corrupted.                                              |
| `INVALID_HEADER_DATA`            | 4     | Parser/Decoder                           | Primary header fields are malformed or violate CCSDS constraints.                  |
| `INVALID_SECONDARY_HEADER_DATA`  | 5     | Parser/Decoder                           | Secondary header fields are malformed or violate CCSDS constraints.                |
| `INVALID_APPLICATION_DATA`       | 6     | Decoder                                  | Application data segment is invalid or incomplete.                                 |
| `NULL_POINTER`                   | 7     | Anywhere                                 | Null pointer dereferenced or passed where non-null was required.                    |
| `INVALID_CHECKSUM`               | 8     | Validator, Manager                       | Checksum/CRC validation failed.                                                    |
| `VALIDATION_FAILURE`             | 9     | Validator, Manager                       | General validation failure (header fields, lengths, flags, etc.).                  |
| `TEMPLATE_SET_FAILURE`           | 10    | Manager                                  | Failed to set or load the template packet.                                         |
| `FILE_READ_ERROR`                | 11    | Binary I/O, CLIs                         | Input path invalid or OS read failed.                                              |
| `FILE_WRITE_ERROR`               | 12    | Binary I/O, CLIs                         | Output path invalid or OS write failed.                                            |
| `CONFIG_FILE_ERROR`              | 13    | Config loader, CLIs                      | Configuration file missing, unreadable, or malformed.                              |

> **Tip:** Keep codes stable across releases; extend by adding new values rather than renaming existing ones.

---

## Helper Macros

These are the macros defined in `inc/Result.h` for standardized `Result<T>`/`Error` handling:

| Macro | Signature | Intended for functions returning | What it does | Typical use |
|-------|-----------|-----------------------------------|--------------|-------------|
| **RETURN_IF_ERROR** | `RETURN_IF_ERROR(condition, errorCode)` | error code (`int`/enum) | If `condition` is true, immediately returns `errorCode`. | Precondition checks in code returning a numeric error code. |
| **RET_IF_ERR_MSG** | `RET_IF_ERR_MSG(condition, errorCode, message)` | `CCSDS::Error` or `Result<T>` | If `condition` is true, returns a constructed `CCSDS::Error{ errorCode, message }`. | Guard checks with specific error messages. |
| **ASSIGN_MV** | `ASSIGN_MV(var, result)` | `Result<T>` | Evaluates a `Result<T>`; on error returns `Error`; on success moves the value into `var`. | Assign large/movable types efficiently. |
| **ASSIGN_CP** | `ASSIGN_CP(var, result)` | `Result<T>` | Evaluates a `Result<T>`; on error returns `Error`; on success copies the value into `var`. | Assign small/cheap-to-copy types. |
| **ASSIGN_OR_PRINT** | `ASSIGN_OR_PRINT(var, result)` | `Result<T>` | If error, prints a standardized message to `std::cerr`; otherwise moves into `var`. | Non-fatal flows or CLI tools where you want logging. |
| **ASSERT_SUCCESS** | `ASSERT_SUCCESS(result)` | `void` | If error, returns immediately from a `void` function. | Early exit for `void` helpers. |
| **FORWARD_RESULT** | `FORWARD_RESULT(result)` | `Result<U>` | If error, returns it as-is; otherwise continues. | Thin wrappers forwarding underlying results. |

---

## Design Notes

- **Library vs CLI:** The library returns `Result<T>`/`Error`. CLIs map `Error::code` to process exit codes (0 = OK).
- **Messages are UX:** Keep `Error::message` short, cause-first, and suggest the next action (“…check config path”).
- **ABI stability:** Treat the error *codes* as part of your public contract; don’t reshuffle meanings between releases.
- **Testing:** Include failing-path tests to lock behavior (e.g., bad config, truncated files, CRC faults, APID mismatch).

---

## FAQ

**Q: Why not exceptions?**  
A: Many embedded and tooling contexts compile without exceptions; explicit `Result<T>` keeps costs visible and flow predictable.

**Q: How should I choose the right code?**  
A: Prefer the most specific code available (e.g., `INVALID_CHECKSUM` instead of `VALIDATION_FAILURE`). Reserve `UNKNOWN_ERROR` for truly unexpected states.

**Q: Can I extend the codes?**  
A: Yes—append new codes (don’t renumber existing ones) and document them here. Keep messages human-friendly.
