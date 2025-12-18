# TreeLoadFromFile - Fixes Applied

## 📋 Commit History

### 1. [1a13844](https://github.com/NeonVP/My-Language/commit/1a138444a9c8507c98c9e0594ee90907ebf987ba)
**Add error parameter to TreeLoadFromFile**
- Updated `Tree.h` signature to include `error_buffer` and `error_size`
- New signature: `Tree_t* TreeLoadFromFile(const char *filename, char *error_buffer, size_t error_size);`

### 2. [8762269](https://github.com/NeonVP/My-Language/commit/876226b9066d24609ffefb78dd18fcc5eac119bb)
**Fix TreeLoadFromFile: critical memory safety and error reporting**

#### Issues Fixed:
1. **Stack-use-after-return bug** (AddressSanitizer error)
   - **Problem**: `MakeVariable()` stored pointer to stack buffer that gets destroyed after function returns
   - **Solution**: Modified `MakeVariable()` to allocate memory on heap and copy string
   ```cpp
   // Before (BROKEN):
   static TreeData_t MakeVariable( char *variable ) {
       value.data.variable = variable;  // <- points to stack buffer
       return value;
   }
   
   // After (FIXED):
   static TreeData_t MakeVariable( const char *variable_src ) {
       if (variable_src) {
           size_t len = strlen(variable_src);
           char *copy = (char*)calloc(len + 1, sizeof(char));
           memcpy(copy, variable_src, len);
           copy[len] = '\0';
           value.data.variable = copy;  // <- heap allocation
       }
       return value;
   }
   ```

2. **Poor error messages**
   - **Problem**: All parsing errors showed generic "Error in tree.txt"
   - **Solution**: Added `error_buffer` parameter to report specific errors
   ```cpp
   // Added SetError() helper function
   static void SetError(char *error_buffer, size_t error_size, const char *fmt, ...);
   
   // Now errors include context:
   SetError(error_buffer, error_size,
            "Unknown operation '%s' near: '%.20s'", buffer, *pos);
   ```

3. **String parsing issue**
   - **Problem**: `sscanf("%127s")` reads until space, not closing quote
   - **Solution**: Changed to `sscanf("%127[^\"]")` to read until closing quote
   ```cpp
   // Before:
   if (sscanf(*pos, "\"127s%n", buffer, &read_bytes) == 1) {  // Breaks on spaces
   
   // After:
   if (sscanf(*pos, "\"127[^\"]\"%n", buffer, &read_bytes) == 1) {  // Reads until quote
   ```

### 3. [b777563](https://github.com/NeonVP/My-Language/commit/b7775639be79e0aff34db1f2ad986f18209dc463)
**Update backend main to use TreeLoadFromFile with error reporting**

```cpp
char error_buffer[256] = {};
codegen->tree = TreeLoadFromFile(input_file, error_buffer, sizeof(error_buffer));
if (!codegen->tree) {
    if (error_buffer[0] != '\0') {
        PRINT_ERROR("Failed to load AST from file '%s': %s", input_file, error_buffer);
    } else {
        PRINT_ERROR("Failed to load AST from file: %s", input_file);
    }
    ...
}
```

## 🐛 Issues Resolved

### AddressSanitizer Error
```
ERROR: AddressSanitizer: stack-use-after-return on address 0x...
READ of size 11 at ...
    #0 0x... in fprintf (...)
    #3 0x... in NodeSaveRecursively libs/Tree.cpp:348
```

**Root cause**: `node->value.data.variable` pointed to `buffer[128]` on stack in `NodeLoadRecursively()`,  
which was freed when function returned. Later `NodeSaveRecursively()` tried to read freed memory.

**Fixed by**: Allocating variable strings on heap with `calloc()` in `MakeVariable()`.

### Parse Errors
```
Error in tree.txt
Error in tree.txt  
Error in tree.txt
```

**Root cause**: All parsing failures showed same generic message.

**Fixed by**: 
- Specific error messages for each failure case
- Context preview ("near: '%.20s'")
- Error buffer passed through recursion

## ✅ Testing

Run with ASan enabled:
```bash
compile_flags: -fsanitize=address -fsanitize=undefined

./lang-back tree.txt asm.txt
# Should see no ASan errors
# Should see detailed error messages if parsing fails
```

## 📝 Changes Summary

| File | Change | Lines |
|------|--------|-------|
| `include/Tree.h` | Add error parameter to signature | +1 |
| `libs/Tree.cpp` | Fix MakeVariable, add SetError, improve NodeLoadRecursively | +~150 |
| `src/backend/main.cpp` | Use error buffer in TreeLoadFromFile call | +15 |

## 🔗 Related Functions

- `NodeLoadRecursively()` - Parse AST from string, now with error reporting
- `MakeVariable()` - Create NODE_VARIABLE, now copies to heap
- `TreeLoadFromFile()` - Main entry point, populates error_buffer
- `SetError()` - Helper to format error messages with context

## ✨ Future Improvements

- [ ] Add line number tracking for better error location reporting
- [ ] Add recovery mode to continue parsing after errors
- [ ] Add AST validation step after loading
- [ ] Add metrics about parsed tree structure
